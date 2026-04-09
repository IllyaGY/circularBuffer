#include <stddef.h> 
#include <curses.h>
#include <string.h>
#include <stdlib.h>


#include "buffer_api.h"


// use linux error codes -- done
// Put into void* -- done (sort of built an API on top)
// Remove macros make user api's usable - done
// Make it a library - done (static for now)
// DLL (done)


void test(); 

int ctr = 0; 

static buffer_inf* active_buf = NULL; 


typedef struct student student; 

struct student{
    char name[50]; 
    int id; 
};


void input_buffer(){
    student obj; 

    sprintf(obj.name, "Student %d", ctr); 

    obj.name[49] = '\0';
    obj.id = ctr; 

    ctr++; 

    push_value(active_buf, &obj); 
}


void call(int input, void *val){
    if(active_buf == NULL || val == NULL) 
        return; 
    
    
    switch (input)
    {
        case 'r':
            pop_value(active_buf, val);
            break;
        
        case 'i':
            input_buffer();  
            break; 
        
        case 'd':
            delete_cb(&active_buf); 
            break; 

        case 'c':
            // create_cb();
            break; 


        default:
            break; 
    }
}

void output(const void* item, int i){
    const student *s = item;
    mvprintw(i, 0, "%s %d\n", s->name, s->id);
}


void run(){
    student val; 
    while(1){
        if(active_buf == NULL){
            printw("%s", "No buffer to show\n"); 
            break;
        }
        clear();  

        nodelay(stdscr, TRUE);
        
        foreach_value(active_buf, output);

        
        int ch = getch();
        if(ch != ERR){
            memset(&val, 0, sizeof(student)); 
            call(ch, &val);
        }

        refresh();
        napms(16);

        
    }

    

}

int main(){

    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    //Circular Buffer
    test();
    run(); 

    endwin();
    
    return 0; 

}











void test(){
    create_cb(&active_buf, sizeof(student), 10,  ARRAY);


    #if TEST_CASE == 0
    for(int i = 0; i < get_capacity(active_buf); i++) {
        student a; 
        char b[50]; 
        sprintf(b, "Student %d", i); 
        strcpy(a.name, b); 
        a.name[49] = '\0'; 
        a.id = ctr; 
        ctr++; 
        push_value(active_buf, &a); 
    }


    #elif TEST_CASE == 1
    for(int i = 0; i < get_capacity(active_buf)/2; i++) {
        student a; 
        char b[50]; 
        sprintf(b, "Student %d", i); 
        strcpy(a.name, b); 
        a.name[49] = '\0'; 
        a.id = ctr; 
        ctr++; 
        push_value(active_buf, &a); 
    }

    #endif


}
