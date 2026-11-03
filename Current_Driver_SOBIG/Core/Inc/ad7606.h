#ifndef AD7606_H
#define AD7606_H

#include "main.h"
#define POS_GAIN 1.026806347
#define NEG_GAIN 1.019415584
typedef enum {
    ADC_ERROR_NONE = 0,
    ADC_ERROR_SAMPLE_OVERRUN,
    ADC_ERROR_BUSY_STUCK,
    ADC_ERROR_DMA_START,
    ADC_ERROR_SPI,
    ADC_ERROR_NO_SAMPLES,
    ADC_ERROR_ZERO_NOISE,
    ADC_ERROR_ZERO_DRIFT
} AD7606_Error_t;
AD7606_Error_t AD7606_GetError(void);
// Mảng chứa điện áp đã quy đổi
extern volatile float adc_voltage[8];
extern volatile float current_mean;

void AD7606_Init(void);
void AD7606_Trigger(void);
void AD7606_Start_DMA_Read(void);
void AD7606_End_DMA_Read(void);
void AD7606_Process_Data(void);
uint8_t AD7606_IsReady(void);
void AD7606_SetAcquisitionFault(void);
//int16_t AD7606_Calibrate_Offset_CH1(uint16_t num_samples);
HAL_StatusTypeDef AD7606_Calibrate_Offset_CH1(
    uint16_t num_samples,
    int16_t *result);
#endif /* AD7606_H */
