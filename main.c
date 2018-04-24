#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"
#include "log.h"
#include "database.h"
#include "sniffer_connection_handler.h"
#include "tick.h"
#include "worker.h"
#include "zmq_rpc.h"
#include "app_settings.h"
#include "zmq_nnm.h"

const static char* TAG = "MAIN";

/*
 * Initialize the database
 * Create tables and rows if not exists
 */
uint8_t init_database(sqlite3* db) {
	uint8_t i;
	int rc;
	char *zErrMsg = 0;

	/* Table Creation */
	// sniffer_settings
	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS sniffer_settings(sniffer_id INTEGER PRIMARY KEY, v_flip INTEGER, h_flip INTEGER, led INTEGER, rssi_threshold INTEGER, aec_on INTEGER, aec_value INTEGER);", NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 table creation error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return 0;
	}
	// tracked_victims
	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS tracked_victims(mac_address TEXT PRIMARY KEY, name TEXT, is_active INTEGER);", NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 table creation error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return 0;
	}
	// app_settings
	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS app_settings(key TEXT PRIMARY KEY, value INTEGER);", NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 table creation error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return 0;
	}
	// victims_photos
	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS victims_photos(mac_address TEXT, photos_id INTEGER, rssi INTEGER, PRIMARY KEY(mac_address, photos_id));", NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 table creation error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return 0;
	}

	/* Row Creation */
	char stmt[256];
	for(i=0; i<64; i++) {
		snprintf(stmt, 256, "INSERT OR IGNORE INTO sniffer_settings(sniffer_id, v_flip, h_flip, led, rssi_threshold, aec_on, aec_value) VALUES(%d, 0, 0, 2, -64, 1, 0);", i);
		rc = sqlite3_exec(db, stmt, NULL, NULL, &zErrMsg);
		if(rc != SQLITE_OK) {
			log_error(TAG, "SQLite3 data insertion error: %s", zErrMsg);
			sqlite3_free(zErrMsg);
			return 0;
		}
	}
	rc = sqlite3_exec(db, "INSERT OR IGNORE INTO app_settings(key, value) VALUES(\"rotate_cw_90deg\", 1);", NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 data insertion error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return 0;
	}

	return 1;
}

static uint8_t v_flip, h_flip, led, aec_on;
static uint16_t aec_value;
static int16_t rssi_threshold;
static int sniffer_settings_sqlite_result_callback(void *data, int argc, char **argv, char **azColName) {
	uint32_t i;
	v_flip=h_flip=255;
	for(i=0; i<argc; i++) {
		if(strcmp(azColName[i], "v_flip") == 0) v_flip=atoi(argv[i]);
		if(strcmp(azColName[i], "h_flip") == 0) h_flip=atoi(argv[i]);
		if(strcmp(azColName[i], "led") == 0) led=atoi(argv[i]);
		if(strcmp(azColName[i], "rssi_threshold") == 0) rssi_threshold=atoi(argv[i]);
		if(strcmp(azColName[i], "aec_on") == 0) aec_on=atoi(argv[i]);
		if(strcmp(azColName[i], "aec_value") == 0) aec_value=atoi(argv[i]);
	}
	return 0;
}

static uint8_t value;
static int app_settings_sqlite_result_callback(void *data, int argc, char **argv, char **azColName) {
	uint32_t i;
	value=0;
	for(i=0; i<argc; i++) {
		if(strcmp(azColName[i], "value") == 0) value=atoi(argv[i]);
	}
	return 0;
}

extern sniffer_context sniffers[64];
extern discovered_ble_devices_list ble_devices_list;
extern victims_to_track_list victims_list;

database data_db;
int main(int argc, char** argv) {
	log_info(TAG, "Starting BlueTrack Backend...");

	struct sockaddr_in server, client;
	int client_socket, socket_length;
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	uint32_t i;

	// Handle data.db
	pthread_mutex_init(&data_db.mutex, NULL);
	int rc = sqlite3_open("data.db", &data_db.db);
	if(rc) {
		log_error(TAG, "Unable to open SQLite3 Database: %s", sqlite3_errmsg(data_db.db));
		return -1;
	}
	if(!init_database(data_db.db)) {
		log_error(TAG, "Unable to initialize SQLite3 Database.");
		sqlite3_close(data_db.db);
		return -1;
	}

	// Create paths
	struct stat st = {0};
	if(stat(VICTIMS_PHOTOS_DIRECTORY, &st) == -1) {
		if(mkdir(VICTIMS_PHOTOS_DIRECTORY, 0777) != 0) { // Unable to create directory
			log_error(TAG, "Unable to create directory: %s", VICTIMS_PHOTOS_DIRECTORY);
			log_error(TAG, "%d: %s", errno, strerror(errno));
			return -1;
		}
	}

	// Initialize all variables
	char stmt[256];
	char *zErrMsg = 0;
	for(i=0; i<64; i++) {
		sniffers[i].info.sniffer_id=i;
		sniffers[i].connected=0;
		pthread_mutex_init(&sniffers[i].mutex, NULL);

		// Sniffer settings
		snprintf(stmt, 256, "SELECT * FROM sniffer_settings WHERE sniffer_id = %d;", i);
		if(sqlite3_exec(data_db.db, stmt, sniffer_settings_sqlite_result_callback, NULL, &zErrMsg) != SQLITE_OK) {
			log_error(TAG, "SQLite3 read sniffer_settings error: %s", zErrMsg);
			sqlite3_free(zErrMsg);
			return -1;
		}
		sniffers[i].settings.h_flip=h_flip;
		sniffers[i].settings.v_flip=v_flip;

		sniffers[i].settings.led=led;
		sniffers[i].settings.rssi_threshold=rssi_threshold;

		sniffers[i].settings.aec_on=aec_on;
		sniffers[i].settings.aec_value=aec_value;

		resetConfigFlags(i);

		sniffers[i].snapshot.img=NULL;
		sniffers[i].snapshot.img_sz=0;
		sniffers[i].snapshot.img_valid=0;
		pthread_mutex_init(&sniffers[i].snapshot.mutex, NULL);
	}

	if(sqlite3_exec(data_db.db, "SELECT * FROM app_settings WHERE key = \"rotate_cw_90deg\";", app_settings_sqlite_result_callback, NULL, &zErrMsg) != SQLITE_OK) {
		log_error(TAG, "SQLite3 read app_settings error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return -1;
	}
	set_rotate_cw_90deg(value);

	ble_devices_list.list_size=0;
	ble_devices_list.list_max_size=128;
	ble_devices_list.devices_list = malloc(sizeof(discovered_ble_device) * ble_devices_list.list_max_size);
	if(ble_devices_list.devices_list == NULL) {
			log_error(TAG, "Unable to allocate memory for BLE devices list");
			return -1;
	}
	for(i=0; i<ble_devices_list.list_max_size; i++) {
		ble_devices_list.devices_list[i].discovered_sniffer_nodes_count=0;
		ble_devices_list.devices_list[i].is_tracking=0;
	}
	pthread_mutex_init(&ble_devices_list.mutex, NULL);

	victims_list.list_size=0;
	victims_list.list_max_size=128;
	victims_list.victims_list = malloc(sizeof(victims_to_track_list) * victims_list.list_max_size);
	if(victims_list.victims_list == NULL) {
			log_error(TAG, "Unable to allocate memory for victims list");
			return -1;
	}
	pthread_mutex_init(&victims_list.mutex, NULL);


	// Start required threads
	pthread_t tick_thrd, worker_thrd, zmq_rpc_thrd, zmq_nnm_thrd;
	if(pthread_create(&tick_thrd, NULL, tick_thread, NULL) != 0) {
		log_error(TAG, "Unable to start tick thread");
		return -1;
	}
	if(pthread_create(&worker_thrd, NULL, worker_thread, NULL) != 0) {
		log_error(TAG, "Unable to start worker thread");
		return -1;
	}
	if(pthread_create(&zmq_rpc_thrd, NULL, zmq_rpc_thread, NULL) != 0) {
		log_error(TAG, "Unable to start ZMQ RPC thread");
		return -1;
	}
	if(pthread_create(&zmq_nnm_thrd, NULL, zmq_nnm_thread, NULL) != 0) {
		log_error(TAG, "Unable to start ZMQ NNM thread");
		return -1;
	}

	// Configure server
	if(server_socket == -1) {
		log_error(TAG, "Unable to create socket");
		log_error(TAG, "%d: %s", errno, strerror(errno));
		return -1;
	}

	server.sin_family=AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(SERVER_LISTEN_PORT);


	if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) {
		log_error(TAG, "Unable to set socket reuse");
		log_error(TAG, "%d: %s", errno, strerror(errno));
		return -1;
	}

	if(bind(server_socket, (struct sockaddr *)&server, sizeof(server)) == -1) {
		log_error(TAG, "Unable to bind server");
		log_error(TAG, "%d: %s", errno, strerror(errno));
		return -1;
	}

	if(listen(server_socket, 32) == -1) {
		log_error(TAG, "Server unable to listen");
		log_error(TAG, "%d: %s", errno, strerror(errno));
		return -1;
	}

	socket_length = sizeof(struct sockaddr_in);

	while((client_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t *)&socket_length))) {
		pthread_t thread;

		log_info(TAG, "Sniffer Connected");
		if(pthread_create(&thread, NULL, sniffer_connection_handler, (void*)&client_socket) != 0) {
			log_error(TAG, "Unable to start thread to serve client");
			close(client_socket);
		}
	}

	if(client_socket == -1) {
		log_error(TAG, "Server failed to accept client connections");
		log_error(TAG, "%d: %s", errno, strerror(errno));
		return -1;
	}


	return 0;
}
