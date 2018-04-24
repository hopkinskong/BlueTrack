/*
 * config.h
 *
 *  Created on: 2018¦~2¤ë19¤é
 *      Author: hopkins
 */

#ifndef MAIN_CONFIG_H_
#define MAIN_CONFIG_H_

// Constants
#undef  IOE_PCA9534
#define IOE_PCA9534 1
#undef  IOE_TCA6408
#define IOE_TCA6408 2

// BlueTrack Wi-Fi SSID
#undef  WIFI_SSID
#define WIFI_SSID "BlueTrack_Server"

// BlueTrack Wi-Fi Password
#undef  WIFI_PW
#define WIFI_PW "BlueTrack1234"

// BlueTrack Backend IP
#undef  BLUETRACK_BACKEND_IP
#define BLUETRACK_BACKEND_IP "192.168.0.2"

// BlueTrack Backend Port
#undef  BLUETRACK_BACKEND_PORT
#define BLUETRACK_BACKEND_PORT "9998"

// Maximum BLE scan items
#undef  MAX_SCAN_ITEMS
#define MAX_SCAN_ITEMS 64

// IOE Device
#undef  IOE_DEVICE
//#define IOE_DEVICE IOE_TCA6408
#define IOE_DEVICE IOE_PCA9534

// Camera Images Buffer
#undef  CAMERA_IMAGES_BUFFER_SIZE
#define CAMERA_IMAGES_BUFFER_SIZE 5

// Max images sent per iteration
#undef  MAX_IMAGES_SENT_PER_ITERATION
#define MAX_IMAGES_SENT_PER_ITERATION 2

#endif /* MAIN_CONFIG_H_ */
