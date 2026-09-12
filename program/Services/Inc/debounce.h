#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	bool stable_state;
	bool candidate_state;

	uint32_t candidate_since_ms;
	uint32_t debounce_time_ms;

} debounce_t;

/**
 * @brief Initialize a debounce instance.
 *
 * @param db                Debounce instance.
 * @param initial_state     Current logical state of the input.
 * @param now_ms            Current system time in milliseconds.
 * @param debounce_time_ms  Required stable time before accepting a change.
 */
void debounce_init(debounce_t *db, bool initial_state, uint32_t now_ms, uint32_t debounce_time_ms);

/**
 * @brief Process a new raw input sample.
 *
 * @param db         Debounce instance.
 * @param raw_state  Current logical raw input state.
 * @param now_ms     Current system time in milliseconds.
 *
 * @return true  Stable debounced state changed.
 * @return false No stable state change.
 */
bool debounce_update(debounce_t *db, bool raw_state, uint32_t now_ms);

/**
 * @brief Get current debounced state.
 */
bool debounce_get_state(const debounce_t *db);

#ifdef __cplusplus
}
#endif

#endif /* DEBOUNCE_H */