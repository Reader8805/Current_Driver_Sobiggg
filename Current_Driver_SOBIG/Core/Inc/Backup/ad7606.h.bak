#ifndef AD7606_H
#define AD7606_H

#include "main.h"
#define POS_GAIN 1.026806347
#define NEG_GAIN 1.019415584
// Mảng chứa điện áp đã quy đổi
extern volatile float adc_voltage[8];
extern volatile float current_mean;

void AD7606_Init(void);
void AD7606_Trigger(void);
void AD7606_Start_DMA_Read(void);
void AD7606_End_DMA_Read(void);
void AD7606_Process_Data(void);
int16_t AD7606_Calibrate_Offset_CH1(uint16_t num_samples);
#endif /* AD7606_H */
