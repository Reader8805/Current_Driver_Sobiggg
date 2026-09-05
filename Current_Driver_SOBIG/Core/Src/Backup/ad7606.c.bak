#include "ad7606.h"

// Biến hspi1 từ main.c
extern SPI_HandleTypeDef hspi1;

// Buffer RAW nhận từ SPI
volatile int16_t adc_raw[8];
volatile float adc_voltage[8];

// Cần 1 buffer rỗng đẩy qua SPI để tạo clock cho quá trình nhận data
static uint16_t dummy_tx[8] = {0};

// Hàm delay ảo dùng NOP, tốn khoảng vài chục nano-giây, không làm nghẽn CPU
static inline void delay_short(void) {
    for(int i = 0; i < 15; i++) __NOP();
}

void AD7606_Init(void) {
    // Đảm bảo các chân điều khiển ở trạng thái chờ
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // CS High
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET); // CONVST High

    // Xung Reset module theo chuẩn datasheet
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET); // RST High
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET); // RST Low
    HAL_Delay(1);
}

void AD7606_Trigger(void) {
    // Tạo xung mức thấp kéo dài ít nhất 50ns để bắt đầu lấy mẫu đồng loạt
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET); // CONVST LOW
    delay_short();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);   // CONVST HIGH
}

void AD7606_Start_DMA_Read(void) {
    // Kéo chân Chip Select xuống để báo hiệu bắt đầu truyền data
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // CS LOW

    // Gọi hàm SPI truyền/nhận qua DMA. CPU hoàn toàn rảnh tay đi làm việc khác.
    HAL_SPI_TransmitReceive_DMA(&hspi1, (uint8_t*)dummy_tx, (uint8_t*)adc_raw, 8);
}

void AD7606_End_DMA_Read(void) {
    // DMA đọc xong sẽ gọi vào hàm này -> Kéo CS lên để nhả bus SPI
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // CS HIGH
}

void AD7606_Process_Data(void) {
    // Scale giá trị RAW sang Float. 5.0V ứng với dải 16-bit (-32768 đến 32767)
    for(int i = 0; i < 8; i++) {
        adc_voltage[i] = (float)adc_raw[i] * (5.0f / 32768.0f);
    }
}
