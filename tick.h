#ifndef TICK_H_
#define TICK_H_

#include <time.h>
#include <stdatomic.h>

#include "config.h"
#include "log.h"

void *tick_thread(void *arg);

atomic_uint global_tick;


#endif /* TICK_H_ */
