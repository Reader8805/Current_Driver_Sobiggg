#include "LCD.h"
#include <stdio.h> // Hỗ trợ hàm snprintf

extern I2C_HandleTypeDef hi2c1;
#define SLAVE_ADDRESS_LCD 0x4E
extern volatile float measured_current ;
/* =========================================================
 * HÀM ĐIỀU KHIỂN MÀN HÌNH LCD (Đã sửa lỗi thiếu ngoặc)
 * ========================================================= */
void lcd_send_cmd (char cmd) {
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (cmd & 0xf0);
    data_l = ((cmd << 4) & 0xf0);
    data_t[0] = data_u | 0x0C;  // en=1, rs=0
    data_t[1] = data_u | 0x08;  // en=0, rs=0
    data_t[2] = data_l | 0x0C;  // en=1, rs=0
    data_t[3] = data_l | 0x08;  // en=0, rs=0
    HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDRESS_LCD, (uint8_t *) data_t, 4, 100);
}

void lcd_send_data (char data) {
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (data & 0xf0);
    data_l = ((data << 4) & 0xf0);
    data_t[0] = data_u | 0x0D;  // en=1, rs=1
    data_t[1] = data_u | 0x09;  // en=0, rs=1
    data_t[2] = data_l | 0x0D;  // en=1, rs=1
    data_t[3] = data_l | 0x09;  // en=0, rs=1
    HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDRESS_LCD, (uint8_t *) data_t, 4, 100);
}

void lcd_clear (void) {
    lcd_send_cmd(0x01);
    HAL_Delay(2);
}

void lcd_put_cur(int row, int col) {
    switch (row) {
        case 0: col |= 0x80; break;
        case 1: col |= 0xC0; break;
    }
    lcd_send_cmd(col);
}

void lcd_init (void) {
    HAL_Delay(50);
    lcd_send_cmd(0x30); HAL_Delay(5);
    lcd_send_cmd(0x30); HAL_Delay(1);
    lcd_send_cmd(0x30); HAL_Delay(10);
    lcd_send_cmd(0x20); HAL_Delay(10);
    lcd_send_cmd(0x28); HAL_Delay(1);
    lcd_send_cmd(0x08); HAL_Delay(1);
    lcd_send_cmd(0x01); HAL_Delay(1);
    lcd_send_cmd(0x06); HAL_Delay(1);
    lcd_send_cmd(0x0C);
}

void lcd_send_string (char *str) {
    while (*str) lcd_send_data(*str++);
}

/* =========================================================
 * LOGIC GIAO DIỆN & STATE MACHINE
 * ========================================================= */

void UI_System_Init(void) {
    lcd_init();
    lcd_clear();
}

void UI_System_Update(SystemState_t current_state, float measured_current) {
    char lcd_buffer[16];

    // 1. Cập nhật Dòng 1: Hiển thị Trạng thái (State)
    lcd_put_cur(0, 0); 
    switch (current_state) {
        case STATE_INIT:
            // Khoảng trắng ở đuôi để xóa sạch các ký tự cũ
            lcd_send_string("STATE: INIT     "); 
            break;
        case STATE_READY:
            lcd_send_string("STATE: READY    ");
            break;
        case STATE_RUN:
            lcd_send_string("STATE: RUN      ");
            break;
        case STATE_FAULT:
            lcd_send_string("STATE: FAULT    ");
            break;
        default:
            lcd_send_string("STATE: UNKNOWN  ");
            break;
    }

    // 2. Cập nhật Dòng 2: Hiển thị Dòng điện
    lcd_put_cur(1, 0);
    
    // Chỉ hiển thị dòng điện nếu đang ở trạng thái READY, RUN, hoặc FAULT
    if (current_state == STATE_READY || current_state == STATE_RUN || current_state == STATE_FAULT) {
        // Format chuỗi hiển thị dòng điện. 
        // Ví dụ: "Current: 12.54 A" hoặc để gọn hơn "I: 12.54 A      "
        snprintf(lcd_buffer, sizeof(lcd_buffer), "I: %+06.2f A    ", measured_current);
        lcd_send_string(lcd_buffer);
    } 
    else {
        // Ở trạng thái INIT, dòng 2 hiển thị khoảng trắng để giấu thông số
        lcd_send_string("                "); 
    }
}
