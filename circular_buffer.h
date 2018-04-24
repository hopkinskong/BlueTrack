#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <inttypes.h>
#include <stdlib.h>

typedef struct circular_buffer {
	uint8_t *head, *tail;
	uint8_t *buf, *buf_end;
	size_t max_bytes;
	size_t used_bytes;
} circular_buffer;


int circular_buffer_init(circular_buffer** cb, size_t size);
void circular_buffer_deinit(circular_buffer** cb);
int circular_buffer_push(circular_buffer* cb, uint8_t* data, size_t data_size);
int circular_buffer_pop(circular_buffer* cb, uint8_t* data, size_t data_size);
int circular_buffer_read(circular_buffer* cb, uint8_t* data, size_t data_size);
size_t circular_buffer_free_size(circular_buffer* cb);

#endif /* CIRCULAR_BUFFER_H_ */
