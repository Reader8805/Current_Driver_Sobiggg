#include "modbus_rtu.h"
#include "control_loop.h"
#include "pwm_control.h"
#include "fsm.h"
#include <string.h>

#define SLAVE_ID 0x01 // Đặt địa chỉ cho board này

extern UART_HandleTypeDef huart1;

uint8_t modbus_rx_buf[16]; // Buffer 16 bytes là đủ cho frame 8 bytes
volatile uint8_t modbus_frame_ready = 0;
volatile uint16_t modbus_rx_len = 0;

// ========================================================
// HÀM TÍNH CRC16 CHUẨN MODBUS RTU
// ========================================================
uint16_t Modbus_Calculate_CRC16(uint8_t *buffer, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)buffer[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}
// ========================================================

void Modbus_Init(void) {
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, modbus_rx_buf, sizeof(modbus_rx_buf));
}

void Modbus_Rx_Callback(uint16_t size) {
    modbus_rx_len = size;
    modbus_frame_ready = 1;
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, modbus_rx_buf, sizeof(modbus_rx_buf));
}

void Modbus_Process(void) {
    if (modbus_frame_ready) {
        // 1. Kiểm tra độ dài Frame (Bắt buộc phải là 8 bytes)
        if (modbus_rx_len == 8) {

            // 2. Tính và so sánh mã CRC16 (Tính 6 bytes đầu, so với 2 bytes cuối)
            uint16_t calculated_crc = Modbus_Calculate_CRC16(modbus_rx_buf, 6);
            uint16_t received_crc = (modbus_rx_buf[7] << 8) | modbus_rx_buf[6]; // Chú ý Endian (Byte thấp trước, Byte cao sau)

            // 3. Nếu CRC đúng và đúng địa chỉ Slave
            if ((calculated_crc == received_crc) && (modbus_rx_buf[0] == SLAVE_ID)) {

                uint8_t func_code = modbus_rx_buf[1];

                // --------------------------------------------------
                // LỆNH 1: ĐIỀU KHIỂN RUN / STOP / RESET LỖI (VD dùng func_code = 0x05)
                // --------------------------------------------------
                if (func_code == 0x05) {
                    // Giả sử byte 5 quyết định lệnh
                    uint8_t cmd_byte = modbus_rx_buf[5];

                    if (cmd_byte == 0x00) {        // LỆNH STOP
                        PWM_Stop();
                        FSM_SetState(STATE_READY);
                        Control_SetTarget(0.0f);
                    }
                    else if (cmd_byte == 0x01) { // LỆNH RUN
                        if (FSM_GetCurrentState() == STATE_READY) {
                            FSM_SetState(STATE_RUN);
                        }
                    }
                    else if (cmd_byte == 0x02) { // LỆNH RESET LỖI
                        if (FSM_GetCurrentState() == STATE_FAULT) {
                            // Check lại chân phần cứng xem đã hết lỗi chưa
                            if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) {
                                FSM_SetState(STATE_READY);
                            }
                        }
                    }
                }

                // --------------------------------------------------
                // LỆNH 2: GỬI SETPOINT 32-BIT FLOAT (VD dùng func_code = 0x10)
                // --------------------------------------------------
                else if (func_code == 0x10) {
                    // Gép 4 bytes (Byte 2, 3, 4, 5) thành 1 số Float 32-bit
                    // Cần chú ý thứ tự Byte (Byte Order) từ LabVIEW đẩy xuống là Big-Endian hay Little-Endian
                    // Dưới đây là kiểu Little-Endian (Thường dùng cho vi điều khiển ARM)
                    float target_val;
                    uint8_t *float_ptr = (uint8_t *)&target_val;

                    float_ptr[0] = modbus_rx_buf[2]; // LSB
                    float_ptr[1] = modbus_rx_buf[3];
                    float_ptr[2] = modbus_rx_buf[4];
                    float_ptr[3] = modbus_rx_buf[5]; // MSB

                    // Nạp setpoint cho thuật toán PID (Hỗ trợ cả âm dương để đảo chiều cầu H)
                    Control_SetTarget(target_val);
                }
            }
        }

        modbus_frame_ready = 0; // Giải phóng buffer chờ frame mới
    }
}
