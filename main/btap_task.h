/*
 * btap.h
 *
 *  Created on: 2018¦~1¤ë10¤é
 *      Author: hopkins
 */

#ifndef MAIN_BTAP_TASK_H_
#define MAIN_BTAP_TASK_H_


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_log.h"

#include "config.h"
#include "ble_task.h"
#include "camera_manager_task.h"

#include "sniffer_info.h"

#include <string.h>

void btap_task(void *pvParameters);

extern ble_scan_item ble_scan_items[];
extern SemaphoreHandle_t ble_scan_items_semaphore;

extern sniffer_info device_info;
extern SemaphoreHandle_t device_info_semaphore;

extern camera_manager_jpeg_buffer camera_images_buffers[];
extern camera_snapshot_ble_devices_list snapshot_devices_list;

#endif /* MAIN_BTAP_TASK_H_ */
