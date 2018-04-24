/*
 * ioe_task.h
 *
 *  Created on: 2017¦~11¤ë8¤é
 *      Author: hopkins
 */

#ifndef MAIN_BLE_TASK_H_
#define MAIN_BLE_TASK_H_

#define SCAN_ITEM_SEQ_TIMEOUT 5

#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_log.h"

#include "config.h"
#include "util.h"

typedef struct ble_device {
        uint8_t mac_address[6];
        int16_t rssi;
        esp_bt_dev_type_t type;
} ble_device;

typedef struct ble_scan_item {
	ble_device device;
	uint32_t last_seen;
	uint8_t active;
} ble_scan_item;

extern ble_scan_item ble_scan_items[];
extern uint32_t ble_scan_item_counts;
extern SemaphoreHandle_t ble_scan_items_semaphore;

void ble_task(void *pvParameters);

#endif /* MAIN_BLE_TASK_H_ */
