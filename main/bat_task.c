#include "bat_task.h"

static const char *TAG = "BAT";

void bat_task(void *pvParameters) {
	uint32_t adc_reading = 0, voltage = 0;
	int battery=0;
	uint8_t chg=0;
	uint8_t seq=0;
	while(1) {
		// Do ADC Read every 20*500ms
		if(seq%20 == 0) {
			seq=0;
			// Get ADC Reading
			adc_reading=0;
			for(int i=0; i<512; i++) {
				adc_reading += adc1_get_raw(ADC1_CHANNEL_4);
			}
			adc_reading /= 512;
			// Get ADC Voltage from reading
			voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_characteristics);
			voltage += 42; // Little correction
			// Calculate Battery Percentage
			// Assume 0%=3.5V, 100%=4.2V
			battery = voltage*2-3500;
			battery *= 100;
			battery /= (4200-3500);
			if(battery < 0) battery=0;
			if(battery > 100) battery=100;

			ESP_LOGI(TAG, "%dmV(%d)", voltage, battery);

			while(xSemaphoreTake(device_info_semaphore, (TickType_t)5) != pdTRUE);
			setBatteryPercent(&device_info, battery);
			xSemaphoreGive(device_info_semaphore);
		}
		// Do CHG_DET read every 500ms
		// Get CHG_DET
		chg = gpio_get_level(33);
		while(xSemaphoreTake(device_info_semaphore, (TickType_t)5) != pdTRUE);
		setBatteryCharging(&device_info, chg);
		xSemaphoreGive(device_info_semaphore);
		seq++;

		vTaskDelay(500 / portTICK_RATE_MS);
	}
}
