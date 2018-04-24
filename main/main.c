/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_event_loop.h"

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"

#include "esp_wifi.h"

#include "esp_log.h"
#include "camera.h"

#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "lwip/ip.h"


#include "nvs.h"
#include "nvs_flash.h"

#include "bat_task.h"
#include "config.h"

#include "ioe_task.h"
#include "ble_task.h"
#include "btap_task.h"
#include "camera_manager_task.h"
#include "bat_task.h"
#include "sniffer_info.h"

#include "util.h"


#define BT_BD_ADDR_HEX(addr)   addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

static const char* TAG = "main";

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
static ip4_addr_t s_ip_addr;

ble_scan_item ble_scan_items[MAX_SCAN_ITEMS];
uint32_t ble_scan_item_counts;
SemaphoreHandle_t ble_scan_items_semaphore;

sniffer_info device_info;
SemaphoreHandle_t device_info_semaphore;


camera_manager_jpeg_buffer camera_images_buffers[CAMERA_IMAGES_BUFFER_SIZE];

camera_snapshot_ble_devices_list snapshot_devices_list;

esp_adc_cal_characteristics_t *adc_characteristics;


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            s_ip_addr = event->event_info.got_ip.ip_info.ip;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
             auto-reassociate. */
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void app_main()
{
    esp_chip_info_t chip_info;
    uint8_t mac[6];
    uint32_t i;
    uint8_t ioe_readout;

    vTaskDelay(2000/portTICK_PERIOD_MS); // Delay 2s before starting the application

    printf("BlueTrack sniffer firmware based on ESP32\n");

    printf("#####################\n");
    printf("# ESP32 Information #\n");
    printf("#####################\n");

    // Print ESP32 Information
    esp_chip_info(&chip_info);
    printf("Cores: %d\n", chip_info.cores);
    printf("802.11b/g/n Support: %s\n", (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "YES" : "NO");
    printf("BT Support: %s\n", (chip_info.features & CHIP_FEATURE_BT) ? "YES" : "NO");
    printf("BLE Support: %s\n", (chip_info.features & CHIP_FEATURE_BLE) ? "YES" : "NO");
    printf("Chip Revision: %d\n", chip_info.revision);
    printf("\n");

    printf("######################\n");
    printf("# Module Information #\n");
    printf("######################\n");

    // Print Module Information
    printf("Flash size: %dMB\n", spi_flash_get_chip_size() / (1024*1024));
    printf("Flash type: %s\n", (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    esp_efuse_mac_get_default(mac);
    printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("\n");

    printf("\n");
    printf("Probing and initializing camera, please wait...\n");
    camera_config_t config = {
		.ledc_channel = LEDC_CHANNEL_0,
		.ledc_timer = LEDC_TIMER_0,
		.pin_d0 = 4,
		.pin_d1 = 5,
		.pin_d2 = 18,
		.pin_d3 = 19,
		.pin_d4 = 36,
		.pin_d5 = 39,
		.pin_d6 = 34,
		.pin_d7 = 35,
		.pin_xclk = 21,
		.pin_pclk = 22,
		.pin_vsync = 25,
		.pin_href = 23,
		.pin_sscb_sda = 26,
		.pin_sscb_scl = 27,
		.pin_reset = 2,
		.xclk_freq_hz = 10000000,
		.pixel_format = CAMERA_PF_JPEG,
		//.pixel_format = CAMERA_PF_RGB565,
		//.frame_size = CAMERA_FS_UXGA,
		.frame_size = CAMERA_FS_SXGA,
		.jpeg_quality = 255
	};

	esp_err_t err  = camera_init(&config);
    //esp_err_t err = ESP_OK;
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Camera initialization failed with error = %d", err);
		return;
	}
	printf("Camera initialized.");

	printf("\n\nI2C initialization\n");
	i2c_config_t i2c_conf;
	i2c_conf.mode = I2C_MODE_MASTER;
	i2c_conf.sda_io_num = 12;
	i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	i2c_conf.scl_io_num = 13;
	i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	i2c_conf.master.clk_speed = 800; // 1KHz
	i2c_param_config(I2C_NUM_0, &i2c_conf);
	i2c_driver_install(I2C_NUM_0, i2c_conf.mode, 0, 0, 0);

	err = ioe_write_register(0x03, 0x7F);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "ERROR when communicating via I2C: %d\n", err);
		return;
	}

	// Read device ID from I2C
	err = ioe_read_register(0x00, &ioe_readout);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "ERROR when reading device ID via I2C: %d\n", err);
		return;
	}

	printf("\nADC initialization\n");
	if(esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
		printf("eFuse TP: Supported\n");
	}else{
		printf("eFuse TP: NOT Supported\n");
	}
	if(esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
		printf("eFuse VRef: Supported\n");
	}else{
		printf("eFuse VRef: NOT Supported\n");
	}
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11); // Full-scale voltage 3.9V, input is max 3.3V (1/2 Voltage Divider)
	adc_characteristics = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t cal_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, adc_characteristics);
	if(cal_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
		printf("ADC is characterized using Two Point Value\n");
	}else if(cal_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
		printf("ADC is characterized using eFuse VRef\n");
	}else if(cal_type == ESP_ADC_CAL_VAL_DEFAULT_VREF) {
		printf("ADC is characterized using Default VRef\n");
	}

	printf("\nCHG_DET initialization...\n");
	gpio_config_t io_config;
	io_config.intr_type=GPIO_PIN_INTR_DISABLE;
	io_config.mode=GPIO_MODE_INPUT;
	io_config.pin_bit_mask=(1ULL << 33);
	io_config.pull_down_en=1;
	io_config.pull_up_en=0;
	gpio_config(&io_config);

	printf("\nNVS initialization...\n");
	err = nvs_flash_init();
	if(err == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	printf("\nWi-Fi spinning up...\n");
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	wifi_config_t wifi_config = {
			.sta = {
					.ssid = WIFI_SSID,
					.password = WIFI_PW
			}
	};

	tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "bluetrack-sniffer-0");
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

	if(!(ioe_readout & (1 << 6))) {
		// IOE Button is pressed
		ESP_LOGW(TAG, "IOE Button is pressed, but no function is implemented yet...");
	}

	printf("\nBLE spinning up...\n");
	// BT Controller Init
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	err = esp_bt_controller_init(&bt_cfg);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Bluetooth controller initialization failed, err = %x", err);
		return;
	}
	// BT Controller Enable
	err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Bluetooth controller enable failed, err = %x", err);
		return;
	}
	// Bluedroid Init
	err = esp_bluedroid_init();
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Bluedroid initialization failed, err = %x", err);
		return;
	}
	// Bluedroid Enable
	err = esp_bluedroid_enable();
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Bluedroid enable failed, err = %x", err);
		return;
	}
	// Other GAP stuff will be configured in ble_task

	// Variables initializations
	// BLE Scan items
	for(i=0; i<MAX_SCAN_ITEMS; i++) {
		ble_scan_items[i].active=0;
	}
	ble_scan_items_semaphore = xSemaphoreCreateMutex();
	if(ble_scan_items_semaphore == NULL) {
		ESP_LOGE(TAG, "ble_scan_items_semaphore creation failed.");
		return;
	}
	ble_scan_item_counts=0;

	// Sniffer device info
	setBatteryPercent(&device_info, 100);
	setBatteryCharging(&device_info, 0);
	setIP(&device_info, s_ip_addr.addr);
	setID(&device_info, (ioe_readout & 0x3F));
	device_info_semaphore = xSemaphoreCreateMutex();
	if(device_info_semaphore == NULL) {
		ESP_LOGE(TAG, "device_info_semaphore creation failed.");
		return;
	}
	setCameraFlipConfig(&device_info, 0x03);
	setLEDConfig(&device_info, 0x02);
	setBLESnapshotRSSI(&device_info, -60);

	// Camera Framebuffer
	for(i=0; i<CAMERA_IMAGES_BUFFER_SIZE; i++) {
		camera_images_buffers[i].camera_fb=NULL;
		camera_images_buffers[i].camera_fb_size=0;
		camera_images_buffers[i].camera_fb_consume_needed=0;
		camera_images_buffers[i].camera_fb_type = 2;
		camera_images_buffers[i].camera_fb_semaphore=xSemaphoreCreateMutex();
	}

	// BLE Snapshot Devices List
	snapshot_devices_list.list_size=0;
	snapshot_devices_list.list_max_size=128;
	snapshot_devices_list.list = malloc(sizeof(camera_snapshot_ble_device) *snapshot_devices_list.list_max_size);
	snapshot_devices_list.semaphore = xSemaphoreCreateMutex();
	if(snapshot_devices_list.list == NULL) {
		ESP_LOGE(TAG, "snapshot_devices_list.list creation failed.");
		return;
	}
	if(snapshot_devices_list.semaphore == NULL) {
		ESP_LOGE(TAG, "snapshot_devices_list.semaphore creation failed.");
		return;
	}

	char binary[9];
	ESP_LOGI(TAG, "Sniffer ID: %d(%s) is ready, starting all the tasks!", getID(device_info), getBinary(getID(device_info), binary));

	// Start HTTP Server
	//ESP_LOGI(TAG, "Free heap: %d", system_get_free_heap_size());
	//xTaskCreate(&http_server, "http_server", 2048, NULL, 5, NULL);
	ESP_LOGI(TAG, "Free heap: %d", esp_get_free_heap_size());
	xTaskCreate(&camera_manager_task, "camera_mgr", 2800, NULL, 5, NULL);
	ESP_LOGI(TAG, "Started HTTP Server");
	ESP_LOGI(TAG, "Free heap: %d", esp_get_free_heap_size());
	xTaskCreate(&ioe_task, "ioe", 1024, NULL, 8, NULL);
	ESP_LOGI(TAG, "Started IOE Task");
	ESP_LOGI(TAG, "Free heap: %d", esp_get_free_heap_size());
	xTaskCreate(&btap_task, "btap", 2048, NULL, 5, NULL);
	ESP_LOGI(TAG, "Started BTAP Task");
	ESP_LOGI(TAG, "Free heap: %d", esp_get_free_heap_size());
	xTaskCreate(&ble_task, "ble", 7168, NULL, 5, NULL);
	ESP_LOGI(TAG, "Started BLE Task");
	ESP_LOGI(TAG, "Free heap: %d", esp_get_free_heap_size());
	xTaskCreate(&bat_task, "bat", 2048, NULL, 5, NULL);
	ESP_LOGI(TAG, "Started BAT Task");
	ESP_LOGI(TAG, "Free heap: %d", esp_get_free_heap_size());
    /*for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();*/
}
