#pragma once
#include <stddef.h> 


typedef struct circular_node circular_node; 


typedef struct cl_node {
    void *item;
    struct cl_node *next;
    struct cl_node *prev;
} cl_node;

typedef struct circular_list {
    cl_node *head;
    cl_node *tail;
    size_t type_size;
    int count;
    int capacity;
} circular_list;

int create_cb_list(circular_list**, size_t, int); 
int delete_cb_list(circular_list**); 

int push_value_list(circular_list*, void*, int);
int pop_value_list(circular_list *buf, void*); 

int is_empty_list(circular_list* buf); 
int is_full_list(circular_list* buf); 