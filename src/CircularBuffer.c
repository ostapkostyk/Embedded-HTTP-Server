#include "CircularBuffer.h"

STATUS cb_init(circular_buffer *cb, size_t capacity, size_t sz)
{
    cb->buffer = memmgr_alloc(capacity * sz);
    if(cb->buffer == NULL) { return ERROR; }
    cb->buffer_end = (char *)cb->buffer + capacity * sz;
    cb->capacity = capacity;
    cb->count = 0;
    cb->sz = sz;
    cb->head = cb->buffer;
    cb->tail = cb->buffer;

    return SUCCESS;
}

void cb_free(circular_buffer *cb)
{
    memmgr_free(cb->buffer);	//	TODO: remove free
    // clear out other fields too, just to be safe
}

STATUS cb_push_back(circular_buffer *cb, const void *item)
{
    if(cb->count == cb->capacity) { return ERROR; }
    memcpy(cb->head, item, cb->sz);
    cb->head = (char*)cb->head + cb->sz;
    if(cb->head == cb->buffer_end)
        cb->head = cb->buffer;
    cb->count++;
    return SUCCESS;
}

STATUS cb_pop_front(circular_buffer *cb, void *item)
{
    if(cb->count == 0) { return ERROR; }
    memcpy(item, cb->tail, cb->sz);
    cb->tail = (char*)cb->tail + cb->sz;
    if(cb->tail == cb->buffer_end)
        cb->tail = cb->buffer;
    cb->count--;
    return SUCCESS;
}

size_t cb_space_left(circular_buffer *cb)
{
   return (cb->capacity - cb->count);
}

size_t cb_space_occupied(circular_buffer *cb)
{
   return cb->count;
}

