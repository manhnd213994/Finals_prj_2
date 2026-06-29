#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"			//Thư viện RTOS
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/i2c_master.h"			//Chứa các hàm phục vụ giao tiếp I2C
#include "esp_lcd_io_i2c.h"				//Chứa các hàm giao tiếp với LCD qua I2C
#include "nvs_flash.h" 					// Có vài hàm cần tương tác với bộ nhớ flash
#include "nvs.h"		
#include "esp_wifi.h"       // Thư viện chứa các hàm liên quan đến giao tiếp với WiFi
#include "esp_event.h"      // Thư viện quản lý sự kiện để check các event: mất mạng, kết nối, ...
#include "esp_netif.h"		// Thư viện quản lý tầng mạng
#include "esp_http_client.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_crt_bundle.h"
#include <time.h>			// Thời gian
#include <sys/time.h>
#include "esp_sntp.h"

#include "bme280.h"				// Thư viện tự tạo, chứa các hàm giao tiếp tự viết, marco để phục vụ project
#include "pms7003.h"
#include "lcd.h"

// MACRO CHO KẾT NỐI I2C DO CHƯA KHAI BÁO TRONG CÁC THƯ VIỆN TRÊN
#define I2C_MASTER_NUM           I2C_NUM_0    
#define I2C_MASTER_SDA_IO 		 21
#define I2C_MASTER_SCL_IO		 22
#define I2C_MASTER_FREQ_HZ 		 100000 

// MARCO CHO KẾT NỐI WIFI
#define WIFI_SSID       		 "Manh"
#define WIFI_PASSWORD   		 "15092003"
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT 		( 1 << 0 )
/*
// setup sntp
void init_sntp(void) {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "time.google.com"); 
    esp_sntp_init();

    // Thiết lập múi giờ Việt Nam (GMT+7)
    setenv("TZ", "WIB-7", 1);
    tzset();
}
*/
// fetch data (Gemini fix)
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		// Nếu wifi mất kết nối
        printf("[WIFI] Mat ket noi, dang thu ket noi lai...\n");
        esp_wifi_connect(); 
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		//reset lại trạng thái cho cờ wifi_event
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		// Nếu kết nối thành công và đc cấp phát IP thành công
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("[WIFI] Da nhan duoc IP chuẩn: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void firebase_patch_data(double temp, double hum, uint16_t pm25) {
	printf("bat dau firebase_patch_data \n");
/*	
	// lấy thời gian hiện tại 
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo); //gán thời gian hiện tại

    // Định dạng thời gian thành chuỗi "YYYY-MM-DD HH:MM:SS"
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
*/	
    char json_payload[256];
    snprintf(json_payload, sizeof(json_payload), 
             "{\"temperature\": %.2f, \"humidity\": %.1f, \"pm25\": %u }", 
             temp, hum, pm25);
	printf("\nPayload kiem tra: %s\n", json_payload);
	fflush(stdout);
	
	const char *url = "https://env-monitor-f98bb-default-rtdb.firebaseio.com/.json?auth=u1siQpGDI5rUIsqJrMuqAqNz83oTEhut9HKconA2";
    
    // Cấu hình HTTP Client 
    esp_http_client_config_t config_client = {
        .url = url,
        .method = HTTP_METHOD_POST, //Tạo node mới liên tục thay vì update lên node cũ 
        .timeout_ms = 5000,          
		.transport_type = HTTP_TRANSPORT_OVER_SSL,
		.skip_cert_common_name_check = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config_client);
	
    if (client == NULL) {
        printf("Firebase: Khoi tao client that bai \n");
        return;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        printf("Firebase: Gui thanh cong! HTTP Status Code = %d \n", status_code);
    } else {
        printf("Firebase: Gui that bai! Loi = %s \n", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}
void app_main(void) {
	printf("chip reseted!");
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
	
	//pms7003
	uart_config_t uart_config = {
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT,
	};
	uart_param_config(PMS_UART_PORT, &uart_config);
	uart_set_pin(PMS_UART_PORT, PMS_TX_PIN, PMS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
		
	
	//khởi tạo
	
	// KHỞI TẠO MẠNG (Gemini)
	nvs_flash_init();
	esp_netif_init();
	esp_event_loop_create_default();
	esp_netif_create_default_wifi_sta();
	
	// ĐĂNG KÝ HÀM XỬ LÝ SỰ KIỆN WIFI VÀO HỆ THỐNG
	esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
	esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
	
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASSWORD,
		},
	};
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
	esp_wifi_start();
	esp_wifi_connect();
//	init_sntp(); // lấy thời gian 

	// event group: để theo dõi trạng thái kết nối wifi
	s_wifi_event_group = xEventGroupCreate();
	
	// lcd
	lcd_init(lcd_handle);
	vTaskDelay(pdMS_TO_TICKS(100));
	
	//pms7003
	uart_driver_install(PMS_UART_PORT, 1024, 0, 0, NULL, 0);
	//bme280
	esp_err_t bme_err = bme_init(bme_handle);
	if (bme_err != ESP_OK) 
		printf("bme280 init failed! \n");
	//biến đựng dữ liệu thô từ bme280
	int32_t raw_T = 0;
    int32_t raw_H = 0;
	
	        		
	const char lcd_header[17] = "Temp  Hum  PM";
	lcd_put_str(lcd_handle, 0, 0, lcd_header);

	double temp = 0;
	double hum = 0;
	uint16_t valid_pm25 = 0;
	
	while (1){
	
		
		//i2c_scanner(bus_handle);
		esp_err_t err_bme = bme280_read_raw(bme_handle, &raw_T, &raw_H);

		
		if (err_bme == ESP_OK){
				temp = bme_read_temp(bme_handle, raw_T);
				hum = bme_read_hum(bme_handle, raw_H);
			}
	
				
		uint16_t pm25 = read_pm25(PMS_UART_PORT);
		if (pm25 == 0xFFFF){ 
			//Nếu bị lỗi
			pm25 = valid_pm25; // trở lại dữ liệu gần nhất 
		}
		
		char lcd_buffer[64]; 
		
		snprintf(lcd_buffer, sizeof(lcd_buffer), "%.1f"  , temp); 
		lcd_put_str(lcd_handle,0 ,1 , (const char *)lcd_buffer);
		
		snprintf(lcd_buffer, sizeof(lcd_buffer),"%.0f ", hum);
		lcd_put_str(lcd_handle,6 ,1 , (const char *)lcd_buffer);
		
		snprintf(lcd_buffer, sizeof(lcd_buffer),"%03u", valid_pm25);
		lcd_put_str(lcd_handle,11 ,1 , (const char *)lcd_buffer);

		valid_pm25 = pm25; // cảm biến bụi mịn lấy mẫu rất chậm và thường lỗi đường truyền,
							  //tạm thời hi sinh vài chu kỳ đầu để đợi giá trị valid.
		

		//Check trạng thái WiFi trước khi gửi
		EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, 
                                       WIFI_CONNECTED_BIT, 
                                       pdFALSE, 
                                       pdTRUE, 
                                       pdMS_TO_TICKS(5000));
		if ((bits & WIFI_CONNECTED_BIT) != 0) { // Nếu có mạng (khác 0)
			printf("\n [SYSTEM] Wifi connect successfully! Dang gui data... \n");	
			firebase_patch_data(temp, hum, valid_pm25);
		} else {                                // Nếu mất mạng / Chưa có IP
			printf("\n [SYSTEM] Failed to connect to wifi hoặc chưa nhận được IP thô! \n");
			esp_wifi_connect(); 
		}
		
		vTaskDelay(pdMS_TO_TICKS(18000));
	}
}


