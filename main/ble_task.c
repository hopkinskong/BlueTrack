/*
 * ble_task.c
 *
 *  Created on: 2017¦~11¤ë8¤é
 *      Author: hopkins
 */
#include "ble_task.h"

static char *TAG="BLE_TASK";


static uint32_t ble_scan_seq=0;


// Scan duration=1s
// We make interval be 1s/2 = 50ms
static esp_ble_scan_params_t gap_scan_params = {
		//.scan_type              = BLE_SCAN_TYPE_ACTIVE,
		.scan_type              = BLE_SCAN_TYPE_PASSIVE,
		.own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
		.scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
		.scan_interval          = 0x550, // Scan every 200ms
		.scan_window            = 0x370 // Scan window 100ms
		//.scan_window            = 0x480 // Scan window 100ms
		//.scan_interval          = 0x550, // Scan every 200ms
		//.scan_window            = 0x550 // Scan window 100ms
};

static uint8_t ignore_ble_scan_item(ble_device dev) {
	if(dev.rssi > 128) {
		//ESP_LOGE(TAG, "%02X:%02X:%02X:%02X:%02X:%02X RSSI(%d) too high, ignored.", dev.rssi, BT_BD_ADDR_HEX(dev.mac_address));
		return 1;
	}

	if(dev.rssi < -256) {
		//ESP_LOGE(TAG, "%02X:%02X:%02X:%02X:%02X:%02X RSSI(%d) too low, ignored.", dev.rssi, BT_BD_ADDR_HEX(dev.mac_address));
		return 1;
	}

	if(dev.type != ESP_BT_DEVICE_TYPE_BLE && dev.type != ESP_BT_DEVICE_TYPE_DUMO) {
		//ESP_LOGE(TAG, "%02X:%02X:%02X:%02X:%02X:%02X is a non BLE device, ignored.", BT_BD_ADDR_HEX(dev.mac_address));
		return 1;
	}

	if(dev.mac_address[0] == 0xFF && dev.mac_address[1] == 0xFF && dev.mac_address[2] == 0xFF && dev.mac_address[3] == 0xFF) {
		//ESP_LOGE(TAG, "%02X:%02X:%02X:%02X:%02X:%02X is a filtered MAC address, ignored.", BT_BD_ADDR_HEX(dev.mac_address));
		return 1;
	}
	return 0;
}

static void add_ble_scan_item(ble_device dev, uint8_t scan_completed) {
	ble_scan_item item;
	uint32_t i;
	uint8_t updated=0;

	if(ignore_ble_scan_item(dev)) {
		updated=1;
	}

	item.active=1;
	item.device=dev;
	item.last_seen=ble_scan_seq;

	if(!updated) {
		// Update if exists
		for(i=0; i<MAX_SCAN_ITEMS; i++) {
			if(ble_scan_items[i].active == 1) {
				if(mac_address_match(ble_scan_items[i].device.mac_address, item.device.mac_address)) {
					// MAC address match, update
					ble_scan_items[i]=item;
					updated=1;
					break;
				}
			}
		}
	}

	if(!updated) { // Not exists, now we add a new entry
		// Find first available items, and add it
		for(i=0; i<MAX_SCAN_ITEMS; i++) {
			if(ble_scan_items[i].active == 0) {
				ble_scan_items[i]=item;
				ble_scan_item_counts++;
				break;
			}
		}
	}

	if(scan_completed) {
		// Reset ble_scan_item_counts
		ble_scan_item_counts=0;
		// Purge inactive items
		for(i=0; i<MAX_SCAN_ITEMS; i++) {
			if(ble_scan_items[i].active == 1) {
				if(ble_scan_items[i].last_seen+SCAN_ITEM_SEQ_TIMEOUT < ble_scan_seq) {
					// Remove, timed out
					ble_scan_items[i].active=0;
				}else{
					ble_scan_item_counts++;
				}
			}
		}
	}
}

static const char *bt_event_type_to_string(uint32_t eventType) {
	switch(eventType) {
		case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
			return "Advertising data set complete";
		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
			return "Set scan param operation success";
		case ESP_GAP_BLE_SCAN_RESULT_EVT:
			return "One scan result ready";
		case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
			return "Scan response data set complete";
		default:
			return "Unknown event type";
	}
} // bt_event_type_to_string


static const char *bt_dev_type_to_string(esp_bt_dev_type_t type) {
	switch(type) {
	case ESP_BT_DEVICE_TYPE_BREDR:
		return "ESP_BT_DEVICE_TYPE_BREDR";
	case ESP_BT_DEVICE_TYPE_BLE:
		return "ESP_BT_DEVICE_TYPE_BLE";
	case ESP_BT_DEVICE_TYPE_DUMO:
		return "ESP_BT_DEVICE_TYPE_DUMO";
	default:
		return "Unknown";
	}
} // bt_dev_type_to_string

static const char *bt_addr_t_to_string(esp_ble_addr_type_t type) {
	switch(type) {
		case BLE_ADDR_TYPE_PUBLIC:
			return "BLE_ADDR_TYPE_PUBLIC";
		case BLE_ADDR_TYPE_RANDOM:
			return "BLE_ADDR_TYPE_RANDOM";
		case BLE_ADDR_TYPE_RPA_PUBLIC:
			return "BLE_ADDR_TYPE_RPA_PUBLIC";
		case BLE_ADDR_TYPE_RPA_RANDOM:
			return "BLE_ADDR_TYPE_RPA_RANDOM";
		default:
			return "Unknown addr_t";
	}
} // bt_addr_t_to_string


static uint32_t ble_task_tick=0;
static uint8_t ble_controller_idle=1;
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
	ESP_LOGI(TAG, "GAP Callback, recv evt=%s (%d)", bt_event_type_to_string(event), (uint32_t)event);

	switch(event) {
	case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {

		// Start scanning when parameter has been set
		if(esp_ble_gap_start_scanning(1) != ESP_OK) {
			ESP_LOGE(TAG, "Unable to start scan.");
		}
		ble_controller_idle=0;
		ble_scan_seq++;
	}
		break;
	case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
		// Scanning started
		if(param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
			ESP_LOGE(TAG, "Start scan failed.");
		}

	}
		break;
	case ESP_GAP_BLE_SCAN_RESULT_EVT: {

		//ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_RESULT_EVT: search_evt=%d", param->scan_rst.search_evt);

		ble_device dev;
		BT_BD_ADDR_ASSIGN(dev.mac_address, param->scan_rst.bda);
		dev.rssi=param->scan_rst.rssi;
		dev.type=param->scan_rst.dev_type;

		while(xSemaphoreTake(ble_scan_items_semaphore, (TickType_t)2) != pdTRUE);
		add_ble_scan_item(dev, (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT ? 1 : 0));
		xSemaphoreGive(ble_scan_items_semaphore);

		if(param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
			// Search completed.
			// Restart scan

			// Fixing BUG of poor scan result, by restarting BLE controller
			ble_controller_idle=1;
			if(esp_bt_controller_disable() != ESP_OK) {
				ESP_LOGE(TAG, "Unable to disable BLE controller.");
			}
			if(esp_bt_controller_enable(ESP_BT_MODE_BLE) != ESP_OK) {
				ESP_LOGE(TAG, "Unable to enable BLE controller.");
			}
			if(esp_ble_gap_set_scan_params(&gap_scan_params) != ESP_OK) {
				ESP_LOGE(TAG, "Unable to set BLE scan param");
			}
			// It will automatically start scanning after SCAN_PARAM is set
			//if(esp_ble_gap_start_scanning(1) != ESP_OK) {
			//	ESP_LOGE(TAG, "Unable to start scan.");
			//}
		}
	}
		break;
	case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
		if(param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
				ESP_LOGE(TAG, "Stop scan failed.");
		}else{
				ESP_LOGI(TAG, "BLE Scan stopped.");
		}
	}
		break;
	case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT: {
		if(param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
				ESP_LOGE(TAG, "Stop advertising failed.");
		}else{
				ESP_LOGI(TAG, "BLE advertising stopped.");
		}
	}
		break;
	default:
		break;
	}
}

void ble_task(void *pvParameters) {
	// Setup scan result
	esp_err_t err;

	ESP_LOGI(TAG, "Starting...");

	// GAP Callback registration
	err = esp_ble_gap_register_callback(esp_gap_cb);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "BLE GAP callback registration failed, err = %x", err);
		return;
	}
	// GAP Scan Param Set
	err = esp_ble_gap_set_scan_params(&gap_scan_params);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "BLE GAP scan parameters set failed, err = %x", err);
		return;
	}

	ESP_LOGI(TAG, "Configuration completed.");

	uint32_t tmp=0;
	while(1) {

		/*if(tmp+2 < ble_task_tick && ble_controller_idle) {
			if(esp_bt_controller_disable() != ESP_OK) {
				ESP_LOGE(TAG, "Unable to disable BLE controller.");
			}
			if(esp_bt_controller_enable(ESP_BT_MODE_BLE) != ESP_OK) {
				ESP_LOGE(TAG, "Unable to enable BLE controller.");
			}
			if(esp_ble_gap_set_scan_params(&gap_scan_params) != ESP_OK) {
				ESP_LOGE(TAG, "Unable to set BLE scan param");
			}
			tmp=ble_task_tick;
		}

		ble_task_tick++;*/
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}
