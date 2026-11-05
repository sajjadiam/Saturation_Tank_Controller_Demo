#include "tank_controller.h"

#include "app_config.h"
#include "tank_policy.h"

#include "bsp_inputs.h"
#include "bsp_relays.h"
#include "bsp_buzzer.h"
#include "bsp_leds.h"

#include "debounce.h"
#include "pressure.h"

static debounce_t start_debounce;
static debounce_t upper_level_debounce;
static debounce_t lower_level_debounce;

static tank_state_t tank_state = TANK_STATE_IDLE;
static tank_policy_t policy;
static bool level_fault = false;

static void tank_controller_enter_idle(void)
{
    tank_state = TANK_STATE_IDLE;
    tank_policy_close_air(&policy);

    /* Alarm relay is independent of the operator start selector. */
    bsp_relay_set(BSP_RELAY_PUMP, false);
    bsp_relay_set(BSP_RELAY_AIR, false);
    bsp_relay_set(BSP_RELAY_AIR_VENT, false);
    bsp_leds_all_off();
}

static void tank_controller_apply_alarms(void)
{
    const bool alarm = policy.high_pressure_alarm ||
                       policy.emergency_trip || level_fault;
    bsp_buzzer_set(alarm);
    bsp_relay_set(BSP_RELAY_SIREN, alarm);
}

static void tank_controller_apply_outputs(bool pressure_ok)
{
    const bool running = (tank_state != TANK_STATE_IDLE);
    const bool outputs_allowed = running && pressure_ok &&
                                 !policy.emergency_trip && !level_fault;
    const bool pump_on = outputs_allowed && policy.filling;
    const bool air_on = outputs_allowed && policy.air_inlet_open &&
                        !policy.high_pressure_alarm;
    /*
     * A controlled vent must remain usable during the 6.0 bar alarm or an
     * emergency trip, while valid pressure data is still available.
     */
    const bool vent_on = running && pressure_ok && policy.air_vent_open;

    bsp_relay_set(BSP_RELAY_PUMP, pump_on);
    bsp_relay_set(BSP_RELAY_AIR, air_on);
    bsp_relay_set(BSP_RELAY_AIR_VENT, vent_on);

    bsp_led_set(BSP_LED_BLUE, pump_on);
    bsp_led_set(BSP_LED_RED1, air_on);
    bsp_led_set(BSP_LED_RED2, vent_on);
}

bool tank_controller_init(uint32_t now_ms)
{
    const bool start_active = bsp_inputs_start_active();
    const bool upper_wet = bsp_inputs_upper_level_reached();
    const bool lower_wet = bsp_inputs_lower_level_reached();

    tank_policy_init(&policy, upper_wet, lower_wet);
    level_fault = tank_policy_level_fault(upper_wet, lower_wet);
    tank_controller_enter_idle();
    tank_controller_apply_alarms();

    debounce_init(&start_debounce, start_active, now_ms,
                  APP_START_DEBOUNCE_MS);
    debounce_init(&upper_level_debounce, upper_wet, now_ms,
                  APP_LEVEL_DEBOUNCE_MS);
    debounce_init(&lower_level_debounce, lower_wet, now_ms,
                  APP_LEVEL_DEBOUNCE_MS);

    /* ADC calibration -> circular DMA -> TIM3 TRGO */
    if (!pressure_init()) {
        bsp_relays_all_off();
        bsp_buzzer_set(false);
        return false;
    }
    return true;
}

void tank_controller_process(uint32_t now_ms)
{
    bool pressure_ready;
    bool pressure_valid = false;
    uint16_t pressure_mbar = 0U;
    bool start_active;
    bool upper_wet;
    bool lower_wet;

    pressure_process(now_ms);
    pressure_ready = pressure_is_ready();
    if (pressure_ready) {
        pressure_valid = pressure_is_valid();
        if (pressure_valid) {
            pressure_mbar = pressure_get_mbar();
        }
    }

    (void)debounce_update(&start_debounce,
                          bsp_inputs_start_active(), now_ms);
    (void)debounce_update(&upper_level_debounce,
                          bsp_inputs_upper_level_reached(), now_ms);
    (void)debounce_update(&lower_level_debounce,
                          bsp_inputs_lower_level_reached(), now_ms);

    start_active = debounce_get_state(&start_debounce);
    upper_wet = debounce_get_state(&upper_level_debounce);
    lower_wet = debounce_get_state(&lower_level_debounce);

    level_fault = tank_policy_level_fault(upper_wet, lower_wet);
    tank_policy_update_level(&policy, upper_wet, lower_wet);

    tank_policy_update_pressure_protection(
        &policy, pressure_ready && pressure_valid,
        pressure_get_status() == PRESSURE_STATUS_OVERRANGE,
        pressure_mbar, start_active);
    tank_controller_apply_alarms();

    if (!start_active) {
        tank_controller_enter_idle();
        return;
    }

    if (tank_state == TANK_STATE_IDLE) {
        tank_state = TANK_STATE_PRESSURIZING;
        tank_policy_close_air(&policy);
    }

    if (!pressure_ready || !pressure_valid) {
        tank_policy_close_air(&policy);
        tank_controller_apply_outputs(false);
        return;
    }

    if (tank_state == TANK_STATE_PRESSURIZING) {
        if (pressure_mbar >= APP_PRESSURE_READY_MBAR) {
            /* First 5.0 bar crossing completes initial air charging. */
            tank_state = TANK_STATE_RUNNING;
            policy.air_inlet_open = false;
        } else {
            tank_policy_update_air(&policy, pressure_mbar, true);
        }
    } else {
        tank_policy_update_air(&policy, pressure_mbar, false);
    }

    tank_policy_update_vent(&policy, pressure_mbar);
    tank_controller_apply_outputs(true);
}

tank_state_t tank_controller_get_state(void)
{
    return tank_state;
}
