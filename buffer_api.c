#include "buffer_api.h"
#include "buffer_array.h"
#include "buffer_list.h"

int create_cb(void** buf, size_t type_size, int size, const char type){
    if(type == 1){
        return create_cb_array((circular_buffer**)buf, type_size, size);
    }
    return create_cb_list((circular_list**)buf, type_size, size);
}



int delete_cb(void** buf, const char type){
    if(type == 1){
        return delete_cb_array((circular_buffer**)buf);
    }
    
    return delete_cb_list((circular_list**) buf); 
    
}



int push_value(void* buf, void* item, int override, const char type){
    if(type == 1){
        return push_value_array((circular_buffer*)buf, item, override);
    }
    return push_value_list((circular_list*) buf, item, override); 
    
    
}



int pop_value(void *buf, void* val, const char type){
    if(type == 1){
        return pop_value_array((circular_buffer*)buf, val);
    }
    return pop_value_list((circular_list*) buf, val); 
    
}




int is_empty(void* buf, const char type){
    if(type == 1){
        return is_empty_array((circular_buffer*)buf);
    }
    return is_empty_list((circular_list*)buf);
    
}
int is_full(void* buf, const char type){
    if(type == 1){
        return is_full_array((circular_buffer*)buf);
    }
    return is_full_list((circular_list*)buf);
    
}