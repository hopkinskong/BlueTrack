#ifndef SNIFFER_DATA_PARSER_H_
#define SNIFFER_DATA_PARSER_H_

#include <stdlib.h>
#include <stdatomic.h>
#include <time.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>

#include "config.h"
#include "log.h"
#include "util.h"
#include "bluetrack_application_protocol.h"
#include "circular_buffer.h"

typedef struct parser_context {
	circular_buffer *parserBuffer;
	pthread_mutex_t mutex;
	atomic_uint connectionClosed;
	atomic_uint invalidDataReceived;
	uint8_t sniffer_id;
	uint8_t sniffer_id_assigned;
} parser_context;


void *sniffer_data_parser(void *ctx);

#endif /* SNIFFER_DATA_PARSER_H_ */
