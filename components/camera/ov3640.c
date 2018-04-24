/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * OV2640 driver.
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sccb.h"
#include "ov3640.h"
#include "esp_log.h"

#define SVGA_HSIZE     (800)
#define SVGA_VSIZE     (600)

#define UXGA_HSIZE     (1600)
#define UXGA_VSIZE     (1200)

static const char* TAG = "ov3640-camera";

static const uint16_t default_regs[][2] = {
		{0x3012, 0x80},
		 {0x3012, 0x80},
		 {0x304d, 0x45},
		 {0x3087, 0x16},
		 {0x30aa, 0x45},
		 {0x30b0, 0xff},
		 {0x30b1, 0xff},
		 {0x30b2, 0x10},
		 {0x30d7, 0x10},
		 {0x309e, 0x00},
		 {0x3602, 0x26},
		 {0x3603, 0x4D},
		 {0x364c, 0x04},
		 {0x360c, 0x12},
		 {0x361e, 0x00},
		 {0x361f, 0x11},
		 {0x3633, 0x32},
		 {0x3629, 0x3c},
		 {0x300e, 0x32},
		 {0x300f, 0x21},
		 {0x3010, 0x21},
		 {0x3011, 0x00},
		 {0x304c, 0x81},
		 {0x3018, 0x58},
		 {0x3019, 0x59},
		 {0x301a, 0x61},
		 {0x307d, 0x00},
		 {0x3077, 0x00}, // HREF=POS
		 {0x3087, 0x02},
		 {0x3082, 0x20},
		 {0x303c, 0x08},
		 {0x303d, 0x18},
		 {0x303e, 0x06},
		 {0x303F, 0x0c},
		 {0x3030, 0x62},
		 {0x3031, 0x26},
		 {0x3032, 0xe6},
		 {0x3033, 0x6e},
		 {0x3034, 0xea},
		 {0x3035, 0xae},
		 {0x3036, 0xa6},
		 {0x3037, 0x6a},
		 {0x3015, 0x12},
		 {0x3014, 0x84},
		 {0x3013, 0xf7},
		 {0x3104, 0x02},
		 {0x3105, 0xfd},
		 {0x3106, 0x00},
		 {0x3107, 0xff},
		 {0x3308, 0xa5},
		 {0x3316, 0xff},
		 {0x3317, 0x00},
		 {0x3087, 0x02},
		 {0x3082, 0x20},
		 {0x3300, 0x13},
		 {0x3301, 0xd6},
		 {0x3302, 0xef},
		 {0x30b8, 0x20},
		 {0x30b9, 0x17},
		 {0x30ba, 0x04},
		 {0x30bb, 0x08},
		 {0x3100, 0x02},
		 {0x3304, 0xfc},
		 {0x3400, 0x00},
		 {0x3404, 0x00},
		 {0x3020, 0x01},
		 {0x3021, 0x1d},
		 {0x3022, 0x00},
		 {0x3023, 0x0a},
		 {0x3024, 0x08},
		 {0x3025, 0x18},
		 {0x3026, 0x06},
		 {0x3027, 0x0c},
		 {0x335f, 0x68},
		 {0x3360, 0x18},
		 {0x3361, 0x0c},
		 {0x3362, 0x68},
		 {0x3363, 0x08},
		 {0x3364, 0x04},
		 {0x3403, 0x42},
		 {0x3088, 0x08},
		 {0x3089, 0x00},
		 {0x308a, 0x06},
		 {0x308b, 0x00},
		 {0x3507, 0x06},
		 {0x350a, 0x4f},
		 {0x3600, 0xc4},
		 {0x3011, 0x86},
		 //{0xffff, 0xff},
    { 0x00,     0x00 }
};

static const uint16_t jpeg_regs[][2] = {
		{0x3302, 0xef},
			{0x335f, 0x68},
			{0x3360, 0x18},
			{0x3361, 0x0c},
			{0x3362, 0x11},
			{0x3363, 0x68},
			{0x3364, 0x24},
			{0x3403, 0x42},
			{0x3088, 0x01},
			{0x3089, 0x60},
			{0x308a, 0x01},
			{0x308b, 0x20},

			//{0xffff,0xff},

    { 0x00,     0x00 }
};

static int reset(sensor_t *sensor)
{
    int i=0;
    const uint8_t (*regs)[2];

    while(default_regs[i][0]) {
    	SCCB_Write_TwoBytesReg(sensor->slv_addr, default_regs[i][0], default_regs[i][1]);
    	i++;
    }

    /* delay n ms */
    delay(10);

    i=0;
    while(jpeg_regs[i][0]) {
		SCCB_Write_TwoBytesReg(sensor->slv_addr, jpeg_regs[i][0], jpeg_regs[i][1]);
		i++;
	}

    return 0;
}

static int set_pixformat(sensor_t *sensor, pixformat_t pixformat)
{


    return 0;
}

static int set_framesize(sensor_t *sensor, framesize_t framesize)
{


    return 0;
}

static int set_framerate(sensor_t *sensor, framerate_t framerate)
{
	return 0;
}

static int set_contrast(sensor_t *sensor, int level)
{

	return 0;
}

static int set_brightness(sensor_t *sensor, int level)
{

	return 0;
}

static int set_saturation(sensor_t *sensor, int level)
{


	return 0;
}

static int set_gainceiling(sensor_t *sensor, gainceiling_t gainceiling)
{

	return 0;
}

static int set_quality(sensor_t *sensor, int qs)
{

	return 0;
}

static int set_colorbar(sensor_t *sensor, int enable)
{
	return 0;
}

static int set_whitebal(sensor_t *sensor, int enable)
{
	return 0;
}

static int set_gain_ctrl(sensor_t *sensor, int enable)
{
	return 0;
}

static int set_exposure_ctrl(sensor_t *sensor, int enable)
{
	return 0;
}

static int set_hmirror(sensor_t *sensor, int enable)
{
	return 0;
}

static int set_vflip(sensor_t *sensor, int enable)
{
	return 0;
}

int ov3640_init(sensor_t *sensor)
{
	ESP_LOGI(TAG, "Initializing OV3640...");
    /* set function pointers */
    sensor->reset = reset;
    sensor->set_pixformat = set_pixformat;
    sensor->set_framesize = set_framesize;
    sensor->set_framerate = set_framerate;
    sensor->set_contrast  = set_contrast;
    sensor->set_brightness= set_brightness;
    sensor->set_saturation= set_saturation;
    sensor->set_gainceiling = set_gainceiling;
    sensor->set_quality = set_quality;
    sensor->set_colorbar = set_colorbar;
    sensor->set_gain_ctrl = set_gain_ctrl;
    sensor->set_exposure_ctrl = set_exposure_ctrl;
    sensor->set_whitebal = set_whitebal;
    sensor->set_hmirror = set_hmirror;
    sensor->set_vflip = set_vflip;

    // Set sensor flags
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_VSYNC, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_HSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_PIXCK, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_FSYNC, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_JPEGE, 0);

    ESP_LOGI(TAG, "OV3640 Initialization Done.");
    return 0;
}
