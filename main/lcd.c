#include <stdio.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_io_i2c.h"
#include "lcd.h"

// gửi dữ liệu sang pcf8574
void lcd_send(i2c_master_dev_handle_t dev_handle, uint8_t value, uint8_t mode) {
    uint8_t high_nibble = value & 0xF0;
    uint8_t low_nibble = (value << 4) & 0xF0;
    uint8_t data[4];

	//data 

    data[0] = high_nibble | mode | BACKLIGHT | PIN_EN;
    data[1] = high_nibble | mode | BACKLIGHT;

    data[2] = low_nibble | mode | BACKLIGHT | PIN_EN;
    data[3] = low_nibble | mode | BACKLIGHT;  

    i2c_master_transmit(dev_handle, data, 4, 50);
}

// Hàm gửi Lệnh (RS = 0)
void lcd_send_cmd(i2c_master_dev_handle_t dev_handle, uint8_t cmd) {
    lcd_send(dev_handle, cmd, 0);
}

// Hàm gửi Dữ liệu ký tự (RS = PIN_RS)
void lcd_send_data(i2c_master_dev_handle_t dev_handle, uint8_t data) {
    lcd_send(dev_handle, data, PIN_RS);
}

// Khởi tạo lcd
esp_err_t lcd_init(i2c_master_dev_handle_t dev_handle) {
    vTaskDelay(pdMS_TO_TICKS(50)); // Chờ màn hình ổn định nguồn
    
    // Trình tự khởi tạo bắt buộc cho chế độ 4-bit theo datasheet LCD
    lcd_send_cmd(dev_handle, 0x03); vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_cmd(dev_handle, 0x03); vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_cmd(dev_handle, 0x03);
    lcd_send_cmd(dev_handle, 0x02); // Chuyển hẳn sang chế độ 4-bit

    lcd_send_cmd(dev_handle, 0x28); // Giao tiếp 4-bit, 2 hàng, font 5x8
    lcd_send_cmd(dev_handle, 0x0C); // Bật màn hình, tắt con trỏ nhấp nháy
    lcd_send_cmd(dev_handle, 0x06); // Tự động tăng con trỏ dịch phải
    lcd_send_cmd(dev_handle, 0x01); // Xóa toàn bộ màn hình
    vTaskDelay(pdMS_TO_TICKS(2));
    
    return ESP_OK;
}




//put ở vị trí cụ thể trên mh
void lcd_put_str(i2c_master_dev_handle_t dev_handle,uint8_t col, uint8_t row, const char *str) {
	uint8_t base_addr = (row == 0) ? LCD_LINE_1 : LCD_LINE_2;
	if (row > 1)  row = 1;
	if (col > 15) col = 15;
	lcd_send_cmd(dev_handle, base_addr + col);
    while (*str) { //loop đến khi gặp "\0"
        lcd_send_data(dev_handle, (uint8_t)(*str));
        str++; 
    }
}