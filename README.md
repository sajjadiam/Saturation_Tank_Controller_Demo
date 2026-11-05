# Saturation Tank Controller Demo

Firmware for the STM32F103C8T6 Blue Pill controlling a roughly 50 L pressurized water/air saturation tank.

## Current firmware (dual-level / air-vent feature branch)

The controller uses three states: `IDLE`, `PRESSURIZING` and `RUNNING`. A maintained start selector enables operation. The initial air charge closes its air inlet at 5.0 bar, after which the air inlet controls at <4.9 bar ON / >=5.5 bar OFF. Relay 3 now controls **air vent** (not a process-liquid outlet), opening at >=6.0 bar and closing at <=5.5 bar.

A two-probe level latch controls the water pump: start when lower probe is dry; stop when upper probe is wet; retain previous request between the probes. Initial start with both probes dry is immediate if pressure acquisition is valid. The former 120-second timer and routine pump cutoff at 5.5 bar have been removed.

A 6.0-bar audible overpressure alarm remains active during vent control. A **provisional 6.2-bar emergency trip** disables pump and inlet, and requires selector OFF with valid pressure <=5.5 bar to reset. Invalid pressure data inhibits pump and inlet. A contradictory level reading (upper wet/lower dry) also inhibits outputs. Setpoint selection and vent capacity have **not** been validated on the plant.

## Hardware / I/O

| Connection | Function |
| --- | --- |
| PA0 | 0–10 bar / 4–20 mA pressure through shunt, ADC DMA |
| PA2 | Upper level probe, optocoupler active-low |
| PA6 | Lower level probe, optocoupler active-low |
| PB0 | Maintained start selector, active-low |
| PB5 / relay 1 | Pump contactor |
| PB4 / relay 2 | Air inlet solenoid |
| PB3 / relay 3 | Air vent solenoid |
| PA15 / relay 4 | Siren |
| PB12 | Buzzer |

Controller: 72 MHz STM32F103, TIM3 100 Hz ADC trigger, DMA circular buffer.

## Implementation

- `program/App/Src/tank_controller.c` — state coordination, safety gating and IO
- `program/App/Inc/tank_policy.h` — pure, host-testable control policy
- `program/App/Inc/app_config.h` — threshold configuration
- `tests/tank_policy_test.c` — host C assertions
- `docs/dual_level_air_vent.md` — full behavior, wiring and commissioning checklist
- `docs/design_baseline.md` — historical original single-probe design, superseded where it conflicts with the new document

**Not production-approved.** Build, hardware-in-loop validation, independent mechanical overpressure relief sizing, sensor polarity, switch geometry and vessel pressure ratings must be verified before energizing the pressurized system.
