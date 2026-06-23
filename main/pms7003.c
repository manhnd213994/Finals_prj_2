#include <stdio.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "pms7003.h"


uint16_t read_pm25 (uart_port_t uart_num){
    size_t available_bytes = 0;
    
    // Kiểm tra bộ đệm đang tích lũy bao nhiêu byte
    uart_get_buffered_data_len(uart_num, &available_bytes);
    
    // Nếu ít hơn 32 byte, chưa đủ 1 packet
    if (available_bytes < 32) {
        return (uint16_t)0xFFFF; 
    }

    uint8_t single_byte = 0;
    uint8_t remaining_buf[30];
    bool header_found = false;


    for (size_t i = 0; i < (available_bytes - 31); i++) {
        // Đọc ra 1 byte từ đỉnh FIFO
        if (uart_read_bytes(uart_num, &single_byte, 1, pdMS_TO_TICKS(0)) <= 0) {
            break; // Thoát vòng lặp để xử lý tình huống không tìm thấy gói
        }

        // Phát hiện thấy byte khởi đầu 0x42
        if (single_byte == 0x42) {
            // Đọc tiếp byte thứ 2
            if (uart_read_bytes(uart_num, &single_byte, 1, pdMS_TO_TICKS(0)) > 0) {
                if (single_byte == 0x4D) {
                    // Khớp cặp 0x42 0x4D! Đọc nốt 30 byte còn lại
                    int read_len = uart_read_bytes(uart_num, remaining_buf, 30, pdMS_TO_TICKS(10));
                    if (read_len == 30) {
                        header_found = true;
                        break; // ĐÃ TÌM THẤY GÓI HỢP LỆ -> THOÁT KHỎI FOR NGAY
                    }
                }
            }
        }
    }


    // 3. Nếu đã chạy hết vòng lặp FOR (hoặc bị break sớm) mà không tìm thấy gói hợp lệ
    if (!header_found) {
        // Xoá bộ đệm nếu rác tích tụ quá nhiều
        if (available_bytes > 512) {
            uart_flush_input(uart_num);
        }
        return (uint16_t)0xFFFF; // Trả về mã lỗi nếu quét hết đệm mà thất bại
    }

    // 4. Tiến hành tính toán và xác thực Checksum cho gói tin đã tìm được
    uint16_t calculated_checksum = 0x42 + 0x4D;
    for (int i = 0; i < 28; i++) {
        calculated_checksum += remaining_buf[i];
    }
    
    uint16_t pms_checksum = (remaining_buf[28] << 8) | remaining_buf[29];

    if (calculated_checksum != pms_checksum) {
        return (uint16_t)0xFFFF; // Sai checksum, hủy gói
    }

    // 5. Trả về kết quả PM2.5 chuẩn
    uint16_t pm25_value = (remaining_buf[10] << 8) | remaining_buf[11];
    return pm25_value;
}