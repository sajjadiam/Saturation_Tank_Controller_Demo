#include "bsp_leds.h"

#include "main.h"

void bsp_led_set(bsp_led_t led, bool on) {
	GPIO_PinState state = on ? GPIO_PIN_RESET : GPIO_PIN_SET;
	
	switch(led) {
		case BSP_LED_BLUE:
			HAL_GPIO_WritePin(LED_BLUE_N_GPIO_Port,LED_BLUE_N_Pin,state);
			break;
		
		case BSP_LED_RED1:
			HAL_GPIO_WritePin(LED_RED1_N_GPIO_Port,LED_RED1_N_Pin,state);
			break;
		
		case BSP_LED_RED2:
			HAL_GPIO_WritePin(LED_RED2_N_GPIO_Port,LED_RED2_N_Pin,state);
			break;
		
		default:
			break;
	}
}

void bsp_led_toggle(bsp_led_t led) {
	switch(led) {
		case BSP_LED_BLUE:
			HAL_GPIO_TogglePin(LED_BLUE_N_GPIO_Port,LED_BLUE_N_Pin);
			break;
		
		case BSP_LED_RED1:
			HAL_GPIO_TogglePin(LED_RED1_N_GPIO_Port,LED_RED1_N_Pin);
			break;
		
		case BSP_LED_RED2:
			HAL_GPIO_TogglePin(LED_RED2_N_GPIO_Port,LED_RED2_N_Pin);
			break;
		
		default:
			break;
	}
}

void bsp_leds_all_off(void) {
	bsp_led_set(BSP_LED_BLUE,false);
	bsp_led_set(BSP_LED_RED1,false);
	bsp_led_set(BSP_LED_RED2,false);
}