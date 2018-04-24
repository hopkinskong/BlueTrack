#include "btap_task.h"
#include "esp_system.h"

const static char* TAG = "BTAP";

uint8_t executeSnifferCommand(uint8_t cmd, uint8_t* arg) {
	ESP_LOGW(TAG, "Execute command: %02X, arg: %02X,%02X,%02X,%02X,%02X,%02X", cmd, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);

	if(cmd == 0x02) { // LED Command
		while(xSemaphoreTake(device_info_semaphore, (TickType_t)5) != pdTRUE);
		setLEDConfig(&device_info, arg[5]);
		ESP_LOGW(TAG, "Set LEDConfig: %02X", getLEDConfig(device_info));
		xSemaphoreGive(device_info_semaphore);
		return 1;
	}else if(cmd == 0x03) { // Add BLE Snapshot hook
		while(xSemaphoreTake(snapshot_devices_list.semaphore, (TickType_t)20) != pdTRUE);
		// Reallocate and enlarge the list if needed
		if(snapshot_devices_list.list_size == snapshot_devices_list.list_max_size) {
			snapshot_devices_list.list_max_size *= 2;
			snapshot_devices_list.list = realloc(snapshot_devices_list.list, sizeof(camera_snapshot_ble_device) * snapshot_devices_list.list_max_size);
			if(snapshot_devices_list.list == NULL) {
				ESP_LOGE(TAG, "Unable to reallocate memory for snapshot list");
				esp_restart(); // force restart
			}
		}

		// Now add new item to the list
		memcpy(snapshot_devices_list.list[snapshot_devices_list.list_size].mac_address, arg, 6);
		snapshot_devices_list.list_size++;
		xSemaphoreGive(snapshot_devices_list.semaphore);
		return 1;
	}else if(cmd == 0x04) { // Remove ALL BLE Snapshot hook
		while(xSemaphoreTake(snapshot_devices_list.semaphore, (TickType_t)20) != pdTRUE);
		// Reallocate to default size
		snapshot_devices_list.list_size=0;
		snapshot_devices_list.list_max_size = 128;
		snapshot_devices_list.list = realloc(snapshot_devices_list.list, sizeof(camera_snapshot_ble_device) * snapshot_devices_list.list_max_size);
		if(snapshot_devices_list.list == NULL) {
			ESP_LOGE(TAG, "Unable to reallocate memory for snapshot list");
			esp_restart(); // force restart
		}
		xSemaphoreGive(snapshot_devices_list.semaphore);
		return 1;
	}else if(cmd == 0x05) { // RSSI Threshold for camera snapshot
		while(xSemaphoreTake(device_info_semaphore, (TickType_t)5) != pdTRUE);
		setBLESnapshotRSSI(&device_info, ((arg[4] << 8) | arg[5]));
		ESP_LOGW(TAG, "Set BLESnapshotRSSI: %d", getBLESnapshotRSSI(device_info));
		xSemaphoreGive(device_info_semaphore);
		return 1;
	}else if(cmd == 0x06) { // VFlip/HFlip
		while(xSemaphoreTake(device_info_semaphore, (TickType_t)5) != pdTRUE);
		setCameraFlipConfig(&device_info, arg[5]);
		ESP_LOGW(TAG, "Set CameraConfig: %02X", getCameraFlipConfig(device_info));
		xSemaphoreGive(device_info_semaphore);
		return 1;
	}else if(cmd == 0x07) { // Test snapshot
		while(xSemaphoreTake(device_info_semaphore, (TickType_t)5) != pdTRUE);
		setTestSnapshot(&device_info);
		ESP_LOGW(TAG, "Set TestSnapshot: 1");
		xSemaphoreGive(device_info_semaphore);
		return 1;
	}else if(cmd == 0x08) { // Reboot Sniffer
		esp_restart();
		return 1;
	}else if(cmd == 0x09) { // AEC
		while(xSemaphoreTake(device_info_semaphore, (TickType_t)5) != pdTRUE);
		setAECConfig(&device_info, arg[3], (arg[4] << 8 | arg[5]));
		ESP_LOGW(TAG, "Set AECConfig: %d, %d", getAECOn(device_info), getAECValue(device_info));
		xSemaphoreGive(device_info_semaphore);
		return 1;
	}

	return 0;
}


uint8_t writeToServer(int socket, uint8_t* data, uint32_t sz, uint32_t* seq) {
	if (write(socket, data, sz) < 0) {
		ESP_LOGE(TAG, "Write BTAP error(%d), restarting connection...", errno);
		close(socket);
		(*seq)=0;
		vTaskDelay(1100 / portTICK_RATE_MS);
		return 0;
	}else{
		return 1;
	}
}

void btap_task(void *pvParameters) {
	const struct addrinfo hints = {
			.ai_family = AF_INET,
			.ai_socktype = SOCK_STREAM
	};
	struct addrinfo *res;
	struct in_addr *addr;
	int s;
	uint8_t sniffer_info_sent;
	uint8_t recv_buf[16];

	while(1) {
		ESP_LOGI(TAG, "Starting connection to BlueTrack Backend");
		sniffer_info_sent=0;

		int err = getaddrinfo(BLUETRACK_BACKEND_IP, BLUETRACK_BACKEND_PORT, &hints, &res);

		if(err != 0 || res == NULL) {
			ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
			vTaskDelay(1000 / portTICK_RATE_MS);
			continue;
		}

		addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
		ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

		s = socket(res->ai_family, res->ai_socktype, 0);
		if(s < 0) {
			ESP_LOGE(TAG, "Failed to allocate socket. errno=%d", errno);
			freeaddrinfo(res);
			vTaskDelay(1000 / portTICK_RATE_MS);
			continue;
		}
		ESP_LOGI(TAG, "Socket allocated.");

		if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
			ESP_LOGE(TAG, "Socket connection failed errno=%d", errno);
			close(s);
			freeaddrinfo(res);
			vTaskDelay(2000 / portTICK_RATE_MS);
			continue;
		}

		ESP_LOGI(TAG, "Connected to BlueTrack Backend!");
		freeaddrinfo(res);

		struct timeval interval = {2, 0};
		if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))	{
			interval.tv_sec = 0;
			interval.tv_usec = 100;
		}

		//setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

		uint32_t seq=0;
		while(1) {
			// SNIFFER_INFO = 0x02
			if(xSemaphoreTake(device_info_semaphore, (TickType_t)5) == pdTRUE) {
				uint8_t packet[] = {'B', 'T', 'A', 'H',
						0x00, 0x00, 0x00, 0x07,
						0x02,
						device_info.id, device_info.battery_percent, 0, 0, 0, 0};
				dword_to_four_bytes(device_info.ip, packet+11);
				xSemaphoreGive(device_info_semaphore);

				if(writeToServer(s, packet, 15, &seq) == 0) break;
				sniffer_info_sent=1;
			}

			if(!sniffer_info_sent) continue; // Must send out SNIFFER_INFO first before sending other types of payload

			// SCAN_RESULT = 0x03
			if(seq % 2 == 0) {
				if(xSemaphoreTake(ble_scan_items_semaphore, (TickType_t)20) == pdTRUE) {
					uint32_t i, payload_size, current_ble_scan_item_written;
					uint8_t tmp[4], fail;

					payload_size=8*ble_scan_item_counts+1+1; // Payload Type + (BLE Results Count + 8*BLE Results)

					if(writeToServer(s, (uint8_t*)((char*)("BTAH")), 4, &seq) == 0) { xSemaphoreGive(ble_scan_items_semaphore); break; }
					dword_to_four_bytes(payload_size, tmp);
					if(writeToServer(s, tmp, 4, &seq) == 0) { xSemaphoreGive(ble_scan_items_semaphore); break; }
					tmp[0]=0x03;
					tmp[1]=ble_scan_item_counts;
					if(writeToServer(s, tmp, 2, &seq) == 0) { xSemaphoreGive(ble_scan_items_semaphore); break; }

					current_ble_scan_item_written=0;
					fail=0;

					for(i=0; i<MAX_SCAN_ITEMS; i++) {
						if(ble_scan_items[i].active == 1) {
							if(writeToServer(s, ble_scan_items[i].device.mac_address, 6, &seq) == 0) fail=1;
							if(fail) break;

							/* NOTE: int16_t need to be sent in reverse, not sure why... */
							tmp[0]=ble_scan_items[i].device.rssi;
							tmp[1]=(ble_scan_items[i].device.rssi >> 8);
							if(writeToServer(s, tmp, 2, &seq) == 0) fail=1;
							if(fail) break;

							ESP_LOGW(TAG, "Sent: %02X:%02X:%02X:%02X:%02X:%02X [%d]", BT_BD_ADDR_HEX(ble_scan_items[i].device.mac_address), ble_scan_items[i].device.rssi);

							current_ble_scan_item_written++;
						}
						if(current_ble_scan_item_written == ble_scan_item_counts) break; // Guard
					}

					if(fail) { xSemaphoreGive(ble_scan_items_semaphore); break; }

					ESP_LOGW(TAG, "Sent BLE items to server: %dBytes, Sent Items=%d, Discovered Items=%d", 4+4+payload_size, current_ble_scan_item_written, ble_scan_item_counts);

					xSemaphoreGive(ble_scan_items_semaphore);
				}
			}

			// SNAPSHOT_IMAGE = 0x04
			uint16_t i=0;
			uint8_t fail=0;
			uint8_t sent_images_in_current_iteration=0;
			for(i=0; i<CAMERA_IMAGES_BUFFER_SIZE; i++) {
				// Don't send too much for within single iteration, or else other packets (i.e. sniffer_info, scan_result) might blocked
				// We will also never receive any data from server
				if(sent_images_in_current_iteration == MAX_IMAGES_SENT_PER_ITERATION) break;
				while(xSemaphoreTake(camera_images_buffers[i].camera_fb_semaphore, (TickType_t)5) != pdTRUE);
				if(camera_images_buffers[i].camera_fb_consume_needed == 1) {
					xSemaphoreGive(camera_images_buffers[i].camera_fb_semaphore);

					// As the flag (camera_images_buffers[i].camera_fb_consume_needed) is set to 1
					// We can safely assume that the camera task will never access our buffer and its data,
					// Until we set it back to 0 again
					// We should also make sure we free the buffer before setting it to 0 to avoid memory leaks

					uint8_t packet_hdr[] = {'B', 'T', 'A', 'H',
							0x00, 0x00, 0x00, 0x00, // Size is updated below
							0x04,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8Bytes 0x00 BLE Result set (if test snapshot),
							0x00, 0x00, 0x00, 0x00 // JPEG Size
					};

					if(camera_images_buffers[i].camera_fb_type == 2) { // BLE RSSI Triggered Snapshot
						uint8_t *ble_result_set_ptr = packet_hdr+9;
						BT_BD_ADDR_ASSIGN(ble_result_set_ptr, camera_images_buffers[i].ble_device.mac_address);
						/* NOTE: int16_t need to be sent in reverse, not sure why... */
						ble_result_set_ptr[6] = camera_images_buffers[i].ble_device_rssi;
						ble_result_set_ptr[7] = camera_images_buffers[i].ble_device_rssi >> 8;
					}
					// Payload size: 1-byte PL Type + 8-Bytes BLE Res. Set + 4-Bytes JPEG Size + FB SIZE
					dword_to_four_bytes((1+8+4+camera_images_buffers[i].camera_fb_size), packet_hdr+4);
					// Camera FB Size:
					dword_to_four_bytes((camera_images_buffers[i].camera_fb_size), packet_hdr+17);
					// Write packet header first
					if(writeToServer(s, packet_hdr, 21, &seq) == 0) {
						// If fail to write to server, we should restart the connection and retransmit to server
						// So, we should NOT set the flag to 0
						fail=1;
						break;
					}
					// Then write JPEG
					if(writeToServer(s, camera_images_buffers[i].camera_fb, camera_images_buffers[i].camera_fb_size, &seq) == 0) {
						// If fail to write to server, we should restart the connection and retransmit to server
						// So, we should NOT set the flag to 0
						fail=1;
						break;
					}
					free(camera_images_buffers[i].camera_fb);
					sent_images_in_current_iteration++;
					ESP_LOGW(TAG, "Snapshot written to server. Size=%d", camera_images_buffers[i].camera_fb_size);
					while(xSemaphoreTake(camera_images_buffers[i].camera_fb_semaphore, (TickType_t)8) != pdTRUE);
					camera_images_buffers[i].camera_fb_consume_needed=0;
					xSemaphoreGive(camera_images_buffers[i].camera_fb_semaphore);
				}else{
					xSemaphoreGive(camera_images_buffers[i].camera_fb_semaphore);
				}
			}
			if(fail == 1) break; // Let's reconnect to the server if something went wrong
			seq++;

			// Read commands from server + send back 0x06 for COMMAND_ACCEPT
			uint8_t needToRead = 1;
			fail=0;
			while(needToRead) {
				ssize_t sizeRead = recv(s, recv_buf, 16, MSG_DONTWAIT);
				if(sizeRead == 0) {
					fail=1;
					ESP_LOGE(TAG, "Read BTAP error: Server closed the connection");
					close(s);
					break;
				}else if(sizeRead < 0) {
					if(errno == 11) {
						needToRead=0; // We have already read all data from TCP buffer
						break;
					}else{
						fail=1;
						ESP_LOGE(TAG, "Read BTAP error(%d), restarting connection...", errno);
						close(s);
						break;
					}
				}else if(sizeRead == 16){
					if(recv_buf[0] != 'B' || recv_buf[1] != 'T' || recv_buf[2] != 'A' || recv_buf[3] != 'H' ||
							recv_buf[4] != 0x00 || recv_buf[5] != 0x00 || recv_buf[6] != 0x00 || recv_buf[7] != 0x08 ||
							recv_buf[8] != 0x05) {
						fail=1;
						ESP_LOGE(TAG, "Read BTAP error: Invalid BTAP HDR/PL");
						ESP_LOGE(TAG, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", recv_buf[0], recv_buf[1], recv_buf[2], recv_buf[3], recv_buf[4], recv_buf[5], recv_buf[6], recv_buf[7], recv_buf[8], recv_buf[9]);
						close(s);
						break;
					}
					if(executeSnifferCommand(recv_buf[9], recv_buf+10)) {
						recv_buf[8] = 0x06; // COMMAND_ACCEPT
						if(writeToServer(s, recv_buf, 16, &seq) == 0) { fail=1; break; }
					}else{
						ESP_LOGE(TAG, "Execute sniffer command failed.");
					}
				}else{
					fail=1;
					ESP_LOGE(TAG, "Read BTAP error: Invalid packet size");
					close(s);
					break;
				}
			}
			if(fail) break;

			vTaskDelay(500 / portTICK_RATE_MS);
		}
	}
}
