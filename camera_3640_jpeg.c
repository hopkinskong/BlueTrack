/*
 * Portions of this file come from OpenMV project (see sensor_* functions in the end of file)
 * Here is the copyright for these parts:
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 *
 * Rest of the functions are licensed under Apache license as found below:
 */

// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "camera.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "soc/soc.h"
#include "sccb.h"
#include "wiring.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "soc/gpio_sig_map.h"
#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
#include "soc/io_mux_reg.h"
#include "sensor.h"
#include "ov2640.h"
#include "ov3640.h"
#include <stdlib.h>
#include <string.h>
#include "rom/lldesc.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "driver/periph_ctrl.h"
#include "camera_common.h"
#include "time.h"
#include "sys/time.h"
#include "esp_system.h"
#include "esp_heap_alloc_caps.h"

static const char* TAG = "camera";

static camera_config_t s_config;
static uint8_t* s_fb;
static size_t s_data_size;
static sensor_t s_sensor;
static bool s_initialized = false;
static int s_fb_w;
static int s_fb_h;
static size_t s_fb_size;
static QueueHandle_t s_data_ready;
static SemaphoreHandle_t s_frame_ready;
static intr_handle_t s_i2s_intr_handle = NULL;
static size_t frame_count;

static lldesc_t *dma_desc;
static dma_elem_t **dma_buf;
static bool dma_done;
static size_t dma_desc_count;
static size_t dma_desc_cur;
static size_t dma_received_count;
static size_t dma_filtered_count;
static size_t dma_per_line;
static size_t dma_buf_width;
static size_t dma_sample_count;
static dma_filter_t dma_filter;
static i2s_sampling_mode_t sampling_mode;
static size_t bytes_per_pixel;
static intr_handle_t s_vsync_intr_handle;
static TaskHandle_t dma_filter_task_hnd;

const int resolution[][2] = {
    {40,    30 },    /* 40x30 */
    {64,    32 },    /* 64x32 */
    {64,    64 },    /* 64x64 */
    {88,    72 },    /* QQCIF */
    {160,   120},    /* QQVGA */
    {128,   160},    /* QQVGA2*/
    {176,   144},    /* QCIF  */
    {240,   160},    /* HQVGA */
    {320,   240},    /* QVGA  */
    {352,   288},    /* CIF   */
    {640,   480},    /* VGA   */
    {800,   600},    /* SVGA  */
    {1280,  1024},   /* SXGA  */
    {1600,  1200},   /* UXGA  */
};

static void i2s_init();
static void i2s_run();
static void IRAM_ATTR gpio_isr(void* arg);
static void IRAM_ATTR i2s_isr(void* arg);
static esp_err_t dma_desc_init();
static void dma_desc_deinit();
static void dma_filter_task(void *pvParameters);
static void dma_filter_jpeg(const dma_elem_t* src, lldesc_t* dma_desc, uint8_t* dst);

static bool is_hs_mode() {
	return (s_config.xclk_freq_hz > 10000000);
}

static size_t i2s_bytes_per_sample(i2s_sampling_mode_t mode)
{
    switch(mode) {
        case SM_0A00_0B00:
            return 4;
        case SM_0A0B_0B0C:
            return 4;
        case SM_0A0B_0C0D:
            return 2;
        default:
            assert(0 && "invalid sampling mode");
            return 0;
    }
}

static esp_err_t enable_out_clock()
{
    periph_module_enable(PERIPH_LEDC_MODULE);

    ledc_timer_config_t timer_conf;
    timer_conf.bit_num = 1;
    timer_conf.freq_hz = s_config.xclk_freq_hz;
    timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    timer_conf.timer_num = s_config.ledc_timer;
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed, rc=%x", err);
        return err;
    }

    ledc_channel_config_t ch_conf;
    ch_conf.channel = s_config.ledc_channel;
    ch_conf.timer_sel = s_config.ledc_timer;
    ch_conf.intr_type = LEDC_INTR_DISABLE;
    ch_conf.duty = 1;
    ch_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    ch_conf.gpio_num = s_config.pin_xclk;
    err = ledc_channel_config(&ch_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed, rc=%x", err);
        return err;
    }
    return ESP_OK;
}

esp_err_t camera_init(const camera_config_t* config)
{
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(&s_config, config, sizeof(s_config));
    ESP_LOGI(TAG, "Enabling XCLK output");
    esp_err_t err = enable_out_clock();
    if(err != ESP_OK) {
    	return err;
    }
    ESP_LOGI(TAG, "Initializing SSCB");
    SCCB_Init(s_config.pin_sscb_sda, s_config.pin_sscb_scl);
    ESP_LOGI(TAG, "Resetting camera");
    gpio_config_t conf = {0};
    conf.pin_bit_mask = 1LL << s_config.pin_reset;
    conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&conf);

    gpio_set_level(s_config.pin_reset, 0);
    delay(1);
    gpio_set_level(s_config.pin_reset, 1);
    delay(10);
    ESP_LOGI(TAG, "Searching for camera address");
    uint8_t addr = SCCB_Probe();
    if (addr == 0) {
        ESP_LOGE(TAG, "Camera address not found");
        return ESP_ERR_CAMERA_NOT_DETECTED;
    }
    ESP_LOGI(TAG, "Detected camera at address=0x%02x", addr);
    s_sensor.slv_addr = addr;

    sensor_id_t* sensor_id=&s_sensor.id;
    sensor_id->pid = SCCB_Read(s_sensor.slv_addr, 0x0A);
    sensor_id->ver = SCCB_Read(s_sensor.slv_addr, 0x0B);
    sensor_id->midl = SCCB_Read(s_sensor.slv_addr, 0x1D);
    sensor_id->midh = SCCB_Read(s_sensor.slv_addr, 0x1C);
    ESP_LOGI(TAG, "Camera PID=0x%02x VER=0x%02x MIDL=0x%02x MIDH=0x%02x",
                s_sensor.id.pid, s_sensor.id.ver, s_sensor.id.midh,
                s_sensor.id.midl);

    if(s_sensor.id.pid == OV2640_PID) {
    	ESP_LOGI(TAG, "OV2640 found, performing initialization...");
        ov2640_init(&s_sensor);
    }else{
    	ESP_LOGI(TAG, "Not OV2640, trying OV3640...");
        sensor_id->pid = SCCB_Read_TwoBytesReg(s_sensor.slv_addr, 0x300a);
        sensor_id->ver = SCCB_Read_TwoBytesReg(s_sensor.slv_addr, 0x300b);
        ESP_LOGI(TAG, "Camera PID=0x%02x VER=0x%02x MIDL=0x%02x MIDH=0x%02x",
                    s_sensor.id.pid, s_sensor.id.ver, s_sensor.id.midh,
                    s_sensor.id.midl);

        if(s_sensor.id.pid == 0x36 && s_sensor.id.ver == 0x4c) {;
        	ESP_LOGI(TAG, "OV3640 found, performing initialization...");
            ov3640_init(&s_sensor);
        }else{
        	ESP_LOGE(TAG, "Not OV2640 or OV3640, exiting...");
        	return ESP_ERR_CAMERA_NOT_SUPPORTED;
        }
    }

    ESP_LOGI(TAG, "Doing SW reset of sensor");
    s_sensor.reset(&s_sensor);



    //ESP_LOGI(TAG, "Reading OV3640 REGs...");
    //ESP_LOGI(TAG, "0xef=0x%02x 0x42=%02x 0xfc=%02x", SCCB_Read_TwoBytesReg(s_sensor.slv_addr, 0x3302), SCCB_Read_TwoBytesReg(s_sensor.slv_addr, 0x3403), SCCB_Read_TwoBytesReg(s_sensor.slv_addr, 0x3304));

    framesize_t framesize = (framesize_t)config->frame_size;
    pixformat_t pix_format = (pixformat_t)config->pixel_format;
    s_fb_w = resolution[framesize][0];
    s_fb_h = resolution[framesize][1];
    s_sensor.set_pixformat(&s_sensor, pix_format);

    ESP_LOGI(TAG, "Setting frame size at %dx%d", s_fb_w, s_fb_h);
    if (s_sensor.set_framesize(&s_sensor, framesize) != 0) {
        ESP_LOGE(TAG, "Failed to set frame size");
        return ESP_ERR_CAMERA_FAILED_TO_SET_FRAME_SIZE;
    }
    s_sensor.set_pixformat(&s_sensor, pix_format);

    if(pix_format == PIXFORMAT_JPEG) {
    	int qp = config->jpeg_quality;
    	int compression_ratio_bound;
    	if(qp >= 30) {
    		compression_ratio_bound = 5;
    	}else if(qp >= 10) {
    		compression_ratio_bound = 10;
    	}else{
    		compression_ratio_bound = 20;
    	}
    	(*s_sensor.set_quality)(&s_sensor, qp);
    	size_t equiv_line_count = s_fb_h / compression_ratio_bound;
    	s_fb_size = s_fb_w * equiv_line_count * 2 /* bpp */;
    	s_fb_size = 110000; // Force FB size
		//s_fb_size = 262144; // Force FB size
    	dma_filter = &dma_filter_jpeg;
    	if(is_hs_mode()) {
    		sampling_mode = SM_0A0B_0B0C;
    	}else{
    		sampling_mode = SM_0A00_0B00;
    	}
    	bytes_per_pixel=2;
    }else{
    	ESP_LOGE(TAG, "Requested format is not supported");
    	return ESP_ERR_NOT_SUPPORTED;
    }

    ESP_LOGI(TAG, "bpp: %d, fb_size: %d, mode: %d, width: %d height: %d",
               bytes_per_pixel, s_fb_size, sampling_mode,
               s_fb_w, s_fb_h);


    ESP_LOGI(TAG, "Allocating frame buffer (%d bytes)", s_fb_size);
    s_fb = (uint8_t*)calloc(s_fb_size, 1);
    if (s_fb == NULL) {
        ESP_LOGE(TAG, "Failed to allocate frame buffer");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Initializing I2S and DMA");
    i2s_init();
    err = dma_desc_init();
    if (err != ESP_OK) {
        free(s_fb);
        return err;
    }

    s_data_ready = xQueueCreate(16, sizeof(size_t));
    s_frame_ready = xSemaphoreCreateBinary();
    if(s_data_ready == NULL || s_frame_ready == NULL) {
    	ESP_LOGE(TAG, "Failed to create semaphores");
    	return ESP_ERR_NO_MEM;
    }

    if(!xTaskCreatePinnedToCore(&dma_filter_task, "dma_filter", 4096, NULL, 10, &dma_filter_task_hnd, 1)) {
    	ESP_LOGE(TAG, "Failed to create DMA filter task");
    	return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Initializing GPIO interrupts");
    gpio_set_intr_type(s_config.pin_vsync, GPIO_INTR_NEGEDGE);
    gpio_intr_enable(s_config.pin_vsync);
    err = gpio_isr_register(&gpio_isr, (void *)TAG, ESP_INTR_FLAG_INTRDISABLED | ESP_INTR_FLAG_IRAM, &s_vsync_intr_handle);
    if(err != ESP_OK) {
    	ESP_LOGE(TAG, "gpio_isr_register failed (%x)", err);
    	return err;
    }

    // skip at least one frame after changing camera settings
    while (gpio_get_level(s_config.pin_vsync) == 0) {
        ;
    }
    while (gpio_get_level(s_config.pin_vsync) != 0) {
        ;
    }
    while (gpio_get_level(s_config.pin_vsync) == 0) {
        ;
    }

    frame_count = 0;
    s_initialized=true;

    ESP_LOGI(TAG, "Initialization done");
    return ESP_OK;
}

uint8_t* camera_get_fb()
{
    if (!s_initialized) {
        return NULL;
    }
    return s_fb;
}

int camera_get_fb_width()
{
    if (!s_initialized) {
        return 0;
    }
    return s_fb_w;
}

int camera_get_fb_height()
{
    if (!s_initialized) {
        return 0;
    }
    return s_fb_h;
}

size_t camera_get_data_size()
{
    if (!s_initialized) {
        return 0;
    }
    return s_data_size;
}

esp_err_t camera_run()
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    struct timeval tv_start, tv_end;
    gettimeofday(&tv_start, NULL);
    i2s_run();
    //ESP_LOGI(TAG, "Waiting for frame");
    xSemaphoreTake(s_frame_ready, portMAX_DELAY);
    gettimeofday(&tv_end, NULL);
    int time_ms = ((tv_end.tv_sec - tv_start.tv_sec) * 1000 + (tv_end.tv_usec - tv_start.tv_usec) / 1000);
    ESP_LOGI(TAG, "F%d: %dms", frame_count, time_ms);
	frame_count++;
    return ESP_OK;
}

void camera_print_fb()
{
	/* Number of pixels to skip
	   in order to fit into terminal screen.
	   Assumed picture to be 80 columns wide.
	   Skip twice as more rows as they look higher.
	 */
	int pixels_to_skip = s_fb_w / 80;

    for (int ih = 0; ih < s_fb_h; ih += pixels_to_skip * 2){
        for (int iw = 0; iw < s_fb_w; iw += pixels_to_skip){
    	    uint8_t px = (s_fb[iw + (ih * s_fb_w)]);
    	    if      (px <  26) printf(" ");
    	    else if (px <  51) printf(".");
    	    else if (px <  77) printf(":");
    	    else if (px < 102) printf("-");
    	    else if (px < 128) printf("=");
    	    else if (px < 154) printf("+");
    	    else if (px < 179) printf("*");
    	    else if (px < 205) printf("#");
    	    else if (px < 230) printf("%%");
    	    else               printf("@");
        }
        printf("\n");
    }
}

static esp_err_t dma_desc_init()
{

	assert(s_fb_w%4 == 0);
	size_t line_size = s_fb_w * bytes_per_pixel * i2s_bytes_per_sample(sampling_mode);
	ESP_LOGI(TAG, "Line width for DMA: %d bytes", line_size);
	dma_per_line = 1;
	size_t buf_size = line_size;
	while(buf_size >= 4096) {
		buf_size /= 2;
		dma_per_line *= 2;
	}
	dma_buf_width = line_size;
	dma_desc_count = dma_per_line * 4;
	ESP_LOGI(TAG, "DMA buffer size: %d, DMA buffers per line: %d", buf_size, dma_per_line);
	ESP_LOGI(TAG, "DMA buffer count: %d", dma_desc_count);

	dma_buf = (dma_elem_t**)malloc(sizeof(dma_elem_t*) * dma_desc_count);
	if(dma_buf == NULL) {
		ESP_LOGE(TAG, "Unable to allocate memory for DMA buffer");
		return ESP_ERR_NO_MEM;
	}
	dma_desc = (lldesc_t*)malloc(sizeof(lldesc_t) * dma_desc_count);
	if(dma_desc == NULL) {
		ESP_LOGE(TAG, "Unable to allocate memory for DMA DESC");
		return ESP_ERR_NO_MEM;
	}

	dma_sample_count = 0;
	for(int i=0; i<dma_desc_count; ++i) {
		ESP_LOGI(TAG, "Allocating DMA buffer #%d, size=%d", i, buf_size);
		dma_elem_t* buf = (dma_elem_t*)malloc(buf_size);
		if(buf == NULL) {
			ESP_LOGE(TAG, "Unable to allocate memory for #%d", i);
			return ESP_ERR_NO_MEM;
		}
		dma_buf[i] = buf;

		lldesc_t* pd = &dma_desc[i];
		pd->length = buf_size;
		if(sampling_mode == SM_0A0B_0B0C && (i+1)%dma_per_line == 0) {
			pd->length -= 4;
		}
		dma_sample_count += pd->length / 4;
		pd->size = pd->length;
		pd->owner = 1;
		pd->sosf = 1;
		pd->buf = (uint8_t*) buf;
		pd->offset = 0;
		pd->empty = 0;
		pd->eof = 1;
		pd->qe.stqe_next = &dma_desc[(i + 1) % dma_desc_count];
	}

	dma_done = false;

    return ESP_OK;
}

static void dma_desc_deinit()
{
    if (dma_buf) {
        for (int i = 0; i < dma_desc_count; ++i) {
            free(dma_buf[i]);
        }
    }
    free(dma_buf);
    free(dma_desc);
}

static inline void i2s_conf_reset()
{
	const uint32_t lc_conf_reset_flags = I2S_IN_RST_S | I2S_AHBM_RST_S | I2S_AHBM_FIFO_RST_S;
	I2S0.lc_conf.val |= lc_conf_reset_flags;
	I2S0.lc_conf.val &= ~lc_conf_reset_flags;

    const uint32_t conf_reset_flags = I2S_RX_RESET_M | I2S_RX_FIFO_RESET_M
            | I2S_TX_RESET_M | I2S_TX_FIFO_RESET_M;
    I2S0.conf.val |= conf_reset_flags;
    I2S0.conf.val &= ~conf_reset_flags;
    while (I2S0.state.rx_fifo_reset_back) {
        ;
    }
}

static void i2s_init()
{
    // Configure input GPIOs
    gpio_num_t pins[] = { s_config.pin_d7, s_config.pin_d6, s_config.pin_d5,
            s_config.pin_d4, s_config.pin_d3, s_config.pin_d2, s_config.pin_d1,
            s_config.pin_d0, s_config.pin_vsync, s_config.pin_href,
            s_config.pin_pclk };
    gpio_config_t conf = { .mode = GPIO_MODE_INPUT, .pull_up_en =
            GPIO_PULLUP_ENABLE, .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE };
    for (int i = 0; i < sizeof(pins) / sizeof(gpio_num_t); ++i) {
        conf.pin_bit_mask = 1LL << pins[i];
        gpio_config(&conf);
    }

    // Route input GPIOs to I2S peripheral using GPIO matrix
    gpio_matrix_in(s_config.pin_d0, I2S0I_DATA_IN0_IDX, false);
    gpio_matrix_in(s_config.pin_d1, I2S0I_DATA_IN1_IDX, false);
    gpio_matrix_in(s_config.pin_d2, I2S0I_DATA_IN2_IDX, false);
    gpio_matrix_in(s_config.pin_d3, I2S0I_DATA_IN3_IDX, false);
    gpio_matrix_in(s_config.pin_d4, I2S0I_DATA_IN4_IDX, false);
    gpio_matrix_in(s_config.pin_d5, I2S0I_DATA_IN5_IDX, false);
    gpio_matrix_in(s_config.pin_d6, I2S0I_DATA_IN6_IDX, false);
    gpio_matrix_in(s_config.pin_d7, I2S0I_DATA_IN7_IDX, false);
    gpio_matrix_in(s_config.pin_vsync, I2S0I_V_SYNC_IDX, false);
    gpio_matrix_in(0x38, I2S0I_H_SYNC_IDX, false);
    gpio_matrix_in(s_config.pin_href, I2S0I_H_ENABLE_IDX, false);
    gpio_matrix_in(s_config.pin_pclk, I2S0I_WS_IN_IDX, false);

    // Enable and configure I2S peripheral
    periph_module_enable(PERIPH_I2S0_MODULE);
    // Toggle some reset bits in LC_CONF register
    // Toggle some reset bits in CONF register
    i2s_conf_reset();
    // Enable slave mode (sampling clock is external)
    I2S0.conf.rx_slave_mod = 1;
    // Enable parallel mode
    I2S0.conf2.lcd_en = 1;
    // Use HSYNC/VSYNC/HREF to control sampling
    I2S0.conf2.camera_en = 1;
    // Configure clock divider
    I2S0.clkm_conf.clkm_div_a = 1;
    I2S0.clkm_conf.clkm_div_b = 0;
    I2S0.clkm_conf.clkm_div_num = 2;
    // FIFO will sink data to DMA
    I2S0.fifo_conf.dscr_en = 1;
    // FIFO configuration, TBD if needed
    I2S0.fifo_conf.rx_fifo_mod = sampling_mode;
    I2S0.fifo_conf.rx_fifo_mod_force_en = 1;
    I2S0.conf_chan.rx_chan_mod = 1;
    // Clear flags which are used in I2S serial mode
    I2S0.sample_rate_conf.rx_bits_mod = 0;
    I2S0.conf.rx_right_first = 0;
    I2S0.conf.rx_msb_right = 0;
    I2S0.conf.rx_msb_shift = 0;
    I2S0.conf.rx_mono = 0;
    I2S0.conf.rx_short_sync = 0;
    I2S0.timing.val = 0;

    // Allocate I2S interrupt, keep it disabled
    esp_intr_alloc(ETS_I2S0_INTR_SOURCE,
            ESP_INTR_FLAG_INTRDISABLED | ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM,
            &i2s_isr, NULL, &s_i2s_intr_handle);
}

static void i2s_stop()
{
    esp_intr_disable(s_i2s_intr_handle);
    esp_intr_disable(s_vsync_intr_handle);
    i2s_conf_reset();
    I2S0.conf.rx_start = 0;
    size_t val = SIZE_MAX;
    BaseType_t higher_priority_task_woken;
    xQueueSendFromISR(s_data_ready, &val, &higher_priority_task_woken);
}

static void i2s_run()
{
    // wait for vsync
    //ESP_LOGI(TAG, "Waiting for positive edge on VSYNC");
    while (gpio_get_level(s_config.pin_vsync) == 0) {
        ;
    }
    while (gpio_get_level(s_config.pin_vsync) != 0) {
        ;
    }
    //ESP_LOGI(TAG, "Got VSYNC");

    dma_done = false;
	dma_desc_cur = 0;
	dma_received_count = 0;
	dma_filtered_count = 0;
	esp_intr_disable(s_i2s_intr_handle);
	i2s_conf_reset();

	I2S0.rx_eof_num = dma_sample_count;
	I2S0.in_link.addr = (uint32_t) &dma_desc[0];
	I2S0.in_link.start = 1;
	I2S0.int_clr.val = I2S0.int_raw.val;
	I2S0.int_ena.val = 0;
	I2S0.int_ena.in_done = 1;
	esp_intr_enable(s_i2s_intr_handle);
	if (s_config.pixel_format == CAMERA_PF_JPEG) {
		esp_intr_enable(s_vsync_intr_handle);
	}
	I2S0.conf.rx_start = 1;
}

static void IRAM_ATTR signal_dma_buf_received(bool* need_yield)
{
    size_t dma_desc_filled = dma_desc_cur;
    dma_desc_cur = (dma_desc_filled + 1) % dma_desc_count;
    dma_received_count++;
    BaseType_t higher_priority_task_woken;
    BaseType_t ret = xQueueSendFromISR(s_data_ready, &dma_desc_filled, &higher_priority_task_woken);
    if (ret != pdTRUE) {
        ESP_EARLY_LOGW(TAG, "queue send failed (%d), dma_received_count=%d", ret, dma_received_count);
    }
    *need_yield = (ret == pdTRUE && higher_priority_task_woken == pdTRUE);
}

static void IRAM_ATTR i2s_isr(void* arg)
{
    I2S0.int_clr.val = I2S0.int_raw.val;
    bool need_yield;
    signal_dma_buf_received(&need_yield);
    //ESP_EARLY_LOGI(TAG, "i2s_isr, cnt=%d", dma_received_count);
    if(dma_received_count == s_fb_h * dma_per_line) {
    	i2s_stop();
    }

    if(need_yield) {
    	portYIELD_FROM_ISR();
    }
}

static void IRAM_ATTR dma_filter_jpeg(const dma_elem_t* src, lldesc_t* dma_desc, uint8_t* dst)
{
    size_t end = dma_desc->length / sizeof(dma_elem_t) / 4;
    // manual unrolling 4 iterations of the loop here
    for (size_t i = 0; i < end; ++i) {
        dst[0] = src[0].sample1;
        dst[1] = src[1].sample1;
        dst[2] = src[2].sample1;
        dst[3] = src[3].sample1;
        src += 4;
        dst += 4;
    }
    // for
    if ((dma_desc->length & 0x7) != 0) {
        dst[0] = src[0].sample1;
        dst[1] = src[1].sample1;
        dst[2] = src[2].sample1;
        dst[3] = src[2].sample2;
    }
}

static void IRAM_ATTR gpio_isr(void* arg)
{
    GPIO.status1_w1tc.val = GPIO.status1.val;
    GPIO.status_w1tc = GPIO.status;
    bool need_yield = false;
    //ESP_EARLY_LOGI(TAG, "gpio isr, cnt=%d", dma_received_count);
    if (gpio_get_level(s_config.pin_vsync) == 0 &&
            dma_received_count > 0 &&
            !dma_done) {
        signal_dma_buf_received(&need_yield);
        i2s_stop();
    }
    if (need_yield) {
        portYIELD_FROM_ISR();
    }
}

static size_t get_fb_pos()
{
    return dma_filtered_count * s_fb_w *
            bytes_per_pixel / dma_per_line;
}

static void IRAM_ATTR dma_filter_task(void *pvParameters)
{
    while (true) {
        size_t buf_idx;
        xQueueReceive(s_data_ready, &buf_idx, portMAX_DELAY);
        if (buf_idx == SIZE_MAX) {
            s_data_size = get_fb_pos();
            xSemaphoreGive(s_frame_ready);
            continue;
        }

        uint8_t* pfb = s_fb  + get_fb_pos();
        const dma_elem_t* buf = dma_buf[buf_idx];
        lldesc_t* desc = &dma_desc[buf_idx];
        //ESP_LOGI(TAG, "dma_flt: pos=%d ", get_fb_pos());
        (*dma_filter)(buf, desc, pfb);
        dma_filtered_count++;
        //ESP_LOGI(TAG, "dma_flt: flt_count=%d ", dma_filtered_count);
    }
}
