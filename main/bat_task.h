#ifndef MAIN_BAT_TASK_H_
#define MAIN_BAT_TASK_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

#include "config.h"
#include "sniffer_info.h"

extern sniffer_info device_info;
extern SemaphoreHandle_t device_info_semaphore;
extern esp_adc_cal_characteristics_t *adc_characteristics;

void bat_task(void *pvParameters);

#endif /* MAIN_BAT_TASK_H_ */
