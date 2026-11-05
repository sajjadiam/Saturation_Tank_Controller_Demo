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

int main(void)
{
    test_level_latch();
    test_mid_level_restart();
    test_air_pressure_regulation();
    test_vent_and_alarm();
    test_emergency_trip_and_sensor_fault();
    puts("tank_policy: all host assertions passed");
    return 0;
}
