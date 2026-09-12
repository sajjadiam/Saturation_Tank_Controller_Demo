#include "debounce.h"

void debounce_init(debounce_t *db, bool initial_state, uint32_t now_ms, uint32_t debounce_time_ms) {
	db->stable_state 				= initial_state;
	db->candidate_state 		= initial_state;
	db->candidate_since_ms 	= now_ms;
	db->debounce_time_ms 		= debounce_time_ms;
}

bool debounce_update(debounce_t *db, bool raw_state, uint32_t now_ms) {
	if (raw_state != db->candidate_state) {
		db->candidate_state = raw_state;
		db->candidate_since_ms = now_ms;

		return false;
	}

	if (db->candidate_state != db->stable_state) {
		if ((uint32_t)(now_ms - db->candidate_since_ms) >= db->debounce_time_ms) {
			db->stable_state = db->candidate_state;

			return true;
		}
	}

	return false;
}

bool debounce_get_state(const debounce_t *db) {
	return db->stable_state;
}