/*
 * ioe_task.h
 *
 *  Created on: 2017¦~11¤ë8¤é
 *      Author: hopkins
 */

#ifndef MAIN_IOE_TASK_H_
#define MAIN_IOE_TASK_H_

#include "driver/i2c.h"
#include "esp_log.h"

#include "config.h"
#include "sniffer_info.h"

#if IOE_DEVICE == IOE_PCA9534
	#define A2 1
	#define A1 0
	#define A0 1

	#define IOE_ADDRESS ((0x20) | (A2 << 2) | (A1 << 1) | (A0 << 0))
#elif IOE_DEVICE == IOE_TCA6408

	#define ADDR 0

	#define IOE_ADDRESS ((0x20) | (ADDR << 0))
#else
	#error "Unknown IOE_DEVICE SELECTED."
#endif


extern sniffer_info device_info;
extern SemaphoreHandle_t device_info_semaphore;

void ioe_task(void *pvParameters);

esp_err_t ioe_write_register(uint8_t reg, uint8_t data);
esp_err_t ioe_read_register(uint8_t reg, uint8_t* data);

#endif /* MAIN_IOE_TASK_H_ */
