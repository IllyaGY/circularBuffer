#include <curses.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer_api.h"
#include "thread_buffer.h"

#define NAME_CAPACITY 128
#define STATUS_CAPACITY 256
#define INPUT_CAPACITY 32
#define DEFAULT_CAPACITY 10
#define THREAD_PRODUCER_COUNT 3
#define THREAD_CONSUMER_COUNT 2
#define THREAD_PRODUCER_ITEMS 4
#define THREAD_REMAINING_TARGET 4

typedef struct student {
    char name[50];
    int id;
} student;

typedef struct render_ctx {
    int row;
    int max_rows;
    int hidden_count;
} render_ctx;

typedef enum demo_style {
    DEMO_STYLE_SINGLE = 0,
    DEMO_STYLE_THREAD = 1
} demo_style;

typedef struct worker_ctx {
    thread_buffer *tb;
    int worker_id;
    int item_count;
    int last_err;
} worker_ctx;

static buffer_inf *active_buf = NULL;
static thread_buffer *active_thread_buf = NULL;
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

static int prompt_demo_style(const char *prompt, demo_style *style){
    char input[INPUT_CAPACITY];
    int rc = prompt_non_empty_line(prompt, input, sizeof(input));
    if (rc != 0)
    {
        return rc;
    }

    if (input[0] == 'n')
    {
        *style = DEMO_STYLE_SINGLE;
        return 0;
    }
    if (input[0] == 't')
    {
        *style = DEMO_STYLE_THREAD;
        return 0;
    }

    return EINVAL;
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

static const char *demo_style_label(demo_style style){
    switch (style)
    {
        case DEMO_STYLE_SINGLE:
            return "single";
        case DEMO_STYLE_THREAD:
            return "thread";
        default:
            return "unknown";
    }
}

static int destroy_active_buffers(void){
    int first_err = 0;

    if (active_buf != NULL)
    {
        int err = delete_cb(&active_buf);
        if (err != 0 && first_err == 0)
        {
            first_err = err;
        }
    }

    if (active_thread_buf != NULL)
    {
        int err = delete_thread_buffer(&active_thread_buf);
        if (err != 0 && first_err == 0)
        {
            first_err = err;
        }
    }

    return first_err;
}

static int make_student(const char *name, int id, student *entry){
    size_t copy_len = 0;

    if (name == NULL || entry == NULL)
    {
        return EFAULT;
    }

    memset(entry, 0, sizeof(*entry));
    copy_len = strlen(name);
    if (copy_len >= sizeof(entry->name))
    {
        copy_len = sizeof(entry->name) - 1;
    }

    memcpy(entry->name, name, copy_len);
    entry->name[copy_len] = '\0';
    entry->id = id;
    return 0;
}

static int create_active_buffer(demo_style style, int type, int capacity){
    int err = destroy_active_buffers();
    if (err != 0)
    {
        return err;
    }

    if (style == DEMO_STYLE_SINGLE)
    {
        return create_cb(&active_buf, sizeof(student), capacity, type);
    }

    return create_thread_buffer(&active_thread_buf, sizeof(student), capacity, type);
}

static int push_student_single(const char *name, student *added){
    student entry;
    int err = make_student(name, next_student_id++, &entry);
    if (err != 0)
    {
        return err;
    }

    err = push_value(active_buf, &entry);
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

static int push_student_thread(const char *name, student *added){
    student entry;
    int err = make_student(name, next_student_id++, &entry);
    if (err != 0)
    {
        return err;
    }

    err = push_thread_buffer(active_thread_buf, &entry);
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

static int push_student(demo_style style, const char *name, student *added){
    if (style == DEMO_STYLE_SINGLE)
    {
        if (active_buf == NULL)
        {
            return EFAULT;
        }
        return push_student_single(name, added);
    }

    if (active_thread_buf == NULL)
    {
        return EFAULT;
    }
    return push_student_thread(name, added);
}

static int pop_student(demo_style style, student *removed){
    if (removed == NULL)
    {
        return EFAULT;
    }

    memset(removed, 0, sizeof(*removed));

    if (style == DEMO_STYLE_SINGLE)
    {
        if (active_buf == NULL)
        {
            return EFAULT;
        }
        return pop_value(active_buf, removed);
    }

    if (active_thread_buf == NULL)
    {
        return EFAULT;
    }
    return pop_thread_buffer(active_thread_buf, removed);
}

static int get_buffer_state(
    demo_style style,
    int *size,
    int *capacity,
    int *empty,
    int *full
){
    int err = 0;

    if (style == DEMO_STYLE_SINGLE)
    {
        err = get_size(active_buf, size);
        if (err != 0)
        {
            return err;
        }
        err = get_capacity(active_buf, capacity);
        if (err != 0)
        {
            return err;
        }
        err = is_empty(active_buf, empty);
        if (err != 0)
        {
            return err;
        }
        return is_full(active_buf, full);
    }

    err = get_size_thread_buffer(active_thread_buf, size);
    if (err != 0)
    {
        return err;
    }
    err = get_capacity_thread_buffer(active_thread_buf, capacity);
    if (err != 0)
    {
        return err;
    }
    err = is_empty_thread_buffer(active_thread_buf, empty);
    if (err != 0)
    {
        return err;
    }
    return is_full_thread_buffer(active_thread_buf, full);
}

static int foreach_students(demo_style style, cb_iter_fn func){
    if (style == DEMO_STYLE_SINGLE)
    {
        return foreach_value(active_buf, func);
    }

    return foreach_thread_buffer(active_thread_buf, func);
}

static int has_active_buffer(demo_style style){
    return style == DEMO_STYLE_SINGLE ? active_buf != NULL : active_thread_buf != NULL;
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

static void *producer_main(void *arg){
    worker_ctx *ctx = arg;

    for (int i = 0; i < ctx->item_count; i++)
    {
        char name[NAME_CAPACITY];
        student entry;

        snprintf(name, sizeof(name), "Producer %d Item %d", ctx->worker_id, i);
        if (make_student(name, ctx->worker_id * 100 + i, &entry) != 0)
        {
            ctx->last_err = EFAULT;
            return NULL;
        }

        ctx->last_err = push_thread_buffer(ctx->tb, &entry);
        if (ctx->last_err != 0)
        {
            return NULL;
        }

        usleep(30000);
    }

    return NULL;
}

static void *consumer_main(void *arg){
    worker_ctx *ctx = arg;

    for (int i = 0; i < ctx->item_count; i++)
    {
        student removed;
        ctx->last_err = pop_thread_buffer(ctx->tb, &removed);
        if (ctx->last_err != 0)
        {
            return NULL;
        }

        usleep(45000);
    }

    return NULL;
}

static int run_thread_demo(char *status, size_t status_size){
    pthread_t producers[THREAD_PRODUCER_COUNT];
    pthread_t consumers[THREAD_CONSUMER_COUNT];
    worker_ctx producer_ctx[THREAD_PRODUCER_COUNT];
    worker_ctx consumer_ctx[THREAD_CONSUMER_COUNT];
    int err = 0;
    int size = 0;
    int capacity = 0;
    int empty = 0;
    int full = 0;
    int producer_total = THREAD_PRODUCER_COUNT * THREAD_PRODUCER_ITEMS;
    int desired_remaining = 0;
    int consumer_total = 0;

    if (active_thread_buf == NULL)
    {
        return EFAULT;
    }

    err = get_buffer_state(DEMO_STYLE_THREAD, &size, &capacity, &empty, &full);
    if (err != 0)
    {
        return err;
    }

    desired_remaining = THREAD_REMAINING_TARGET;
    if (desired_remaining > capacity - size)
    {
        desired_remaining = capacity - size;
    }
    if (desired_remaining < 0)
    {
        desired_remaining = 0;
    }

    consumer_total = producer_total - desired_remaining;

    for (int i = 0; i < THREAD_PRODUCER_COUNT; i++)
    {
        producer_ctx[i].tb = active_thread_buf;
        producer_ctx[i].worker_id = i + 1;
        producer_ctx[i].item_count = THREAD_PRODUCER_ITEMS;
        producer_ctx[i].last_err = 0;

        err = pthread_create(&producers[i], NULL, producer_main, &producer_ctx[i]);
        if (err != 0)
        {
            return err;
        }
    }

    for (int i = 0; i < THREAD_CONSUMER_COUNT; i++)
    {
        consumer_ctx[i].tb = active_thread_buf;
        consumer_ctx[i].worker_id = i + 1;
        consumer_ctx[i].item_count = consumer_total / THREAD_CONSUMER_COUNT;
        if (i < consumer_total % THREAD_CONSUMER_COUNT)
        {
            consumer_ctx[i].item_count++;
        }
        consumer_ctx[i].last_err = 0;

        err = pthread_create(&consumers[i], NULL, consumer_main, &consumer_ctx[i]);
        if (err != 0)
        {
            return err;
        }
    }

    for (int i = 0; i < THREAD_PRODUCER_COUNT; i++)
    {
        pthread_join(producers[i], NULL);
        if (producer_ctx[i].last_err != 0)
        {
            return producer_ctx[i].last_err;
        }
    }

    for (int i = 0; i < THREAD_CONSUMER_COUNT; i++)
    {
        pthread_join(consumers[i], NULL);
        if (consumer_ctx[i].last_err != 0)
        {
            return consumer_ctx[i].last_err;
        }
    }

    snprintf(
        status,
        status_size,
        "thread demo complete: produced %d, consumed %d, left %d",
        producer_total,
        consumer_total,
        desired_remaining
    );
    return 0;
}

static int draw_screen(const char *status, int active_type, demo_style style){
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
    mvprintw(1, 0, "mode=%s | m = switch mode, a = new array, l = new list, i = insert, r = remove, d = destroy, q = quit", demo_style_label(style));
    mvprintw(2, 0, "thread mode extra: w = run 3 producers / 2 consumers");

    if (!has_active_buffer(style))
    {
        mvprintw(4, 0, "buffer: (none)");
        mvprintw(6, 0, "Create a buffer with 'a' or 'l' to begin.");
    }
    else
    {
        err = get_buffer_state(style, &size, &capacity, &empty, &full);
        if (err != 0)
        {
            return err;
        }

        mvprintw(4, 0, "buffer: %s", buffer_type_label(active_type));
        mvprintw(5, 0, "size=%d capacity=%d empty=%d full=%d", size, capacity, empty, full);

        active_render.row = 7;
        active_render.max_rows = max_y - 2;
        active_render.hidden_count = 0;

        if (size == 0)
        {
            mvprintw(active_render.row, 0, "(empty)");
        }
        else
        {
            err = foreach_students(style, render_student);
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
    demo_style style = DEMO_STYLE_SINGLE;

    set_status(status, sizeof(status), "ready");

    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);

    int prompt_rc = prompt_demo_style("mode: n = single, t = thread: ", &style);
    if (prompt_rc == EINVAL)
    {
        style = DEMO_STYLE_SINGLE;
        set_status(status, sizeof(status), "invalid mode, defaulted to single");
    }
    else if (prompt_rc == ERR)
    {
        set_status(status, sizeof(status), "mode prompt cancelled, defaulted to single");
    }
    else
    {
        snprintf(status, sizeof(status), "selected %s mode", demo_style_label(style));
    }

    while (1)
    {
        int err = draw_screen(status, active_type, style);
        if (err != 0)
        {
            endwin();
            fprintf(stderr, "display failed: %d\n", err);
            destroy_active_buffers();
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
            case 'm':
            {
                demo_style new_style = style;
                prompt_rc = prompt_demo_style("mode: n = single, t = thread: ", &new_style);
                if (prompt_rc == ERR)
                {
                    set_status(status, sizeof(status), "mode switch cancelled");
                    break;
                }
                if (prompt_rc == EINVAL)
                {
                    set_status(status, sizeof(status), "please enter 'n' or 't'");
                    break;
                }

                err = destroy_active_buffers();
                if (err != 0)
                {
                    snprintf(status, sizeof(status), "mode switch cleanup failed: %d", err);
                    break;
                }

                style = new_style;
                snprintf(status, sizeof(status), "switched to %s mode; create a new buffer", demo_style_label(style));
                break;
            }

            case 'a':
            case 'l':
            {
                int capacity = DEFAULT_CAPACITY;
                prompt_rc = prompt_capacity("capacity: ", &capacity);
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
                err = create_active_buffer(style, active_type, capacity);
                if (err != 0)
                {
                    snprintf(status, sizeof(status), "create failed: %d", err);
                }
                else
                {
                    snprintf(
                        status,
                        sizeof(status),
                        "created %s buffer with capacity %d in %s mode",
                        buffer_type_label(active_type),
                        capacity,
                        demo_style_label(style)
                    );
                }
                break;
            }

            case 'i':
            {
                if (!has_active_buffer(style))
                {
                    set_status(status, sizeof(status), "create a buffer first");
                    break;
                }

                prompt_rc = prompt_non_empty_line("student name to add: ", name, sizeof(name));
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
                err = push_student(style, name, &added);
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

                if (!has_active_buffer(style))
                {
                    set_status(status, sizeof(status), "create a buffer first");
                    break;
                }

                err = pop_student(style, &removed);
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
                if (!has_active_buffer(style))
                {
                    set_status(status, sizeof(status), "no active buffer to destroy");
                    break;
                }

                err = destroy_active_buffers();
                if (err != 0)
                {
                    snprintf(status, sizeof(status), "destroy failed: %d", err);
                }
                else
                {
                    set_status(status, sizeof(status), "buffer destroyed");
                }
                break;

            case 'w':
                if (style != DEMO_STYLE_THREAD)
                {
                    set_status(status, sizeof(status), "switch to thread mode first");
                    break;
                }
                if (active_thread_buf == NULL)
                {
                    set_status(status, sizeof(status), "create a thread buffer first");
                    break;
                }

                set_status(status, sizeof(status), "running 3 producers / 2 consumers");
                err = draw_screen(status, active_type, style);
                if (err != 0)
                {
                    endwin();
                    fprintf(stderr, "display failed: %d\n", err);
                    destroy_active_buffers();
                    return 1;
                }

                err = run_thread_demo(status, sizeof(status));
                if (err != 0)
                {
                    snprintf(status, sizeof(status), "thread demo failed: %d", err);
                }
                break;

            case 'q':
                endwin();
                destroy_active_buffers();
                return 0;

            default:
                set_status(status, sizeof(status), "unknown command");
                break;
        }
    }
}
