#ifndef _CIRCULARBUFFER_H_
#define _CIRCULARBUFFER_H_

#include "common.h"
#include <string.h>

typedef struct circular_buffer
{
    void *buffer;     // data buffer
    void *buffer_end; // end of data buffer
    size_t capacity;  // maximum number of items in the buffer
    size_t count;     // number of items in the buffer
    size_t sz;        // size of each item in the buffer
    void *head;       // pointer to head
    void *tail;       // pointer to tail
} circular_buffer;

STATUS cb_init(circular_buffer *cb, size_t capacity, size_t sz);
void cb_free(circular_buffer *cb);
STATUS cb_push_back(circular_buffer *cb, const void *item);
STATUS cb_pop_front(circular_buffer *cb, void *item);
size_t cb_space_left(circular_buffer *cb);
size_t cb_space_occupied(circular_buffer *cb);

#endif
