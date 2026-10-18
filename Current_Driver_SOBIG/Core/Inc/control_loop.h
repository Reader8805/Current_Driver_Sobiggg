#ifndef CONTROL_LOOP_H
#define CONTROL_LOOP_H

#include "main.h"

void Control_SetTarget(float target);
void Control_UpdateLoop_ISR(void);
void Control_SetSineMode(uint8_t enable, uint8_t amp, uint8_t freq, uint8_t points);

#endif /* CONTROL_LOOP_H */
