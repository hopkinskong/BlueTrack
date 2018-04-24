#include "ioe_task.h"

static const char *TAG = "IOE";

esp_err_t ioe_write_register(uint8_t reg, uint8_t data) {
	esp_err_t ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ((IOE_ADDRESS) << 1)|I2C_MASTER_WRITE, 0x01); // Write to I2C Address
	i2c_master_write_byte(cmd, reg, 0x01); // Write Register Address
	i2c_master_write_byte(cmd, data, 0x01); // Write Register data
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return ret;
}

esp_err_t ioe_read_register(uint8_t reg, uint8_t* data) {
	esp_err_t ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ((IOE_ADDRESS) << 1)|I2C_MASTER_WRITE, 0x01);
	i2c_master_write_byte(cmd, reg, 0x01); // Write Register Address
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ((IOE_ADDRESS) << 1)|I2C_MASTER_READ, 0x01); // Write to I2C Address
	i2c_master_read_byte(cmd, data, 0x01); // Read directly
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return ret;
}


void ioe_task(void *pvParameters) {
	esp_err_t ret;

	while(1) {
		while(xSemaphoreTake(device_info_semaphore, (TickType_t)5) != pdTRUE);
		if(getLEDFlash(device_info)) {
			xSemaphoreGive(device_info_semaphore);

			// LED Flash
			ret = ioe_write_register(0x01, (1 << 7));
			if(ret != ESP_OK) ESP_LOGE(TAG, "ERROR when communicating via I2C: %d\n", ret);
			vTaskDelay(2 / portTICK_RATE_MS);
			ret = ioe_write_register(0x01, 0x00);
			if(ret != ESP_OK) ESP_LOGE(TAG, "ERROR when communicating via I2C: %d\n", ret);
			vTaskDelay(2000 / portTICK_RATE_MS);
		}else if(getLEDFullOn(device_info)) {
			xSemaphoreGive(device_info_semaphore);

			// LED ON
			ret = ioe_write_register(0x01, (1 << 7));
			if(ret != ESP_OK) ESP_LOGE(TAG, "ERROR when communicating via I2C: %d\n", ret);
		}else if(getLEDOff(device_info)) {
			xSemaphoreGive(device_info_semaphore);

			// LED OFF
			ret = ioe_write_register(0x01, 0x00);
			if(ret != ESP_OK) ESP_LOGE(TAG, "ERROR when communicating via I2C: %d\n", ret);
		}else{
			xSemaphoreGive(device_info_semaphore);

			// Do nothing
		}
	}
}
