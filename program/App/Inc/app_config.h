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

#define APP_PUMP_RESTART_DELAY_MS    1000U

#define APP_PRESSURE_OVERPRESSURE_MBAR        6000U
#define APP_PRESSURE_OVERPRESSURE_CLEAR_MBAR  5800U

#endif /* APP_CONFIG_H */