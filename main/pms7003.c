#include <stdio.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "pms7003.h"

uint16_t read_pm25 (uart_port_t uar_num){
	uint8_t buf[SIZE];
    uint8_t *read = buf; //địa chỉ thanh ghi đang đọc
	while (1) {

		int len = uart_read_bytes(uart_num, read, 1, pdMS_TO_TICKS(TIMEOUT_MS));

		if (len <= 0) 
			return 0; // ko có dữ liệu
		
		if (*read == 0x42){
			read ++;
			len = uart_read_bytes(uart_num, read, 1, pdMS_TO_TICKS(TIMEOUT_MS));
			if (len > 0 && *read == 0x4D) { 
				read++; 
				int bytes_to_read = PACKET_SIZE - 2; //chưa đọc
				int bytes_read = uart_read_bytes(uart_num, read, bytes_to_read, pdMS_TO_TICKS(TIMEOUT_MS)); //đã đọc
				
				if (bytes_read < bytes_to_read) 
					return 0xFFFF; // Thiếu dữ liệu

				// 4. Tính toán và kiểm tra Checksum
				uint16_t calculated_checksum = 0;
				for (int i = 0; i < PACKET_SIZE - 2; i++) {
					calculated_checksum += *(buf + i); // tự tính checksum
				}

				uint16_t received_checksum = ((uint16_t)buf[30] << 8) | buf[31]; //checksum do cảm biến gửi

				if (calculated_checksum != received_checksum) // Sai checksum
					return 0xFFFF;
					
				uint16_t pm25 = ((uint16_t)*(buf + 10) << 8) | *(buf + 11);
				return pm25;
			}
			read = buf; // Nếu ko đúng start byte reset lại vị trí
		}
	}
}