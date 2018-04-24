#include "database.h"

const static char* TAG = "SQLITE";

extern database data_db;
static char stmt[512];

static uint32_t total_victims_photos=0;
static int get_total_victims_photos_sqlite_result_callback(void *data, int argc, char **argv, char **azColName) {
	uint32_t i;
	for(i=0; i<argc; i++) {
		if(strcmp(azColName[i], "cnt") == 0) total_victims_photos = atoi(argv[i]);
	}
	return 0;
}

static uint32_t next_victim_photo_id=0;
static int get_next_victim_photo_id_sqlite_result_callback(void *data, int argc, char **argv, char **azColName) {
	uint32_t i;
	for(i=0; i<argc; i++) {
		if(strcmp(azColName[i], "cnt") == 0) next_victim_photo_id = atoi(argv[i]);
	}
	return 0;
}

static uint8_t check_exists_result=0;
static int check_exists_sqlite_result_callback(void *data, int argc, char **argv, char **azColName) {
	check_exists_result=1;
	return 0;
}

static int get_all_tracked_victims_result_callback(void *data, int argc, char **argv, char **azColName) {
	uint32_t i;

	victims_to_track_list *list = (victims_to_track_list *)data;

	if(!autoResizeList(&list->list_size, &list->list_max_size, (void **)&list->victims_list, sizeof(tracked_victim))) {
		log_error(TAG, "Unable to reallocate memory for tracked victims list");
		return -1;
	}

	/*if(list->list_size == list->list_max_size) {
		list->list_max_size *= 2;
		list->victims_list=realloc(list->victims_list, list->list_max_size);
		if(list->victims_list == NULL) {
			log_error(TAG, "Unable to reallocate memory for tracked victims list");
			return -1;
		}
	}*/

	for(i=0; i<argc; i++) {
		if(strcmp(azColName[i], "mac_address") == 0) strncpy(list->victims_list[list->list_size].mac, argv[i], 18);
		if(strcmp(azColName[i], "name") == 0) strncpy(list->victims_list[list->list_size].name, argv[i], 128);
		if(strcmp(azColName[i], "is_active") == 0) list->victims_list[list->list_size].is_active=atoi(argv[i]);
	}
	list->list_size++;
	return 0;
}

static int get_all_victims_photos_summary_result_callback(void *data, int argc, char **argv, char **azColName) {
	uint32_t i;

	victims_photos_summary_list *list = (victims_photos_summary_list *)data;

	if(!autoResizeList(&list->list_size, &list->list_max_size, (void **)&list->list, sizeof(victim_photos_summary))) {
		log_error(TAG, "Unable to reallocate memory for victims photos summary");
		return -1;
	}

	for(i=0; i<argc; i++) {
		if(strcmp(azColName[i], "mac_address") == 0) strncpy(list->list[list->list_size].mac, argv[i], 18);
		if(strcmp(azColName[i], "name") == 0) strncpy(list->list[list->list_size].name, argv[i], 128);
		if(strcmp(azColName[i], "is_active") == 0) list->list[list->list_size].is_active=atoi(argv[i]);
		if(strcmp(azColName[i], "cnt") == 0) list->list[list->list_size].photos_captured=atoi(argv[i]);
	}
	list->list_size++;
	return 0;
}

static int get_all_victim_photos_result_callback(void *data, int argc, char **argv, char **azColName) {
	uint32_t i;

	victim_photos_list *list = (victim_photos_list *)data;

	if(!autoResizeList(&list->list_size, &list->list_max_size, (void **)&list->photos, sizeof(victim_photo))) {
		log_error(TAG, "Unable to reallocate memory for victim photos");
		return -1;
	}

	for(i=0; i<argc; i++) {
		if(strcmp(azColName[i], "photos_id") == 0) list->photos[list->list_size].id=atoi(argv[i]);
		if(strcmp(azColName[i], "rssi") == 0) list->photos[list->list_size].rssi=atoi(argv[i]);
	}
	list->list_size++;
	return 0;
}

uint8_t database_get_victim_photos(const char *mac, victim_photos_list* photos_list) {
	char *zErrMsg;
	int rc;

	pthread_mutex_lock(&data_db.mutex);
	snprintf(stmt, 512, "SELECT photos_id, rssi FROM victims_photos WHERE mac_address = \"%s\";", mac);
	rc = sqlite3_exec(data_db.db, stmt, get_all_victim_photos_result_callback, (void *)photos_list, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec select: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}

	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_get_total_victims_photos(uint32_t *count) {
	char* zErrMsg;
	int rc;

	pthread_mutex_lock(&data_db.mutex);
	total_victims_photos=0;
	rc = sqlite3_exec(data_db.db, "SELECT COUNT(*) AS cnt FROM victims_photos;", get_total_victims_photos_sqlite_result_callback, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec select: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}
	*count = total_victims_photos;
	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_get_next_victim_photo_id(const char *mac, uint32_t *id) {
	char* zErrMsg;
	int rc;

	pthread_mutex_lock(&data_db.mutex);
	snprintf(stmt, 512, "SELECT COUNT(*) AS cnt FROM victims_photos WHERE mac_address=\"%s\";", mac);
	next_victim_photo_id=0;
	rc = sqlite3_exec(data_db.db, stmt, get_next_victim_photo_id_sqlite_result_callback, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec select: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}
	*id = next_victim_photo_id;
	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_insert_next_victim_photo(const char *mac, uint32_t id, int16_t rssi) {
	char* zErrMsg;
	int rc;

	pthread_mutex_lock(&data_db.mutex);
	snprintf(stmt, 512, "INSERT INTO victims_photos VALUES (\"%s\", %d, %d);", mac, id, rssi);
	rc = sqlite3_exec(data_db.db, stmt, NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec insert: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}
	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_get_victims_photos_summary(victims_photos_summary_list* list) {
	char *zErrMsg;
	int rc;

	pthread_mutex_lock(&data_db.mutex);
	snprintf(stmt, 512, "SELECT tracked_victims.name, tracked_victims.is_active, tracked_victims.mac_address, COUNT(victims_photos.photos_id) AS cnt FROM tracked_victims LEFT JOIN victims_photos ON tracked_victims.mac_address=victims_photos.mac_address GROUP BY tracked_victims.mac_address;");
	rc = sqlite3_exec(data_db.db, stmt, get_all_victims_photos_summary_result_callback, (void *)list, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec select: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}

	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_get_all_tracked_victims(victims_to_track_list *list) {
	char *zErrMsg;
	int rc;

	pthread_mutex_lock(&data_db.mutex);
	snprintf(stmt, 512, "SELECT * FROM tracked_victims;");
	rc = sqlite3_exec(data_db.db, stmt, get_all_tracked_victims_result_callback, (void *)list, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec select: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}

	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_start_track_victim(const char *mac) {
	char* zErrMsg;
	int rc;

	pthread_mutex_lock(&data_db.mutex);
	snprintf(stmt, 512, "INSERT OR IGNORE INTO tracked_victims VALUES (\"%s\", \"%s\", 1);", mac, mac);
	rc = sqlite3_exec(data_db.db, stmt, NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec insert: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}

	snprintf(stmt, 512, "UPDATE tracked_victims SET is_active=1 WHERE mac_address=\"%s\";", mac);
	rc = sqlite3_exec(data_db.db, stmt, NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec update: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}
	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_stop_track_victim(const char *mac) {
	char* zErrMsg;
	int rc;


	pthread_mutex_lock(&data_db.mutex);
	snprintf(stmt, 512, "UPDATE tracked_victims SET is_active=0 WHERE mac_address=\"%s\";", mac);
	rc = sqlite3_exec(data_db.db, stmt, NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec update: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}
	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_rename_victim(const char *mac, const char *name) {
	char* zErrMsg;
	int rc;

	pthread_mutex_lock(&data_db.mutex);
	snprintf(stmt, 512, "UPDATE tracked_victims SET name=\"%s\" WHERE mac_address=\"%s\";", name, mac);
	rc = sqlite3_exec(data_db.db, stmt, NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec update: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}
	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_check_victim_active(const char *mac, uint8_t *result) {
	char* zErrMsg;
	int rc;


	pthread_mutex_lock(&data_db.mutex);
	snprintf(stmt, 512, "SELECT * FROM tracked_victims WHERE mac_address=\"%s\" AND is_active=1;", mac);
	check_exists_result=0;
	rc = sqlite3_exec(data_db.db, stmt, check_exists_sqlite_result_callback, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec select: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}
	*result = check_exists_result;
	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_update_value_integer_val(const char* tbl, const char* cond_key, uint32_t cond_val, const char* target_key, uint32_t target_val) {
	char* zErrMsg;
	int rc;


	pthread_mutex_lock(&data_db.mutex);
	snprintf(stmt, 512, "UPDATE %s SET %s=%d WHERE %s=%d", tbl, target_key, target_val, cond_key, cond_val);
	rc = sqlite3_exec(data_db.db, stmt, NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec update: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}
	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_update_value_string_val(const char* tbl, const char* cond_key, const char *cond_val, const char* target_key, uint32_t target_val) {
	char* zErrMsg;
	int rc;


	pthread_mutex_lock(&data_db.mutex);
	snprintf(stmt, 512, "UPDATE %s SET %s=%d WHERE %s=\"%s\"", tbl, target_key, target_val, cond_key, cond_val);
	rc = sqlite3_exec(data_db.db, stmt, NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		log_error(TAG, "SQLite3 exec update: %s,failed: %s", stmt, zErrMsg);
		sqlite3_free(zErrMsg);
		pthread_mutex_unlock(&data_db.mutex);
		return 1;
	}
	pthread_mutex_unlock(&data_db.mutex);
	return 0;
}

uint8_t database_enable_rotate_cw_90deg() {
	return database_update_value_string_val("app_settings", "key", "rotate_cw_90deg", "value", 1);
}

uint8_t database_disable_rotate_cw_90deg() {
	return database_update_value_string_val("app_settings", "key", "rotate_cw_90deg", "value", 0);
}
