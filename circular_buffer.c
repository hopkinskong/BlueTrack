#include "circular_buffer.h"
#include "log.h"

int circular_buffer_init(circular_buffer** cb, size_t size) {
	(*cb) = malloc(sizeof(circular_buffer));
	if((*cb) == NULL) return 0;

	(*cb)->used_bytes=0;
	(*cb)->max_bytes=size;

	(*cb)->buf = malloc(size);
	if((*cb)->buf == NULL) return 0;
	(*cb)->buf_end = (*cb)->buf + size;
	(*cb)->head = (*cb)->buf;
	(*cb)->tail = (*cb)->buf;
	return 1;
}

void circular_buffer_deinit(circular_buffer** cb) {
	free((*cb)->buf);
	(*cb)->used_bytes=0;
	(*cb)->max_bytes=0;
	(*cb)->head = (*cb)->tail = (*cb)->buf = (*cb)->buf_end = NULL;
	free((*cb));
}

int circular_buffer_push(circular_buffer* cb, uint8_t* data, size_t data_size) {
	if((cb->used_bytes + data_size) > cb->max_bytes) return 0;

	size_t i;
	uint8_t *ptr = data;

	for(i=0; i<data_size; i++) {
		*cb->head = *ptr;

		ptr++;
		cb->head++;

		if(cb->head == cb->buf_end) cb->head = cb->buf;
		cb->used_bytes++;
	}
	return 1;
}

int circular_buffer_pop(circular_buffer* cb, uint8_t* data, size_t data_size) {
	if(cb->used_bytes-data_size < 0) return 0;

	size_t i;
	uint8_t *ptr = data;
	for(i=0; i<data_size; i++) {
		*ptr = *cb->tail;

		ptr++;
		cb->tail++;

		if(cb->tail == cb->buf_end) cb->tail = cb->buf;
		cb->used_bytes--;
	}
	return 1;
}

// Read without popping (Increment internal tail pointer)
int circular_buffer_read(circular_buffer* cb, uint8_t* data, size_t data_size) {
	if(cb->used_bytes-data_size < 0) return 0;

	size_t i;
	uint8_t *ptr = data;
	uint8_t *tailPtr = cb->tail; // never move the original tail
	for(i=0; i<data_size; i++) {
		*ptr = *tailPtr;

		ptr++;
		tailPtr++;

		if(tailPtr == cb->buf_end) tailPtr = cb->buf;
	}
	return 1;
}

int circular_buffer_erase(circular_buffer* cb, size_t data_size) {
	if(cb->used_bytes-data_size < 0) return 0;

	size_t i;
	for(i=0; i<data_size; i++) {
		cb->tail++;
		if(cb->tail == cb->buf_end) cb->tail = cb->buf;
		cb->used_bytes--;
	}
	return 1;
}

size_t circular_buffer_free_size(circular_buffer* cb) {
	return (cb->max_bytes - cb->used_bytes);
}
