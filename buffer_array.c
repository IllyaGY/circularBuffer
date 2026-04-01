#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "buffer_array.h"




int create_cb_array(circular_buffer** dest_ptr, size_t type_size, int size){
    
    circular_buffer* cb; 
    cb = malloc(sizeof(circular_buffer)); 
    if(cb == NULL)
        //Error allocating size
        return 0; 
    
    cb->buf = malloc(type_size * size); 

    if(cb->buf == NULL){
        //Error allocating size free the cb
        free(cb); 
        return 0; 
    }
    
    cb->head = 0;
    cb->tail = 0; 

    cb->count = 0; 

    cb->capacity = size;
    cb->type_size = type_size; 


   
    printf("%p\n", cb) ;
    *dest_ptr = cb; 
    return 1; 
    
}

int delete_cb_array(circular_buffer** buf){ 
    if(*buf == NULL){ 
        printf("Error\n"); 
        return 0; 
    }

    if((*buf)->buf == NULL){
        printf("Error\n"); 
        return 0;  
    }

    
    free((*buf)->buf);
    
    free(*buf); 
    
    //Null out the pointer to avoid malloc issues
    *buf = NULL; 
    
    return 1; 
}

int push_value_array(circular_buffer* buf, void* item, int override){


    if(buf == NULL)
        return 0; 
    
    if(is_full_array(buf)) {
        if(override == 0) return 0; 
        buf->head++; 
        buf->head %= buf->capacity;
    }
    else {
        buf->count++;
    }

    //copying data to the last position of the buffer from the item
    memcpy((char *)buf->buf + buf->tail * buf->type_size, item, buf->type_size); 


    buf->tail++; 

    //wrap around
    buf->tail %= buf->capacity; 



    return 1; 
    
}
int pop_value_array(circular_buffer *buf, void* val){
    if(buf == NULL)
        return 0; 
    
    if(buf->buf == NULL)
        return 0; 

    if(is_empty_array(buf))
    {
        printf("The buffer is empty\n"); 
        return 0; 
    }

    buf->count--; 
    
    //Copy the value first
    memcpy(val, (char *)buf->buf + buf->head * buf->type_size, buf->type_size);


    //Clearing out the element
    memset((char *)buf->buf + buf->head * buf->type_size, 0, buf->type_size);


    buf->head++;
    //wrap around
    buf->head %= buf->capacity;  
    


    return 1; 
}

int is_empty_array(circular_buffer* buf){
    if(buf == NULL) return -1;
    return buf->count == 0; 
}
int is_full_array(circular_buffer* buf){
    if(buf == NULL) return -1;
    return buf->count == buf->capacity;  
}