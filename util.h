#ifndef UTIL_H_
#define UTIL_H_

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "log.h"

#define MAC_ADDR_ASSIGN(src, dst) src[0]=dst[0]; src[1]=dst[1]; src[2]=dst[2]; src[3]=dst[3]; src[4]=dst[4]; src[5]=dst[5];

uint32_t to32Bits(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
char* getIPString(uint8_t* ip, char* buf, size_t sz);
char* getMACString(uint8_t* mac, char* buf, size_t sz);
uint8_t mac_address_match(uint8_t* addr1, uint8_t* addr2);
void getMACHexVal(const char* str, uint8_t* mac);
uint8_t autoResizeList(uint32_t* list_size, uint32_t* list_max_size, void** listPtr, size_t objSize);
uint8_t countFilesDirectoryWithExt(const char *dir, const char *ext, uint32_t *cnt);

#endif /* UTIL_H_ */
