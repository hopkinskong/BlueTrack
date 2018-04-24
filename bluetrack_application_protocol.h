#ifndef BLUETRACK_APPLICATION_PROTOCOL_H_
#define BLUETRACK_APPLICATION_PROTOCOL_H_

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"

/*
 * TCP Stream
 * BTAH (BlueTrack Application Header) + 32-bits payload length (in bytes) + Payload Data
 *
 * Payload
 * 8-bits payload type
 *
 * 0x00 RESERVED
 * 0x01 RESERVED
 * 0x02 SNIFFER_INFO 	(SNIFFER->BACKEND)
 * 0x03 SCAN_RESULT 	(SNIFFER->BACKEND)
 * 0x04 SNAPSHOT_IMAGE 	(SNIFFER->BACKEND)
 * 0x05 SNIFFER_COMMAND	(BACKEND->SNIFFER)
 * 0x06 COMMAND_ACCEPT	(SNIFFER->BACKEND)
 *
 * Payload: 0x02 - SNIFFER_INFO
 * 1-byte ID
 * 1-byte Battery Percentage
 * 4-byte IP Address
 *
 * Payload: 0x03 - SCAN_RESULTS
 * 1-byte Result Count (Max 255 Results)
 * 8n-byte Result Sets, structure of a result set will be outlined below
 *
 * Payload: 0x04 - SNAPSHOT_IMAGE
 * 8-byte Result Set (Includes snapshot RSSI+BLE Address)
 * 4-byte JPEG Size
 * n-byte JPEG data
 *
 * Payload: 0x05 - SNIFFER_COMMAND
 * 1-byte Command
 * 6-byte Argument
 *
 * Payload: 0x06 - COMMAND_ACCEPT
 * 1-byte Command
 * 6-byte Argument
 *
 * BLE Scan Result Set
 * 6-byte MAC Address
 * 2-byte Signed RSSI value (int16_t)
 *
 * Command
 * 0x00 RESERVED
 * 0x01 RESERVED
 * 0x02 LED Command (ARG: 0x00 OFF, 0x01 FULL ON, 0x02 FLASH)
 * 0x03 Add Snapshot Hook (ARG: BLE Address)
 * 0x04 Remove Snapshot Hook (ARG: BLE Address)
 * 0x05 Snapshot RSSI (ARG: int16_t 16-bit RSSI)
 * 0x06 Camera Flip (ARG: 0x00 H-FLIP, ARG: 0x01 V-FLIP, 0x02 HV-FLIP, 0x03 NO-FLIP)
 * 0x07 Test snapshot (ARG: None), returns with payload 0x04, with all 0x00 BLE result set
 * 0x08 Reboot(Reset) Sniffer
 */


typedef struct sniffer_settings {
	// Camera flip settings
	uint8_t h_flip;
	uint8_t v_flip;

	// Camera AEC settings
	uint8_t aec_on;
	uint16_t aec_value;

	// LED settings
	uint8_t led;

	// RSSI Threshold
	int16_t rssi_threshold;

	// Reboot
	uint8_t reboot;

	// Test snapshot
	uint8_t test_snapshot;
} sniffer_settings;

typedef struct sniffer_info {
	uint8_t sniffer_id;
	uint8_t battery_percentage;
	uint8_t ip_address[4];
} sniffer_info;

typedef struct ble_scan_result_set {
	uint8_t ble_mac_address[6];
	int16_t rssi;
} scan_result_set;

typedef enum command {
	LED = 0x02,
	SNAPSHOT_HOOK = 0x03,
	SNAPSHOT_HOOK_CLEAR = 0x04,
	SNAPSHOT_RSSI = 0x05,
	CAMERA_FLIP = 0x06,
	TEST_SNAPSHOT = 0x07,
	REBOOT = 0x08,
	AEC_CONFIG = 0x09
} command;

typedef enum payload_type {
	SNIFFER_INFO = 0x02,
	SCAN_RESULT = 0x03,
	SNAPSHOT_IMAGE = 0x04,
	SNIFFER_COMMAND = 0x05,
	COMMAND_ACCEPT = 0x06
} payload_type;

typedef struct command_set {
	command cmd;
	uint8_t args[6];
} command_set;

typedef struct sniffer_snapshot_ble_device {
	uint8_t mac_address[6];
} sniffer_snapshot_ble_device;

typedef struct sniffer_snapshot_ble_devices_list {
	uint32_t list_size;
	uint32_t list_max_size;
	sniffer_snapshot_ble_device *list;
} sniffer_snapshot_ble_devices_list;

uint8_t isPayloadValid(uint8_t* payload);
uint8_t isSnifferCharging(sniffer_info info);
uint8_t getSnifferBatteryPercentage(sniffer_info info);
int processPayload(uint8_t* payload, uint32_t len, uint8_t* sniffer_id, uint8_t sniffer_id_assigned);
void generateCommand(uint8_t* buf, command cmd, uint8_t* arg);
void generateVFlipHFlipArg(uint8_t* arg, sniffer_settings settings);
size_t getInitialConfigPackets(uint8_t sniffer_id, sniffer_settings* current_sniffer_settings, sniffer_snapshot_ble_devices_list *snapshot_list, uint8_t* ptr);
size_t getNewConfigPackets(uint8_t sniffer_id, sniffer_settings *current_sniffer_settings, sniffer_snapshot_ble_devices_list *snapshot_list, uint8_t* ptr);
void resetConfigFlags(uint8_t sniffer_id);



#endif /* BLUETRACK_APPLICATION_PROTOCOL_H_ */
