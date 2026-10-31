#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Input debounce */
#define APP_START_DEBOUNCE_MS          50U
#define APP_LEVEL_DEBOUNCE_MS         100U

/* Pressure control */
#define APP_PRESSURE_AIR_ON_MBAR      4800U
#define APP_PRESSURE_AIR_OFF_MBAR     5200U

/* Pressure required to enter RUNNING */
#define APP_PRESSURE_READY_MBAR       5000U

/* Minimum continuously dry time after the level switch was reached. */
#define APP_PUMP_RESTART_DELAY_MS    120000UL

/* Provisional pump cutout thresholds (5.5 / 5.3 bar); verify on tank. */
#define APP_PRESSURE_PUMP_STOP_MBAR   5500U
#define APP_PRESSURE_PUMP_RESUME_MBAR 5300U

#define APP_PRESSURE_OVERPRESSURE_MBAR        6000U
#define APP_PRESSURE_OVERPRESSURE_CLEAR_MBAR  5800U

#if APP_PRESSURE_PUMP_RESUME_MBAR >= APP_PRESSURE_PUMP_STOP_MBAR
#error "Pump pressure resume threshold must be below stop threshold"
#endif

#if APP_PRESSURE_PUMP_STOP_MBAR >= APP_PRESSURE_OVERPRESSURE_MBAR
#error "Pump must cut out below the overpressure alarm threshold"
#endif

#endif /* APP_CONFIG_H */