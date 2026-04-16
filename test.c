#include <curses.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer_api.h"

#define NAME_CAPACITY 128
#define STATUS_CAPACITY 256
#define INPUT_CAPACITY 32
#define DEFAULT_CAPACITY 10

typedef struct student {
    char name[50];
    int id;
} student;

typedef struct render_ctx {
    int row;
    int max_rows;
    int hidden_count;
} render_ctx;

static buffer_inf *active_buf = NULL;
static int next_student_id = 0;
static render_ctx active_render = {0};

static void set_status(char *status, size_t status_size, const char *message){
    if (status == NULL || status_size == 0)
    {
        return;
    }

    snprintf(status, status_size, "%s", message);
}

static int prompt_line(const char *prompt, char *buffer, size_t buffer_size){
    int max_y = 0;
    int max_x = 0;

    if (buffer == NULL || buffer_size == 0)
    {
        return EFAULT;
    }

    getmaxyx(stdscr, max_y, max_x);
    nodelay(stdscr, FALSE);
    echo();
    curs_set(1);

    move(max_y - 1, 0);
    clrtoeol();
    mvprintw(max_y - 1, 0, "%s", prompt);
    refresh();

    int rc = getnstr(buffer, (int)buffer_size - 1);

    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);

    if (rc == ERR)
    {
        return ERR;
    }

    buffer[buffer_size - 1] = '\0';
    (void)max_x;
    return 0;
}

static int prompt_non_empty_line(const char *prompt, char *buffer, size_t buffer_size){
    int rc = prompt_line(prompt, buffer, buffer_size);
    if (rc != 0)
    {
        return rc;
    }

    if (buffer[0] == '\0')
    {
        return EINVAL;
    }

    return 0;
}

static int prompt_capacity(const char *prompt, int *capacity){
    char input[INPUT_CAPACITY];
    char *end = NULL;
    long parsed = 0;

    if (capacity == NULL)
    {
        return EFAULT;
    }

    int rc = prompt_non_empty_line(prompt, input, sizeof(input));
    if (rc != 0)
    {
        return rc;
    }

    errno = 0;
    parsed = strtol(input, &end, 10);
    if (errno != 0 || end == input || *end != '\0')
    {
        return EINVAL;
    }
    if (parsed <= 0 || parsed > INT_MAX)
    {
        return EINVAL;
    }

    *capacity = (int)parsed;
    return 0;
}

static const char *buffer_type_label(int type){
    switch (type)
    {
        case ARRAY:
            return "array";
        case LLIST:
            return "linked-list";
        default:
            return "unknown";
    }
}

static void render_student(const void *item, int index){
    const student *entry = item;

    if (entry == NULL)
    {
        return;
    }
    if (active_render.row >= active_render.max_rows)
    {
        active_render.hidden_count++;
        return;
    }

    mvprintw(
        active_render.row,
        0,
        "index=%d id=%d name=%s",
        index,
        entry->id,
        entry->name
    );
    active_render.row++;
}

static int create_active_buffer(int type, int capacity){
    if (active_buf != NULL)
    {
        int err = delete_cb(&active_buf);
        if (err != 0)
        {
            return err;
        }
    }

    return create_cb(&active_buf, sizeof(student), capacity, type);
}

static int push_student(const char *name, student *added){
    student entry;
    size_t copy_len = 0;

    if (active_buf == NULL)
    {
        return EFAULT;
    }
    if (name == NULL)
    {
        return EFAULT;
    }

    memset(&entry, 0, sizeof(entry));
    copy_len = strlen(name);
    if (copy_len >= sizeof(entry.name))
    {
        copy_len = sizeof(entry.name) - 1;
    }

    memcpy(entry.name, name, copy_len);
    entry.name[copy_len] = '\0';
    entry.id = next_student_id++;

    int err = push_value(active_buf, &entry);
    if (err != 0)
    {
        return err;
    }

    if (added != NULL)
    {
        *added = entry;
    }

    return 0;
}

static int draw_screen(const char *status, int active_type){
    int max_y = 0;
    int max_x = 0;
    int size = 0;
    int capacity = 0;
    int empty = 1;
    int full = 0;
    int err = 0;

    getmaxyx(stdscr, max_y, max_x);
    clear();

    mvprintw(0, 0, "=== Circular Buffer ===");
    mvprintw(1, 0, "commands: a = new array, l = new list, i = insert, r = remove, d = destroy, q = quit");

    if (active_buf == NULL)
    {
        mvprintw(3, 0, "buffer: (none)");
        mvprintw(5, 0, "Create a buffer with 'a' or 'l' to begin.");
    }
    else
    {
        err = get_size(active_buf, &size);
        if (err != 0)
        {
            return err;
        }

        err = get_capacity(active_buf, &capacity);
        if (err != 0)
        {
            return err;
        }

        err = is_empty(active_buf, &empty);
        if (err != 0)
        {
            return err;
        }

        err = is_full(active_buf, &full);
        if (err != 0)
        {
            return err;
        }

        mvprintw(3, 0, "buffer: %s", buffer_type_label(active_type));
        mvprintw(4, 0, "size=%d capacity=%d empty=%d full=%d", size, capacity, empty, full);

        active_render.row = 6;
        active_render.max_rows = max_y - 2;
        active_render.hidden_count = 0;

        if (size == 0)
        {
            mvprintw(active_render.row, 0, "(empty)");
        }
        else
        {
            err = foreach_value(active_buf, render_student);
            if (err != 0)
            {
                return err;
            }
        }

        if (active_render.hidden_count > 0)
        {
            mvprintw(
                max_y - 2,
                0,
                "... %d more entr%s hidden",
                active_render.hidden_count,
                active_render.hidden_count == 1 ? "y" : "ies"
            );
        }
    }

    move(max_y - 1, 0);
    clrtoeol();
    mvprintw(max_y - 1, 0, "%s", status);
    (void)max_x;
    refresh();
    return 0;
}

int main(void){
    char name[NAME_CAPACITY];
    char status[STATUS_CAPACITY];
    int active_type = ARRAY;

    set_status(status, sizeof(status), "ready");

    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);

    while (1)
    {
        int err = draw_screen(status, active_type);
        if (err != 0)
        {
            endwin();
            fprintf(stderr, "display failed: %d\n", err);
            if (active_buf != NULL)
            {
                delete_cb(&active_buf);
            }
            return 1;
        }

        int ch = getch();
        if (ch == ERR)
        {
            napms(16);
            continue;
        }

        switch (ch)
        {
            case 'a':
            case 'l':
            {
                int capacity = DEFAULT_CAPACITY;
                int prompt_rc = prompt_capacity("capacity: ", &capacity);
                if (prompt_rc == ERR)
                {
                    set_status(status, sizeof(status), "input cancelled");
                    break;
                }
                if (prompt_rc == EINVAL)
                {
                    set_status(status, sizeof(status), "please enter a positive whole number");
                    break;
                }

                active_type = ch == 'a' ? ARRAY : LLIST;
                err = create_active_buffer(active_type, capacity);
                if (err != 0)
                {
                    snprintf(status, sizeof(status), "create failed: %d", err);
                }
                else
                {
                    snprintf(
                        status,
                        sizeof(status),
                        "created %s buffer with capacity %d",
                        buffer_type_label(active_type),
                        capacity
                    );
                }
                break;
            }

            case 'i':
            {
                if (active_buf == NULL)
                {
                    set_status(status, sizeof(status), "create a buffer first");
                    break;
                }

                int prompt_rc = prompt_non_empty_line("student name to add: ", name, sizeof(name));
                if (prompt_rc == ERR)
                {
                    set_status(status, sizeof(status), "input cancelled");
                    break;
                }
                if (prompt_rc == EINVAL)
                {
                    set_status(status, sizeof(status), "please enter a non-empty name");
                    break;
                }

                student added;
                err = push_student(name, &added);
                if (err == ENOBUFS)
                {
                    set_status(status, sizeof(status), "buffer is full");
                }
                else if (err != 0)
                {
                    snprintf(status, sizeof(status), "insert failed: %d", err);
                }
                else
                {
                    snprintf(status, sizeof(status), "added: %s (id=%d)", added.name, added.id);
                }
                break;
            }

            case 'r':
            {
                student removed;

                if (active_buf == NULL)
                {
                    set_status(status, sizeof(status), "create a buffer first");
                    break;
                }

                memset(&removed, 0, sizeof(removed));
                err = pop_value(active_buf, &removed);
                if (err == EAGAIN)
                {
                    set_status(status, sizeof(status), "buffer is empty");
                }
                else if (err != 0)
                {
                    snprintf(status, sizeof(status), "remove failed: %d", err);
                }
                else
                {
                    snprintf(status, sizeof(status), "removed: %s (id=%d)", removed.name, removed.id);
                }
                break;
            }

            case 'd':
                if (active_buf == NULL)
                {
                    set_status(status, sizeof(status), "no active buffer to destroy");
                    break;
                }

                err = delete_cb(&active_buf);
                if (err != 0)
                {
                    snprintf(status, sizeof(status), "destroy failed: %d", err);
                }
                else
                {
                    set_status(status, sizeof(status), "buffer destroyed");
                }
                break;

            case 'q':
                endwin();
                if (active_buf != NULL)
                {
                    delete_cb(&active_buf);
                }
                return 0;

            default:
                set_status(status, sizeof(status), "unknown command");
                break;
        }
    }
}
