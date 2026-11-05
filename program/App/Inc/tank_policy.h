#ifndef TANK_POLICY_H
#define TANK_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"

/*
 * Pure control policy. No HAL or GPIO calls: host-testable and deterministic.
 * "Reached" means the corresponding probe is wet.
 */
typedef struct {
    bool filling;
    bool air_inlet_open;
    bool air_vent_open;
    bool high_pressure_alarm;
    bool emergency_trip;
} tank_policy_t;

static inline bool tank_policy_level_fault(bool upper_wet, bool lower_wet)
{
    /* A wet upper probe with a dry lower probe is physically inconsistent. */
    return upper_wet && !lower_wet;
}

static inline void tank_policy_init(tank_policy_t *policy,
                                    bool upper_wet,
                                    bool lower_wet)
{
    /* On reset between probes, wait for the lower probe to go dry. */
    policy->filling = !upper_wet && !lower_wet;
    policy->air_inlet_open = false;
    policy->air_vent_open = false;
    policy->high_pressure_alarm = false;
    policy->emergency_trip = false;
}

static inline void tank_policy_update_level(tank_policy_t *policy,
                                            bool upper_wet,
                                            bool lower_wet)
{
    if (upper_wet) {
        policy->filling = false;
    } else if (!lower_wet) {
        policy->filling = true;
    }
    /* Between the two probes, hold the previously latched fill request. */
}

static inline void tank_policy_update_pressure_protection(
    tank_policy_t *policy,
    bool valid,
    bool overrange,
    uint16_t pressure_mbar,
    bool start_active)
{
    if (overrange) {
        policy->high_pressure_alarm = true;
        policy->emergency_trip = true;
    }

    if (!valid) {
        /* Never clear a latched alarm/trip using invalid measurement data. */
        return;
    }

    if (pressure_mbar >= APP_PRESSURE_EMERGENCY_TRIP_MBAR) {
        policy->emergency_trip = true;
    }

    if (pressure_mbar >= APP_PRESSURE_OVERPRESSURE_MBAR) {
        policy->high_pressure_alarm = true;
    } else if (pressure_mbar <= APP_PRESSURE_OVERPRESSURE_CLEAR_MBAR) {
        policy->high_pressure_alarm = false;
    }

    /*
     * A hard trip needs deliberate operator OFF and valid low pressure
     * before it can clear. Simply falling below the threshold is not enough.
     */
    if (!start_active &&
        pressure_mbar <= APP_PRESSURE_EMERGENCY_RESET_MBAR) {
        policy->emergency_trip = false;
    }
}

static inline void tank_policy_update_air(tank_policy_t *policy,
                                          uint16_t pressure_mbar,
                                          bool initial_pressurizing)
{
    if (initial_pressurizing) {
        /* First pressurization closes inlet at 5.0 bar. */
        policy->air_inlet_open =
            pressure_mbar < APP_PRESSURE_READY_MBAR;
    } else if (pressure_mbar < APP_PRESSURE_AIR_ON_MBAR) {
        policy->air_inlet_open = true;
    } else if (pressure_mbar >= APP_PRESSURE_AIR_OFF_MBAR) {
        policy->air_inlet_open = false;
    }
}

static inline void tank_policy_update_vent(tank_policy_t *policy,
                                           uint16_t pressure_mbar)
{
    if (pressure_mbar >= APP_PRESSURE_VENT_ON_MBAR) {
        policy->air_vent_open = true;
    } else if (pressure_mbar <= APP_PRESSURE_VENT_OFF_MBAR) {
        policy->air_vent_open = false;
    }
    /* Prevent contradictory air commands regardless of actuator timing. */
    if (policy->air_vent_open) {
        policy->air_inlet_open = false;
    }
}

static inline void tank_policy_close_air(tank_policy_t *policy)
{
    policy->air_inlet_open = false;
    policy->air_vent_open = false;
}

#endif /* TANK_POLICY_H */
