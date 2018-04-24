#include "sniffer_info.h"

void setBatteryPercent(sniffer_info* dev, uint8_t percent) {
	dev->battery_percent &= 0x80;
	dev->battery_percent |= (percent & 0x7F);
}
uint8_t getBatteryPercent(sniffer_info dev) {
	return (dev.battery_percent & 0x7F);
}

void setBatteryCharging(sniffer_info* dev, uint8_t charging) {
	dev->battery_percent &= 0x7F;
	dev->battery_percent |= (charging << 7);
}
uint8_t getBatteryCharging(sniffer_info dev) {
	return (dev.battery_percent >> 7);
}

void setIP(sniffer_info* dev, uint32_t ip) {
	// The order is reversed, don't know why...
	dev->ip = (((ip&0x000000FF) << 24) | ((ip&0x0000FF00) << 8) | ((ip&0x00FF0000) >> 8) | ((ip&0xFF000000) >> 24));
}

uint32_t getIP(sniffer_info dev) {
	return dev.ip;
}

void setID(sniffer_info* dev, uint8_t id) {
	dev->id=id;
}

uint8_t getID(sniffer_info dev) {
	return dev.id;
}

void setCameraFlipConfig(sniffer_info* dev, uint8_t conf) {
	dev->camera_flip_config=conf;
}

uint8_t getCameraFlipConfig(sniffer_info dev) {
	return dev.camera_flip_config;
}

uint8_t getVFlipFromValue(uint8_t val) {
	return (val == 0x01 || val == 0x02);
}
uint8_t getHFlipFromValue(uint8_t val) {
	return (val == 0x00 || val == 0x02);
}

uint8_t getHFlip(sniffer_info dev) {
	return getHFlipFromValue(getCameraFlipConfig(dev));
}

uint8_t getVFlip(sniffer_info dev) {
	return getVFlipFromValue(getCameraFlipConfig(dev));
}

void setAECConfig(sniffer_info *dev, uint8_t aec_on, uint16_t aec_value) {
	dev->aec_on=aec_on;
	dev->aec_value=aec_value;
}

uint8_t getAECOn(sniffer_info dev) {
	return dev.aec_on;
}

uint16_t getAECValue(sniffer_info dev) {
	return dev.aec_value;
}

void setLEDConfig(sniffer_info* dev, uint8_t conf) {
	dev->led_config=conf;
}

uint8_t getLEDConfig(sniffer_info dev) {
	return dev.led_config;
}

uint8_t getLEDFlash(sniffer_info dev) {
	uint8_t config = getLEDConfig(dev);
	return (config == 0x02);
}

uint8_t getLEDFullOn(sniffer_info dev) {
	uint8_t config = getLEDConfig(dev);
	return (config == 0x01);
}

uint8_t getLEDOff(sniffer_info dev) {
	uint8_t config = getLEDConfig(dev);
	return (config == 0x00);
}

void setBLESnapshotRSSI(sniffer_info* dev, int16_t val) {
	dev->ble_snapshot_rssi=val;
}

int16_t getBLESnapshotRSSI(sniffer_info dev) {
	return dev.ble_snapshot_rssi;
}

void setTestSnapshot(sniffer_info* dev) {
	dev->test_snapshot=1;
}

void resetTestSnapshot(sniffer_info* dev) {
	dev->test_snapshot=0;
}

