#include "RS485_transmit_func.h"
#include "serial.h" 
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
                // Bước 1: Kiểm tra đúng địa chỉ thiết bị (SLAVE_ID)
                if (valid_frame_buffer.address == SLAVE_ID) 
                {
                    // --- CASE 1: Lệnh GHI điều khiển (0x10) - Bắt buộc Length = 5 ---
                    if (valid_frame_buffer.command == CMD_WRITE_CONTROL && valid_frame_buffer.length == 5)
                    {
                        target_state = valid_frame_buffer.payload[0];
                        uint16_t current_raw = (valid_frame_buffer.payload[3] << 8) | valid_frame_buffer.payload[4];
                        target_current = (float)current_raw / 100.0f; 

                        delay_start_time = HAL_GetTick(); 
                        app_state = APP_STATE_WAIT_TX;    
                    }
                    // --- CASE 2: Lệnh ĐỌC trạng thái (0x03) - Bắt buộc Length = 0 ---
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
            // Đợi 5ms không chặn CPU
            if ((HAL_GetTick() - delay_start_time) >= 5)
            {
                uint8_t resp[5];

                // --- PHẢN HỒI CHO LỆNH ĐỌC (Mã gửi về: 0x83) ---
                if (valid_frame_buffer.command == CMD_READ_STATUS)
                {
                    uint8_t  state   = 1;     // 1 byte On/Off state 
                    uint16_t temp    = 2850;  // 2 byte Nhiệt độ
                    uint16_t current = 1205;  // 2 byte Dòng điện
                    
                    resp[0] = state;                      
                    resp[1] = (temp >> 8) & 0xFF;         
                    resp[2] = temp & 0xFF;                
                    resp[3] = (current >> 8) & 0xFF;      
                    resp[4] = current & 0xFF;  

                    // Truyền huart vào hàm gửi để module không bị phụ thuộc vào biến toàn cục huart1
                    RS485_Send_Frame_DMA(huart, SLAVE_ID, RESP_READ_STATUS, resp, 5);
                }
                // --- PHẢN HỒI CHO LỆNH GHI (Mã gửi về: 0x90) ---
                else if (valid_frame_buffer.command == CMD_WRITE_CONTROL)
                {
                    resp[0] = valid_frame_buffer.payload[0];
                    resp[1] = valid_frame_buffer.payload[1];
                    resp[2] = valid_frame_buffer.payload[2];
                    resp[3] = valid_frame_buffer.payload[3];
                    resp[4] = valid_frame_buffer.payload[4];
                    
                    RS485_Send_Frame_DMA(huart, SLAVE_ID, RESP_WRITE_CONTROL, resp, 5);
                }
                
                frame_received_flag = 0; 
                app_state = APP_STATE_IDLE;
            }
            break;
    }
}