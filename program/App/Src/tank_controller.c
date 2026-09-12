#include "tank_controller.h"

#include "app_config.h"

#include "bsp_inputs.h"
#include "bsp_relays.h"

#include "debounce.h"
#include "pressure.h"

#include "bsp_buzzer.h"
#include "bsp_leds.h"

static debounce_t start_debounce;
static debounce_t level_debounce;

static tank_state_t tank_state = TANK_STATE_IDLE;

static bool air_request = false;

static bool pump_request = false;

static bool pump_restart_timer_active = false;
static uint32_t pump_restart_since_ms = 0U;

static bool overpressure_alarm = false;
/* -------------------------------------------------------------------------- */
/* Internal functions                                                         */
/* -------------------------------------------------------------------------- */

static void tank_controller_enter_idle(void) {
	tank_state = TANK_STATE_IDLE;
	air_request = false;

	pump_request = false;
	pump_restart_timer_active = false;
	pump_restart_since_ms = 0U;

	/*
	 * Turn process outputs off.
	 * SIREN is controlled independently by alarm logic.
	 */
	bsp_relay_set(BSP_RELAY_PUMP, false);
	bsp_relay_set(BSP_RELAY_AIR, false);
	bsp_relay_set(BSP_RELAY_OUTLET, false);

	bsp_leds_all_off();
}

static void tank_controller_update_air(uint16_t pressure_mbar) {
	/*
	 * Hysteresis pressure control:
	 *
	 * P <= AIR_ON   -> Air ON
	 * P >= AIR_OFF  -> Air OFF
	 *
	 * Between these limits, keep previous state.
	 */
	if (pressure_mbar <= APP_PRESSURE_AIR_ON_MBAR) {
		air_request = true;
	}
	else if (pressure_mbar >= APP_PRESSURE_AIR_OFF_MBAR) {
		air_request = false;
	}
}

static void tank_controller_apply_outputs(bool pressure_ok) {
	bool pump_on;
	bool air_on;
	bool outlet_on;

	if (tank_state == TANK_STATE_IDLE) {
			bsp_relays_all_off();
			bsp_leds_all_off();
			return;
	}

	pump_on = pump_request;
	air_on = pressure_ok && air_request && !overpressure_alarm;
	outlet_on = (tank_state == TANK_STATE_RUNNING);

	bsp_relay_set(BSP_RELAY_PUMP, pump_on);
	bsp_relay_set(BSP_RELAY_AIR, air_on);
	bsp_relay_set(BSP_RELAY_OUTLET, outlet_on);
	
	bsp_led_set(BSP_LED_BLUE, pump_on);
	bsp_led_set(BSP_LED_RED1, air_on);
	bsp_led_set(BSP_LED_RED2, outlet_on);
}

static void tank_controller_update_pump(bool level_reached, uint32_t now_ms) {
	/*
	 * Reaching the level switch always stops the pump.
	 */
	if (level_reached) {
		pump_request = false;
		pump_restart_timer_active = false;
		return;
	}

	/*
	 * Pump is already running.
	 */
	if (pump_request) {
		return;
	}

	/*
	 * Level has just gone below the switch.
	 * Start restart qualification timer.
	 */
	if (!pump_restart_timer_active) {
		pump_restart_timer_active = true;
		pump_restart_since_ms = now_ms;
		return;
	}

	/*
	 * Level must remain below the switch continuously
	 * for the configured time before restarting.
	 */
	if ((uint32_t)(now_ms - pump_restart_since_ms) >= APP_PUMP_RESTART_DELAY_MS) {
		pump_request = true;
		pump_restart_timer_active = false;
	}
}
static void tank_controller_update_overpressure(bool pressure_ready, bool pressure_valid, uint16_t pressure_mbar) {
	pressure_status_t pressure_status;

  pressure_status = pressure_get_status();

	/*
	 * Electrical high overrange is treated as an alarm condition.
	 */
	if (pressure_status == PRESSURE_STATUS_OVERRANGE) {
		overpressure_alarm = true;
		bsp_buzzer_set(true);
		bsp_relay_set(BSP_RELAY_SIREN ,true);
		return;
	}
	
	/*
	 * No usable pressure information.
	 */
	if (!pressure_ready || !pressure_valid) {
		/*
		 * Do not clear an existing overpressure alarm
		 * without a valid pressure measurement below
		 * the clear threshold.
		 */
		bsp_buzzer_set(overpressure_alarm);
    bsp_relay_set(BSP_RELAY_SIREN, overpressure_alarm);
		
		return;
	}

	if (!overpressure_alarm) {
		if (pressure_mbar >= APP_PRESSURE_OVERPRESSURE_MBAR) {
			overpressure_alarm = true;
		}
	}
	else {
		if (pressure_mbar <= APP_PRESSURE_OVERPRESSURE_CLEAR_MBAR) {
			overpressure_alarm = false;
		}
	}

	bsp_buzzer_set(overpressure_alarm);
	bsp_relay_set(BSP_RELAY_SIREN ,overpressure_alarm);
}
/* -------------------------------------------------------------------------- */
/* Public functions                                                           */
/* -------------------------------------------------------------------------- */

bool tank_controller_init(uint32_t now_ms) {
	bool start_active;
	bool level_reached;

	/*
	 * Always establish a safe output state first.
	 */
	tank_controller_enter_idle();

	overpressure_alarm = false;
	bsp_buzzer_set(false);
	bsp_relay_set(BSP_RELAY_SIREN ,false);
	
	/*
	 * Initialize debouncers from the actual current logical
	 * input states.
	 *
	 * This is important for the maintained start selector:
	 * if it is ON during MCU reset, its initial stable state
	 * will also be ON.
	 */
	start_active = bsp_inputs_start_active();
	level_reached = bsp_inputs_level_reached();

	debounce_init(&start_debounce,
								start_active,
								now_ms,
								APP_START_DEBOUNCE_MS);

	debounce_init(&level_debounce,
								level_reached,
								now_ms,
								APP_LEVEL_DEBOUNCE_MS);

	/*
	 * Start pressure acquisition:
	 * ADC calibration -> DMA -> TIM3.
	 */
	if (!pressure_init()) {
		bsp_relays_all_off();
		return false;
	}

	return true;
}


void tank_controller_process(uint32_t now_ms) {
	bool start_active;
	bool level_reached;
	bool pressure_ready;
	bool pressure_valid;
	uint16_t pressure_mbar = 0U;

	/*
	 * Process a newly completed ADC block, if available.
	 */
	pressure_process(now_ms);

	pressure_ready = pressure_is_ready();
	pressure_valid = false;

	if (pressure_ready) {
		pressure_valid = pressure_is_valid();

		if (pressure_valid) {
			pressure_mbar = pressure_get_mbar();
		}
	}

	tank_controller_update_overpressure( pressure_ready, pressure_valid, pressure_mbar);

	/*
	 * Update digital input debouncing.
	 */
	(void)debounce_update(&start_debounce, bsp_inputs_start_active(), now_ms);

	(void)debounce_update(&level_debounce, bsp_inputs_level_reached(), now_ms);

	start_active = debounce_get_state(&start_debounce);
	level_reached = debounce_get_state(&level_debounce);
	
	/*
	 * Maintained selector:
	 *
	 * OFF -> system must always be IDLE.
	 */
	if (!start_active) {
		tank_controller_enter_idle();
		return;
	}

	tank_controller_update_pump(level_reached, now_ms);
	
	/*
	 * Selector is ON.
	 *
	 * This also covers automatic restart after MCU reset.
	 */
	if (tank_state == TANK_STATE_IDLE) {
		tank_state = TANK_STATE_PRESSURIZING;
		air_request = false;
	}

	/*
	 * Until the first pressure block is available:
	 *
	 * - Pump may operate from level control.
	 * - Air remains OFF.
	 * - Outlet remains CLOSED.
	 */
	if (!pressure_ready) {
		tank_controller_apply_outputs(false);
		return;
	}

	if (!pressure_valid) {
		air_request = false;

		tank_controller_apply_outputs(false);
		return;
	}

	/*
	 * Continuous air pressure control.
	 */
	tank_controller_update_air(pressure_mbar);

	if (overpressure_alarm) {
    air_request = false;
	}
	
	/*
	 * The outlet opens only after pressure first reaches
	 * the startup-ready threshold.
	 *
	 * Once RUNNING is entered, pressure dropping again does
	 * NOT return the controller to PRESSURIZING.
	 */
	if (tank_state == TANK_STATE_PRESSURIZING) {
		if (pressure_mbar >= APP_PRESSURE_READY_MBAR) {
			tank_state = TANK_STATE_RUNNING;
		}
	}

	tank_controller_apply_outputs(true);
}


tank_state_t tank_controller_get_state(void) {
	return tank_state;
}