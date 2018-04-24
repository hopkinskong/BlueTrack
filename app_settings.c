#include "app_settings.h"

#include <stdatomic.h>
typedef struct app_settings {
	atomic_uint rotate_cw_90deg;
} app_settings;

static app_settings global_app_settings;

uint8_t get_rotate_cw_90deg() {
	return global_app_settings.rotate_cw_90deg;
}

void set_rotate_cw_90deg(uint8_t val) {
	global_app_settings.rotate_cw_90deg=val;
}
