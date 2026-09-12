#include "fsm.h"
#include "pwm_control.h"
#include "ad7606.h"

// Trạng thái ban đầu
static SystemState_t currentState = STATE_INIT;
// Lệnh từ LabVIEW: 0 = STOP, 1 = RUN, 2 = RESET FAULT
static uint8_t labview_cmd = 0;

// --- CÁC NGƯỠNG BẢO VỆ AN TOÀN ---
#define MAX_CURRENT_LIMIT   10.0f   // Dòng điện tối đa (Ampe)
#define MAX_TEMP_LIMIT      80.0f   // Nhiệt độ tối đa (Độ C)

void FSM_Init(void) {
    currentState = STATE_INIT;
    labview_cmd = 0;
}

SystemState_t FSM_GetCurrentState(void) {
    return currentState;
}

void FSM_SetState(SystemState_t new_state) {
    currentState = new_state;
}

// RS485 gọi hàm này để truyền lệnh từ LabVIEW vào
void FSM_SetCommand(uint8_t cmd) {
    labview_cmd = cmd;
}

// Cập nhật State Machine liên tục trong while(1) của main
void FSM_Update(void) {

    // Đọc giá trị Feedback (đã scale ra số thực) từ mảng DMA ADC
    float current_feedback = current_mean;
    //float temp_feedback    = adc_voltage[1];

    // BẢO VỆ PHẦN CỨNG ƯU TIÊN CAO NHẤT (Luôn check ở mọi trạng thái)
    // Nếu nhiệt độ hoặc dòng điện vượt ngưỡng -> Ép vào trạng thái LỖI ngay lập tức
    if (currentState != STATE_FAULT && currentState != STATE_INIT) {
        if (current_feedback > MAX_CURRENT_LIMIT || current_feedback < -MAX_CURRENT_LIMIT) {
            PWM_Stop();
            labview_cmd = 0; // Xóa luôn lệnh chạy cũ từ LabVIEW cho an toàn
            currentState = STATE_FAULT;
        }
    }

    // Xử lý các trạng thái
    switch (currentState) {
        case STATE_INIT:
            // Khởi tạo ngoại vi xong, tự động chuyển sang READY
            PWM_Stop();
            currentState = STATE_READY;
            break;

        case STATE_READY:
            PWM_Stop(); // Đảm bảo phần cứng tắt hoàn toàn

            // Đợi lệnh Start (1) từ LabVIEW mới cho chạy
            if (labview_cmd == 1) {
                currentState = STATE_RUN;
            }
            break;

        case STATE_RUN:
            // Ghi chú: Ở trạng thái RUN, việc tính toán PID và xuất PWM
            // được thực hiện bên trong ngắt của hàm Control_UpdateLoop_ISR (control_loop.c)

            // Nếu LabVIEW gửi lệnh Stop (0)
            if (labview_cmd == 0) {
                PWM_Stop();
                currentState = STATE_READY;
            }
            break;

        case STATE_FAULT:
            // KHÓA CỨNG PWM KHẨN CẤP
            PWM_Stop();

            // Trong trạng thái lỗi, chỉ chấp nhận duy nhất lệnh Reset (2) từ LabVIEW
            if (labview_cmd == 0) {
                // Kiểm tra xem nhiệt độ đã hạ xuống ngưỡng an toàn chưa mới cho Reset
            	if ((HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET))
            	 {
            	   // Hệ thống đã an toàn, cho phép về trạng thái chờ
            		currentState = STATE_READY;
            	 }

            }
            // Nếu có lệnh Start (1) gửi xuống lúc này cũng sẽ bị lờ đi (Ignore)
            break;
    }
}
