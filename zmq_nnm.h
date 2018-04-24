#ifndef ZMQ_NNM_H_
#define ZMQ_NNM_H_

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <pthread.h>

#include <zmq.h>
#include <jansson.h>

#include "b64/b64.h"


#include "config.h"
#include "util.h"
#include "log.h"
#include "tick.h"

typedef struct nnm_context {
	atomic_uint connected;

	pthread_mutex_t mutex;
	uint8_t nnm_status;
	uint8_t mac[6];
	uint32_t current_progress;
	uint32_t max_progress;
} nnm_context;

typedef struct nnm_pending_cmd {
	uint8_t start_nnm;
	uint8_t start_nnm_mac[6];
	pthread_mutex_t mutex;
} nnm_pending_cmd;

void *zmq_nnm_thread(void *arg);

#endif /* ZMQ_RPC_H_ */
