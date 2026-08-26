#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include "main.h"
#include <stdint.h>

/**
 * @brief Khởi tạo giao tiếp Modbus RTU (Kích hoạt nhận DMA lần đầu)
 */
void Modbus_Init(void);

/**
 * @brief Hàm callback được gọi khi ngắt UART IDLE xảy ra (đã nhận xong 1 frame)
 * @param size Kích thước data nhận được
 */
void Modbus_Rx_Callback(uint16_t size);

/**
 * @brief Hàm xử lý logic Modbus (Kiểm tra CRC, giải mã Setpoint, điều khiển FSM)
 * @note Hàm này phải được gọi liên tục trong vòng lặp while(1) của main.c
 */
void Modbus_Process(void);

/**
 * @brief Tính toán mã kiểm tra lỗi CRC16 chuẩn Modbus
 * @param buffer Con trỏ mảng dữ liệu cần tính
 * @param length Độ dài số byte cần tính
 * @return Giá trị CRC16 (16-bit)
 */
uint16_t Modbus_Calculate_CRC16(uint8_t *buffer, uint16_t length);

#endif /* MODBUS_RTU_H */
