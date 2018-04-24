#include "bluetrack_application_protocol.h"
#include "worker.h"
#include "image_preprocessing.h"
#include "victims_photos.h"

uint8_t isPayloadValid(uint8_t* payload) {
	if(payload[0] == 0x02) return 1; // SNIFFER_INFO
	if(payload[0] == 0x03) return 1; // SCAN_RESULT
	if(payload[0] == 0x04) return 1; // SNAPSHOT_IMAGE
	if(payload[0] == 0x05) return 1; // SNIFFER_COMMAND
	if(payload[0] == 0x06) return 1; // COMMAND_ACCEPT
	return 0;
}

uint8_t isSnifferCharging(sniffer_info info) {
	return ((info.battery_percentage & 0x80) ? 1 : 0);
}

uint8_t getSnifferBatteryPercentage(sniffer_info info) {
	return (info.battery_percentage & 0x7F);
}

extern sniffer_context sniffers[64];
int processPayload(uint8_t* payload, uint32_t len, uint8_t* sniffer_id, uint8_t sniffer_id_assigned) {
	payload_type type = (payload_type)payload[0]; // Payload type
	uint8_t *payloadDataPtr = payload+1; // Payload data
	char TAG[12];

	// Set the TAG
	if(!sniffer_id_assigned) {
		strncpy(TAG, "PROCESS???", sizeof(TAG));
	}else{
		snprintf(TAG, sizeof(TAG), "PROCESS_%d", *sniffer_id);
	}

	if(!sniffer_id_assigned && type != SNIFFER_INFO) {
		// Only SNIFFER_INFO can set sniffer ID
		// With sniffer_id == NULL, other payload types are invalid
		log_error(TAG, "Payload received but no sniffer id is set, disconnecting...");
		return 1;
	}

	switch(type) {
	case SNIFFER_INFO:
		// 6-bytes data
		// make sure payload data is large enough
		if((4+4+len) == (4+4+1+6)) {
			// Valid SNIFFER_INFO payload
			//log_success(TAG, "Recv: SNIFFER_INFO");
			sniffer_info* info = (sniffer_info *)payloadDataPtr;

			if(sniffer_id_assigned) {
				if(*sniffer_id != info->sniffer_id) {
				// Sniffer ID has changed
				// This is not allowed, disconnect the client.
				log_error(TAG, "Sniffer ID changed, disconnecting...");
				return 0;
				}
			}else{
				// Previously not assigned, now we assign it
				// Also give a new TAG
				*sniffer_id = info->sniffer_id;
				snprintf(TAG, sizeof(TAG), "PROCESS_%d", *sniffer_id);


				// Assigning to existing ID is prohibited
				if(sniffers[info->sniffer_id].connected) {
					log_error(TAG, "Sniffer ID is already exists, disconnecting...");
					return 0;
				}

				sniffers[info->sniffer_id].connected=1;
			}

			pthread_mutex_lock(&sniffers[info->sniffer_id].mutex);
			sniffers[info->sniffer_id].info.battery_percentage = info->battery_percentage; // Already includes %+CHG_DET
			memcpy(sniffers[info->sniffer_id].info.ip_address, info->ip_address, 4);
			pthread_mutex_unlock(&sniffers[info->sniffer_id].mutex);

		}else{
			// Invalid SNIFFER_INFO payload
			log_error(TAG, "SNIFFER_INFO Length mismatch, disconnecting...");
			return 0;
		}
		break;
	case SCAN_RESULT:
		//log_success(TAG, "Notify SCAN_RESULT to worker");
		//uint8_t i;
		;
		uint8_t totalBLEResults = payloadDataPtr[0];

		if(totalBLEResults > 64) {
			log_error(TAG, "Received too many BLE devices: %d, disconnecting...", totalBLEResults);
			return 0;
		}

		if((4+4+len) != (4+4+1+1+totalBLEResults*8)) {
			log_error(TAG, "Payload length mismatch, disconnecting...");
			return 0;
		}

		pthread_mutex_lock(&sniffers[(*sniffer_id)].mutex);
		uint8_t* resultPtr = payloadDataPtr+1; // Payload Data: <Result Count> <8*Result Counts>
		sniffers[(*sniffer_id)].ble_devices_count=totalBLEResults;
		memcpy(sniffers[(*sniffer_id)].ble_devices, resultPtr, sizeof(scan_result_set)*totalBLEResults);
		pthread_mutex_unlock(&sniffers[(*sniffer_id)].mutex);

		//log_error(TAG, "===BLE RESULTS===");
		//uint8_t* resultPtr = payloadDataPtr+1; // Payload Data: <Result Count> <8*Result Counts>
		//for(i=0; i<totalBLEResults; i++) {
		//	scan_result_set* dev = (scan_result_set*) resultPtr;
		//	log_error(TAG, "MAC: %02X:%02X:%02X:%02X:%02X:%02X RSSI:%d", dev->ble_mac_address[0], dev->ble_mac_address[1], dev->ble_mac_address[2], dev->ble_mac_address[3],dev->ble_mac_address[4], dev->ble_mac_address[5], dev->rssi);
		//	resultPtr += sizeof(scan_result_set);
		//}
		break;
	case SNAPSHOT_IMAGE:
		;
		uint32_t jpegDataSize = to32Bits(payloadDataPtr[8], payloadDataPtr[9], payloadDataPtr[10], payloadDataPtr[11]);

		if((4+4+len) != (4+4+1+8+4+jpegDataSize)) {
			log_error(TAG, "JPEG Payload length mismatch, disconnecting...");
			return 0;
		}
		if(payloadDataPtr[0] == 0x00 && payloadDataPtr[1] == 0x00 &&
				payloadDataPtr[2] == 0x00 && payloadDataPtr[3] == 0x00 &&
				payloadDataPtr[4] == 0x00 && payloadDataPtr[5] == 0x00 &&
				payloadDataPtr[6] == 0x00 && payloadDataPtr[7] == 0x00) {
			// Test snapshot
			log_info(TAG, "Received JPEG data for test snapshot for #%d, size=%d", *sniffer_id, jpegDataSize);
			pthread_mutex_lock(&sniffers[(*sniffer_id)].snapshot.mutex);
			if(sniffers[(*sniffer_id)].snapshot.img_valid == 1) free(sniffers[(*sniffer_id)].snapshot.img); // Previously have image stored (img_valid == 1), free it first
			sniffers[(*sniffer_id)].snapshot.img=malloc(jpegDataSize);
			if(sniffers[(*sniffer_id)].snapshot.img == NULL) {
				sniffers[(*sniffer_id)].snapshot.img_valid=0;
				log_error(TAG, "Unable to allocate memory for snapshot image");
			}else{
				memcpy(sniffers[(*sniffer_id)].snapshot.img, (payloadDataPtr+8+4), jpegDataSize);
				preprocess_image(&sniffers[(*sniffer_id)].snapshot.img, &jpegDataSize);
				sniffers[(*sniffer_id)].snapshot.img_valid=1;
				sniffers[(*sniffer_id)].snapshot.img_sz=jpegDataSize;
			}
			pthread_mutex_unlock(&sniffers[(*sniffer_id)].snapshot.mutex);
		}else{
			uint8_t *mac = payloadDataPtr;
			int16_t rssi = 0;
			memcpy(&rssi, mac+6, 2);
			//log_info(TAG, "Received JPEG Data for RSSI shot for #%d, size=%d, RSSI=%d, MAC=%02X:%02X:%02X:%02X:%02X:%02X", *sniffer_id, jpegDataSize, rssi, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			uint8_t *tmp = malloc(jpegDataSize);
			if(tmp == NULL) {
				log_error(TAG, "Unable to allocate memory before image preprocessing for RSSI Image.");
				break;
			}
			memcpy(tmp, (payloadDataPtr+8+4), jpegDataSize);
			preprocess_image(&tmp, &jpegDataSize);
			save_victim_photo(mac, &tmp, jpegDataSize, rssi);
		}


		break;
	case SNIFFER_COMMAND:
		// We are not sniffers
		break;
	case COMMAND_ACCEPT:
		// Do nothing for now
		break;
	}
	return 1;

}

/*
 * Make sure buf, is at least 16bytes, this function will not check against size!
 */
void generateCommand(uint8_t* buf, command cmd, uint8_t* arg) {
	// BTAH+0x00000008+0x05+CMD+ARGS(6bytes)

	payload_type type = SNIFFER_COMMAND;

	buf[0]='B'; buf[1]='T'; buf[2]='A'; buf[3]='H';
	buf[4]=0x00; buf[5]=0x00; buf[6]=0x00; buf[7]=0x08;
	buf[8]=(uint8_t)type; buf[9]=(uint8_t)cmd;

	if(arg != NULL) {
		buf[10]=arg[0];
		buf[11]=arg[1];
		buf[12]=arg[2];
		buf[13]=arg[3];
		buf[14]=arg[4];
		buf[15]=arg[5];
	}else{
		buf[10]=buf[11]=buf[12]=buf[13]=buf[14]=buf[15]=0x00;
	}
}

void generateVFlipHFlipArg(uint8_t* arg, sniffer_settings settings) {
	if(!settings.h_flip && !settings.v_flip) arg[5]=0x03;
	if(settings.h_flip && settings.v_flip) arg[5]=0x02;
	if(!settings.h_flip && settings.v_flip) arg[5]=0x01;
	if(settings.h_flip && !settings.v_flip) arg[5]=0x00;
}

extern victims_to_track_list victims_list;

/*
 * WARNING: Should have locked victims_list mutex before calling this function
 */
size_t getInitialConfigPackets(uint8_t sniffer_id, sniffer_settings* current_sniffer_settings, sniffer_snapshot_ble_devices_list *snapshot_list, uint8_t* ptr) {
	size_t sz=0;
	// Get current sniffer settings from sniffer_context
	pthread_mutex_lock(&sniffers[sniffer_id].mutex);
	*current_sniffer_settings = sniffers[sniffer_id].settings;
	pthread_mutex_unlock(&sniffers[sniffer_id].mutex);

	// Initialize current_sniffer_victims
	snapshot_list->list_max_size=128;
	snapshot_list->list_size=0;
	snapshot_list->list=malloc(sizeof(sniffer_snapshot_ble_device)*snapshot_list->list_max_size);

	uint8_t *currentPtr=ptr;
	uint8_t arg[6];
	memset(arg, 0x00, 6);
	// Camera configuration
	generateVFlipHFlipArg(arg, *current_sniffer_settings);
	generateCommand(currentPtr, CAMERA_FLIP, arg);
	currentPtr+=16;
	sz+=16;

	// LED configuration
	arg[5]=current_sniffer_settings->led;
	generateCommand(currentPtr, LED, arg);
	currentPtr+=16;
	sz+=16;

	// Send out Snapshot RSSI Config
	arg[4]=current_sniffer_settings->rssi_threshold >> 8;
	arg[5]=current_sniffer_settings->rssi_threshold;
	generateCommand(currentPtr, SNAPSHOT_RSSI, arg);
	currentPtr+=16;
	sz+=16;

	// Clear snapshot hooks
	generateCommand(currentPtr, SNAPSHOT_HOOK_CLEAR, NULL);
	currentPtr+=16;
	sz+=16;

	// Populate current snapshot hooks
	for(uint32_t i=0; i<victims_list.list_size; i++) {
		if(victims_list.victims_list[i].is_active) {
			getMACHexVal(victims_list.victims_list[i].mac, arg);
			generateCommand(currentPtr, SNAPSHOT_HOOK, arg);
			currentPtr+=16;
			sz+=16;

			// Add to snapshot_list
			// Resize if needed
			if(!autoResizeList(&snapshot_list->list_size, &snapshot_list->list_max_size, (void **)&snapshot_list->list, sizeof(sniffer_snapshot_ble_device))) {
				log_error("MALLOC", "Failed to reallocate memory for local sniffer snapshot list.");
				return 0;
			}
			// Then, we add to the list
			memcpy(snapshot_list->list[snapshot_list->list_size].mac_address, arg, 6);
			snapshot_list->list_size++;
		}
	}

	// Send out AEC Config
	arg[3]=current_sniffer_settings->aec_on;
	arg[4]=current_sniffer_settings->aec_value >> 8;
	arg[5]=current_sniffer_settings->aec_value;
	generateCommand(currentPtr, AEC_CONFIG, arg);
	currentPtr+=16;
	sz+=16;

	return sz;
}

uint8_t issueCommandNeeded(command cmd, sniffer_settings* old_settings, sniffer_settings* new_settings) {
	if(cmd == LED) {
		return (old_settings->led != new_settings->led);
	}else if(cmd == SNAPSHOT_RSSI) {
		return (old_settings->rssi_threshold != new_settings->rssi_threshold);
	}else if(cmd == CAMERA_FLIP) {
		return (old_settings->h_flip != new_settings->h_flip || old_settings->v_flip != new_settings->v_flip);
	}else if(cmd == TEST_SNAPSHOT) {
		return (old_settings->test_snapshot == 0 && new_settings->test_snapshot == 1);
	}else if(cmd == REBOOT) {
		return (old_settings->reboot != new_settings->reboot);
	}else if(cmd == AEC_CONFIG) {
		return ((old_settings->aec_on != new_settings->aec_on) || (old_settings->aec_value != new_settings->aec_value));
	}
	return 0;
}

/*
 * Size is not fixed:
 * 16*n-packets, as n-packets could change depends on whether that settings is updated
 * So, we need to return the packet size accurately
 */
size_t getNewConfigPackets(uint8_t sniffer_id, sniffer_settings *current_sniffer_settings, sniffer_snapshot_ble_devices_list *snapshot_list, uint8_t* ptr) {
	sniffer_settings settings_from_context;
	uint8_t *currentPtr=ptr;
	uint8_t arg[6];
	size_t sz=0;

	memset(arg, 0x00, 6);

	pthread_mutex_lock(&sniffers[sniffer_id].mutex);
	settings_from_context = sniffers[sniffer_id].settings; // Get the "new" version of sniffer_settings from sniffer_context
	pthread_mutex_unlock(&sniffers[sniffer_id].mutex);

	// Test if we need to update camera settings
	if(issueCommandNeeded(CAMERA_FLIP, current_sniffer_settings, &settings_from_context)) {
		current_sniffer_settings->h_flip=settings_from_context.h_flip;
		current_sniffer_settings->v_flip=settings_from_context.v_flip;
		generateVFlipHFlipArg(arg, *current_sniffer_settings);
		generateCommand(currentPtr, CAMERA_FLIP, arg);
		currentPtr+=16;
		sz+=16;
	}
	// Test if we need to update LED settings
	if(issueCommandNeeded(LED, current_sniffer_settings, &settings_from_context)) {
		current_sniffer_settings->led=settings_from_context.led;
		arg[5]=current_sniffer_settings->led;
		generateCommand(currentPtr, LED, arg);
		currentPtr+=16;
		sz+=16;
	}
	// Test if we need to update RSSI settings
	if(issueCommandNeeded(SNAPSHOT_RSSI, current_sniffer_settings, &settings_from_context)) {
		current_sniffer_settings->rssi_threshold=settings_from_context.rssi_threshold;
		arg[4]=current_sniffer_settings->rssi_threshold >> 8;
		arg[5]=current_sniffer_settings->rssi_threshold;
		generateCommand(currentPtr, SNAPSHOT_RSSI, arg);
		currentPtr+=16;
		sz+=16;
	}
	// Test if we need to reboot
	if(issueCommandNeeded(REBOOT, current_sniffer_settings, &settings_from_context)) {
		// This flag we got reset when the sniffer reconnects
		// No special treatment needed
		generateCommand(currentPtr, REBOOT, arg);
		currentPtr+=16;
		sz+=16;
	}
	// Test if we need to test snapshot
	if(issueCommandNeeded(TEST_SNAPSHOT, current_sniffer_settings, &settings_from_context)) {
		// We need to reset this flag as sniffer won't reconnect
		pthread_mutex_lock(&sniffers[sniffer_id].mutex);
		sniffers[sniffer_id].settings.test_snapshot=0;
		pthread_mutex_unlock(&sniffers[sniffer_id].mutex);
		current_sniffer_settings->test_snapshot=0;
		generateCommand(currentPtr, TEST_SNAPSHOT, arg);
		currentPtr+=16;
		sz+=16;

	}

	// Test if the list is updated
	uint8_t updated=0;
	uint32_t i, j=0, active_count=0;
	if(victims_list.list_size > 0) {
		for(i=0; i<victims_list.list_size; i++) {
			if(victims_list.victims_list[i].is_active) {
				if(snapshot_list->list_size > j) { // prevent out of bound access
					uint8_t mac[6];
					getMACHexVal(victims_list.victims_list[i].mac, mac);
					if(!mac_address_match(snapshot_list->list[j].mac_address, mac)) {
						updated=1;
						break;
					}
				}else{
					// Wanted to access out of bound, that means current list is larger
					updated=1;
					break;
				}
				j++;
				active_count++;
			}
		}
	}
	if(active_count != snapshot_list->list_size) updated=1; // If two lists are identical, the active count from victims_list should equal to snapshot_list

	if(updated) {
		// Clear snapshot hooks
		generateCommand(currentPtr, SNAPSHOT_HOOK_CLEAR, NULL);
		currentPtr+=16;
		sz+=16;

		// New list
		snapshot_list->list_max_size=128;
		snapshot_list->list_size=0;
		snapshot_list->list=realloc(snapshot_list->list, sizeof(sniffer_snapshot_ble_device)*snapshot_list->list_max_size);
		if(snapshot_list->list == NULL) {
			log_error("MALLOC", "Failed to reallocate memory for local sniffer snapshot list.");
			return 0;
		}

		for(uint32_t i=0; i<victims_list.list_size; i++) {
			if(victims_list.victims_list[i].is_active) {
				uint8_t mac[6];
				getMACHexVal(victims_list.victims_list[i].mac, mac);
				memcpy(arg, mac, 6);
				generateCommand(currentPtr, SNAPSHOT_HOOK, arg);
				currentPtr+=16;
				sz+=16;

				// Add to snapshot_list
				// Resize if needed
				if(!autoResizeList(&snapshot_list->list_size, &snapshot_list->list_max_size, (void **)&snapshot_list->list, sizeof(sniffer_snapshot_ble_device))) {
					log_error("MALLOC", "Failed to reallocate memory for local sniffer snapshot list.");
					return 0;
				}
				// Then, we add to the list
				memcpy(snapshot_list->list[snapshot_list->list_size].mac_address, mac, 6);
				snapshot_list->list_size++;
			}
		}
	}

	// Test if we need to update AEC config
	if(issueCommandNeeded(AEC_CONFIG, current_sniffer_settings, &settings_from_context)) {
		current_sniffer_settings->aec_on=settings_from_context.aec_on;
		current_sniffer_settings->aec_value=settings_from_context.aec_value;
		arg[3]=current_sniffer_settings->aec_on;
		arg[4]=current_sniffer_settings->aec_value >> 8;
		arg[5]=current_sniffer_settings->aec_value;
		generateCommand(currentPtr, AEC_CONFIG, arg);
		currentPtr+=16;
		sz+=16;

	}

	return sz;
}

void resetConfigFlags(uint8_t sniffer_id) {
	pthread_mutex_lock(&sniffers[sniffer_id].mutex);

	sniffers[sniffer_id].settings.reboot=0;
	sniffers[sniffer_id].settings.test_snapshot=0;
	pthread_mutex_unlock(&sniffers[sniffer_id].mutex);
}
