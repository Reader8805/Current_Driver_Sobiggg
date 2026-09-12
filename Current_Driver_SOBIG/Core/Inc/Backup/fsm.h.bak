#ifndef FSM_H
#define FSM_H

#include "main.h"

// Định nghĩa các trạng thái
typedef enum {
    STATE_INIT,
    STATE_READY,
    STATE_RUN,
    STATE_FAULT
} SystemState_t;

void FSM_Init(void);
void FSM_Update(void);
SystemState_t FSM_GetCurrentState(void);
void FSM_SetState(SystemState_t new_state);
void FSM_SetCommand(uint8_t cmd);


#endif /* FSM_H */
