#ifndef PWM_CONTROL_H
#define PWM_CONTROL_H

#include "main.h" // BẮT BUỘC để trình biên dịch hiểu TIM_HandleTypeDef

/**
 * @brief Khởi tạo hệ thống xuất xung PWM
 */
void PWM_Init(void);

/**
 * @brief Ngừng xuất xung PWM khẩn cấp (Set Duty về 0)
 */
void PWM_Stop(void);

/**
 * @brief Cập nhật độ rộng xung (Duty Cycle)
 * @param duty_cycle Giá trị từ 0.0f (0%) đến 1.0f (100%)
 */
void PWM_SetDuty(float duty_cycle);

#endif /* PWM_CONTROL_H */
