#ifndef WORKER_H_
#define WORKER_H_

#include <time.h>
#include <inttypes.h>
#include <stdatomic.h>

#include <pthread.h>


#include "bluetrack_application_protocol.h"

void *worker_thread(void *arg);

typedef struct test_snapshot {
	uint8_t *img;
	uint32_t img_sz;
	uint8_t img_valid;
	pthread_mutex_t mutex; /* only locks test_snapshot */
} test_snapshot;

/*
 * Note: Variables that is not atomic needs to be locked by mutex
 */
typedef struct sniffer_context {
	sniffer_info info;
	sniffer_settings settings;
	uint8_t ble_devices_count;
	scan_result_set ble_devices[64]; // Maximum 64 devices per sniffer
	atomic_uint connected;
	pthread_mutex_t mutex;
	test_snapshot snapshot;
} sniffer_context;

/*
 * For each BLE device, it is possible being discovered by *up to* 64 sniffers
 */
typedef struct discovered_sniffers {
	uint8_t sniffer_id;
	int16_t rssi;
} discovered_sniffers;

/*
 * One ble device in the ble devices list
 * Contains one ble_deivce information of multiple sniffers received RSSI
 */
typedef struct discovered_ble_device {
	uint8_t ble_addr[6];
	uint8_t discovered_sniffer_nodes_count;
	discovered_sniffers sniffer_nodes[64];
	uint8_t is_tracking;
} discovered_ble_device;

/*
 * The BLE Device list itself
 */
typedef struct discovered_ble_devices_list {
	atomic_uint list_size;
	atomic_uint list_max_size;
	discovered_ble_device* devices_list;
	pthread_mutex_t mutex;
} discovered_ble_devices_list;

/*
 * One victim in the victims list
 */
typedef struct tracked_victim {
	char mac[18];
	char name[128];
	uint8_t is_active;
} tracked_victim;

/*
 * The Victim list itself
 */
typedef struct victims_to_track_list {
	uint32_t list_size;
	uint32_t list_max_size;
	tracked_victim* victims_list;
	pthread_mutex_t mutex;
} victims_to_track_list;



#include "config.h"
#include "log.h"
#include "util.h"
#include "database.h"

#endif /* WORKER_H_ */
