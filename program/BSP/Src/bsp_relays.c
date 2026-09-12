#include "bsp_relays.h"

#include "main.h"

void bsp_relay_set(bsp_relay_t relay, bool on) {
	GPIO_PinState state = on ? GPIO_PIN_RESET : GPIO_PIN_SET;

	switch (relay) {
		case BSP_RELAY_PUMP:
			HAL_GPIO_WritePin(RELAY_1_N_GPIO_Port, RELAY_1_N_Pin, state);
			break;

		case BSP_RELAY_AIR:
			HAL_GPIO_WritePin(RELAY_2_N_GPIO_Port, RELAY_2_N_Pin, state);
			break;

		case BSP_RELAY_OUTLET:
			HAL_GPIO_WritePin(RELAY_3_N_GPIO_Port, RELAY_3_N_Pin, state);
			break;

		case BSP_RELAY_SIREN:
			HAL_GPIO_WritePin(RELAY_4_N_GPIO_Port, RELAY_4_N_Pin, state);
			break;

		default:
			break;
	}
}

void bsp_relays_all_off(void) {
	bsp_relay_set(BSP_RELAY_PUMP	, false);
	bsp_relay_set(BSP_RELAY_AIR		, false);
	bsp_relay_set(BSP_RELAY_OUTLET, false);
	bsp_relay_set(BSP_RELAY_SIREN	, false);
}