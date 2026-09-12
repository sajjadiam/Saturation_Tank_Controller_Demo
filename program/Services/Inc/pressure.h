#ifndef PRESSURE_H
#define PRESSURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize pressure acquisition.
 *
 * Performs ADC calibration and starts ADC DMA and TIM3.
 *
 * @return true  Initialization successful.
 * @return false Initialization failed.
 */
bool pressure_init(void);

/**
 * @brief Process a completed ADC sample block.
 *
 * Must be called periodically from the main loop.
 */
void pressure_process(void);

/**
 * @brief Check whether at least one valid sample block has been processed.
 */
bool pressure_is_ready(void);

/**
 * @brief Get filtered pressure in millibar.
 *
 * 5000 mbar = 5.000 bar
 */
uint16_t pressure_get_mbar(void);

/**
 * @brief Get averaged raw ADC value.
 */
uint16_t pressure_get_raw(void);

bool pressure_is_valid(void);

#ifdef __cplusplus
}
#endif

#endif /* PRESSURE_H */