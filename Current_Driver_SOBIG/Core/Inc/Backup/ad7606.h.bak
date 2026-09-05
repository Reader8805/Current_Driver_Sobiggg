#ifndef AD7606_H
#define AD7606_H

#include "main.h"

// Mảng chứa điện áp đã quy đổi
extern volatile float adc_voltage[8];

void AD7606_Init(void);
void AD7606_Trigger(void);
void AD7606_Start_DMA_Read(void);
void AD7606_End_DMA_Read(void);
void AD7606_Process_Data(void);

#endif /* AD7606_H */
