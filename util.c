#include "util.h"

uint32_t to32Bits(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
	return ((a << 24) | (b << 16) | (c << 8) | (d));
}

char* getIPString(uint8_t* ip, char* buf, size_t sz) {
	snprintf(buf, sz, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	return buf;
}

char* getMACString(uint8_t* mac, char* buf, size_t sz) {
	snprintf(buf, sz, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return buf;
}

uint8_t mac_address_match(uint8_t* addr1, uint8_t* addr2) {
	if(addr1[0] != addr2[0]) return 0;
	if(addr1[1] != addr2[1]) return 0;
	if(addr1[2] != addr2[2]) return 0;
	if(addr1[3] != addr2[3]) return 0;
	if(addr1[4] != addr2[4]) return 0;
	if(addr1[5] != addr2[5]) return 0;
	return 1;
}

void getMACHexVal(const char* str, uint8_t* mac) {
	sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

uint8_t autoResizeList(uint32_t* list_size, uint32_t* list_max_size, void** listPtr, size_t objSize) {
	if((*list_size)==(*list_max_size)) {
		*list_max_size*=2;
		*listPtr=realloc(*listPtr, (*list_max_size)*objSize);
		if(*listPtr == NULL) {
			log_error("MALLOC", "Failed to reallocate memory for a list.");
			return 0;
		}
	}
	return 1;
}

uint8_t countFilesDirectoryWithExt(const char *dir, const char *ext, uint32_t *cnt) {
	*cnt=0;
	struct dirent *pDirEnt;
	DIR *pDir = opendir(dir);
	if(pDir != NULL) {
		while((pDirEnt = readdir(pDir)) != NULL) {
			size_t len = strlen(pDirEnt->d_name);
			if(len >= 4) {
				if(strcmp(ext, &(pDirEnt->d_name[len-strlen(ext)])) == 0) {
					(*cnt)++;
				}
			}
		}
		closedir(pDir);
	}

	return 1;
}
