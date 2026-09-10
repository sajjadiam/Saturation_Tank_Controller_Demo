# Design Baseline — Saturation Tank Controller Demo

این سند وضعیت فعلی طراحی را ثبت می‌کند تا ادامه توسعه Firmware و سخت‌افزار بر اساس یک مبنای مشخص انجام شود.

## 1. هدف سیستم

کنترل یک مخزن اشباع حدود `50 L` برای تولید آب اشباع‌شده از هوا / میکروبابل در یک سیستم Demo تحت نظارت اپراتور.

این نسخه Demo است؛ بنابراین منطق کنترل عمداً ساده نگه داشته می‌شود و از افزودن interlockهای صنعتی غیرضروری در این مرحله خودداری می‌شود.

## 2. تجهیزات فرایندی

### سنسورها

1. سنسور فشار صنعتی
   - Range: `0..10 bar`
   - Output: `4..20 mA`
   - Supply: `24 VDC`

2. Level Switch
   - Type: Reed Switch
   - فقط برای تشخیص رسیدن آب به یک سطح مشخص استفاده می‌شود.

### عملگرها

1. پمپ آب
2. شیر برقی ورودی هوا از کمپرسور
3. شیر برقی خروجی مخزن

شیرهای برقی `230 VAC` در نظر گرفته شده‌اند و از طریق کنتاکت‌های ماژول رله کنترل می‌شوند.

## 3. State Machine فعلی

### `IDLE`

- Pump: OFF
- Air Valve: OFF
- Outlet Valve: CLOSED

### `PRESSURIZING`

پس از Start سیکل:

- Pump control فعال است.
- Air pressure control فعال است.
- Outlet Valve بسته است.
- آب و هوا می‌توانند هم‌زمان وارد مخزن شوند.

در این حالت:

```text
Level below switch -> Pump ON
Level reached       -> Pump OFF

Pressure controller -> Air Valve
Outlet               -> CLOSED
```

به محض اینکه فشار برای اولین بار به محدوده هدف حدود `5 bar` برسد، سیستم وارد `RUNNING` می‌شود.

### `RUNNING`

- Outlet Valve باز می‌شود و تا پایان سیکل باز باقی می‌ماند.
- Level Switch فقط پمپ را کنترل می‌کند.
- فشار به‌صورت پیوسته حول 5 bar کنترل می‌شود.

```text
Level below switch -> Pump ON
Level reached       -> Pump OFF

Pressure controller -> Air Valve
Outlet               -> OPEN continuously
```

پایین آمدن Level در `RUNNING` باعث بسته شدن Outlet نمی‌شود.

## 4. نکته فرایندی قطعی‌شده

برای شروع خروجی، رسیدن آب به Level Switch شرط نیست.

با توجه به ماهیت Demo و هندسه سیستم، حتی مقدار کمی آب نیز برای شروع تست قابل قبول است. بنابراین شرط اولیه باز شدن Outlet فقط رسیدن فشار به محدوده هدف تعریف شده است.

Level Switch فقط برای کنترل ON/OFF پمپ در ادامه سیکل استفاده می‌شود.

## 5. کنترل فشار

Setpoint اسمی:

```text
P_SET ≈ 5 bar
```

کنترل شیر هوا باید دارای hysteresis باشد تا از chatter رله جلوگیری شود.

پارامترهای زیر هنوز نهایی نشده‌اند:

```text
P_CTRL_LOW
P_CTRL_HIGH
P_STARTUP_READY
P_MAX
```

رفتار مفهومی:

```text
P <= P_CTRL_LOW  -> Air Valve ON
P >= P_CTRL_HIGH -> Air Valve OFF
```

## 6. سخت‌افزار کنترل

### MCU

Blue Pill مبتنی بر:

```text
STM32F103C8T6
```

### خروجی رله

ماژول رله چهار کاناله 5 ولت.

تخصیص فعلی:

```text
CH1 -> Pump Contactor
CH2 -> Air Solenoid Valve
CH3 -> Outlet Solenoid Valve
CH4 -> Spare
```

ورودی‌های ماژول Active-Low در نظر گرفته می‌شوند و این جزئیات باید در لایه Driver پنهان شود.

## 7. تغذیه

### منبع اصلی

```text
230 VAC -> 24 VDC / 1 A
```

باس 24V فقط برای Instrumentation و Control استفاده می‌شود و بارهای قدرت 230VAC را تغذیه نمی‌کند.

### تغذیه 5V

ماژول Buck مبتنی بر `MP1584`:

```text
24 VDC -> 5 VDC
```

مصرف‌کننده‌های اصلی 5V:

- Blue Pill
- Relay Module

## 8. مدار ورودی فشار 4..20mA

مقاومت شنت:

```text
R_SHUNT = 150 ohm
```

در نتیجه:

```text
4 mA  -> 0.6 V
20 mA -> 3.0 V
```

برای سنسور `0..10 bar`:

```text
0 bar  -> 0.6 V
10 bar -> 3.0 V
```

بعد از شنت یک فیلتر RC و دو دیود Clamp برای حفاظت ورودی ADC استفاده می‌شود.

پیکربندی فعلی پیشنهادی:

```text
Shunt node -> 4.7k -> ADC
                    |
                    +-> 1uF -> GND
                    +-> Clamp to 3.3V
                    +-> Clamp to GND
```

مقادیر دقیق قطعات می‌توانند در زمان نهایی‌سازی شماتیک تغییر کنند.

### Pin تخصیص‌یافته

```text
PA0 = ADC1_IN0
```

نمونه‌برداری این ورودی با `TIM3 TRGO -> ADC1 -> DMA1 Channel1` انجام خواهد شد. جزئیات در `docs/cubemx_configuration.md` ثبت شده است.

## 9. Level Switch Input

Level Switch از نوع Reed Relay/Switch است.

ورودی آن با `24 VDC` و یک Optocoupler خوانده می‌شود.

ساختار مفهومی:

```text
+24V
 |
Current-limit resistor
 |
Optocoupler LED
 |
Reed Level Switch
 |
GND_24V
```

خروجی اپتو به GPIO میکروکنترلر متصل می‌شود.

Firmware نباید به Active-Low/Active-High بودن سخت‌افزار وابسته باشد و باید از abstraction مانند زیر استفاده کند:

```c
bool level_is_reached(void);
```

## 10. مونتاژ مدارهای کوچک

مدارهای جانبی روی برد سوراخ‌دار حدود `2600` نقطه مونتاژ و لحیم می‌شوند؛ Breadboard استفاده نمی‌شود.

بخش‌های Low Voltage شامل:

```text
3.3V
5V
24V instrumentation
4..20mA interface
Optocoupler input
```

بخش `230 VAC` باید از قسمت کنترل Low-Voltage فیزیکی جدا نگه داشته شود و از مسیرها/ترمینال‌های مناسب عبور کند.

## 11. I/O Map منطقی فعلی

### Analog Input

```text
AI1 -> PA0 / ADC1_IN0 -> Pressure Sensor 0..10 bar / 4..20mA
```

### Digital Input

```text
DI1 -> Level Switch via Optocoupler
```

### Digital Outputs

```text
DO1 -> Pump Contactor
DO2 -> Air Solenoid Valve
DO3 -> Outlet Solenoid Valve
DO4 -> Spare
```

پایه‌های واقعی DI1 و DO1..DO4 هنوز در این سند ثبت نشده‌اند.

## 12. STM32CubeMX Baseline

تنظیمات اصلی فعلی:

```text
HSE              = 8 MHz crystal
PLL               = HSE x9
SYSCLK/HCLK       = 72 MHz
PCLK1             = 36 MHz
PCLK2             = 72 MHz
ADC Clock         = 12 MHz (PCLK2 / 6)
Debug             = Serial Wire
PA0               = ADC1_IN0
ADC Trigger       = TIM3 TRGO
ADC Sampling Time = 239.5 cycles
DMA               = DMA1 Channel1 / Circular / Half-word
TIM3 TRGO         = Update Event
TIM3 IRQ          = Disabled
```

نرخ نمونه‌برداری هدف فشار `100 Hz` است. برای این نرخ:

```text
TIM3 PSC = 7199
TIM3 ARR = 99
```

در commit `48c3480` با عنوان `update cubemx`، `PSC=7199` و `TRGO=Update Event` ثبت شده‌اند اما `ARR=99` هنوز در فایل `.ioc` ثبت نشده است. این مورد قبل از ادامه Firmware باید در CubeMX اصلاح و commit شود.

مستند کامل تنظیمات در:

```text
docs/cubemx_configuration.md
```

قرار دارد.

## 13. موارد باز برای ادامه

- تعیین Pin Map نهایی Level Switch و خروجی‌های Relay
- اصلاح `TIM3 Counter Period = 99` در CubeMX برای نرخ نمونه‌برداری 100 Hz
- تعیین Start/Stop interface اپراتور
- تعیین مقادیر نهایی Pressure hysteresis
- تعیین limits و timeoutهای لازم برای Demo
- نهایی‌سازی شماتیک مدار 4..20mA
- نهایی‌سازی مدار Optocoupler Level Input
- تعیین روش نهایی فیلتر ADC پس از مشاهده داده واقعی
- ساختار Firmware و Repository
- تست مرحله‌ای I/O و State Machine
