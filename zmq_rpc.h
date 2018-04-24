#ifndef ZMQ_RPC_H_
#define ZMQ_RPC_H_

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <zmq.h>
#include <jansson.h>

#include "b64/b64.h"

#include "jsonrpc.h"

#include "config.h"
#include "bluetrack_application_protocol.h"
#include "worker.h"
#include "log.h"
#include "database.h"
#include "victims_photos.h"
#include "app_settings.h"
#include "zmq_nnm.h"

void *zmq_rpc_thread(void *arg);

#endif /* ZMQ_RPC_H_ */
