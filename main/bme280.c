#include <stdio.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "bme280.h"

// Các hàm xử lý cho bme280

/*	Mẫu:
	uint8_t data_to_send = 0x01;
	uint8_t data_received[2];
	i2c_master_transmit(dev_handle_A, &data_to_send, 1, -1);
	i2c_master_receive(dev_handle_B, data_received, 2, -1);
*/

// config
esp_err_t bme_init(i2c_master_dev_handle_t bme_handle) {
    uint8_t chip_id = 0;
    uint8_t reg_id = BME280_REG_ID;
    
    esp_err_t err = i2c_master_transmit_receive(bme_handle, &reg_id, 1, &chip_id, 1, 500);         
    
    if ((err == ESP_OK) && (chip_id == 0x60)) {
		//AI gợi ý sửa phần này do code gốc chèn từng thanh ghi một, khá dài
        // Tạo mảng gói lệnh [Địa chỉ thanh ghi, Dữ liệu ghi]
        uint8_t config_hum[2] = {BME280_REG_CTRL_HUM, 0x01}; 
        uint8_t config_meas[2] = {BME280_REG_CTRL_MEAS, 0x23}; 
        
        // Ghi cấu hình qua I2C Master
        i2c_master_transmit(bme_handle, config_hum, 2, 50); 
        i2c_master_transmit(bme_handle, config_meas, 2, 50);
        return ESP_OK;
	}
	return ESP_FAIL;
}
// Hàm đọc dữ liệu thô: Nhận vào địa chỉ của 2 biến bên ngoài qua con trỏ
esp_err_t bme280_read_raw(i2c_master_dev_handle_t bme_handle, int32_t *adc_T, int32_t *adc_H) {
	uint8_t buf[5];  // 3 byte cho T, 2 byte cho H
    uint8_t start_reg = BME280_REG_DATA; 
	
    i2c_master_transmit_receive(bme_handle, &start_reg, 1, buf, 5, 500);

	i2c_master_transmit_receive(bme_handle, &start_reg, 1, buf, 5, 500);


    // Ghép dữ liệu 
	*adc_T = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4)  | ((int32_t)buf[2] >> 4);
	*adc_H = ((int32_t)buf[3] << 8) | (int32_t)buf[4];
   
   return ESP_OK; 
}
int32_t t_fine;
 
double bme_read_temp(i2c_master_dev_handle_t bme_handle, int32_t adc_T) {
	
	uint8_t buf_t[6];  
	uint8_t reg_t_start = 0x88; // địa chỉ chứa dữ liệu calib
	i2c_master_transmit_receive(bme_handle, &reg_t_start, 1, buf_t, 6, 500);
	uint16_t dig_T1 = (uint16_t)((buf_t[1] << 8) | buf_t[0]);
	int16_t dig_T2 = (int16_t)((buf_t[3] << 8) | buf_t[2]);
	int16_t dig_T3 = (int16_t)((buf_t[5] << 8) | buf_t[4]);
	
	// Tiến hành compensate theo công thức trong datasheet
	double var1, var2, T; 
	var1  = (((double)adc_T)/16384.0 - ((double)dig_T1)/1024.0) * ((double)dig_T2); 
	var2  = ((((double)adc_T)/131072.0 - ((double)dig_T1)/8192.0) * (((double)adc_T)/131072.0 - ((double) dig_T1)/8192.0)) * ((double)dig_T3); 
	t_fine = (int32_t)(var1 + var2); 
	T  = (var1 + var2) / 5120.0; 
	return T; 
} 
		
double bme_read_hum(i2c_master_dev_handle_t bme_handle, int32_t adc_H) {
	
	uint8_t reg_h1_start = 0xA1;	// H1 đứng lẻ 
    uint8_t reg_h2_start = 0xE1;	// còn lại
	uint8_t buf_h1[1]; // Mảng hứng 1 byte dig_H1
    uint8_t buf_h2[7]; // Mảng hứng 7 byte từ 0xE1 đến 0xE7
	i2c_master_transmit_receive(bme_handle, &reg_h1_start, 1, buf_h1, 1, 500); 
	i2c_master_transmit_receive(bme_handle, &reg_h2_start, 1, buf_h2, 7, 500);
	uint8_t dig_H1 = buf_h1[0];
    int16_t dig_H2 = (int16_t)((buf_h2[1] << 8) | buf_h2[0]);
    uint8_t dig_H3 = buf_h2[2];
    int16_t dig_H4 = (int16_t)(((int16_t)buf_h2[3] << 4) | (buf_h2[4] & 0x0F));
    int16_t dig_H5 = (int16_t)(((int16_t)buf_h2[5] << 4) | ((buf_h2[4] >> 4) & 0x0F));
    int8_t  dig_H6 = (int8_t)buf_h2[6];
	
	double var_H; 
	var_H = (((double)t_fine) - 76800.0); 
	var_H = (adc_H - (((double)dig_H4) * 64.0 + ((double)dig_H5) / 16384.0 * var_H)) * (((double)dig_H2) / 65536.0 * (1.0 + ((double)dig_H6) / 67108864.0 * var_H *  (1.0 + ((double)dig_H3) / 67108864.0 * var_H))); 
	var_H = var_H * (1.0 - ((double)dig_H1) * var_H / 524288.0); 
	if (var_H > 100.0) var_H = 100.0; 
	else if (var_H < 0.0) var_H = 0.0; 
	return var_H; 
}