#include <rs485.h>
#include "serial.h"
#include "ad7606.h"
#include "fsm.h"
#include "control_loop.h"

// Đảm bảo bạn đã định nghĩa mã lệnh này trong serial.h (Ví dụ: #define CMD_BROADCAST_CONTROL 0x20)
#ifndef CMD_BROADCAST_CONTROL
#define CMD_BROADCAST_CONTROL 0x20 
#endif

static AppState_t app_state = APP_STATE_IDLE;
static uint32_t delay_start_time = 0;

void APP_RS485_Task(UART_HandleTypeDef *huart)
{
    static uint8_t target_state = 0;
    static float target_current = 0.0f;

    switch (app_state) {
        case APP_STATE_IDLE:
            if (frame_received_flag == 1)
            {
                // Bước 1: Kiểm tra đúng địa chỉ thiết bị (SLAVE_ID) hoặc địa chỉ Broadcast (0x00)
                if (valid_frame_buffer.address == SLAVE_ID || valid_frame_buffer.address == 0x00)
                {
                    // --- Tính toán Time-Slot để tránh các Slave tranh chấp đường truyền RS485 ---
                    if (valid_frame_buffer.address == 0x00)
                    {
                        // Phân khe thời gian: Slave 1 (5ms), Slave 2 (25ms), Slave 3 (45ms), Slave 4 (65ms)
                        tx_slot_delay = 5 + (SLAVE_ID - 1) * 20;
                    }
                    else {
                        tx_slot_delay = 5;
                    }

                    // ====================================================================
                    // --- CASE 0: Lệnh GHI ĐIỀU KHIỂN BROADCAST (Gộp 4 Slave) - Length = 12
                    // ====================================================================
                    if (valid_frame_buffer.command == CMD_BROADCAST_CONTROL && valid_frame_buffer.length == 12)
                    {
                        // Tính toán Offset (khoảng lùi) trong mảng Payload dựa vào SLAVE_ID
                        // Slave 1: offset = 0 | Slave 2: offset = 3 | Slave 3: offset = 6 | Slave 4: offset = 9
                        uint8_t offset = (SLAVE_ID - 1) * 3;

                        // 1. Lấy 1 byte trạng thái (Enable State)
                        target_state = valid_frame_buffer.payload[offset];

                        // 2. Lấy 2 byte giá trị dòng điện (Dòng I16 từ LabVIEW gửi High Byte trước)
                        uint16_t raw_unsigned = (valid_frame_buffer.payload[offset + 1] << 8) | valid_frame_buffer.payload[offset + 2];
                        
                        // Ép kiểu về int16_t để giữ lại dấu âm/dương (nếu có)
                        int16_t current_raw = (int16_t)raw_unsigned;
                        target_current = (float)current_raw / 100.0f;

                        // Cập nhật giá trị vào hệ thống
                        FSM_SetCommand(target_state);
                        Control_SetTarget(target_current);

                        delay_start_time = HAL_GetTick();
                        app_state = APP_STATE_WAIT_TX;
                    }
                    // ====================================================================
                    // --- CASE 1: Lệnh GHI điều khiển ĐƠN LẺ (0x10) - Bắt buộc Length = 5 
                    // ====================================================================
                    else if (valid_frame_buffer.command == CMD_WRITE_CONTROL && valid_frame_buffer.length == 5)
                    {
                        target_state = valid_frame_buffer.payload[0];
                        uint16_t raw_unsigned = (valid_frame_buffer.payload[3] << 8) | valid_frame_buffer.payload[4];
                        int16_t current_raw = (int16_t)raw_unsigned;
                        target_current = (float)current_raw / 100.0f;

                        FSM_SetCommand(target_state);
                        Control_SetTarget(target_current);

                        delay_start_time = HAL_GetTick();
                        app_state = APP_STATE_WAIT_TX;
                    }
                    // ====================================================================
                    // --- CASE 2: Lệnh ĐỌC trạng thái (0x03) - Bắt buộc Length = 0
                    // ====================================================================
                    else if (valid_frame_buffer.command == CMD_READ_STATUS && valid_frame_buffer.length == 0)
                    {
                        delay_start_time = HAL_GetTick();
                        app_state = APP_STATE_WAIT_TX;
                    }
                    else
                    {
                        frame_received_flag = 0;
                    }
                }
                else
                {
                    frame_received_flag = 0;
                }
            }
            break;

        case APP_STATE_WAIT_TX:
            // Sử dụng tx_slot_delay để các Slave xếp hàng trả lời lần lượt, không chặn CPU
            if ((HAL_GetTick() - delay_start_time) >= tx_slot_delay)
            {
                uint8_t resp[5];

                // --- PHẢN HỒI CHO LỆNH ĐỌC (0x03) hoặc LỆNH GHI BROADCAST (Báo cáo lại trạng thái) ---
                if (valid_frame_buffer.command == CMD_READ_STATUS || valid_frame_buffer.command == CMD_BROADCAST_CONTROL)
                {
                    uint8_t  state   = (uint8_t)FSM_GetCurrentState();     // 1 byte On/Off state
                    uint16_t temp    = 2850;  // 2 byte Nhiệt độ giả lập
                    uint16_t current = (uint16_t)(current_mean * 100.0f);  // 2 byte Dòng điện thực tế

                    resp[0] = state;
                    resp[1] = (temp >> 8) & 0xFF;
                    resp[2] = temp & 0xFF;
                    resp[3] = (current >> 8) & 0xFF;
                    resp[4] = current & 0xFF;

                    // Gửi mã phản hồi tùy thuộc vào lệnh gốc
                    uint8_t resp_cmd = (valid_frame_buffer.command == CMD_READ_STATUS) ? RESP_READ_STATUS : 0xA0; // 0xA0 giả sử là mã phản hồi Broadcast
                    
                    RS485_Send_Frame_DMA(huart, SLAVE_ID, resp_cmd, resp, 5);
                }
                // --- PHẢN HỒI CHO LỆNH GHI ĐƠN LẺ (Mã gửi về: 0x90) ---
                else if (valid_frame_buffer.command == CMD_WRITE_CONTROL)
                {
                    uint8_t  state   = (uint8_t)FSM_GetCurrentState();     
                    uint16_t temp    = 2850;  
                    uint16_t current = (uint16_t)(current_mean * 100.0f);
                    
                    resp[0] = state;
                    resp[1] = (temp >> 8) & 0xFF;
                    resp[2] = temp & 0xFF;
                    resp[3] = (current >> 8) & 0xFF;
                    resp[4] = current & 0xFF;
                    
                    RS485_Send_Frame_DMA(huart, SLAVE_ID, RESP_WRITE_CONTROL, resp, 5);
                }

                frame_received_flag = 0;
                app_state = APP_STATE_IDLE;
            }
            break;
    }
}