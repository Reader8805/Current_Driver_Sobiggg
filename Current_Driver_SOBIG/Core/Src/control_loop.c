//#include "control_loop.h"
//#include "ad7606.h"
//#include "pwm_control.h"
//#include "fsm.h"
//
//// --- CÁC THÔNG SỐ CỦA BỘ ĐIỀU KHIỂN ---
//// Tần số trích mẫu 25kHz -> Ts = 40us
//#define DT 0.00004f
//
//// Hệ số PID (Cần dò (tuning) trên mô hình thực tế)
//static float Kp = 0.05f;
//static float Ki = 0.0f;
//static float Kd = 0.0f;
//
//// Hệ số Feed-Forward
//// Với tải thuần trở-cảm (R-L), V = R*I + L*di/dt. Ở chế độ tĩnh, V_ff = R*I_setpoint.
//static float K_ff = 0.045833f; //R 2.2ohm
//
//// Các biến trạng thái của bộ điều khiển số
//static float integral = 0.0f;
//static float prev_error = 0.0f;
//float total_duty = 0;
//// Giới hạn bão hòa đầu ra (Saturation)
//// Đặt max 0.95 (95%) để chừa lại dead-time cho phần cứng mạch cầu (nếu có)
//#define DUTY_MAX  0.95f
//#define DUTY_MIN -0.95f
//
//// Biến lưu giá trị mong muốn (Setpoint)
//volatile float current_setpoint = 0.0f;
//
//void Control_SetTarget(float target) {
//    current_setpoint = target;
//}
//
//// Hàm nội bộ tính Feed-Forward
//static float FF_Calc(float setpoint) {
//    // Có thể thay bằng một phương trình mô hình toán phức tạp hơn nếu tải là động cơ (bù sức điện động - EMF)
//    return setpoint * K_ff;
//}
//
//// Hàm gọi trong Ngắt DMA ADC (chu kỳ 40us / 25kHz)
//void Control_UpdateLoop_ISR(void) {
//    // CHỈ ĐIỀU KHIỂN KHI HỆ THỐNG ĐANG Ở TRẠNG THÁI RUN
//    if (FSM_GetCurrentState() == STATE_RUN) {
//
//        // 1. Đọc giá trị hồi tiếp (Kênh 0: Dòng điện)
//        float i_feedback = current_mean;
//
//        // 2. Tính toán sai số e(k)
//        float error = current_setpoint - i_feedback;
//
//        // 3. Khâu Tích phân I (Integral) - Tích lũy sai số
//        integral += error * DT;
//
//        // 4. Khâu Vi phân D (Derivative)
//        float derivative = (error - prev_error) / DT;
//
//        // 5. Khâu Tỷ lệ P (Proportional)
//        float proportional = Kp * error;
//
//        // 6. Tính đầu ra bộ PID: u_pid(k)
//        float pid_out = proportional + (Ki * integral) + (Kd * derivative);
//
//        // 7. Tính đầu ra Feed-Forward: u_ff(k)
//        float ff_out = FF_Calc(current_setpoint);
//
//        // 8. Tổng hợp tín hiệu điều khiển: u(k) = u_pid(k) + u_ff(k)
//        total_duty = pid_out + ff_out;
//
//        // 9. Khâu Bão hòa (Saturation) & Clamping Anti-Windup
//        if (total_duty > DUTY_MAX) {
//            total_duty = DUTY_MAX;
//            // Nếu đã chạm trần, ngừng tích lũy I theo chiều dương để tránh Windup
//            integral -= error * DT;
//        }
//        else if (total_duty < DUTY_MIN) {
//            total_duty = DUTY_MIN;
//            // Nếu đã chạm đáy, ngừng tích lũy I theo chiều âm
//            integral -= error * DT;
//        }
//
//        // Cập nhật biến trạng thái cho chu kỳ k+1
//        prev_error = error;
//
//        // 10. Xuất PWM
//        PWM_SetDuty(total_duty);
//
//    } else {
//        // NẾU HỆ THỐNG CHUYỂN SANG FAULT HOẶC READY
//        PWM_Stop();
//
//        // Reset bộ tích phân (I) và sai số cũ (D) về 0.
//        // Vô cùng quan trọng để tránh giật cục (Jolt) khi bấm Start lại
//        integral = 0.0f;
//        prev_error = 0.0f;
//    }
//}
// #include "control_loop.h"
// #include "ad7606.h"
// #include "pwm_control.h"
// #include "fsm.h"
// #include <math.h>

// // --- CÁC THÔNG SỐ CỦA BỘ ĐIỀU KHIỂN ---
// // Tần số trích mẫu 25kHz -> Ts = 40us
// #define DT 0.00004f
// #define PI 3.1415926535f

// // Hệ số PID (Cần dò (tuning) trên mô hình thực tế)
// static float Kp = 0.075f;
// static float Ki = 0.6f;
// static float Kd = 0.0f;

// // Hệ số Feed-Forward
// // Với tải thuần trở-cảm (R-L), V = R*I + L*di/dt. Ở chế độ tĩnh, V_ff = R*I_setpoint.
// static float K_ff = 0.045833f; //R 2.2ohm

// // --- CẤU HÌNH SLEW RATE LIMITER ---
// #define SLEW_RATE_A_PER_SEC 20.0f      // Tốc độ tăng dòng tối đa (50A/giây)
// static float internal_setpoint = 0.0f; // Setpoint "mềm" dùng cho PID để tránh sốc

// // Các biến trạng thái của bộ điều khiển số
// static float integral = 0.0f;
// static float prev_error = 0.0f;
// float total_duty = 0;

// // Giới hạn bão hòa đầu ra (Saturation)
// // Đặt max 0.95 (95%) để chừa lại dead-time cho phần cứng mạch cầu (nếu có)
// #define DUTY_MAX  0.95f
// #define DUTY_MIN -0.95f

// // Biến lưu giá trị mong muốn (Setpoint)
// volatile float current_setpoint = 0.0f;

// // --- THÊM CÁC BIẾN CHO SÓNG SINE ---
// static uint8_t sine_enabled = 0;
// static float sine_time = 0.0f;
// static float sine_amplitude = 2.0f; // Cài đặt biên độ mặc định (Ví dụ: 2 Ampe)
// static float sine_frequency = 5.0f; // Cài đặt tần số mặc định (Ví dụ: 5 Hz)

// // Hàm bật/tắt chế độ Sine từ RS485
// void Control_SetSineMode(uint8_t enable) {
//     sine_enabled = enable;
//     if (enable == 0) {
//         sine_time = 0.0f; // Reset góc pha khi tắt
//     }
// }

// void Control_SetTarget(float target) {
//     current_setpoint = target;
// }

// // Hàm nội bộ tính Feed-Forward
// static float FF_Calc(float setpoint) {
//     // Có thể thay bằng một phương trình mô hình toán phức tạp hơn nếu tải là động cơ (bù sức điện động - EMF)
//     return setpoint * K_ff;
// }

// void Control_UpdateLoop_ISR(void) {
//     if (FSM_GetCurrentState() == STATE_RUN) {
        
//         // --- 1. TỰ ĐỘNG SINH ĐIỂM SINE NẾU ĐƯỢC BẬT ---
//         if (sine_enabled) {
//             sine_time += DT;
//             // Reset thời gian để chống tràn biến float khi chạy lâu
//             if (sine_time > (1.0f / sine_frequency)) {
//                 sine_time -= (1.0f / sine_frequency); 
//             }
//             // Tính toán giá trị dòng điện mong muốn tại thời điểm t
//             current_setpoint = sine_amplitude * sinf(2.0f * PI * sine_frequency * sine_time);
//         }

//         // --- 2. XỬ LÝ SLEW RATE LIMITER ---
//         if (sine_enabled) {
//             // NẾU CHẠY SINE: Phải bỏ qua Slew Rate để dòng bám kịp tốc độ thay đổi của sóng
//             internal_setpoint = current_setpoint; 
//         } else {
//             // NẾU CHẠY TAY: Dùng Slew Rate như cũ để chống sốc
//             float max_step = SLEW_RATE_A_PER_SEC * DT;
//             if (internal_setpoint < current_setpoint) {
//                 internal_setpoint += max_step;
//                 if (internal_setpoint > current_setpoint) internal_setpoint = current_setpoint;
//             }
//             else if (internal_setpoint > current_setpoint) {
//                 internal_setpoint -= max_step;
//                 if (internal_setpoint < current_setpoint) internal_setpoint = current_setpoint;
//             }
//         }

//         // --- CÁC BƯỚC PID GIỮ NGUYÊN ---
//         float i_feedback = current_mean;
//         float error = internal_setpoint - i_feedback;
//         integral += error * DT;
//         float derivative = (error - prev_error) / DT;
//         float proportional = Kp * error;
//         float pid_out = proportional + (Ki * integral) + (Kd * derivative);
//         float ff_out = FF_Calc(internal_setpoint);
        
//         total_duty = pid_out + ff_out;

//         if (total_duty > DUTY_MAX) {
//             total_duty = DUTY_MAX;
//             integral -= error * DT;
//         }
//         else if (total_duty < DUTY_MIN) {
//             total_duty = DUTY_MIN;
//             integral -= error * DT;
//         }

//         prev_error = error;
//         PWM_SetDuty(total_duty);

//     } else {
//         PWM_Stop();
//         integral = 0.0f;
//         prev_error = 0.0f;
//         sine_time = 0.0f; // Dừng hệ thống thì reset góc pha Sine
//         internal_setpoint = current_mean;
//     }
// }

#include "control_loop.h"
#include "ad7606.h"
#include "pwm_control.h"
#include "fsm.h"
#include <math.h>     // <-- BẮT BUỘC THÊM ĐỂ DÙNG sinf()

#define PI 3.1415926535f
#define DT 0.00004f   // Chu kỳ PID 40us (25kHz)

// --- CÁC THÔNG SỐ BỘ ĐIỀU KHIỂN ---
static float Kp = 0.075f;
static float Ki = 0.6f;
static float Kd = 0.0f;
static float K_ff = 0.045833f; 

#define SLEW_RATE_A_PER_SEC 20.0f      
static float internal_setpoint = 0.0f; 

static float integral = 0.0f;
static float prev_error = 0.0f;
float total_duty = 0;

#define DUTY_MAX  0.95f
#define DUTY_MIN -0.95f

volatile float current_setpoint = 0.0f;

// --- BIẾN CHO CHẾ ĐỘ SINE RỜI RẠC ---
static uint8_t sine_enabled = 0;
static float sine_time = 0.0f;
static float sine_amplitude = 0.0f; 
static float sine_frequency = 0.0f; 
static uint8_t sine_points = 0;

// Hàm cập nhật cấu hình Sine từ RS485
void Control_SetSineMode(uint8_t enable, uint8_t amp, uint8_t freq, uint8_t points) {
    sine_enabled = enable;
    if (enable) {
        // Lưu ý: Đang ép kiểu 1 byte = 1 Ampe (vd gửi 5 = 5A). 
        // Nếu bạn muốn gửi số thập phân từ LabVIEW, hãy sửa thành: (float)amp / 10.0f
        sine_amplitude = (float)amp; 
        sine_frequency = (float)freq;
        sine_points = points;
    } else {
        sine_time = 0.0f; 
    }
}

void Control_SetTarget(float target) {
    if (sine_enabled == 0) { // Chỉ nhận Setpoint chỉnh tay nếu đang TẮT Sine
        current_setpoint = target;
    }
}

static float FF_Calc(float setpoint) {
    return setpoint * K_ff;
}

void Control_UpdateLoop_ISR(void) {
    if (FSM_GetCurrentState() == STATE_RUN) {
        
        // --- 1. TỰ ĐỘNG SINH ĐIỂM SINE THEO SAMPLING RATE ---
        if (sine_enabled && sine_frequency > 0.0f && sine_points > 0) {
            sine_time += DT;
            
            // Chống tràn biến thời gian (Reset lại sau mỗi 1 chu kỳ)
            float period = 1.0f / sine_frequency;
            if (sine_time >= period) {
                sine_time -= period; 
            }
            
            // Tìm xem thời điểm t hiện tại đang rơi vào "điểm" (step) thứ mấy trong chu kỳ
            // Công thức: index = (t / T) * Số_Điểm
            uint32_t current_step = (uint32_t)((sine_time / period) * sine_points) % sine_points;
            
            // Tính Setpoint tương ứng cho step hiện tại
            current_setpoint = sine_amplitude * sinf(2.0f * PI * (float)current_step / (float)sine_points);
        }

        // --- 2. XỬ LÝ SLEW RATE LIMITER ---
        if (sine_enabled) {
            // Ở chế độ Sine, bỏ qua (Bypass) Slew Rate để bám sát tốc độ của điểm ảnh
            internal_setpoint = current_setpoint; 
        } else {
            // Ở chế độ Manual, dùng Slew Rate để tránh Jolt (sốc dòng)
            float max_step = SLEW_RATE_A_PER_SEC * DT;
            if (internal_setpoint < current_setpoint) {
                internal_setpoint += max_step;
                if (internal_setpoint > current_setpoint) internal_setpoint = current_setpoint;
            }
            else if (internal_setpoint > current_setpoint) {
                internal_setpoint -= max_step;
                if (internal_setpoint < current_setpoint) internal_setpoint = current_setpoint;
            }
        }

        // --- 3. TÍNH TOÁN PID THƯỜNG QUY ---
        float i_feedback = current_mean;
        float error = internal_setpoint - i_feedback;
        integral += error * DT;
        float derivative = (error - prev_error) / DT;
        float proportional = Kp * error;
        float pid_out = proportional + (Ki * integral) + (Kd * derivative);
        float ff_out = FF_Calc(internal_setpoint);
        
        total_duty = pid_out + ff_out;

        if (total_duty > DUTY_MAX) {
            total_duty = DUTY_MAX;
            integral -= error * DT;
        }
        else if (total_duty < DUTY_MIN) {
            total_duty = DUTY_MIN;
            integral -= error * DT;
        }

        prev_error = error;
        PWM_SetDuty(total_duty);

    } else {
        PWM_Stop();
        integral = 0.0f;
        prev_error = 0.0f;
        sine_time = 0.0f;
        total_duty = 0.0f;
//        current_setpoint = 0.0f;
        internal_setpoint = current_mean;
    }
}
