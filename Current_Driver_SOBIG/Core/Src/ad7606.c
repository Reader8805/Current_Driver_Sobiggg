#include "ad7606.h"
#include "pwm_control.h"
#include "fsm.h"

extern SPI_HandleTypeDef hspi1;
extern volatile int16_t offset_ch1;
extern volatile float current_display;
volatile int16_t adc_raw[8];
volatile float adc_voltage[8];
volatile float current_mean = 0.0f;
static uint16_t dummy_tx[8];

/* Initial bench limits, in ADC counts. Tune using actual zero-current noise.
 * 8 counts is approximately 0.019 A with the existing +/-5 V scaling.
 */
#define ZERO_SETTLE_MS       1000U
#define ZERO_TIMEOUT_MS      8000U
#define ZERO_MAX_SPAN        128
#define ZERO_MAX_MEAN_DRIFT  8.0f
#define ZERO_STABLE_WINDOWS  3U
static volatile uint8_t calibration_ready;
static volatile uint8_t acquisition_fault;
static volatile uint8_t conversion_pending;
static volatile uint8_t collecting;
static volatile uint8_t window_ready;
static volatile uint16_t window_count;
static uint16_t window_target;
static volatile int32_t window_sum;
static volatile int16_t window_min, window_max;
static float zero_offset, display_filtered;
static volatile AD7606_Error_t adc_error;
/* Retain measurements for inspection in the debugger after a failed boot. */
volatile int32_t adc_zero_span;
volatile float adc_zero_mean;
volatile uint32_t adc_zero_windows;

static inline void delay_short(void)
{
    for (int i = 0; i < 15; i++) __NOP();
}

uint8_t AD7606_IsReady(void)
{
    return calibration_ready && !acquisition_fault;
}

AD7606_Error_t AD7606_GetError(void)
{
    return adc_error;
}

static void acquisition_failed(AD7606_Error_t reason)
{
    if (adc_error == ADC_ERROR_NONE) adc_error = reason;
    acquisition_fault = 1U;
    calibration_ready = 0U;
    PWM_Stop();
    FSM_SetState(STATE_FAULT);
}

void AD7606_SetAcquisitionFault(void)
{
    acquisition_failed(ADC_ERROR_SPI);
}

void AD7606_Init(void)
{
    /* Call before starting the sampling timer. */
    calibration_ready = acquisition_fault = conversion_pending = 0U;
    collecting = window_ready = 0U;
    adc_error = ADC_ERROR_NONE;
    adc_zero_span = 0;
    adc_zero_mean = 0.0f;
    adc_zero_windows = 0U;
    zero_offset = current_mean = display_filtered = current_display = 0.0f;
    HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ADC_CONVST_GPIO_Port, ADC_CONVST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ADC_RST_GPIO_Port, ADC_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(ADC_RST_GPIO_Port, ADC_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
}

void AD7606_Trigger(void)
{
    if (acquisition_fault) return;
    if (conversion_pending)
    {
        acquisition_failed(ADC_ERROR_SAMPLE_OVERRUN);
        return;
    }
    if (HAL_GPIO_ReadPin(ADC_BUSY_GPIO_Port, ADC_BUSY_Pin) == GPIO_PIN_SET)
    {
        acquisition_failed(ADC_ERROR_BUSY_STUCK);
        return;
    }
    conversion_pending = 1U;
    HAL_GPIO_WritePin(ADC_CONVST_GPIO_Port, ADC_CONVST_Pin, GPIO_PIN_RESET);
    delay_short();
    HAL_GPIO_WritePin(ADC_CONVST_GPIO_Port, ADC_CONVST_Pin, GPIO_PIN_SET);
}

void AD7606_Start_DMA_Read(void)
{
    if (acquisition_fault || !conversion_pending) return;
    HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_TransmitReceive_DMA(&hspi1, (uint8_t *)dummy_tx,
                                  (uint8_t *)adc_raw, 8) != HAL_OK)
    {
        HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_SET);
        acquisition_failed(ADC_ERROR_DMA_START);
    }
}

void AD7606_End_DMA_Read(void)
{
    HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_SET);
    conversion_pending = 0U;
}

/* Main context only, with timer + EXTI + DMA already running.
 * Physical current MUST be zero: stability alone cannot prove zero current.
 */
HAL_StatusTypeDef AD7606_Calibrate_Offset_CH1(uint16_t num_samples,
                                          int16_t *result)
{
    if (num_samples == 0U || result == NULL) return HAL_ERROR;
    calibration_ready = collecting = window_ready = 0U;
    window_target = num_samples;
    HAL_Delay(ZERO_SETTLE_MS);
    uint32_t start = HAL_GetTick();
    uint8_t stable = 0U;
    float mean_min = 0.0f, mean_max = 0.0f, mean_sum = 0.0f;
    uint32_t noisy_windows = 0U;
    while ((HAL_GetTick() - start) < ZERO_TIMEOUT_MS)
    {
        if (acquisition_fault) return HAL_ERROR;
        if (window_ready)
        {
            float mean = (float)window_sum / num_samples;
            int32_t span = (int32_t)window_max - window_min;
            adc_zero_mean = mean;
            adc_zero_span = span;
            adc_zero_windows++;
            if (span > ZERO_MAX_SPAN)
            {
                noisy_windows++;
                stable = 0U;
            }
            else
            {
                float low = (stable && mean_min < mean) ? mean_min : mean;
                float high = (stable && mean_max > mean) ? mean_max : mean;
                if (!stable || high - low > ZERO_MAX_MEAN_DRIFT)
                {
                    stable = 1U;
                    mean_min = mean_max = mean;
                    mean_sum = mean;
                }
                else
                {
                    mean_min = low;
                    mean_max = high;
                    mean_sum += mean;
                    stable++;
                }
                if (stable >= ZERO_STABLE_WINDOWS)
                {
                    /* Retain fractional counts in the feedback offset. */
                    zero_offset = mean_sum / stable;
                    *result = (int16_t)zero_offset;
                    offset_ch1 = *result;
                    display_filtered = 0.0f;
                    __DMB();
                    calibration_ready = 1U;
                    return HAL_OK;
                }
            }
            window_ready = 0U;
        }
        if (!collecting && !window_ready)
        {
            window_sum = 0;
            window_count = 0U;
            window_min = INT16_MAX;
            window_max = INT16_MIN;
            __DMB();
            collecting = 1U;
        }
        HAL_Delay(1);
    }
    collecting = 0U;
    /* Do not overwrite an acquisition fault arriving at the deadline. */
    if (acquisition_fault) return HAL_ERROR;
    adc_error = adc_zero_windows == 0U ? ADC_ERROR_NO_SAMPLES :
                (noisy_windows == adc_zero_windows ? ADC_ERROR_ZERO_NOISE :
                                                    ADC_ERROR_ZERO_DRIFT);
    return HAL_TIMEOUT;
}

void AD7606_Process_Data(void)
{
    int16_t raw = adc_raw[0];
    if (acquisition_fault) return;
    if (collecting)
    {
        window_sum += raw;
        if (raw < window_min) window_min = raw;
        if (raw > window_max) window_max = raw;
        if (++window_count >= window_target)
        {
            collecting = 0U;
            __DMB();
            window_ready = 1U;
        }
    }
    if (!calibration_ready) return;
    float delta = (float)raw - zero_offset;
    float gain = delta < 0.0f ? (float)NEG_GAIN : (float)POS_GAIN;
    /* Feedback/protection stay at full sample rate; filter only the LCD. */
    current_mean = gain * delta * (5.0f / 32768.0f / 0.06722f);
    display_filtered += 0.0012f * (current_mean - display_filtered);
    if (display_filtered > -0.1f && display_filtered < 0.1f)
        current_display = 0.0f;
    else
        current_display = display_filtered;
}
