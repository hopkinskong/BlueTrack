/*
 * sniffer_info.h
 *
 *  Created on: 2018¦~2¤ë23¤é
 *      Author: hopkins
 */

#ifndef MAIN_SNIFFER_INFO_H_
#define MAIN_SNIFFER_INFO_H_

#include <inttypes.h>

typedef struct sniffer_info {
	uint8_t id;
	uint8_t battery_percent;
	uint32_t ip;

	// Settings
	uint8_t camera_flip_config;
	uint8_t led_config;
	int16_t ble_snapshot_rssi;
	uint8_t aec_on;
	uint16_t aec_value;

	// Flags
	uint8_t test_snapshot;
} sniffer_info;

void setBatteryPercent(sniffer_info* dev, uint8_t percent);
uint8_t getBatteryPercent(sniffer_info dev);

void setBatteryCharging(sniffer_info* dev, uint8_t charging);
uint8_t getBatteryCharging(sniffer_info dev);


void setIP(sniffer_info* dev, uint32_t ip);
uint32_t getIP(sniffer_info dev);

void setID(sniffer_info* dev, uint8_t id);
uint8_t getID(sniffer_info dev);


void setCameraFlipConfig(sniffer_info* dev, uint8_t conf);
uint8_t getCameraFlipConfig(sniffer_info dev);

uint8_t getVFlipFromValue(uint8_t val);
uint8_t getHFlipFromValue(uint8_t val);
uint8_t getHFlip(sniffer_info dev);
uint8_t getVFlip(sniffer_info dev);

void setAECConfig(sniffer_info *dev, uint8_t aec_on, uint16_t aec_value);
uint8_t getAECOn(sniffer_info dev);
uint16_t getAECValue(sniffer_info dev);

void setLEDConfig(sniffer_info* dev, uint8_t conf);
uint8_t getLEDConfig(sniffer_info dev);
uint8_t getLEDFlash(sniffer_info dev);
uint8_t getLEDFullOn(sniffer_info dev);
uint8_t getLEDOff(sniffer_info dev);

void setBLESnapshotRSSI(sniffer_info* dev, int16_t val);
int16_t getBLESnapshotRSSI(sniffer_info dev);

void setTestSnapshot(sniffer_info* dev);
void resetTestSnapshot(sniffer_info* dev);

#endif /* MAIN_SNIFFER_INFO_H_ */
