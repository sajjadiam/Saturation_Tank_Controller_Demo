#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Input debounce: both optocoupler level channels are active low. */
#define APP_START_DEBOUNCE_MS           50U
#define APP_LEVEL_DEBOUNCE_MS          100U

/* First pressurization: air inlet closes at 5.0 bar. */
#define APP_PRESSURE_READY_MBAR       5000U

/* Continuous air inlet regulation after the initial 5.0 bar crossing. */
#define APP_PRESSURE_AIR_ON_MBAR       4900U
#define APP_PRESSURE_AIR_OFF_MBAR      5500U

/* High-side air vent hysteresis: open at 6.0, close at 5.5 bar. */
#define APP_PRESSURE_VENT_ON_MBAR      6000U
#define APP_PRESSURE_VENT_OFF_MBAR     5500U

/* 6.0 bar alarm does not cut the pump; the controlled air vent handles it. */
#define APP_PRESSURE_OVERPRESSURE_MBAR        6000U
#define APP_PRESSURE_OVERPRESSURE_CLEAR_MBAR  5800U

/*
 * Provisional LAST-RESORT software shutdown, not a mechanical safety valve.
 * An emergency trip stops the pump and air inlet but leaves the vent usable.
 * Reset: selector OFF and valid pressure <= 5.5 bar.
 * Verify all thresholds against the vessel MAWP before commissioning.
 */
#define APP_PRESSURE_EMERGENCY_TRIP_MBAR   6200U
#define APP_PRESSURE_EMERGENCY_RESET_MBAR  5500U

/*
 * Commissioning placeholder: maximum cumulative pump-on time per fill.
 * This is a FAULT timeout, NOT a 120-second normal restart delay.
 * Must be set against max pump inflow, minimum safe headspace and
 * upper/lower probe spacing before operation on pressurized equipment.
 */
#define APP_PUMP_MAX_FILL_ENERGIZED_MS 120000UL

#if APP_PRESSURE_AIR_ON_MBAR >= APP_PRESSURE_AIR_OFF_MBAR
#error "Air inlet hysteresis must be positive"
#endif
#if APP_PRESSURE_VENT_OFF_MBAR >= APP_PRESSURE_VENT_ON_MBAR
#error "Air vent hysteresis must be positive"
#endif
#if APP_PRESSURE_OVERPRESSURE_MBAR >= APP_PRESSURE_EMERGENCY_TRIP_MBAR
#error "Emergency trip must exceed normal vent/alarm threshold"
#endif
#if APP_PRESSURE_EMERGENCY_RESET_MBAR >= APP_PRESSURE_EMERGENCY_TRIP_MBAR
#error "Emergency reset must be below trip"
#endif

#if APP_PUMP_MAX_FILL_ENERGIZED_MS == 0U
#error "Pump fault timeout must be nonzero"
#endif

#endif /* APP_CONFIG_H */
