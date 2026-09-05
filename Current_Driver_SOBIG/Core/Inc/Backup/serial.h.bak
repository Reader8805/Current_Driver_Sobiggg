#ifndef SERIAL_H_
#define SERIAL_H_

#include "main.h" // Khắc phục lỗi "GPIOA is undefined"
#include <stdint.h>
#include <string.h>

// ---------------- Cấu hình phần cứng ----------------
#define RS485_PORT GPIOA
#define RS485_DE_PIN GPIO_PIN_15

// ---------------- Cấu hình bản tin ------------------
#define FRAME_HEADER 0xAA
#define FRAME_FOOTER 0x0A
#define MAX_PAYLOAD_SIZE 64
#define RX_BUFFER_SIZE 128

// Định nghĩa mã lệnh (Command Code)
#define CMD_READ_STATUS     0x03    // Master yêu cầu đọc trạng thái
#define CMD_WRITE_CONTROL   0x10    // Master gửi lệnh điều khiển

#define RESP_READ_STATUS    0x83    // MCU phản hồi dữ liệu đọc (0x03 | 0x83)
#define RESP_WRITE_CONTROL  0x90    // MCU phản hồi xác nhận lệnh ghi (0x10 | 0x90)

// Định nghĩa ID cho MCU
#define SLAVE_ID 0x02

// Trạng thái máy trạng thái nhận
typedef enum {
    STATE_WAIT_HEADER = 0,
    STATE_ADDRESS,
    STATE_COMMAND,
    STATE_LENGTH,
    STATE_PAYLOAD,
    STATE_CHECKSUM,
    STATE_FOOTER
} RxState_t;

// Cấu trúc khung bản tin
typedef struct {
    uint8_t address;
    uint8_t command;
    uint8_t length;
    uint8_t payload[MAX_PAYLOAD_SIZE];
} CustomFrame_t;

extern volatile uint8_t frame_received_flag;
extern CustomFrame_t valid_frame_buffer;

// Biến buffer nhận DMA (khai báo extern để main có thể truy cập)
extern uint8_t rxDataBuffer[RX_BUFFER_SIZE];

// ---------------- Các hàm giao tiếp API --------------
void RS485_Init(UART_HandleTypeDef *huart);
void RS485_Send_Frame_DMA(UART_HandleTypeDef *huart, uint8_t addr, uint8_t cmd, uint8_t *data, uint8_t len);
void RS485_Parse_Byte(UART_HandleTypeDef *huart, uint8_t rxByte);
void RS485_RxEvent_Handler(UART_HandleTypeDef *huart, uint16_t Size);
void RS485_TxCplt_Handler(void);

#endif /* SERIAL_H_ */
