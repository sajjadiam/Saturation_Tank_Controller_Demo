#ifndef BSP_LEDS_H
#define BSP_LEDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
	BSP_LED_BLUE = 0,
	BSP_LED_RED1,
	BSP_LED_RED2,

	BSP_LED_COUNT
} bsp_led_t;

void bsp_led_set(bsp_led_t led, bool on);
void bsp_led_toggle(bsp_led_t led);
void bsp_leds_all_off(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LEDS_H */