#pragma once

#include <stddef.h>

#include "buffer_api.h"

typedef struct thread_buffer thread_buffer;

int create_thread_buffer(thread_buffer **dest, size_t type_size, int capacity, int type);
int delete_thread_buffer(thread_buffer **dest);
int push_thread_buffer(thread_buffer *tb, void *item);
int pop_thread_buffer(thread_buffer *tb, void *out);
int foreach_thread_buffer(thread_buffer *tb, cb_iter_fn func);
int get_size_thread_buffer(thread_buffer *tb, int *size);
int get_capacity_thread_buffer(thread_buffer *tb, int *capacity);
int is_empty_thread_buffer(thread_buffer *tb, int *result);
int is_full_thread_buffer(thread_buffer *tb, int *result);
