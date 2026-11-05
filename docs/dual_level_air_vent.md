# Dual-level refill and air-vent control — commissioning notes

## Wiring

- Upper level switch: PA2, active-low optocoupler output. Previously the single level input.
- Lower level switch: PA6, active-low optocoupler output. **PA6 was analog in the prior build**; GPIO and CubeMX configuration are changed.
- Pump contactor: relay 1 / PB5.
- Air inlet: relay 2 / PB4.
- Controlled air vent: relay 3 / PB3 (formerly process outlet).
- Siren: relay 4 / PA15; buzzer: PB12.
- Pressure input: PA0 via 4–20 mA sensor, ADC/DMA/TIM3.

This assumes both optocoupler outputs have defined logic levels with the existing board's pull-up hardware; verify with a meter and test each input under wet/dry conditions.

## Water filling (two-point hysteresis)

- Initially both probes dry: start filling immediately when enabled and pressure data is valid.
- Upper wet: stop the pump.
- Between the probes: retain the previous fill request, including across selector OFF/ON transitions.
- Lower dry: start filling again, continuing until upper wet.
- Upper wet with lower dry is physically inconsistent: inhibit actuators and alarm.
- Neither a 120-second refill delay nor an ordinary 5.5-bar pump cutoff remains.
- A fresh MCU reset while level lies between sensors defaults to no filling (conservative). The next lower-dry event starts the pump.

## Air control

1. At initial pressurization, command air inlet up to 5.0 bar; state becomes RUNNING at first valid pressure >=5.0 bar.
2. Subsequently, open air inlet at P <4.9 bar and close at P >=5.5 bar.
3. Open air vent at P >=6.0 bar and close at P <=5.5 bar.
4. Never command inlet and vent open simultaneously.
5. 6.0-bar threshold triggers the current audible alarm, **but does not normally inhibit pumping**.
6. A provisional, separate 6.2-bar emergency trip inhibits pump and air inlet; after operator selector OFF and valid P <=5.5 bar it can reset. Vent can remain commanded if pressure is valid. This threshold MUST be reviewed against the vessel maximum allowable working pressure (MAWP), compressor regulator, pump dead-head pressure, valve Cv, and response lag.

## Caveats / physical safety

**Control at 6.0 bar is not a hard upper limit.** Sampling latency, pressure sensor uncertainty, valve opening response, relief flow, and pump inflow can cause overshoot. Do NOT operate without an independent appropriately sized, certified mechanical relief device and validated pressure-rated vessel, fittings, hoses, and solenoid valves. The air vent exhaust must be routed to a safe location, away from personnel and water carryover. Check pump differential-pressure and blocked-discharge ratings.

A generic NC solenoid may fail closed on power loss, so it cannot function as independent relief. Never rely on the software alarm, ADC or firmware-controlled vent as the sole overpressure protective device.

## Tests before merging to main / energizing plant

- Host policy test: `cc -std=c11 -Wall -Wextra -Werror -pedantic -Iprogram/App/Inc tests/tank_policy_test.c -o /tmp/tank_policy_test && /tmp/tank_policy_test`
- Clean target build using installed STM32CubeF1 headers/HAL and MDK-ARM or arm-none-eabi-gcc.
- Power-up with both sensors dry, between sensors, and upper wet.
- Wet/dry each input individually and check its polarity; simulate upper wet + lower dry.
- Exercise pressure sequence: 4.8 -> 5.0 -> 4.89 -> 5.5 -> 6.0 -> 5.5 bar.
- Verify pressure sensor disconnect, overrange, stale DMA, and reboot mid-cycle.
- Measure actual worst-case peak pressure when pump runs and vent opens. Commissioning requires a hardware-in-loop test and verified relief sizing.
