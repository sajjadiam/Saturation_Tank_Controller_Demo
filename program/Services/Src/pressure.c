#include "pressure.h"

#include "adc.h"
#include "tim.h"

#define PRESSURE_ADC_BUFFER_SIZE      16U

#define PRESSURE_RAW_0_BAR            745U
#define PRESSURE_RAW_10_BAR           3723U

#define PRESSURE_FULL_SCALE_MBAR      10000U

#define PRESSURE_RAW_VALID_MIN        560U
#define PRESSURE_RAW_VALID_MAX        3900U

#define PRESSURE_STALE_TIMEOUT_MS  		500U

static uint32_t pressure_last_update_ms = 0U;

static uint16_t pressure_dma_buffer[PRESSURE_ADC_BUFFER_SIZE];
static volatile uint16_t pressure_sample_buffer[PRESSURE_ADC_BUFFER_SIZE];

static volatile bool pressure_block_ready = false;

static uint16_t pressure_raw = 0U;
static uint16_t pressure_mbar = 0U;
static bool pressure_ready = false;

static bool pressure_valid = false;

static pressure_status_t pressure_status = PRESSURE_STATUS_NOT_READY;

bool pressure_init(void) {
	pressure_block_ready = false;

	pressure_raw = 0U;
	pressure_mbar = 0U;

	pressure_ready = false;
	pressure_valid = false;

	pressure_last_update_ms = 0U;
	
	pressure_status = PRESSURE_STATUS_NOT_READY;
	
	if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) {
		return false;
	}

	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)pressure_dma_buffer, PRESSURE_ADC_BUFFER_SIZE) != HAL_OK) {
		return false;
	}

	__HAL_DMA_DISABLE_IT(hadc1.DMA_Handle, DMA_IT_HT);

	if (HAL_TIM_Base_Start(&htim3) != HAL_OK) {
		HAL_ADC_Stop_DMA(&hadc1);
		return false;
	}

	return true;
}

void pressure_process(uint32_t now_ms) {
	uint16_t samples[PRESSURE_ADC_BUFFER_SIZE];
	uint32_t sum = 0U;
	uint32_t i;
	uint32_t primask;
	uint16_t raw;

	if (!pressure_block_ready) {
		if (pressure_ready &&
				((uint32_t)(now_ms - pressure_last_update_ms) >=
				PRESSURE_STALE_TIMEOUT_MS))
			{
				pressure_ready = false;
				pressure_valid = false;
				pressure_status = PRESSURE_STATUS_STALE;
			}

		return;
	}

	/*
	 * Take an atomic snapshot of the completed ADC block.
	 * The DMA callback may update pressure_sample_buffer,
	 * so interrupts are disabled only during this short copy.
	 */
	primask = __get_PRIMASK();
	__disable_irq();

	for (i = 0U; i < PRESSURE_ADC_BUFFER_SIZE; i++) {
		samples[i] = pressure_sample_buffer[i];
	}

	pressure_block_ready = false;

	__set_PRIMASK(primask);

	/*
	 * Process the local snapshot outside the critical section.
	 */
	for (i = 0U; i < PRESSURE_ADC_BUFFER_SIZE; i++) {
		sum += samples[i];
	}

	raw = (uint16_t)(sum / PRESSURE_ADC_BUFFER_SIZE);

	pressure_raw = raw;

	pressure_last_update_ms = now_ms;
	pressure_ready = true;

	if (raw < PRESSURE_RAW_VALID_MIN) {
    pressure_valid = false;
    pressure_status = PRESSURE_STATUS_UNDERRANGE;
    pressure_mbar = 0U;
    return;
	}

	if (raw > PRESSURE_RAW_VALID_MAX) {
		pressure_valid = false;
		pressure_status = PRESSURE_STATUS_OVERRANGE;
		pressure_mbar = PRESSURE_FULL_SCALE_MBAR;
		return;
	}

	pressure_valid = true;
	pressure_status = PRESSURE_STATUS_OK;

	if (raw <= PRESSURE_RAW_0_BAR) {
		pressure_mbar = 0U;
	}
	else if (raw >= PRESSURE_RAW_10_BAR) {
		pressure_mbar = PRESSURE_FULL_SCALE_MBAR;
	}
	else {
		pressure_mbar =
			(uint16_t)(
				((uint32_t)(raw - PRESSURE_RAW_0_BAR) *
				 PRESSURE_FULL_SCALE_MBAR) /
				(PRESSURE_RAW_10_BAR - PRESSURE_RAW_0_BAR)
			);
	}

}

bool pressure_is_ready(void) {
	return pressure_ready;
}

uint16_t pressure_get_mbar(void) {
	return pressure_mbar;
}

uint16_t pressure_get_raw(void) {
	return pressure_raw;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	uint32_t i;

	if (hadc->Instance != ADC1) {
		return;
	}

	for (i = 0U; i < PRESSURE_ADC_BUFFER_SIZE; i++) {
		pressure_sample_buffer[i] = pressure_dma_buffer[i];
	}

	pressure_block_ready = true;
}

bool pressure_is_valid(void) {
	return pressure_valid;
}

pressure_status_t pressure_get_status(void) {
	return pressure_status;
}