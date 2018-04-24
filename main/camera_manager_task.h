/*
 * snapshot_manger.h
 *
 *  Created on: 2018¦~3¤ë17¤é
 *      Author: hopkins
 */

#ifndef MAIN_CAMERA_MANAGER_TASK_H_
#define MAIN_CAMERA_MANAGER_TASK_H_


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include <string.h>


#include "esp_log.h"

#include "camera.h"

#include "config.h"
#include "util.h"
#include "sniffer_info.h"
#include "ble_task.h"


extern sniffer_info device_info;
extern SemaphoreHandle_t device_info_semaphore;

typedef struct camera_snapshot_ble_device {
	uint8_t mac_address[6];
} camera_snapshot_ble_device;

typedef struct camera_snapshot_ble_devices_list {
	uint32_t list_size;
	uint32_t list_max_size;
	camera_snapshot_ble_device *list;
	SemaphoreHandle_t semaphore;
} camera_snapshot_ble_devices_list;

typedef struct camera_manager_jpeg_buffer {
	camera_snapshot_ble_device ble_device;
	int16_t ble_device_rssi;
	uint8_t *camera_fb;
	size_t camera_fb_size;
	uint8_t camera_fb_consume_needed;
	uint8_t camera_fb_type; // 1=test snapshot 2=ble snapshot
	SemaphoreHandle_t camera_fb_semaphore;
} camera_manager_jpeg_buffer;



extern camera_manager_jpeg_buffer camera_images_buffers[];
extern camera_snapshot_ble_devices_list snapshot_devices_list;

void camera_manager_task(void *pvParameters);


#endif /* MAIN_CAMERA_MANAGER_TASK_H_ */
