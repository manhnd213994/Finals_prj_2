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
#define LCD_ADDR       0x27     
//địa chỉ bộ nhớ DRAM của LCD
#define LCD_LINE_1              0x80   
#define LCD_LINE_2              0xC0

// Các bit điều khiển nối từ PCF8574 sang LCD
#define PIN_RS                  (1 << 0)
#define PIN_RW                  (1 << 1)
#define PIN_EN                  (1 << 2)
#define BACKLIGHT               (1 << 3)

void lcd_send(i2c_master_dev_handle_t dev_handle, uint8_t value, uint8_t mode);

void lcd_send_cmd(i2c_master_dev_handle_t dev_handle, uint8_t cmd);

void lcd_send_data(i2c_master_dev_handle_t dev_handle, uint8_t data);

esp_err_t lcd_init(i2c_master_dev_handle_t dev_handle);

void lcd_put_str(i2c_master_dev_handle_t dev_handle,uint8_t col, uint8_t row, const char *str);


#endif //LCD_H