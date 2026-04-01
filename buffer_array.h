#pragma once
#include <stddef.h> 


typedef struct circular_buffer{
    void* buf; 
    size_t type_size; 
    int head; 
    int tail; 
    int capacity;
    int count;
} circular_buffer;

int create_cb_array(circular_buffer**, size_t, int); 
int delete_cb_array(circular_buffer**); 

int push_value_array(circular_buffer*, void*, int);
int pop_value_array(circular_buffer *buf, void*);

int is_empty_array(circular_buffer* buf); 
int is_full_array(circular_buffer* buf); 