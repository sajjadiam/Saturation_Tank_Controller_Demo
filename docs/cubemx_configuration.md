# STM32CubeMX Configuration Baseline

این سند تنظیمات فعلی و تصمیم‌های مربوط به `STM32CubeMX` برای پروژه `Saturation Tank Controller Demo` را ثبت می‌کند.

> مبنا: `STM32F103C8T6` روی Blue Pill با کریستال خارجی 8 MHz.

## 1. System / Clock

### RCC

```text
HSE = Crystal/Ceramic Resonator
LSE = Disabled
```

### SYS

```text
Debug = Serial Wire
Timebase Source = SysTick
```

پایه‌های SWD رزرو می‌شوند:

```text
PA13 = SWDIO
PA14 = SWCLK
```

### Clock Tree

```text
HSE              = 8 MHz
PLL Source       = HSE
PLL Multiplier   = x9
SYSCLK           = 72 MHz
AHB Prescaler    = /1
HCLK             = 72 MHz
APB1 Prescaler   = /2
PCLK1            = 36 MHz
APB1 Timer Clock = 72 MHz
APB2 Prescaler   = /1
PCLK2            = 72 MHz
ADC Prescaler    = /6
ADC Clock        = 12 MHz
```

این مقادیر با محدودیت‌های STM32F103x8/xB سازگارند؛ فرکانس حداکثر CPU برابر 72 MHz است و کلاک ADC باید حداکثر 14 MHz باشد.

## 2. Pressure Sensor ADC

ورودی سنسور فشار به پایه زیر تخصیص داده شده است:

```text
PA0 = ADC1_IN0
```

تنظیمات ADC1:

```text
Channel                      = ADC1_IN0
Regular Rank                 = 1
Number of Conversions        = 1
Scan Conversion Mode         = Disabled
Continuous Conversion Mode   = Disabled
Discontinuous Conversion     = Disabled
External Trigger             = TIM3 TRGO
Data Alignment               = Right
Sampling Time                = 239.5 ADC cycles
```

ADC به‌صورت Free Running استفاده نمی‌شود. نرخ نمونه‌برداری توسط TIM3 تعیین خواهد شد.

## 3. TIM3 — ADC Sampling Trigger

TIM3 فقط به‌عنوان منبع Trigger سخت‌افزاری ADC استفاده می‌شود.

تنظیم هدف:

```text
TIM3 Clock          = 72 MHz
Prescaler           = 7199     // divide by 7200
Counter Period ARR  = 99       // divide by 100
TRGO                = Update Event
Master/Slave Mode   = Disabled
TIM3 IRQ            = Disabled
```

نرخ Update Event هدف:

```text
72 MHz / (7199 + 1) / (99 + 1) = 100 Hz
```

بنابراین هر 10 ms یک Trigger سخت‌افزاری برای ADC تولید می‌شود.

### وضعیت فایل .ioc در commit `48c3480` (`update cubemx`)

موارد زیر به‌درستی ثبت شده‌اند:

```text
TIM3.Prescaler = 7200-1
TIM3.TRGO      = Update Event
TIM3 IRQ       = Disabled
```

اما `Counter Period = 99` در فایل `.ioc` فعلی ثبت نشده است. بنابراین نرخ 100 Hz در وضعیت فعلی قابل تأیید نیست.

**اقدام لازم قبل از ادامه Firmware:** در CubeMX مقدار `Counter Period` تایمر 3 روی `99` قرار داده شود و فایل `.ioc` مجدداً commit شود.

## 4. ADC DMA

ADC1 از DMA استفاده می‌کند:

```text
DMA Instance            = DMA1 Channel 1
Direction               = Peripheral to Memory
Mode                    = Circular
Peripheral Increment    = Disabled
Memory Increment        = Enabled
Peripheral Data Width   = Half Word (16-bit)
Memory Data Width       = Half Word (16-bit)
Priority                = Medium
```

DMA Interrupt فعال است. Interrupt تایمر 3 و Interrupt ADC برای Trigger/Conversion لازم نیستند.

زنجیره سخت‌افزاری موردنظر:

```text
TIM3 Update Event
       |
       v
TIM3 TRGO
       |
       v
ADC1 regular conversion / PA0
       |
       v
DMA1 Channel 1
       |
       v
RAM circular buffer
```

CPU برای شروع هر نمونه وارد ISR نمی‌شود.

## 5. Sampling / Filtering Baseline

نرخ نمونه‌برداری هدف:

```text
Fs = 100 Hz
Ts = 10 ms
```

بافر اولیه پیشنهادی:

```c
#define PRESSURE_ADC_BUFFER_SIZE  16U

static uint16_t pressure_adc_buffer[PRESSURE_ADC_BUFFER_SIZE];
```

برای شروع، 16 نمونه جهت کاهش نویز استفاده می‌شود. پنجره زمانی 16 نمونه در 100 Hz برابر است با:

```text
160 ms
```

نوع دقیق فیلتر نرم‌افزاری پس از مشاهده نویز واقعی ADC نهایی خواهد شد. نقطه شروع پیشنهادی Average روی 16 نمونه است؛ در صورت نیاز می‌توان IIR یا روش مقاوم‌تر در برابر Outlier اضافه کرد.

## 6. ADC Startup Sequence

در Firmware، ترتیب اولیه پیشنهادی:

```c
HAL_ADCEx_Calibration_Start(&hadc1);
HAL_ADC_Start_DMA(&hadc1,
                  (uint32_t *)pressure_adc_buffer,
                  PRESSURE_ADC_BUFFER_SIZE);
HAL_TIM_Base_Start(&htim3);
```

از `HAL_TIM_Base_Start_IT()` استفاده نمی‌شود، چون TIM3 Interrupt برای تولید TRGO لازم نیست.

## 7. Pressure Analog Front-End

سنسور فشار:

```text
Range  = 0..10 bar
Output = 4..20 mA
Supply = 24 VDC
```

شنت فعلی:

```text
R_SHUNT = 150 ohm
4 mA  -> 0.6 V
20 mA -> 3.0 V
```

ورودی ADC فعلی:

```text
Shunt node -> 4.7k -> PA0
                    |
                    +-> 1uF -> GND
                    +-> clamp to 3.3V
                    +-> clamp to GND
```

این فیلتر سخت‌افزاری و فیلتر نرم‌افزاری دو لایه مستقل هستند و نباید یکی جایگزین دیگری فرض شود.

## 8. References

مراجع اصلی برای تصمیم‌های CubeMX و Firmware باید مستندات رسمی ST باشند:

- `DS5319` — STM32F103x8 / STM32F103xB Datasheet: https://www.st.com/resource/en/datasheet/stm32f103c8.pdf
- `RM0008` — STM32F101/102/103/105/107 Reference Manual: https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101-103-105-107-stm32f100-series-armbased-32bit-mcus-stmicroelectronics.pdf
- ST STM32F103 Documentation Portal: https://www.st.com/en/microcontrollers-microprocessors/stm32f103/documentation.html

## 9. Open Items

- [ ] Set `TIM3 Counter Period = 99` and commit updated `.ioc`.
- [ ] Complete physical GPIO pin map for Level Switch and relay outputs.
- [ ] Decide DMA buffer processing strategy after first ADC noise measurements.
- [ ] Define pressure conversion/calibration coefficients from real sensor measurements.
- [ ] Define pressure-control hysteresis around the nominal 5 bar setpoint.
