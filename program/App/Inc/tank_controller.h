#ifndef TANK_CONTROLLER_H
#define TANK_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	TANK_STATE_IDLE = 0,
	TANK_STATE_PRESSURIZING,
	TANK_STATE_RUNNING
} tank_state_t;

bool tank_controller_init(uint32_t now_ms);
void tank_controller_process(uint32_t now_ms);

tank_state_t tank_controller_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* TANK_CONTROLLER_H */