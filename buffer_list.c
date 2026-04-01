#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "buffer_list.h"

int create_cb_node(cl_node** dest, size_t type_size, cl_node *next, cl_node* prev){
    cl_node *node = malloc(sizeof(cl_node)); 
    if(node == NULL){
        return 0; 
    }
    node->item = malloc(type_size); 
    if(node->item == NULL){
        free(node); 
        return 0; 
    }
    node->next = next; 
    node->prev = prev; 

    *dest = node; 

    return 1;
}



int create_cb_list(circular_list** dest_ptr, size_t type_size, int size){
    circular_list* cl;
    cl = malloc(sizeof(circular_list)); 
    if(cl == NULL)
        return 0; 


    create_cb_node(&cl->head, type_size, NULL, NULL);
    if(cl->head == NULL){
        //Error allocating size free the cl
        free(cl); 
        return 0; 
    }
    cl->tail = cl->head; 

    
    

    cl->count = 0; 

    cl->capacity = size;
    cl->type_size = type_size;

    *dest_ptr = cl;

    return 1; 
}
int delete_cb_list(circular_list** buf){
    if(*buf == NULL){ 
        printf("Error\n"); 
        return 0; 
    }

    if((*buf)->head == NULL){
        printf("Error\n"); 
        return 0;  
    }

    while((*buf)->head != NULL){
        cl_node* tmp = (*buf)->head->next; 
        free((*buf)->head);
        (*buf)->head = tmp; 
    }
    
    
    free(*buf); 
    
    //Null out the pointer to avoid malloc issues
    *buf = NULL; 
    
    return 1; 
}

int push_value_list(circular_list* buf, void* item, int override){
    
    if(buf == NULL)
        return 0; 
    
    if(is_full_list(buf)) {
        if(override == 0) return 0; 
        buf->tail->next = buf->head;
        buf->head = buf->head->next; 
    }
    else {
        buf->count++;
    }

    
    //copying data to the last position of the buffer from the item
    memcpy((char *)buf->tail->item, item, buf->type_size); 

    if(buf->tail->next == NULL) 
        create_cb_node(&(buf->tail->next), buf->type_size, NULL, buf->tail); 
    
    buf->tail = buf->tail->next;

    return 1; 
    
}
int pop_value_list(circular_list *buf, void* val){
    if(buf == NULL)
        return 0; 
    
    if(buf->head == NULL || buf->tail == NULL)
        return 0; 

    if(is_empty_list(buf))
    {
        printf("The buffer is empty\n"); 
        return 0; 
    }

    
    
    //Copy the value first
    memcpy(val, (char *)buf->head->item, buf->type_size);


    cl_node *tmp = buf->head; 

    if(buf->head->next == NULL){
        create_cb_node(&buf->head->next, buf->type_size, NULL, NULL); 
        buf->head = buf->head->next; 
        buf->tail = buf->head; 
    }
    else buf->head = buf->head->next; 
       //Clearing out the element
    memset((char *)tmp->item, 0, buf->type_size);

    buf->count--; 
    
    return 1; 
}

int is_empty_list(circular_list* buf){
    return buf->count == 0; 
}
int is_full_list(circular_list* buf){
    return buf->capacity == buf->count; 
}