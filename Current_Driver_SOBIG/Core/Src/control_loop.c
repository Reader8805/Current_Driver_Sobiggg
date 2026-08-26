#include "control_loop.h"
#include "ad7606.h"
#include "pwm_control.h"
#include "fsm.h"

// Biến lưu giá trị mong muốn (Setpoint), nhận từ Modbus
volatile float current_setpoint = 0.0f;

void Control_SetTarget(float target) {
    current_setpoint = target;
}

// Hàm này được gọi trong Ngắt DMA hoàn thành (chạy chu kỳ 40us / 25kHz)
void Control_UpdateLoop_ISR(void) {
    // CHỈ ĐIỀU KHIỂN KHI HỆ THỐNG ĐANG Ở TRẠNG THÁI RUN
    if (FSM_GetCurrentState() == STATE_RUN) {

        // 1. Lấy dữ liệu phản hồi (Feedback) từ kênh 0
        float i_feedback = adc_voltage[0];

        // 2. Tính toán sai số
        float error = current_setpoint - i_feedback;

        // 3. Todo: Áp dụng thuật toán FF + PID ở đây
        // VD: float duty = Kp * error + Ki * integral + FF_Calc(current_setpoint);
        float duty_temp = 0.5f; // Khởi tạo tạm giá trị 50%

        // 4. Bơm tín hiệu vào PWM
        PWM_SetDuty(duty_temp);
    } else {
        // Đảm bảo không xuất xung nếu FSM nhảy sang FAULT hoặc READY
        PWM_Stop();
    }
}
