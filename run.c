#include <stddef.h> 
#include <curses.h>
#include <string.h>
#include <stdlib.h>

#include "buffer_api.h"
#include "buffer_array.h"
#include "buffer_list.h"

#ifndef TYPE
#define TYPE 0
#endif

#ifndef TEST_CASE
#define TEST_CASE 0
#endif

#ifndef SIZE
#define SIZE 10
#endif

void test(); 


static int ctr = 0; 

#if TYPE == 1

static circular_buffer* active_buf = NULL; 

#elif TYPE == 0

static circular_list* active_buf = NULL; 

#endif

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

    push_value(active_buf, &obj, 1, TYPE); 
}


void call(int input, void *val, char *popped){
    if(active_buf == NULL) return; 
    switch (input)
    {
        case 'r':
            *popped = pop_value((void *)active_buf, val, TYPE);
            break;
        
        case 'i':
            input_buffer();  
            break; 
        
        case 'd':
            delete_cb((void **)&active_buf, TYPE); 
            break; 

        case 'c':
            // create_cb();
            break; 


        default:
            break; 
    }
}


void run(){
    student val; 
    char popped = 0;
    while(1){
        if(active_buf == NULL){
            //if(located_new_buffer(active_buf) == 0)
            {
                printw("%s", "No buffer to show\n"); 
                break;
            } 
        }
        clear();

        #if TYPE == 1
            for(int i = 0; i < active_buf->capacity; i++){
                student value; 
                memcpy(&value, (char*)active_buf->buf + i * active_buf->type_size, active_buf->type_size); 

                mvprintw(i, 0, ""); 
                if (i == active_buf->head && i == active_buf->tail){
                    printw("%s %d <--- HEAD / TAIL", value.name, value.id);
                }
                else if(i == active_buf->head){
                    printw("%s %d <--- HEAD", value.name, value.id);
                    if(popped)
                        printw("     Popped: %s %d", val.name, val.id);
                }
                else if(i == active_buf->tail)
                    printw("%s %d <--- TAIL", value.name, value.id);
                else
                    printw("%s %d", value.name, value.id);

                
                printw("\n");
                
            }
        
        #elif TYPE == 0
            cl_node* stepper = active_buf->head;

            for (int i = 0; i < active_buf->count; i++) {
                student value;
                memcpy(&value, stepper->item, active_buf->type_size);

                mvprintw(i, 0, "");
                if (i == 0 && i == active_buf->count - 1)
                    printw("%s %d <--- HEAD / TAIL", value.name, value.id);
                else if (i == 0) {
                    printw("%s %d <--- HEAD", value.name, value.id);
                    if (popped)
                        printw("     Popped: %s %d", val.name, val.id);
                }
                else if (i == active_buf->count - 1)
                    printw("%s %d <--- TAIL", value.name, value.id);
                else
                    printw("%s %d", value.name, value.id);

                printw("\n");
                stepper = stepper->next;
            }

            for (int i = active_buf->count; i < active_buf->capacity; i++) {
                mvprintw(i, 0, "0\n");
            }
    

        #endif

        

        nodelay(stdscr, TRUE);
        
        int ch = getch();
        if(ch != ERR){
            memset(&val, 0, sizeof(student)); 
            popped = 0; 
            call(ch, &val, &popped);
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
    create_cb((void **)&active_buf, sizeof(student), SIZE, TYPE); 


    #if TEST_CASE == 0
    for(int i = 0; i < active_buf->capacity; i++) {
        student a; 
        char b[50]; 
        sprintf(b, "Student %d", i); 
        strcpy(a.name, b); 
        a.name[49] = '\0'; 
        a.id = ctr; 
        ctr++; 
        push_value(active_buf, &a, 0, TYPE); 
    }


    #elif TEST_CASE == 1
    for(int i = 0; i < active_buf->capacity/2; i++) {
        student a; 
        char b[50]; 
        sprintf(b, "Student %d", i); 
        strcpy(a.name, b); 
        a.name[49] = '\0'; 
        a.id = ctr; 
        ctr++; 
        push_value(active_buf, &a, 0, TYPE); 
    }


    #endif


}
