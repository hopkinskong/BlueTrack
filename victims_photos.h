#ifndef VICTIMS_PHOTOS_H_
#define VICTIMS_PHOTOS_H_

#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "util.h"
#include "log.h"

#include "image_preprocessing.h"


/*
 * One entry in the Victim Photos Summary List
 */
typedef struct victim_photos_summary {
	char mac[18];
	char name[128];
	uint8_t is_active;
	uint32_t photos_captured;
	uint32_t nnm_processed;
} victim_photos_summary;

/*
 * The Victim Photos Summary List
 */
typedef struct victims_photos_summary_list {
	uint32_t list_size;
	uint32_t list_max_size;
	victim_photos_summary *list;
} victims_photos_summary_list;


/*
 * One photo entry in Victim Photo List
 */
typedef struct victim_photo {
	uint32_t id;
	int16_t rssi;
	uint8_t *data;
	uint32_t data_sz;
} victim_photo;

/*
 * The Victim Photos List (for one victim)
 */
typedef struct victim_photos_list {
	uint32_t list_size;
	uint32_t list_max_size;
	victim_photo *photos;
} victim_photos_list;

#include "database.h"

uint8_t save_victim_photo(uint8_t *mac, uint8_t **jpegData, uint32_t jpegSize, int16_t rssi);
uint8_t get_victims_photos_summary(victims_photos_summary_list* list);
uint8_t get_raw_images(const char *mac, victim_photos_list *list);
uint8_t get_classified_images(const char *mac, victim_photos_list *list);

#endif /* VICTIMS_PHOTOS_H_ */
