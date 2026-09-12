#ifndef BSP_INPUTS_H
#define BSP_INPUTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Returns the logical state of the tank level switch.
 *
 * @return true  Water has reached the level switch.
 * @return false Water is below the level switch.
 */
bool bsp_inputs_level_reached(void);

/**
 * @brief Returns the logical state of the start input.
 *
 * @return true  Start input is active.
 * @return false Start input is inactive.
 *
 * @note The physical input is active-low.
 *       Interpretation as selector or push-button is handled
 *       by higher software layers.
 */
bool bsp_inputs_start_active(void);

#ifdef __cplusplus
}
#endif
#endif //BSP_INPUTS_H