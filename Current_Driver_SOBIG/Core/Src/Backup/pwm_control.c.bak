#include "pwm_control.h"

extern TIM_HandleTypeDef htim1;

void PWM_Init(void) {
    // Khởi động kênh PWM duy nhất (PA8 - TIM1_CH1)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    // Khởi tạo mức duty cycle = 0 cho an toàn
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);

    // Kéo 2 chân chiều (PA9, PA10) xuống LOW để phanh/dừng động cơ
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
}

void PWM_Stop(void) {
    // Cắt hoàn toàn xung PWM
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);

    // Kéo 2 chân chiều xuống LOW để khóa an toàn mạch cầu H
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
}

void PWM_SetDuty(float duty_cycle) {
    uint32_t arr_val = __HAL_TIM_GET_AUTORELOAD(&htim1);

    // Khâu giới hạn (Saturation) an toàn tuyệt đối (-1.0 đến 1.0)
    if (duty_cycle > 1.0f) duty_cycle = 1.0f;
    if (duty_cycle < -1.0f) duty_cycle = -1.0f;

    // 1. Chạy Thuận (Duty >= 0)
    if (duty_cycle >= 0.0f) {
        // Set chiều thuận: PA9 = HIGH, PA10 = LOW
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);

        // Cấp độ rộng xung vào PA8
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)(duty_cycle * arr_val));
    }
    // 2. Chạy Nghịch (Duty < 0)
    else {
        duty_cycle = -duty_cycle; // Lấy trị tuyệt đối của duty_cycle

        // Set chiều nghịch: PA9 = LOW, PA10 = HIGH
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);

        // Cấp độ rộng xung vào PA8
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)(duty_cycle * arr_val));
    }
}
