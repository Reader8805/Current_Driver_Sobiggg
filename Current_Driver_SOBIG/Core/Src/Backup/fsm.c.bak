#include "fsm.h"
#include "pwm_control.h"

// Trạng thái mặc định ban đầu là INIT
static SystemState_t currentState = STATE_INIT;

void FSM_Init(void) {
    currentState = STATE_INIT;
}

SystemState_t FSM_GetCurrentState(void) {
    return currentState;
}

void FSM_SetState(SystemState_t new_state) {
    currentState = new_state;
}

// Hàm này phải được gọi liên tục trong while(1) của main
void FSM_Update(void) {
    switch (currentState) {
        case STATE_INIT:
            // Todo: Test ngoại vi, check ADC bù offset. Pass thì chuyển sang READY
            currentState = STATE_READY;
            break;

        case STATE_READY:
            // Todo: Đèn LED báo hiệu đang chờ lệnh Start từ LabVIEW
            break;

        case STATE_RUN:
            // Todo: Kiểm tra cờ báo quá nhiệt, quá dòng ở đây
            // Nếu có lỗi nhảy sang STATE_FAULT: FSM_SetState(STATE_FAULT);
            break;

        case STATE_FAULT:
            // CẮT PWM KHẨN CẤP
            PWM_Stop();
            // Todo: Chờ lệnh Reset qua Modbus để quay lại READY
            break;
    }
}
