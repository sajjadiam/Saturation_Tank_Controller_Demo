#include "bsp_inputs.h"

#include "main.h"

bool bsp_inputs_level_reached(void) {
	return (HAL_GPIO_ReadPin(LEVEL_SW_N_GPIO_Port, LEVEL_SW_N_Pin) == GPIO_PIN_RESET);
}

bool bsp_inputs_start_active(void) {
	return (HAL_GPIO_ReadPin(START_SEL_N_GPIO_Port, START_SEL_N_Pin) == GPIO_PIN_RESET);
}