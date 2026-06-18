#ifndef LCD_H
#define LCD_H

#include <stdio.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_lcd_io_i2c.h"
// cấu hình phần cứng
//địa chỉ của module i2c
#define LCD_ADDR       0x4D     // Từ 0x40 -> 0x4D (0 1 0 0 A2 A1 A0 0); bit cuối luôn là 0 do chỉ ghi 
//địa chỉ bộ nhớ DRAM của LCD
#define LCD_LINE_1              0x80   
#define LCD_LINE_2              0xC0

// Các bit điều khiển nối từ PCF8574 sang LCD
#define PIN_RS                  (1 << 0)
#define PIN_RW                  (1 << 1)
#define PIN_EN                  (1 << 2)
#define BACKLIGHT               (1 << 3)


esp_err_t lcd_init(i2c_master_dev_handle_t dev_handle);

// Chọn vị trí con trỏ trên LCD, col là cột (0,15), row là hàng (0,1)
void lcd_gotoxy(i2c_master_dev_handle_t dev_handle, uint8_t col, uint8_t row);

// In ra một chuỗi ký tự tính từ vị trí của con trỏ
void lcd_put_str(i2c_master_dev_handle_t dev_handle, const char *str);

#endif //LCD_H