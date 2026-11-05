#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "tank_policy.h"

static void test_level_latch(void)
{
    tank_policy_t p;
    tank_policy_init(&p, false, false);
    assert(p.filling);
    tank_policy_update_level(&p, false, true);
    assert(p.filling);
    tank_policy_update_level(&p, true, true);
    assert(!p.filling);
    tank_policy_update_level(&p, false, true);
    assert(!p.filling);
    tank_policy_update_level(&p, false, false);
    assert(p.filling);
    /* Normal pump state does NOT depend on pressure. */
    assert(!tank_policy_level_fault(false, true));
    assert(tank_policy_level_fault(true, false));
}

static void test_mid_level_restart(void)
{
    tank_policy_t p;
    tank_policy_init(&p, false, true);
    assert(!p.filling); /* No unexpected reset-time start. */
    tank_policy_update_level(&p, false, true);
    assert(!p.filling);
    tank_policy_update_level(&p, false, false);
    assert(p.filling);
    tank_policy_update_level(&p, true, true);
    assert(!p.filling);
}

static void test_air_pressure_regulation(void)
{
    tank_policy_t p;
    tank_policy_init(&p, false, false);
    tank_policy_update_air(&p, 4800U, true);
    assert(p.air_inlet_open);
    tank_policy_update_air(&p, 5000U, true);
    assert(!p.air_inlet_open);
    tank_policy_update_air(&p, 4900U, false);
    assert(!p.air_inlet_open);
    tank_policy_update_air(&p, 4899U, false);
    assert(p.air_inlet_open);
    tank_policy_update_air(&p, 5400U, false);
    assert(p.air_inlet_open);
    tank_policy_update_air(&p, 5500U, false);
    assert(!p.air_inlet_open);
}

static void test_vent_and_alarm(void)
{
    tank_policy_t p;
    tank_policy_init(&p, false, false);
    tank_policy_update_air(&p, 4800U, false);
    assert(p.air_inlet_open);
    tank_policy_update_vent(&p, 6000U);
    assert(p.air_vent_open);
    assert(!p.air_inlet_open);
    tank_policy_update_pressure_protection(&p, true, false, 6000U, true);
    assert(p.high_pressure_alarm);
    assert(!p.emergency_trip);
    assert(p.filling);
    tank_policy_update_vent(&p, 5700U);
    assert(p.air_vent_open);
    tank_policy_update_pressure_protection(&p, true, false, 5700U, true);
    assert(!p.high_pressure_alarm);
    tank_policy_update_vent(&p, 5500U);
    assert(!p.air_vent_open);
}

static void test_emergency_trip_and_sensor_fault(void)
{
    tank_policy_t p;
    tank_policy_init(&p, false, false);
    tank_policy_update_pressure_protection(&p, true, false, 6200U, true);
    assert(p.emergency_trip);
    tank_policy_update_pressure_protection(&p, false, false, 0U, false);
    assert(p.emergency_trip);
    tank_policy_update_pressure_protection(&p, true, false, 5500U, true);
    assert(p.emergency_trip);
    tank_policy_update_pressure_protection(&p, true, false, 5500U, false);
    assert(!p.emergency_trip);
    tank_policy_update_pressure_protection(&p, true, true, 0U, true);
    assert(p.emergency_trip);
}


static void test_high_vent_latch_on_missing_pressure(void)
{
    tank_policy_t p;
    tank_policy_init(&p, false, false);
    tank_policy_update_vent(&p, 6000U);
    assert(p.air_vent_open);
    tank_policy_update_pressure_protection(&p, false, false, 0U, false);
    assert(p.air_vent_open);
    tank_policy_update_vent(&p, 5500U);
    assert(!p.air_vent_open);
    tank_policy_update_pressure_protection(&p, false, true, 0U, true);
    assert(p.air_vent_open);
    assert(p.emergency_trip);
}

static void test_cumulative_pump_timeout_and_reset(void)
{
    tank_policy_t p;
    tank_policy_init(&p, false, false);
    tank_policy_track_pump(&p, 0U, false, false, true);
    tank_policy_record_pump(&p, true);
    tank_policy_track_pump(&p, APP_PUMP_MAX_FILL_ENERGIZED_MS / 2U,
                           false, false, true);
    assert(!p.fill_timeout_fault);
    tank_policy_record_pump(&p, false);
    tank_policy_track_pump(&p, APP_PUMP_MAX_FILL_ENERGIZED_MS / 2U + 100U,
                           false, false, false);
    tank_policy_record_pump(&p, true);
    tank_policy_track_pump(&p, APP_PUMP_MAX_FILL_ENERGIZED_MS + 100U,
                           false, false, true);
    assert(p.fill_timeout_fault);
    assert(!p.filling);

    /* OFF while both probes dry must NOT circumvent a latched timeout. */
    tank_policy_track_pump(&p, APP_PUMP_MAX_FILL_ENERGIZED_MS + 101U,
                           false, false, false);
    assert(p.fill_timeout_fault);

    /* Reset only after OFF with plausible wet lower sensor. */
    tank_policy_track_pump(&p, APP_PUMP_MAX_FILL_ENERGIZED_MS + 102U,
                           false, true, false);
    assert(!p.fill_timeout_fault);
    assert(!p.filling);
    tank_policy_update_level(&p, false, false);
    assert(p.filling);
}

static void test_cumulative_pump_timer_wraparound(void)
{
    tank_policy_t p;
    tank_policy_init(&p, false, false);
    tank_policy_track_pump(&p, UINT32_MAX - 40U, false, false, true);
    tank_policy_record_pump(&p, true);
    tank_policy_track_pump(&p, 79U, false, false, true);
    assert(p.fill_energized_ms == 120U);
}

int main(void)
{
    test_level_latch();
    test_mid_level_restart();
    test_air_pressure_regulation();
    test_vent_and_alarm();
    test_emergency_trip_and_sensor_fault();
    test_high_vent_latch_on_missing_pressure();
    test_cumulative_pump_timeout_and_reset();
    test_cumulative_pump_timer_wraparound();
    puts("tank_policy: all host assertions passed");
    return 0;
}
