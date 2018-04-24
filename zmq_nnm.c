#include "zmq_nnm.h"

const static char *TAG = "ZMQNNM";

nnm_context nnm;
nnm_pending_cmd nnm_cmd;

static void parse_nnm_packet(uint8_t* packet, uint8_t* status, uint8_t* mac, uint32_t *progress, uint32_t *max_images) {
	*status = packet[0];
	mac[0] = packet[1];
	mac[1] = packet[2];
	mac[2] = packet[3];
	mac[3] = packet[4];
	mac[4] = packet[5];
	mac[5] = packet[6];
	*progress = to32Bits(packet[7], packet[8], packet[9], packet[10]);
	*max_images = to32Bits(packet[11], packet[12], packet[13], packet[14]);
}

/* BlueTrack Neural Network Module (BTNNM) */

void *zmq_nnm_thread(void *arg) {
	log_info(TAG, "ZMQ NNM thread is starting...");

	void *zmq_ctx = zmq_ctx_new();

	nnm_cmd.start_nnm=0;
	pthread_mutex_init(&nnm_cmd.mutex, NULL);

	nnm.connected=0;
	pthread_mutex_init(&nnm.mutex, NULL);

	while(1) {
		void *zmq_sock = zmq_socket(zmq_ctx, ZMQ_REQ);
		int zmq_res = zmq_connect(zmq_sock, ZMQ_NNM_CONN_ADDR);
		uint32_t last_tick = global_tick;
		if(zmq_res == -1) {
			log_error(TAG, "Unable to connect to BTNNM, retrying...");
			break;
		}

		while(1) {
			zmq_msg_t msg;
			int zmq_recv;

			// Sending packet out to BTNNM

			pthread_mutex_lock(&nnm_cmd.mutex);
			if(nnm_cmd.start_nnm == 1) { // "Start NNM" is requested (0x03)
				uint8_t data[] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
				uint8_t *start_mac = data+1;
				MAC_ADDR_ASSIGN(start_mac, nnm_cmd.start_nnm_mac);
				nnm_cmd.start_nnm=0;
				pthread_mutex_unlock(&nnm_cmd.mutex);

				if(zmq_send(zmq_sock, data, 7, 0) == -1) {
					log_error(TAG, "Unable to send ZMQ: %d, reconnecting...", (errno));
					break;
				}
				log_info(TAG, "NNM Start request is sent");
			}else{ // No other request, just request for status report (0x02)
				pthread_mutex_unlock(&nnm_cmd.mutex);
				uint8_t data[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

				if(zmq_send(zmq_sock, data, 7, 0) == -1) {
					log_error(TAG, "Unable to send ZMQ: %d, reconnecting...", (errno));
					break;
				}
			}



			// Receiving packet from BTNNM
			zmq_msg_init(&msg);
			while(1) {
				// Check time out
				if(global_tick-last_tick > 2000) {
					log_error(TAG, "ZMQ timed out, reconnecting...");
					zmq_recv=-1;
					break;
				}

				// Do receive
				zmq_recv = zmq_msg_recv(&msg, zmq_sock, ZMQ_NOBLOCK);
				if(zmq_recv == -1) { // Check error
					if(errno != EAGAIN) { // As we are using non-blocking socket, EAGAIN is not an error
						log_error(TAG, "Unable to receive ZMQ, reconnecting...");
						break;
					}
				}else{ // Data received
					last_tick=global_tick; // Data received, update last_tick
					if(nnm.connected == 0) {
						nnm.connected=1;
						log_info(TAG, "Connected to NNM");
					}

					if(zmq_msg_size(&msg) == 15) {
						uint8_t *recv_data = zmq_msg_data(&msg);
						parse_nnm_packet(recv_data, &nnm.nnm_status, nnm.mac, &nnm.current_progress, &nnm.max_progress);
					}

					break;
				}
			}
			if(zmq_recv == -1) break;
			zmq_msg_close(&msg);

			if(global_tick % 1000 == 0) {
				//log_info(TAG, "Current progress: S=%d, %02X:%02X:%02X:%02X:%02X:%02X, %d/%d", nnm.nnm_status, nnm.mac[0], nnm.mac[1], nnm.mac[2], nnm.mac[3], nnm.mac[4], nnm.mac[5], nnm.current_progress, nnm.max_progress);
			}
		}
		zmq_close(zmq_sock);
		nnm.connected=0;
	}


	return NULL;
}
