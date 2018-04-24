#include "camera_manager_task.h"

const static char* TAG = "CAMERA_MGR";


void camera_manager_task(void *pvParameters) {
	uint8_t current_camera_flip_setting = 0x03;
	uint8_t current_aec_setting = 0x01;
	uint16_t current_aec_value = 0xFFFF;
	sniffer_info global_device_info;
	esp_err_t err;
	uint16_t i, j;

	while(1) {
		while(xSemaphoreTake(device_info_semaphore, (TickType_t)5) != pdTRUE);
		global_device_info=device_info;
		xSemaphoreGive(device_info_semaphore);

		// Apply camera flip settings to camera
		if(current_camera_flip_setting != global_device_info.camera_flip_config) {
			if(getVFlip(global_device_info) && !getVFlipFromValue(current_camera_flip_setting)) { // need to enable vflip
				camera_set_v_flip(1);
				ESP_LOGW(TAG, "VFLIP ON");
			}
			if(!getVFlip(global_device_info) && getVFlipFromValue(current_camera_flip_setting)) { // need to disable vflip
				camera_set_v_flip(0);
				ESP_LOGW(TAG, "VFLIP OFF");
			}
			if(getHFlip(global_device_info) && !getHFlipFromValue(current_camera_flip_setting)) { // need to enable hflip
				camera_set_h_flip(1);
				ESP_LOGW(TAG, "HFLIP ON");
			}
			if(!getHFlip(global_device_info) && getHFlipFromValue(current_camera_flip_setting)) { // need to disable hflip
				camera_set_h_flip(0);
				ESP_LOGW(TAG, "HFLIP OFF");
			}

			current_camera_flip_setting = global_device_info.camera_flip_config;
		}

		// Apply camera AEC settings to camera
		if(current_aec_setting != global_device_info.aec_on) { // ON->OFF or OFF->ON
			if(getAECOn(global_device_info)) {
				camera_set_exposure_ctrl(1, 0);
				ESP_LOGW(TAG, "AEC ON");
			}else{
				camera_set_exposure_ctrl(0, global_device_info.aec_value);
				ESP_LOGW(TAG, "AEC OFF, VAL=%d", global_device_info.aec_value);
				current_aec_value = global_device_info.aec_value;
			}
			current_aec_setting=global_device_info.aec_on;
		}else if(current_aec_value != global_device_info.aec_value && current_aec_setting == 0){ // AEC_ON not switched, but value updated
			ESP_LOGW(TAG, "AEC OFF, VAL=%d", global_device_info.aec_value);
			camera_set_exposure_ctrl(0, global_device_info.aec_value);
			current_aec_value=global_device_info.aec_value;
		}

		// Check if there is a BLE device need RSSI triggered snapshot
		while(xSemaphoreTake(snapshot_devices_list.semaphore, (TickType_t)5) != pdTRUE);
		uint8_t found=0;
		uint8_t found_mac[6];
		int16_t found_rssi=0;
		for(i=0; i<snapshot_devices_list.list_size; i++) {
			for(j=0; j<MAX_SCAN_ITEMS; j++) {
				// Lock mutex
				while(xSemaphoreTake(ble_scan_items_semaphore, (TickType_t)20) != pdTRUE);
				if(ble_scan_items[j].active == 1) {
					if(ble_scan_items[j].device.rssi >= global_device_info.ble_snapshot_rssi) {
						if(mac_address_match(ble_scan_items[j].device.mac_address, snapshot_devices_list.list[i].mac_address)) {
							// Item active; RSSI Threshold reached; BLE MAC Address match, this is a match
							found=1;
							BT_BD_ADDR_ASSIGN(found_mac, snapshot_devices_list.list[i].mac_address);
							found_rssi = ble_scan_items[j].device.rssi;
							xSemaphoreGive(ble_scan_items_semaphore);
							break;
						}
					}
				}
				xSemaphoreGive(ble_scan_items_semaphore);
			}
			if(found == 1) break;
		}
		xSemaphoreGive(snapshot_devices_list.semaphore);

		/*
		 * <flag> = camera_images_buffers[i].camera_fb_consume_needed
		 * Consumer (BTAP) will set flag 1->0, if and only if flag is 1, after sending the fb to server
		 * When flag=1 camera task will never touch that FB and its data (size, buffers, etc)
		 * Consumer (BTAP) NEVER make the flag to 1!
		 * Producer (Camera) set the flag 0->1 after it perform camera_run (snapshot)
		 * When flag=0 BTAp task will never touch the FB and its data as it is possible that it is being accessed by camera task
		 * Consumer (BTAP) will never make modification to FB unless flag=0
		 */

		// Make sure the flag is 0 (consumed by BTAP) before performing snapshot
		for(i=0; i<CAMERA_IMAGES_BUFFER_SIZE; i++) {
			while(xSemaphoreTake(camera_images_buffers[i].camera_fb_semaphore, (TickType_t)5) != pdTRUE);
			if(camera_images_buffers[i].camera_fb_consume_needed == 0) {
				xSemaphoreGive(camera_images_buffers[i].camera_fb_semaphore); // Release semaphore first
				// This buffer is free, let's check if we need to perform snapshots
				if(global_device_info.test_snapshot == 1 || found == 1) {
					err = camera_run(); // blocking call
					if(err == ESP_OK) {
						camera_images_buffers[i].camera_fb = malloc(camera_get_data_size());
						if(camera_images_buffers[i].camera_fb != NULL) { // If PSRAM is full, we are not going using this image
							memcpy(camera_images_buffers[i].camera_fb, camera_get_fb(), camera_get_data_size());
							camera_images_buffers[i].camera_fb_size=camera_get_data_size();

							if(global_device_info.test_snapshot == 1) {
								camera_images_buffers[i].camera_fb_type=1;
								// Reset the test snapshot flag after test snapshot done
								while(xSemaphoreTake(device_info_semaphore, (TickType_t)5) != pdTRUE);
								resetTestSnapshot(&device_info);
								xSemaphoreGive(device_info_semaphore);
							}else if(found == 1) {
								camera_images_buffers[i].camera_fb_type=2;
								BT_BD_ADDR_ASSIGN(camera_images_buffers[i].ble_device.mac_address, found_mac);
								camera_images_buffers[i].ble_device_rssi=found_rssi;
							}
							while(xSemaphoreTake(camera_images_buffers[i].camera_fb_semaphore, (TickType_t)5) != pdTRUE);
							camera_images_buffers[i].camera_fb_consume_needed=1; // Set flag to 1, indicate fb is available, we should not touch fb now
							xSemaphoreGive(camera_images_buffers[i].camera_fb_semaphore);
							if(global_device_info.test_snapshot == 1) {
								ESP_LOGW(TAG, "Test Snap done.");
							}else if(found == 1) {
								ESP_LOGW(TAG, "RSSI Snap done.");
							}
						}else{
							ESP_LOGE(TAG, "Not enough memory to copy image to images buffers.");
						}
					}else{
						ESP_LOGE(TAG, "Error when capturing image!");
					}
				}
				break;
			}
			xSemaphoreGive(camera_images_buffers[i].camera_fb_semaphore);
		}
		vTaskDelay(40 / portTICK_RATE_MS);
	}
}
