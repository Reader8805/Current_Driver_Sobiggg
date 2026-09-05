#include "serial.h"

volatile uint8_t frame_received_flag = 0;
CustomFrame_t valid_frame_buffer;

// Biến nội bộ (chỉ file này nhìn thấy)
static RxState_t rxState = STATE_WAIT_HEADER;
static CustomFrame_t rxFrame;
static uint8_t payloadIndex = 0;
static uint8_t receivedChecksum = 0;

// Bộ đệm nhận của DMA
uint8_t rxDataBuffer[RX_BUFFER_SIZE];

// Hàm tính Checksum nội bộ
static uint8_t Calculate_Checksum(uint8_t addr, uint8_t cmd, uint8_t len, uint8_t* payload) {
    uint8_t checksum = addr ^ cmd ^ len;
    for (int i = 0; i < len; i++) {
        checksum ^= payload[i];
    }
    return checksum;
}

// Hàm khởi tạo ban đầu
void RS485_Init(UART_HandleTypeDef *huart) {
    // Đưa DE về mức THẤP (Chế độ nhận)
    HAL_GPIO_WritePin(RS485_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
    
    // Kích hoạt DMA Rx IDLE
    HAL_UARTEx_ReceiveToIdle_DMA(huart, rxDataBuffer, RX_BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT); 
}

// Hàm truyền DMA
// void RS485_Send_Frame_DMA(UART_HandleTypeDef *huart, uint8_t addr, uint8_t cmd, uint8_t *data, uint8_t len) {
//     static uint8_t txBuffer[MAX_PAYLOAD_SIZE + 6]; 
//     uint8_t idx = 0;

//     txBuffer[idx++] = FRAME_HEADER;
//     txBuffer[idx++] = addr;
//     txBuffer[idx++] = cmd;
//     txBuffer[idx++] = len;

//     for(int i = 0; i < len; i++) {
//         txBuffer[idx++] = data[i];
//     }

//     txBuffer[idx++] = Calculate_Checksum(addr, cmd, len, data);
//     txBuffer[idx++] = FRAME_FOOTER;

//     // Kéo DE lên mức CAO để phát
//     HAL_GPIO_WritePin(RS485_PORT, RS485_DE_PIN, GPIO_PIN_SET);
//     HAL_UART_Transmit_DMA(huart, txBuffer, idx);
// }

void RS485_Send_Frame_DMA(UART_HandleTypeDef *huart, uint8_t addr, uint8_t cmd, uint8_t *data, uint8_t len) {
    // Ngăn chặn truyền nếu UART đang bận
    if (huart->gState != HAL_UART_STATE_READY) {
        return; 
    }

    static uint8_t txBuffer[MAX_PAYLOAD_SIZE + 6]; 
    uint8_t idx = 0;

    txBuffer[idx++] = FRAME_HEADER;
    txBuffer[idx++] = addr;
    txBuffer[idx++] = cmd;
    txBuffer[idx++] = len;

    for(int i = 0; i < len; i++) {
        txBuffer[idx++] = data[i];
    }

    txBuffer[idx++] = Calculate_Checksum(addr, cmd, len, data);
    txBuffer[idx++] = FRAME_FOOTER;

    // Kéo DE lên mức CAO để phát
    HAL_GPIO_WritePin(RS485_PORT, RS485_DE_PIN, GPIO_PIN_SET);
    HAL_UART_Transmit_DMA(huart, txBuffer, idx);
}

// Xử lý bản tin hợp lệ (Logic ứng dụng của bạn đặt ở đây)
// static void Process_Valid_Frame(UART_HandleTypeDef *huart, CustomFrame_t *frame) {
//     // uint8_t responsePayload[MAX_PAYLOAD_SIZE];
//     // uint8_t respLen = 0;

//     // if (frame->address == 0x01) { 
//     //     if (frame->command == 0x10) { 
//     //         responsePayload[0] = 0x4F; // 'O'
//     //         responsePayload[1] = 0x4B; // 'K'
            
//     //         if(frame->length > 0) {
//     //             responsePayload[2] = frame->payload[0];
//     //             respLen = 3;
//     //         } else {
//     //             respLen = 2;
//     //         }

//     //         RS485_Send_Frame_DMA(huart, 0x01, 0x90, responsePayload, respLen);
//     //     }
//     // }

//     memcpy(&valid_frame_buffer, frame, sizeof(CustomFrame_t));

//     frame_received_flag = 1;
// }
static void Process_Valid_Frame(UART_HandleTypeDef *huart, CustomFrame_t *frame) {
    // Chỉ chép bản tin mới khi hàm main đã xử lý xong bản tin cũ (flag = 0)
    if (frame_received_flag == 0) {
        memcpy(&valid_frame_buffer, frame, sizeof(CustomFrame_t));
        frame_received_flag = 1;
    }
}

// Máy trạng thái
void RS485_Parse_Byte(UART_HandleTypeDef *huart, uint8_t rxByte) {
    switch (rxState) {
        case STATE_WAIT_HEADER:
            if (rxByte == FRAME_HEADER) rxState = STATE_ADDRESS;
            break;
            
        case STATE_ADDRESS:
            if (rxByte == SLAVE_ID || rxByte == 0xFF) { // 0xFF có thể dùng làm địa chỉ Broadcast (gửi cho tất cả)
                rxFrame.address = rxByte;
                rxState = STATE_COMMAND;
            } else {
                // Nếu không phải gửi cho mình, hủy bỏ luôn, không thèm nhận data hay tính checksum nữa
                rxState = STATE_WAIT_HEADER; 
            }
            break;
            
        case STATE_COMMAND:
            rxFrame.command = rxByte;
            rxState = STATE_LENGTH;
            break;
            
        case STATE_LENGTH:
            if (rxByte <= MAX_PAYLOAD_SIZE) {
                rxFrame.length = rxByte;
                payloadIndex = 0;
                rxState = (rxByte > 0) ? STATE_PAYLOAD : STATE_CHECKSUM;
            } else {
                rxState = STATE_WAIT_HEADER; 
            }
            break;
            
        case STATE_PAYLOAD:
            rxFrame.payload[payloadIndex++] = rxByte;
            if (payloadIndex >= rxFrame.length) rxState = STATE_CHECKSUM;
            break;
            
        case STATE_CHECKSUM:
            receivedChecksum = rxByte;
            rxState = STATE_FOOTER;
            break;
            
        case STATE_FOOTER:
            if (rxByte == FRAME_FOOTER) {
                uint8_t calcCS = Calculate_Checksum(rxFrame.address, rxFrame.command, rxFrame.length, rxFrame.payload);
                if (calcCS == receivedChecksum) {
                    Process_Valid_Frame(huart, &rxFrame);
                }
            }
            rxState = STATE_WAIT_HEADER; 
            break;
    }
}

// Hàm xử lý khi ngắt Rx Event (IDLE) xảy ra
void RS485_RxEvent_Handler(UART_HandleTypeDef *huart, uint16_t Size) {
    for (uint16_t i = 0; i < Size; i++) {
        RS485_Parse_Byte(huart, rxDataBuffer[i]); 
    }
    // Tái khởi động DMA
    HAL_UARTEx_ReceiveToIdle_DMA(huart, rxDataBuffer, RX_BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT);
}

// Hàm xử lý khi gửi xong (Tx Complete)
void RS485_TxCplt_Handler(void) {
    // Kéo DE xuống thấp
    HAL_GPIO_WritePin(RS485_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
}
