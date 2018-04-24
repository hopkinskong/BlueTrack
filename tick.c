#include "tick.h"

const static char* TAG = "TICK";

void *tick_thread(void *arg) {
	log_info(TAG, "Tick thread is starting...");
	global_tick=0;

	while(1) {
		//if(global_tick > 0 && global_tick % 1000 == 0) printf("\n");

		nanosleep((const struct timespec[]){{0, (1000UL*1000UL*TICK_MS)}}, NULL);
		global_tick++;
	}
}
