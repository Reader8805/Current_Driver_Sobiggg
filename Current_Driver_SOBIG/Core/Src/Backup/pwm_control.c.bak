#include "pwm_control.h"

extern TIM_HandleTypeDef htim1;

void PWM_Init(void) {
    // Khởi động cả 2 kênh cho Cầu H
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

    // Đảm bảo an toàn: Tắt cả 2 nhánh cầu
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
}

void PWM_Stop(void) {
    // Ngắt khẩn cấp cả 2 nhánh cầu H để tránh chập chờn
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
}

void PWM_SetDuty(float duty_cycle) {
    uint32_t arr_val = __HAL_TIM_GET_AUTORELOAD(&htim1);

    // 1. Chạy Thuận (Duty >= 0)
    if (duty_cycle >= 0.0f) {
        if (duty_cycle > 1.0f) duty_cycle = 1.0f; // Giới hạn max 100%

        // Cấp xung cho nhánh thuận, TẮT HOÀN TOÀN nhánh nghịch
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)(duty_cycle * arr_val));
    }
    // 2. Chạy Nghịch (Duty < 0)
    else {
        duty_cycle = -duty_cycle; // Trị tuyệt đối
        if (duty_cycle > 1.0f) duty_cycle = 1.0f; // Giới hạn max 100%

        // Cấp xung cho nhánh nghịch, TẮT HOÀN TOÀN nhánh thuận
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint32_t)(duty_cycle * arr_val));
    }
}
