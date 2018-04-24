#ifndef SNIFFER_CONNECTION_HANDLER_H_
#define SNIFFER_CONNECTION_HANDLER_H_

#include <unistd.h>
#include <stdatomic.h>
#include <signal.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "log.h"
#include "config.h"
#include "bluetrack_application_protocol.h"
#include "sniffer_data_parser.h"
#include "worker.h"
#include "circular_buffer.h"

void *sniffer_connection_handler(void *client_socket);

extern atomic_uint global_tick;

#endif /* SNIFFER_CONNECTION_HANDLER_H_ */
