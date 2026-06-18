#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_lcd_io_i2c.h"
#include "driver/uart.h"
#include "esp_err.h"

#include "bme280.h"
#include "pms7003.h"
#include "lcd.h"

#define I2C_MASTER_NUM           I2C_NUM_0    
#define I2C_MASTER_SDA_IO 		 21
#define I2C_MASTER_SCL_IO		 22
#define I2C_MASTER_FREQ_HZ 		 100000 



void app_main(void) {
	// config cho đường bus i2c
	i2c_master_bus_config_t bus_config = {
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.i2c_port = I2C_MASTER_NUM ,
		.scl_io_num = I2C_MASTER_SCL_IO,
		.sda_io_num = I2C_MASTER_SDA_IO,
		.glitch_ignore_cnt = 7,
		.flags.enable_internal_pullup = true,
	};

	// Con trỏ chứa dữ liệu config của master
	i2c_master_bus_handle_t bus_handle;
	i2c_new_master_bus(&bus_config, &bus_handle);

	// Config cho slave 
	// bme280
	i2c_device_config_t bme_cfg = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,  //Thường là 7 bit 
		.device_address = BME280_SENSOR_ADDR ,
		.scl_speed_hz = 100000,
	};

	// Con trở chứa dữ liệu config của cảm biến
	i2c_master_dev_handle_t bme_handle; 
	i2c_master_bus_add_device(bus_handle, &bme_cfg, &bme_handle);    

	// lcd
	i2c_device_config_t lcd_cfg = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,  //Thường là 7 bit 
		.device_address = LCD_ADDR ,
		.scl_speed_hz = 100000,
	};

	i2c_master_dev_handle_t lcd_handle;     
	i2c_master_bus_add_device(bus_handle, &lcd_cfg, &lcd_handle);
	

	// config cho pms7003
	esp_err_t pms7003_init(void) {
		uart_config_t uart_config = {
			.baud_rate = 9600,
			.data_bits = UART_DATA_8_BITS,
			.parity    = UART_PARITY_DISABLE,
			.stop_bits = UART_STOP_BITS_1,
			.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
			.source_clk = UART_SCLK_DEFAULT,
		};
    
    esp_err_t err = uart_param_config(PMS_UART_PORT, &uart_config);
	if (err != ESP_OK) return err;
    
    err = uart_set_pin(PMS_UART_PORT, PMS_TX_PIN, PMS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;
	}	
	uart_driver_install(PMS_UART_PORT, 256, 0, 0, NULL, 0);

	
	//khởi tạo
	// lcd
	lcd_init(lcd_handle);
	
	//bme280
	bme_init(bme_handle);
	//biến đựng dữ liệu thô từ bme280
	int32_t raw_T = 0;
    int32_t raw_H = 0;
	
	// In dòng trên ko thay đổi qua các loop
	lcd_gotoxy(lcd_handle,0,0);
	const char lcd_header[17] = "Temp  Hum  PM";
	lcd_put_str(lcd_handle, lcd_header);
	
/* 
//Nếu bắt đầu ghép LCD và chưa ghép 2 cái còn lại, thử code này
while (1) {
        // Giả sử bạn lấy được dữ liệu nhiệt độ từ cảm biến là 28.54
        float temp = 28.54; 
        char display_buffer[16];
        snprintf(display_buffer, sizeof(display_buffer), "Temp: %.2f C", temp);

        // Cập nhật dữ liệu động liên tục ở hàng 2
        lcd_gotoxy(lcd_handle, 2, 1);   // Lùi vào 2 ô (Cột 2), Hàng 1 (Hàng 2)
        lcd_put_str(lcd_handle, display_buffer);

        vTaskDelay(pdMS_TO_TICKS(2000)); // Cập nhật lại sau 2 giây
    }
}
// Giúp xác định xem lcd có hoạt động bth ko
// Q: Sử dụng như nào?
// A: Ở vòng lặp while(1) bên dưới, hãy đóng nó vào phần chú thích , sau đó gỡ dấu chú thích của cái này ra
*/
	while (1){
		esp_err_t err_bme = bme280_read_raw(bme_handle, &raw_T, &raw_H);
		double temp = 0;
		double hum = 0;
		if (err_bme == ESP_OK){
			temp = bme_read_temp(bme_handle, raw_T);
			hum = bme_read_hum(bme_handle, raw_H);
			
		}

			
		
		uint16_t pm25 = read_pm25(PMS_UART_PORT);
		if (pm25 == 0xFFFF){ 
			//Nếu bị lỗi
			pm25 = 0;
		}
		
		lcd_gotoxy (lcd_handle,0,1);
		
		// Tạo 1 string chứa dữ liệu để in
		char lcd_buffer[64]; 
		snprintf(lcd_buffer, sizeof(lcd_buffer), "T:%.1f H:%.0f P:%03u", temp, hum, pm25); 
		
		lcd_put_str(lcd_handle, (const char *)lcd_buffer);
		
		vTaskDelay(pdMS_TO_TICKS(5000));
}
}


