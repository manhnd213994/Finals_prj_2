#ifndef BME280_H
#define BME280_H

#include <stdio.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

// cấu hình phần cứng
#define BME280_SENSOR_ADDR       0x76         // Địa chỉ I2C của BME280 (Hoặc 0x77)

// Địa chỉ thanh ghi cấu hình và dữ liệu
#define BME280_REG_ID            0xD0        // Chứa ID của cảm biến
#define BME280_REG_CTRL_HUM      0xF2        // Cấu hình oversampling của độ ẩm
#define BME280_REG_CTRL_MEAS     0xF4        // Cấu hình oversampling T/P và Mode
#define BME280_REG_DATA          0xFA        // Bắt đầu dải dữ liệu thô (Temp + Hum)

// Biến toàn cục t_fine dùng chung giữa hàm tính Nhiệt độ và Độ ẩm
extern int32_t t_fine;

esp_err_t bme_init(i2c_master_dev_handle_t dev_handle);

esp_err_t bme280_read_raw(i2c_master_dev_handle_t dev_handle, int32_t *adc_T, int32_t *adc_H);

double bme_read_temp(i2c_master_dev_handle_t dev_handle, int32_t adc_T);

double bme_read_hum(i2c_master_dev_handle_t dev_handle, int32_t adc_H);

#endif // BME280_H