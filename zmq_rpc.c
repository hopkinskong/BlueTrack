#include "zmq_rpc.h"

const static char *TAG = "ZMQRPC";

extern sniffer_context sniffers[64];
extern discovered_ble_devices_list ble_devices_list;
extern victims_to_track_list victims_list;

static int test(json_t *json_params, json_t **result, void *userdata) {
	json_error_t error;
	char* str;

	if(json_unpack_ex(json_params, &error, 0, "[s!]", &str) != -1) {
		*result = json_string("This is a test from C!");
		log_success(TAG, "Test called, string: %s", str);
		return 0;
	}else{
		*result = jsonrpc_error_object_predefined(JSONRPC_INVALID_PARAMS, json_string(error.text));
		return -1;
	}

	return 0;
}

static int rpc_get_sniffers_count(json_t *json_params, json_t **result, void *userdata) {
	uint8_t i, count=0;
	for(i=0; i<64; i++) {
		if(sniffers[i].connected) count++;
	}

	*result = json_integer(count);
	return 0;
}

static int rpc_get_sniffers_list(json_t *json_params, json_t **result, void *userdata) {
	char buf[64];
	uint8_t i;
	json_t *result_array = json_array();
	for(i=0; i<64; i++) {
		if(sniffers[i].connected) {
			pthread_mutex_lock(&sniffers[i].mutex);
			json_t *obj = json_object();
			json_t *settings = json_object();
			json_object_set(obj, "id", json_integer(sniffers[i].info.sniffer_id));
			json_object_set(obj, "ip", json_string(getIPString(sniffers[i].info.ip_address, buf, 64)));
			json_object_set(obj, "bat", json_integer(getSnifferBatteryPercentage(sniffers[i].info)));
			json_object_set(obj, "chg", json_boolean(isSnifferCharging(sniffers[i].info)));
			json_object_set(obj, "devs", json_integer(sniffers[i].ble_devices_count));
			json_object_set(settings, "h_flip", json_integer(sniffers[i].settings.h_flip));
			json_object_set(settings, "v_flip", json_integer(sniffers[i].settings.v_flip));
			json_object_set(settings, "led", json_integer(sniffers[i].settings.led));
			json_object_set(settings, "rssi_threshold", json_integer(sniffers[i].settings.rssi_threshold));
			json_object_set(settings, "aec_on", json_integer(sniffers[i].settings.aec_on));
			json_object_set(settings, "aec_value", json_integer(sniffers[i].settings.aec_value));
			json_object_set(obj, "settings", settings);
			json_array_append(result_array, obj);
			pthread_mutex_unlock(&sniffers[i].mutex);
		}
	}

	*result = result_array;
	return 0;
}

static int rpc_get_total_ble_devices(json_t *json_params, json_t **result, void *userdata) {
	*result = json_integer(ble_devices_list.list_size);
	return 0;
}

static int rpc_get_ble_devices_list(json_t *json_params, json_t **result, void *userdata) {
	char buf[64];
	uint32_t i, j;
	json_t *result_array = json_array();
	pthread_mutex_lock(&ble_devices_list.mutex);
	for(i=0; i<ble_devices_list.list_size; i++) {
		json_t *obj = json_object();
		json_t *rssis_array = json_array();
		json_object_set(obj, "mac", json_string(getMACString(ble_devices_list.devices_list[i].ble_addr, buf, 64)));
		json_object_set(obj, "nodes", json_integer(ble_devices_list.devices_list[i].discovered_sniffer_nodes_count));
		for(j=0; j<ble_devices_list.devices_list[i].discovered_sniffer_nodes_count; j++) {
			json_t *rssi_obj = json_object();
			json_object_set(rssi_obj, "id", json_integer(ble_devices_list.devices_list[i].sniffer_nodes[j].sniffer_id));
			json_object_set(rssi_obj, "rssi", json_integer(ble_devices_list.devices_list[i].sniffer_nodes[j].rssi));
			json_array_append(rssis_array, rssi_obj);
		}
		json_object_set(obj, "rssis", rssis_array);
		json_object_set(obj, "tracking", json_boolean(ble_devices_list.devices_list[i].is_tracking));
		json_array_append(result_array, obj);
	}
	pthread_mutex_unlock(&ble_devices_list.mutex);
	*result = result_array;
	return 0;
}

static int rpc_get_total_victims_to_track(json_t *json_params, json_t **result, void *userdata) {
	uint32_t i=0, count=0;
	pthread_mutex_lock(&ble_devices_list.mutex);
	for(i=0; i<ble_devices_list.list_size; i++) {
		if(ble_devices_list.devices_list[i].is_tracking) count++;
	}
	pthread_mutex_unlock(&ble_devices_list.mutex);
	*result = json_integer(count);
	return 0;
}

static int rpc_get_total_photos_captured(json_t *json_params, json_t **result, void *userdata) {
	uint32_t count;

	if(database_get_total_victims_photos(&count) == 1) return -1;

	*result = json_integer(count);
	return 0;
}

static int rpc_get_victims_photos_summary(json_t *json_params, json_t **result, void *userdata) {
	uint32_t i;
	json_t *result_array = json_array();
	victims_photos_summary_list list;
	list.list_max_size=2;
	list.list_size=0;
	list.list = malloc(sizeof(victim_photos_summary) * 2);
	get_victims_photos_summary(&list);

	for(i=0; i<list.list_size; i++) {
		json_t *obj = json_object();
		json_object_set(obj, "mac", json_string(list.list[i].mac));
		json_object_set(obj, "name", json_string(list.list[i].name));
		json_object_set(obj, "tracking", json_boolean(list.list[i].is_active));
		json_object_set(obj, "photos", json_integer(list.list[i].photos_captured));
		json_object_set(obj, "nnm_processed", json_integer(list.list[i].nnm_processed));
		json_array_append(result_array, obj);
	}

	free(list.list);

	*result = result_array;
	return 0;
}

static int rpc_set_h_flip(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	uint8_t id = 255;
	uint8_t setting = 255;

	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "enabled") == 0) {
			setting = json_boolean_value(value);
		}else if(strcmp(key, "sniffer_id") == 0) {
			id = json_integer_value(value);
		}
	}
	if(id > 63 || setting > 1) return -1;
	pthread_mutex_lock(&sniffers[id].mutex);
	sniffers[id].settings.h_flip=setting;
	pthread_mutex_unlock(&sniffers[id].mutex);

	database_update_value_integer_val("sniffer_settings", "sniffer_id", id, "h_flip", setting);

	return 0;
}

static int rpc_set_v_flip(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	uint8_t id = 255;
	uint8_t setting = 255;

	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "enabled") == 0) {
			setting = json_boolean_value(value);
		}else if(strcmp(key, "sniffer_id") == 0) {
			id = json_integer_value(value);
		}
	}
	if(id > 63 || setting > 1) return -1;
	pthread_mutex_lock(&sniffers[id].mutex);
	sniffers[id].settings.v_flip=setting;
	pthread_mutex_unlock(&sniffers[id].mutex);

	database_update_value_integer_val("sniffer_settings", "sniffer_id", id, "v_flip", setting);

	return 0;
}

static int rpc_set_led(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	uint8_t id = 255;
	uint8_t setting = 255;

	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "mode") == 0) {
			setting = json_integer_value(value);
		}else if(strcmp(key, "sniffer_id") == 0) {
			id = json_integer_value(value);
		}
	}
	if(id > 63 || setting > 2) return -1;
	pthread_mutex_lock(&sniffers[id].mutex);
	sniffers[id].settings.led=setting;
	pthread_mutex_unlock(&sniffers[id].mutex);

	database_update_value_integer_val("sniffer_settings", "sniffer_id", id, "led", setting);

	return 0;
}

static int rpc_set_aec(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	uint8_t id = 255;
	uint8_t setting = 255;
	uint16_t aec_val = 65535;

	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "aec_on") == 0) {
			setting = json_boolean_value(value);
		}else if(strcmp(key, "aec_value") == 0) {
			aec_val = json_integer_value(value);
		}else if(strcmp(key, "sniffer_id") == 0) {
			id = json_integer_value(value);
		}
	}
	if(id > 63 || setting > 1) return -1;
	pthread_mutex_lock(&sniffers[id].mutex);
	sniffers[id].settings.aec_on=setting;
	sniffers[id].settings.aec_value=aec_val;
	pthread_mutex_unlock(&sniffers[id].mutex);

	database_update_value_integer_val("sniffer_settings", "sniffer_id", id, "aec_on", setting);
	database_update_value_integer_val("sniffer_settings", "sniffer_id", id, "aec_value", aec_val);

	return 0;
}

static int rpc_set_rssi_threshold(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	uint8_t id = 255;
	int16_t setting = 255;

	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "threshold") == 0) {
			setting = json_integer_value(value);
		}else if(strcmp(key, "sniffer_id") == 0) {
			id = json_integer_value(value);
		}
	}
	if(id > 63 || (setting > 256 && setting < -256)) return -1;
	pthread_mutex_lock(&sniffers[id].mutex);
	sniffers[id].settings.rssi_threshold=setting;
	pthread_mutex_unlock(&sniffers[id].mutex);


	database_update_value_integer_val("sniffer_settings", "sniffer_id", id, "rssi_threshold", setting);

	return 0;
}

static int rpc_reboot(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	uint8_t id = 255;

	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "sniffer_id") == 0) {
			id = json_integer_value(value);
		}
	}
	if(id > 63) return -1;
	pthread_mutex_lock(&sniffers[id].mutex);
	sniffers[id].settings.reboot=1;
	pthread_mutex_unlock(&sniffers[id].mutex);

	return 0;
}

static int rpc_test_snapshot(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	uint8_t id = 255;

	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "sniffer_id") == 0) {
			id = json_integer_value(value);
		}
	}
	if(id > 63) return -1;
	pthread_mutex_lock(&sniffers[id].mutex);
	sniffers[id].settings.test_snapshot=1;
	pthread_mutex_unlock(&sniffers[id].mutex);

	return 0;
}

static int rpc_get_test_snapshots(json_t *json_params, json_t **result, void *userdata) {
	uint32_t i;
	char *jpeg_base64;
	json_t *result_array = json_array();
	for(i=0; i<64; i++) {
		if(sniffers[i].connected) {
			pthread_mutex_lock(&sniffers[i].snapshot.mutex);
			if(sniffers[i].snapshot.img_valid == 1) {
				json_t *obj = json_object();
				json_object_set(obj, "id", json_integer(i));
				jpeg_base64 = b64_encode(sniffers[i].snapshot.img, sniffers[i].snapshot.img_sz);
				json_object_set(obj, "img", json_string(jpeg_base64));
				json_array_append(result_array, obj);
				free(jpeg_base64);
				sniffers[i].snapshot.img_valid=0;
				free(sniffers[i].snapshot.img);
			}
			pthread_mutex_unlock(&sniffers[i].snapshot.mutex);
		}
	}
	*result = result_array;
	return 0;
}

static int rpc_enable_tracking(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	char *mac = NULL;

	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "mac") == 0) {
			mac = strdup(json_string_value(value));
		}
	}
	if(mac == NULL) return -1;

	log_info(TAG, "Start track victim: %s", mac);
	database_start_track_victim(mac);

	free(mac);

	return 0;
}

static int rpc_disable_tracking(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	char *mac = NULL;

	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "mac") == 0) {
			mac = strdup(json_string_value(value));
		}
	}
	if(mac == NULL) return -1;

	log_info(TAG, "Stop track victim: %s", mac);
	database_stop_track_victim(mac);

	free(mac);

	return 0;
}

static int rpc_get_tracking_list(json_t *json_params, json_t **result, void *userdata) {
	uint32_t i, j;

	json_t *result_array = json_array();
	char buf[64];

	pthread_mutex_lock(&victims_list.mutex);
	for(i=0; i<victims_list.list_size; i++) {
		json_t *obj = json_object();
		json_object_set(obj, "mac", json_string(victims_list.victims_list[i].mac));
		json_object_set(obj, "name", json_string(victims_list.victims_list[i].name));
		uint8_t online=0;
		uint8_t total_sniffers_heard=0;

		// Scan this victim BLE MAC against the ble_devices_list
		pthread_mutex_lock(&ble_devices_list.mutex);
		for(j=0; j<ble_devices_list.list_size; j++) {
			if(strcmp(victims_list.victims_list[i].mac, getMACString(ble_devices_list.devices_list[j].ble_addr, buf, 64)) == 0) {
				// Device is in tracking list and also in ble_devices_list
				online=1;
				total_sniffers_heard = ble_devices_list.devices_list[j].discovered_sniffer_nodes_count;
				break;
			}
		}
		pthread_mutex_unlock(&ble_devices_list.mutex);

		json_object_set(obj, "online", json_boolean(online));
		json_object_set(obj, "active", json_boolean(victims_list.victims_list[i].is_active));
		json_object_set(obj, "nodes", json_integer(total_sniffers_heard));
		json_array_append(result_array, obj);
	}
	pthread_mutex_unlock(&victims_list.mutex);

	*result = result_array;
	return 0;
}

static int rpc_rename_victim(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	char *mac = NULL, *name = NULL;

	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "mac") == 0) mac = strdup(json_string_value(value));
		if(strcmp(key, "name") == 0) name = strdup(json_string_value(value));
	}

	if(mac == NULL || name == NULL) {
		free(mac);
		free(name);
		return -1;
	}

	log_info(TAG, "Rename victim %s to \"%s\"", mac, name);

	database_rename_victim(mac, name);

	free(mac);
	free(name);

	return 0;
}

static int rpc_get_settings(json_t *json_params, json_t **result, void *userdata) {
	json_t *obj=json_object();
	json_object_set(obj, "rotate_cw_90deg", json_boolean(get_rotate_cw_90deg()));
	*result = obj;
	return 0;
}

static int rpc_set_rotate_cw_90deg(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	uint8_t setting = 255;

	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "enabled") == 0) setting = json_boolean_value(value);
	}
	if(setting > 1) return -1;

	set_rotate_cw_90deg(setting);
	if(setting == 1) database_enable_rotate_cw_90deg();
	if(setting == 0) database_disable_rotate_cw_90deg();

	return 0;
}

static int rpc_view_raw_images(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	char *mac = NULL;

	// Get MAC
	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "mac") == 0) {
			mac = strdup(json_string_value(value));
		}
	}
	if(mac == NULL) return -1;

	// Assemblying output
	json_t *obj = json_object(); // the ouput object
	json_t *images_array = json_array(); // images array

	json_object_set(obj, "mac", json_string(mac));

	uint32_t i;
	victim_photos_list photos;
	photos.list_max_size=128;
	photos.list_size=0;
	photos.photos = malloc(sizeof(victim_photo) * 128);

	if(photos.photos == NULL) {
		log_error(TAG, "Unable to allocate memory for images for the victim");
		free(mac);
		return -1;
	}

	if(!get_raw_images(mac, &photos)) {
		log_error(TAG, "Unable to get RAW images for the victim");
		free(mac);
		free(photos.photos);
		return -1;
	}

	for(i=0; i<photos.list_size; i++) {
		json_t *img = json_object();
		char *base64_data = NULL;

		if(photos.photos[i].data_sz != 0) {
			base64_data = b64_encode(photos.photos[i].data, photos.photos[i].data_sz);
			json_object_set(img, "data", json_string(base64_data));
		}else{
			json_object_set(img, "data", json_string(""));
		}
		json_object_set(img, "rssi", json_integer(photos.photos[i].rssi));
		json_array_append(images_array, img);

		free(photos.photos[i].data);
		free(base64_data);
	}


	json_object_set(obj, "photos", images_array);
	*result = obj;

	free(photos.photos);
	free(mac);
	return 0;
}

static int rpc_view_classified_images(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	char *mac = NULL;

	// Get MAC
	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "mac") == 0) {
			mac = strdup(json_string_value(value));
		}
	}
	if(mac == NULL) return -1;

	// Assemblying output
	json_t *obj = json_object(); // the ouput object
	json_t *images_array = json_array(); // images array

	json_object_set(obj, "mac", json_string(mac));

	uint32_t i;
	victim_photos_list photos;
	photos.list_max_size=128;
	photos.list_size=0;
	photos.photos = malloc(sizeof(victim_photo) * 32);

	if(photos.photos == NULL) {
		log_error(TAG, "Unable to allocate memory for images for the victim");
		free(mac);
		return -1;
	}

	if(!get_classified_images(mac, &photos)) {
		log_error(TAG, "Unable to get RAW images for the victim");
		free(mac);
		free(photos.photos);
		return -1;
	}

	for(i=0; i<photos.list_size; i++) {
		json_t *img = json_object();
		char *base64_data = NULL;

		base64_data = b64_encode(photos.photos[i].data, photos.photos[i].data_sz);
		json_object_set(img, "data", json_string(base64_data));
		json_array_append(images_array, img);

		free(photos.photos[i].data);
		free(base64_data);
	}


	json_object_set(obj, "photos", images_array);
	*result = obj;

	free(photos.photos);
	free(mac);
	return 0;
}

extern nnm_context nnm;
static int rpc_get_btnnm_status(json_t *json_params, json_t **result, void *userdata) {
	json_t *obj=json_object();
	pthread_mutex_lock(&nnm.mutex);

	if(nnm.connected == 1) {
		if(nnm.nnm_status == 2) {
			json_object_set(obj, "status", json_string("idle"));
		}else if(nnm.nnm_status == 3) {
			char buf[20];
			json_object_set(obj, "status", json_string("working"));
			json_object_set(obj, "mac", json_string(getMACString(nnm.mac, buf, 20)));
			json_object_set(obj, "progress", json_integer(nnm.current_progress));
			json_object_set(obj, "max_images", json_integer(nnm.max_progress));
		}else{
			json_object_set(obj, "status", json_string("unknown"));
		}
	}else{
		json_object_set(obj, "status", json_string("disconnected"));
	}
	pthread_mutex_unlock(&nnm.mutex);
	*result = obj;
	return 0;
}

extern nnm_pending_cmd nnm_cmd;
static int rpc_start_nnm(json_t *json_params, json_t **result, void *userdata) {
	const char *key;
	json_t *value;
	char *mac = NULL;

	// Get MAC
	if(!json_is_object(json_params)) return -1;
	json_object_foreach(json_params, key, value) {
		if(strcmp(key, "mac") == 0) {
			mac = strdup(json_string_value(value));
		}
	}
	if(mac == NULL) return -1;

	if(nnm.connected == 1) {
		pthread_mutex_lock(&nnm_cmd.mutex);
		getMACHexVal(mac, nnm_cmd.start_nnm_mac);
		nnm_cmd.start_nnm=1;
		pthread_mutex_unlock(&nnm_cmd.mutex);
	}
	free(mac);
	return 0;
}

static jsonrpc_method_entry_t jsonrpc_methods[] = {
		{"test", test, "o"},
		{"get_sniffers_count", rpc_get_sniffers_count, ""},
		{"get_sniffers_list", rpc_get_sniffers_list, ""},
		{"get_ble_devices_count", rpc_get_total_ble_devices, ""},
		{"get_ble_devices_list", rpc_get_ble_devices_list, ""},
		{"get_total_victims_to_track_count", rpc_get_total_victims_to_track, ""},
		{"get_total_photos_captured_count", rpc_get_total_photos_captured, ""},
		{"get_victims_photos_summary", rpc_get_victims_photos_summary, ""},
		{"hFlip", rpc_set_h_flip, "o"},
		{"vFlip", rpc_set_v_flip, "o"},
		{"LED", rpc_set_led, "o"},
		{"AEC", rpc_set_aec, "o"},
		{"rssi_threshold", rpc_set_rssi_threshold, "o"},
		{"reboot", rpc_reboot, "o"},
		{"test_snapshot", rpc_test_snapshot, "o"},
		{"get_test_snapshots", rpc_get_test_snapshots, ""},
		{"enable_tracking", rpc_enable_tracking, "o"},
		{"disable_tracking", rpc_disable_tracking, "o"},
		{"get_tracking_list", rpc_get_tracking_list, ""},
		{"rename_victim", rpc_rename_victim, "o"},
		{"get_settings", rpc_get_settings, ""},
		{"rorate_cw_90Deg", rpc_set_rotate_cw_90deg, "o"},
		{"view_raw_images", rpc_view_raw_images, "o"},
		{"view_classified_images", rpc_view_classified_images, "o"},
		{"get_btnnm_status", rpc_get_btnnm_status, ""},
		{"start_nnm", rpc_start_nnm, "o"},
		{NULL}
};



void *zmq_rpc_thread(void *arg) {
	log_info(TAG, "ZMQ RPC thread is starting...");

	void *zmq_ctx = zmq_ctx_new();
	void *zmq_sock = zmq_socket(zmq_ctx, ZMQ_REP);
	int zmq_res = zmq_bind(zmq_sock, ZMQ_RPC_BIND_ADDR);
	if(zmq_res == -1) {
		log_error(TAG, "Unable to create bind ZMQ Socket, exiting...");
		exit(-1);
	}

	while(1) {
		zmq_msg_t msg;
		zmq_msg_init(&msg);

		zmq_msg_recv(&msg, zmq_sock, 0);

		char* rpc_output = jsonrpc_handler((char *)zmq_msg_data(&msg), zmq_msg_size(&msg), jsonrpc_methods, NULL);
		zmq_msg_close(&msg);

		if(rpc_output) {
			zmq_send(zmq_sock, rpc_output, strlen(rpc_output), 0);
		}else{
			zmq_send(zmq_sock, "", 0, 0);
		}
	}

	return NULL;
}
