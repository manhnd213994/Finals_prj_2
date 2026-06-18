#ifndef PMS7003_H
#define PMS7003_H

#include <stdio.h>
#include <stdint.h>
#include "driver/uart.h"
#include "esp_err.h"

// Cấu hình phần cứng mặc định cho PMS7003
#define PMS_UART_PORT      UART_NUM_1 
#define PMS_RX_PIN         16
#define PMS_TX_PIN         17

#define PACKET_SIZE        32    // Chiều dài gói tin tiêu chuẩn của PMS7003
#define TIMEOUT_MS         50    // Thời gian chờ đọc từng byte UART

uint16_t read_pm25(uart_port_t uart_num);

#endif // PMS7003_H