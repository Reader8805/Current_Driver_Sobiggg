#ifndef CONTROL_LOOP_H
#define CONTROL_LOOP_H

#include "main.h"

void Control_SetTarget(float target);
void Control_UpdateLoop_ISR(void);

#endif /* CONTROL_LOOP_H */
