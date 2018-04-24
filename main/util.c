/*
 * util.c
 *
 *  Created on: 2018¦~2¤ë17¤é
 *      Author: hopkins
 */

#include "util.h"

uint8_t mac_address_match(uint8_t* addr1, uint8_t* addr2) {
	if(addr1[0] != addr2[0]) return 0;
	if(addr1[1] != addr2[1]) return 0;
	if(addr1[2] != addr2[2]) return 0;
	if(addr1[3] != addr2[3]) return 0;
	if(addr1[4] != addr2[4]) return 0;
	if(addr1[5] != addr2[5]) return 0;
	return 1;
}

char* getBinary(uint8_t data, char* out) {

	out[0]=((data & (1 << 7)) ? '1' : '0');
	out[1]=((data & (1 << 6)) ? '1' : '0');
	out[2]=((data & (1 << 5)) ? '1' : '0');
	out[3]=((data & (1 << 4)) ? '1' : '0');
	out[4]=((data & (1 << 3)) ? '1' : '0');
	out[5]=((data & (1 << 2)) ? '1' : '0');
	out[6]=((data & (1 << 1)) ? '1' : '0');
	out[7]=((data & (1 << 0)) ? '1' : '0');
	out[8]=0x00;

	return out;
}

void dword_to_four_bytes(uint32_t dword, uint8_t* ptr) {
	ptr[0]=dword >> 24;
	ptr[1]=dword >> 16;
	ptr[2]=dword >> 8;
	ptr[3]=dword;
}
