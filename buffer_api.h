#pragma once
#include <stddef.h>


int create_cb(void**, size_t, int, const char); 
int delete_cb(void**, const char); 

int push_value(void*, void*, int, const char);
int pop_value(void *buf, void*, const char);

int is_empty(void* buf, const char); 
int is_full(void* buf, const char); 