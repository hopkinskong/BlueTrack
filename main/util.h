/*
 * util.h
 *
 *  Created on: 2018¦~2¤ë17¤é
 *      Author: hopkins
 */

#ifndef MAIN_UTIL_H_
#define MAIN_UTIL_H_

#include <inttypes.h>

#define BT_BD_ADDR_HEX(addr)   addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]
#define BT_BD_ADDR_ASSIGN(src, dst) src[0]=dst[0]; src[1]=dst[1]; src[2]=dst[2]; src[3]=dst[3]; src[4]=dst[4]; src[5]=dst[5];

uint8_t mac_address_match(uint8_t* addr1, uint8_t* addr2);
char* getBinary(uint8_t data, char* out);
void dword_to_four_bytes(uint32_t dword, uint8_t* ptr);

#endif /* MAIN_UTIL_H_ */
