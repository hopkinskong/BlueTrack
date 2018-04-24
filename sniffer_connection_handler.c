#include "sniffer_connection_handler.h"

const static char* TAG = "CONN";

extern sniffer_context sniffers[64];
extern victims_to_track_list victims_list;
void *sniffer_connection_handler(void *client_socket) {
	int socket = *((int *)client_socket);
	uint8_t *recvBuf;
	unsigned int lastActivityTick; // Last tick where we received the sniffer data
	parser_context* parserCtx;
	pthread_t parser_thrd;
	struct timeval timeout;
	uint8_t initial_configuration_completed=0;
	sniffer_settings current_sniffer_settings;
	sniffer_snapshot_ble_devices_list snapshot_list;
	recvBuf = (uint8_t *)malloc(TCP_RECEIVE_BUFFER_SIZE);
	lastActivityTick=global_tick;
	log_success(TAG, "Serving sniffer at tick=%u", lastActivityTick);

	// Setting TCP timeouts
	timeout.tv_sec=2;
	timeout.tv_usec=0;
	if(setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		log_error(TAG, "Unable to set receive timeout, dropping connection");
		close(socket);
		free(recvBuf);
		return NULL;
	}
	if(setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		log_error(TAG, "Unable to set receive timeout, dropping connection");
		close(socket);
		free(recvBuf);
		return NULL;
	}

	// Start parser thread
	parserCtx = malloc(sizeof(parser_context));
	pthread_mutex_init(&parserCtx->mutex, NULL);
	//parserCtx->parserBufferSize=0;
	parserCtx->parserBuffer=0;
	if(!circular_buffer_init(&parserCtx->parserBuffer, PARSER_BUFFER_SIZE)) {
		log_error(TAG, "Unable to initialize circular buffer, dropping connection");
		close(socket);
		free(recvBuf);
		free(parserCtx);
		return NULL;
	}
	parserCtx->connectionClosed=0;
	parserCtx->invalidDataReceived=0;
	parserCtx->sniffer_id_assigned=0;
	if(pthread_create(&parser_thrd, NULL, sniffer_data_parser, (void*)parserCtx) != 0) {
		log_error(TAG, "Unable to start data parser thread, dropping connection");
		close(socket);
		free(recvBuf);
		free(parserCtx);
		return NULL;
	}

	// If the TCP stream idle for more than SNIFFER_TIMEOUT_TICK, we should disconnect the sniffer client
	while((lastActivityTick + SNIFFER_TIMEOUT_TICK) > global_tick && parserCtx->invalidDataReceived == 0) {
		// Poll the socket for 1ms
		int poll_result = poll((struct pollfd[]){{socket, POLLIN, 0}}, 1, 1);

		if(poll_result == -1) {
			// Poll error
			log_error(TAG, "Error when polling sniffer socket");
			log_error(TAG, "%d: %s", errno, strerror(errno));
			break;
		}else if(poll_result == 0) {
			// Poll timeout
		}else{
			// Data available
			ssize_t recvSize = recv(socket, recvBuf, TCP_RECEIVE_BUFFER_SIZE, 0);
			lastActivityTick=global_tick;

			// Check if receive has error
			if(recvSize == -1) {
				log_error(TAG, "Error when receiving data from sniffer");
				log_error(TAG, "%d: %s", errno, strerror(errno));
				break;
			}else if(recvSize == 0) {
				// Disconnected (gracefully)
				break;
			}
			pthread_mutex_lock(&parserCtx->mutex);
			// Copy receive buffer to parser buffer
			if(recvSize <= circular_buffer_free_size(parserCtx->parserBuffer)) { // Warning, buffer overflow will be ignored
				//int i=0;
				//for(i=0; i<recvSize; i++) {
				//	printf("%02X ", recvBuf[i]);
				//}
				//printf("\n");
				circular_buffer_push(parserCtx->parserBuffer, recvBuf, recvSize);
			}else{
				log_error(TAG, "Buffer overflow on parser buffer, continue anyway");
			}

			if(parserCtx->sniffer_id_assigned && !initial_configuration_completed) { // We got sniffer_id, and not done initial config, do it now.
				initial_configuration_completed=1;

				pthread_mutex_lock(&victims_list.mutex); // We have to lock the mutex here this early, or else we can't ensure the list size is still the same
				size_t init_config_packets_size = 16*5+16*victims_list.list_size; // MAX size here
				uint8_t *init_config_packets = (uint8_t *)malloc(init_config_packets_size);
				resetConfigFlags(parserCtx->sniffer_id);

				// The generated config packets might be lower, due to lower victims is_active=1
				size_t generated_config_packets_size = getInitialConfigPackets(parserCtx->sniffer_id, &current_sniffer_settings, &snapshot_list, init_config_packets);
				pthread_mutex_unlock(&victims_list.mutex);
				if(write(socket, init_config_packets, generated_config_packets_size) != -1) {
					log_info(TAG, "Sent initial sniffer configurations for ID: %d, size=%d", parserCtx->sniffer_id, init_config_packets_size);
				}else{
					log_error(TAG, "Error when sending initial sniffer configurations for ID: %d", parserCtx->sniffer_id);
					log_error(TAG, "%d: %s", errno, strerror(errno));
				}
				free(init_config_packets);
			}else if(initial_configuration_completed) { // We already done initial config, check for new config and push to sniffer

				pthread_mutex_lock(&victims_list.mutex); // We have to lock the mutex here this early, or else we can't ensure the list size is still the same
				size_t max_config_packets_size = 16*7+16*victims_list.list_size;
				uint8_t *config_packets = (uint8_t *)malloc(max_config_packets_size);
				// The generated config packets might be lower, due to lower victims is_active=1 or TEST_SNAPSHOT/REBOOT is not needed
				size_t generated_config_packets_size = getNewConfigPackets(parserCtx->sniffer_id, &current_sniffer_settings, &snapshot_list, config_packets);
				pthread_mutex_unlock(&victims_list.mutex);
				if(generated_config_packets_size > 0) {
					if(write(socket, config_packets, generated_config_packets_size) != -1) {
						log_info(TAG, "Sent new sniffer configurations for ID: %d, size=%d", parserCtx->sniffer_id, generated_config_packets_size);
					}else{
						log_error(TAG, "Error when sending new sniffer configurations for ID: %d", parserCtx->sniffer_id);
						log_error(TAG, "%d: %s", errno, strerror(errno));
					}
				}
				free(config_packets);
			}
			pthread_mutex_unlock(&parserCtx->mutex);
		}
	}

	free(snapshot_list.list);

	pthread_mutex_lock(&parserCtx->mutex);
	if(parserCtx->sniffer_id_assigned) {
		log_error(TAG, "Sniffer ID=%d disconnected", parserCtx->sniffer_id);
	}else{
		log_error(TAG, "Sniffer ID=UNKNOWN disconnected");
	}
	pthread_mutex_unlock(&parserCtx->mutex);
	//log_error(TAG, "Sniffer disconnected at tick=%d; exiting application for debug...", global_tick);
	//exit(-1);
	parserCtx->connectionClosed=1;
	close(socket);
	free(recvBuf);
	return NULL;
}
