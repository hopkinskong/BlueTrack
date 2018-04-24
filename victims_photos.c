#include "victims_photos.h"

const char *TAG = "PHOTOS";

uint8_t save_victim_photo(uint8_t *mac, uint8_t **jpegData, uint32_t jpegSize, int16_t rssi) {
	struct stat st = {0};
	char dir[256];
	char mac_str[18];
	uint32_t jpeg_id;

	getMACString(mac, mac_str, 18);
	snprintf(dir, 256, "%s/%s", VICTIMS_PHOTOS_DIRECTORY, mac_str);

	// Create the directory for our victim if it does not exists
	if(stat(dir, &st) == -1) {
		if(mkdir(dir, 0777) != 0) { // Unable to create directory
			log_error(TAG, "Unable to create directory: %s, not saving photos.", dir);
			log_error(TAG, "%d: %s", errno, strerror(errno));
			return 0;
		}
	}

	// Get Next JPEG ID as file name
	if(database_get_next_victim_photo_id(mac_str, &jpeg_id) == 1) {
		log_error(TAG, "Unable to get new JPEG file name, not saving photos.");
		return 0;
	}

	// Insert info of the new JPEG file
	if(database_insert_next_victim_photo(mac_str, jpeg_id, rssi) == 1) {
		log_error(TAG, "Unable to insert new JPEG file name entry in DB, not saving photos.");
		return 0;
	}

	// Generate full filename for saving
	snprintf(dir, 256, "%s/%s/%d.jpg", VICTIMS_PHOTOS_DIRECTORY, mac_str, jpeg_id);

	// File write
	FILE *f = fopen(dir, "wb");
	if(f == NULL) {
		log_error(TAG, "Unable to write to output JPEG file: %s, not saving photos.", dir);
		return 0;
	}

	fwrite(*jpegData, 1, jpegSize, f);
	fclose(f);
	free(*jpegData);
	return 1;
}

/*
 * Victim Name, Captured Photos Count, NNM Person Processed
 */
uint8_t get_victims_photos_summary(victims_photos_summary_list* list) {
	if(database_get_victims_photos_summary(list) != 0) {
		log_error(TAG, "Unable to generate victims photos summary");
		return 0;
	}

	uint32_t i;
	for(i=0; i<list->list_size; i++) {
		char dir[512];
		uint32_t count;
		snprintf(dir, 512, "%s/%s/nnm/", VICTIMS_PHOTOS_DIRECTORY, list->list[i].mac);
		countFilesDirectoryWithExt(dir, ".pkl", &count);
		list->list[i].nnm_processed=count;
	}


	return 1;
}

uint8_t get_raw_images(const char *mac, victim_photos_list *list) {
	if(database_get_victim_photos(mac, list) != 0) {
		log_error(TAG, "Unable to get victim photos");
		return 0;
	}

	uint32_t i;
	char buf[128];
	for(i=0; i<list->list_size; i++) {
		FILE *f;
		snprintf(buf, 128, "%s/%s/%d.jpg", VICTIMS_PHOTOS_DIRECTORY, mac, list->photos[i].id);
		f=fopen(buf, "rb");
		if(f == NULL) { // probably images already lost
			list->photos[i].data=NULL;
			list->photos[i].data_sz=0;
			continue;
		}
		fseek(f, 0, SEEK_END);
		list->photos[i].data_sz=ftell(f);
		fseek(f, 0, SEEK_SET);
		list->photos[i].data=malloc(list->photos[i].data_sz);
		if(list->photos[i].data == NULL) {
			log_error(TAG, "Unable to allocate memory for image");
			fclose(f);
			return 0;
		}

		if(fread(list->photos[i].data, 1, list->photos[i].data_sz, f) != list->photos[i].data_sz) {
			log_error(TAG, "Unable to read image into memory");
			fclose(f);
			return 0;
		}
		fclose(f);

		shrink_image(&list->photos[i].data, &list->photos[i].data_sz, 20);
	}
	return 1;
}

uint8_t get_classified_images(const char *mac, victim_photos_list *list) {
	char buf[128], buf2[128];
	struct dirent *pDirEnt;

	snprintf(buf, 128, "%s/%s/classified", VICTIMS_PHOTOS_DIRECTORY, mac);
	DIR *pDir = opendir(buf);
	if(pDir != NULL) {
		while((pDirEnt = readdir(pDir)) != NULL) {
			size_t len = strlen(pDirEnt->d_name);
			if(len >= 4) {
				if(strcmp(".jpg", &(pDirEnt->d_name[len-strlen(".jpg")])) == 0) { // Confirm file extension is .jpg
					FILE *f;
					snprintf(buf2, 128, "%s/%s/classified/%s", VICTIMS_PHOTOS_DIRECTORY, mac, pDirEnt->d_name);

					if(!autoResizeList(&list->list_size, &list->list_max_size, (void **)&list->photos, sizeof(victim_photo))) {
						log_error(TAG, "Unable to reallocate memory for classified photos");
						return 0;
					}

					f=fopen(buf2, "rb");
					if(f == NULL) {
						closedir(pDir);
						return 0;
					}
					fseek(f, 0, SEEK_END);
					list->photos[list->list_size].data_sz=ftell(f);
					fseek(f, 0, SEEK_SET);
					list->photos[list->list_size].data=malloc(list->photos[list->list_size].data_sz);
					if(list->photos[list->list_size].data == NULL) {
						log_error(TAG, "Unable to allocate memory for image");
						closedir(pDir);
						fclose(f);
						return 0;
					}
					if(fread(list->photos[list->list_size].data, 1, list->photos[list->list_size].data_sz, f) != list->photos[list->list_size].data_sz) {
						log_error(TAG, "Unable to read image into memory");
						closedir(pDir);
						fclose(f);
						return 0;
					}
					fclose(f);
					list->list_size++;
				}
			}
		}
		closedir(pDir);
	}
	return 1;
}
