#ifndef BSP_RELAYS_H
#define BSP_RELAYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
	BSP_RELAY_PUMP = 0,
	BSP_RELAY_AIR,
	BSP_RELAY_OUTLET,
	BSP_RELAY_SIREN,

	BSP_RELAY_COUNT
} bsp_relay_t;

/**
 * @brief Turn a relay on or off.
 *
 * @param relay Relay identifier.
 * @param on    true = ON, false = OFF.
 */
void bsp_relay_set(bsp_relay_t relay, bool on);

/**
 * @brief Turn all relays off.
 */
void bsp_relays_all_off(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_RELAYS_H */