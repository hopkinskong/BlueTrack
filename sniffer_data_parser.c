#include "sniffer_data_parser.h"
#include "worker.h"

const static char* DEFAULT_TAG = "PARSER";

extern sniffer_context sniffers[64];
void *sniffer_data_parser(void *ctx) {
	log_info(DEFAULT_TAG, "Parser is starting...");
	parser_context* context = (parser_context *)ctx;
	char TAG[11];

	// Change TAG to DEFAULT_TAG
	strncpy(TAG, DEFAULT_TAG, sizeof(TAG));

	while(context->connectionClosed == 0 && context->invalidDataReceived == 0) {
		pthread_mutex_lock(&context->mutex);
		if(context->parserBuffer->used_bytes > 8) { // Receive BTAH+32-bits Payload Length
			uint8_t buf[8];
			circular_buffer_read(context->parserBuffer, buf, 8);
			if(buf[0] != 'B' || buf[1] != 'T' || buf[2] != 'A' || buf[3] != 'H') {
				context->invalidDataReceived = 1;
				log_error(TAG, "Invalid Application Header Received, disconnecting...");
				log_error("DEBUG", "Invalid Hdr=%02X %02X %02X %02X %02X %02X %02X %02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
			}else{
				uint32_t payloadLength = to32Bits(buf[4], buf[5], buf[6], buf[7]);
				//log_info("DEBUG", "Receiving payload len=%d", payloadLength);
				if(context->parserBuffer->used_bytes >= (4+4+payloadLength)) { // 4bytes BTAH+32-bit payload length
					// Now all the payload is received in parser buffer
					uint8_t* btapFramePtr = (uint8_t *)malloc(4+4+payloadLength); // 4-bytes BTAH + 4-bytes payload length + actual payload (1-bytes payload type + variable length of payload data)
					if(btapFramePtr == NULL) {
						// Consider this data invalid, kill the connection
						context->invalidDataReceived=1;
						log_error(TAG, "Unable to allocate buffer for payload, disconnecting...");
					}else{
						// We can safely pop the data from our circular buffer,
						// as we already receive a complete BTAP frame and successfully
						// allocate a memory for this BTAP frame
						if(circular_buffer_pop(context->parserBuffer, btapFramePtr, (4+4+payloadLength))) {
							uint8_t* payloadPtr = btapFramePtr+8; // Skip BTAH+4-bytes payload length
							// Try to decode this payload
							if(isPayloadValid(payloadPtr)) {
								// Process the payload associated with this sniffer_id
								if(!processPayload(payloadPtr, payloadLength, &(context->sniffer_id), context->sniffer_id_assigned)) {
									// Process payload failed
									log_error(TAG, "Error when processing payload...");
									context->invalidDataReceived=1;
								}else{
									// Process payload succeed
									if(!context->sniffer_id_assigned) {
										// After processPayload(...), we must already have a sniffer_id
										// We now update our TAG
										snprintf(TAG, sizeof(TAG), "PARSER_%d", context->sniffer_id);
										log_info(TAG, "Sniffer ID assigned: %d", context->sniffer_id);
										context->sniffer_id_assigned=1;
									}
								}
							}else{
								context->invalidDataReceived=1;
								log_error(TAG, "Unknown payload type(0x%02X), disconnecting...", payloadPtr[0]);
							}
							free(btapFramePtr);
						}else{
							// Pop failed, something went wrong
							// Consider this data invalid, kill the connection
							context->invalidDataReceived=1;
							log_error(TAG, "Unable to pop from circular buffer, disconnecting...");
						}
					}
				}
			}
		}
		pthread_mutex_unlock(&context->mutex);
		nanosleep((const struct timespec[]){{0, (1000UL*1000UL)}}, NULL);
	}

	// Make sure connectionClosed == 1
	// This means the connection_handler thread is dead
	// Then we free the memory
	// Need to do this because the another thread might still using the context
	while(context->connectionClosed == 0);

	// Notify worker connection is closed
	if(context->sniffer_id_assigned) {
		sniffers[context->sniffer_id].connected=0;
	}

	circular_buffer_deinit(&context->parserBuffer);
	free(context);
	return NULL;
}
