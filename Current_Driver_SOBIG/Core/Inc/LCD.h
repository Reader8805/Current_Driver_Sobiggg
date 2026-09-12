#ifndef _UI_SYSTEM_H_
#define _UI_SYSTEM_H_

#include "stm32f4xx_hal.h"
#include "fsm.h"
/* =========================================================
 * ĐỊNH NGHĨA CÁC TRẠNG THÁI CỦA HỆ THỐNG (STATE MACHINE)
 * ========================================================= */

/* =========================================================
 * CÁC HÀM GIAO TIẾP LCD CHUẨN
 * ========================================================= */
void lcd_init(void);
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
void lcd_send_string(char *str);
void lcd_put_cur(int row, int col);
void lcd_clear(void);

/* =========================================================
 * CÁC HÀM ỨNG DỤNG GIAO DIỆN (UI APP)
 * ========================================================= */
// Hàm khởi tạo UI ban đầu
void UI_System_Init(void);

// Hàm cập nhật màn hình dựa trên State Machine và giá trị dòng điện
void UI_System_Update(SystemState_t current_state, float measured_current);

#endif /* _UI_SYSTEM_H_ */
