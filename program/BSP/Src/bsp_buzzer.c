#include "bsp_buzzer.h"

#include "main.h"

void bsp_buzzer_set(bool on) {
	HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port, BUZZER_EN_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}