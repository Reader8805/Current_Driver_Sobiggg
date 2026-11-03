#include "LCD.h"
#include <stdio.h> // Hỗ trợ hàm snprintf

extern I2C_HandleTypeDef hi2c1;
#define SLAVE_ADDRESS_LCD 0x4E
extern volatile float measured_current ;
/* =========================================================
 * HÀM ĐIỀU KHIỂN MÀN HÌNH LCD (Đã sửa lỗi thiếu ngoặc)
 * ========================================================= */

static HAL_StatusTypeDef lcd_write_init_nibble(uint8_t nibble)
{
    uint8_t data = (uint8_t)((nibble & 0x0F) << 4);
    uint8_t tx[3];

    tx[0] = data | 0x08;  // EN=0, RW=0, RS=0, đèn nền bật
    tx[1] = data | 0x0C;  // EN=1
    tx[2] = data | 0x08;  // EN=0: LCD chốt dữ liệu

    return HAL_I2C_Master_Transmit(
        &hi2c1, SLAVE_ADDRESS_LCD, tx, sizeof(tx), 100);
}

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

//void lcd_init (void) {
//    HAL_Delay(50);
//    lcd_send_cmd(0x30); HAL_Delay(5);
//    lcd_send_cmd(0x30); HAL_Delay(1);
//    lcd_send_cmd(0x30); HAL_Delay(10);
//    lcd_send_cmd(0x20); HAL_Delay(10);
//    lcd_send_cmd(0x28); HAL_Delay(1);
//    lcd_send_cmd(0x08); HAL_Delay(1);
//    lcd_send_cmd(0x01); HAL_Delay(1);
//    lcd_send_cmd(0x06); HAL_Delay(1);
//    lcd_send_cmd(0x0C);
//}
void lcd_init(void)
{
    /* Cho nguồn LCD và backpack có thời gian ổn định.
     * Đây là khoảng chờ thử nghiệm, cần kiểm tra trên mạch thực tế.
     */
    HAL_Delay(500);

    /* Thử khởi tạo tối đa 3 lần nếu giao tiếp I2C báo lỗi. */
    for (uint8_t attempt = 0; attempt < 3; attempt++)
    {
        if (HAL_I2C_IsDeviceReady(
                &hi2c1, SLAVE_ADDRESS_LCD, 3, 100) != HAL_OK)
        {
            HAL_Delay(100);
            continue;
        }

        /* Đồng bộ LCD về trạng thái 8-bit bằng từng nibble riêng. */
        if (lcd_write_init_nibble(0x03) != HAL_OK)
            continue;
        HAL_Delay(5);

        if (lcd_write_init_nibble(0x03) != HAL_OK)
            continue;
        HAL_Delay(1);

        if (lcd_write_init_nibble(0x03) != HAL_OK)
            continue;
        HAL_Delay(1);

        /* Chuyển sang giao tiếp 4-bit. */
        if (lcd_write_init_nibble(0x02) != HAL_OK)
            continue;
        HAL_Delay(1);

        /* Từ đây, mỗi lệnh được gửi bằng 2 nibble. */
        const uint8_t commands[] = {
            0x28,  // 4-bit, 2 dòng, font 5x8
            0x08,  // Tắt hiển thị trong lúc cấu hình
            0x01,  // Xóa màn hình
            0x06,  // Tăng địa chỉ, không dịch màn hình
            0x0C   // Bật hiển thị, tắt con trỏ và nhấp nháy
        };

        uint8_t success = 1;

        for (uint8_t i = 0; i < sizeof(commands); i++)
        {
            if (lcd_write_init_nibble(commands[i] >> 4) != HAL_OK)
            {
                success = 0;
                break;
            }

            if (lcd_write_init_nibble(commands[i] & 0x0F) != HAL_OK)
            {
                success = 0;
                break;
            }

            HAL_Delay((commands[i] == 0x01) ? 3 : 1);
        }

        if (success)
            return;

        HAL_Delay(100);
    }

    /* Nếu cả 3 lần thất bại, đặt breakpoint tại đây để kiểm tra:
     * hi2c1.ErrorCode và nguồn/SDA/SCL.
     */
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
