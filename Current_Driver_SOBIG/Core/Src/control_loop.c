#include "control_loop.h"
#include "ad7606.h"
#include "pwm_control.h"
#include "fsm.h"

// --- CÁC THÔNG SỐ CỦA BỘ ĐIỀU KHIỂN ---
// Tần số trích mẫu 25kHz -> Ts = 40us
#define DT 0.00004f

// Hệ số PID (Cần dò (tuning) trên mô hình thực tế)
static float Kp = 0.05f;
static float Ki = 0.0f;
static float Kd = 0.0f;

// Hệ số Feed-Forward
// Với tải thuần trở-cảm (R-L), V = R*I + L*di/dt. Ở chế độ tĩnh, V_ff = R*I_setpoint.
static float K_ff = 0.045833f;

// Các biến trạng thái của bộ điều khiển số
static float integral = 0.0f;
static float prev_error = 0.0f;

// Giới hạn bão hòa đầu ra (Saturation)
// Đặt max 0.95 (95%) để chừa lại dead-time cho phần cứng mạch cầu (nếu có)
#define DUTY_MAX  0.95f
#define DUTY_MIN -0.95f

// Biến lưu giá trị mong muốn (Setpoint)
volatile float current_setpoint = 0.0f;

void Control_SetTarget(float target) {
    current_setpoint = target;
}

// Hàm nội bộ tính Feed-Forward
static float FF_Calc(float setpoint) {
    // Có thể thay bằng một phương trình mô hình toán phức tạp hơn nếu tải là động cơ (bù sức điện động - EMF)
    return setpoint * K_ff;
}

// Hàm gọi trong Ngắt DMA ADC (chu kỳ 40us / 25kHz)
void Control_UpdateLoop_ISR(void) {
    // CHỈ ĐIỀU KHIỂN KHI HỆ THỐNG ĐANG Ở TRẠNG THÁI RUN
    if (FSM_GetCurrentState() == STATE_RUN) {

        // 1. Đọc giá trị hồi tiếp (Kênh 0: Dòng điện)
        float i_feedback = current_mean;

        // 2. Tính toán sai số e(k)
        float error = current_setpoint - i_feedback;

        // 3. Khâu Tích phân I (Integral) - Tích lũy sai số
        integral += error * DT;

        // 4. Khâu Vi phân D (Derivative)
        float derivative = (error - prev_error) / DT;

        // 5. Khâu Tỷ lệ P (Proportional)
        float proportional = Kp * error;

        // 6. Tính đầu ra bộ PID: u_pid(k)
        float pid_out = proportional + (Ki * integral) + (Kd * derivative);

        // 7. Tính đầu ra Feed-Forward: u_ff(k)
        float ff_out = FF_Calc(current_setpoint);

        // 8. Tổng hợp tín hiệu điều khiển: u(k) = u_pid(k) + u_ff(k)
        float total_duty = pid_out + ff_out;

        // 9. Khâu Bão hòa (Saturation) & Clamping Anti-Windup
        if (total_duty > DUTY_MAX) {
            total_duty = DUTY_MAX;
            // Nếu đã chạm trần, ngừng tích lũy I theo chiều dương để tránh Windup
            integral -= error * DT;
        }
        else if (total_duty < DUTY_MIN) {
            total_duty = DUTY_MIN;
            // Nếu đã chạm đáy, ngừng tích lũy I theo chiều âm
            integral -= error * DT;
        }

        // Cập nhật biến trạng thái cho chu kỳ k+1
        prev_error = error;

        // 10. Xuất PWM
        PWM_SetDuty(total_duty);

    } else {
        // NẾU HỆ THỐNG CHUYỂN SANG FAULT HOẶC READY
        PWM_Stop();

        // Reset bộ tích phân (I) và sai số cũ (D) về 0.
        // Vô cùng quan trọng để tránh giật cục (Jolt) khi bấm Start lại
        integral = 0.0f;
        prev_error = 0.0f;
    }
}
