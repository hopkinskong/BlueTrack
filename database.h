#ifndef DATABASE_H_
#define DATABASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <sqlite3.h>
#include <pthread.h>

#include "log.h"

#include "worker.h"
#include "victims_photos.h"

typedef struct {
	sqlite3* db;
	pthread_mutex_t mutex;
} database;

uint8_t database_get_victim_photos(const char *mac, victim_photos_list* photos_list);
uint8_t database_get_total_victims_photos(uint32_t *count);
uint8_t database_get_next_victim_photo_id(const char *mac, uint32_t *id);
uint8_t database_insert_next_victim_photo(const char *mac, uint32_t id, int16_t rssi);
uint8_t database_get_victims_photos_summary(victims_photos_summary_list* list);
uint8_t database_get_all_tracked_victims(victims_to_track_list *list);
uint8_t database_start_track_victim(const char *mac);
uint8_t database_stop_track_victim(const char *mac);
uint8_t database_rename_victim(const char *mac, const char *name);
uint8_t database_check_victim_active(const char *mac, uint8_t *result);
uint8_t database_update_value_integer_val(const char* tbl, const char* cond_key, uint32_t cond_val, const char* target_key, uint32_t target_val);
uint8_t database_enable_rotate_cw_90deg();
uint8_t database_disable_rotate_cw_90deg();

#endif /* DATABASE_H_ */
