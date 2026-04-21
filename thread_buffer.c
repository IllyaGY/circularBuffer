#include "thread_buffer.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct thread_buffer {
    buffer_inf *buf;
    pthread_mutex_t mutex;
    sem_t *empty;
    sem_t *full;
    int capacity;
};

static int open_unlinked_semaphore(const char *prefix, const void *token, unsigned int value, sem_t **dest){
    char name[64];

    if (prefix == NULL || dest == NULL)
    {
        return EFAULT;
    }

    int written = snprintf(
        name,
        sizeof(name),
        "/%s_%d_%llx",
        prefix,
        (int)getpid(),
        (unsigned long long)(uintptr_t)token
    );
    if (written < 0 || (size_t)written >= sizeof(name))
    {
        return ENAMETOOLONG;
    }

    sem_unlink(name);

    sem_t *sem = sem_open(name, O_CREAT | O_EXCL, 0600, value);
    if (sem == SEM_FAILED)
    {
        return errno;
    }

    if (sem_unlink(name) != 0)
    {
        int err = errno;
        sem_close(sem);
        return err;
    }

    *dest = sem;
    return 0;
}

static int wait_semaphore(sem_t *sem){
    if (sem == NULL)
    {
        return EFAULT;
    }

    while (sem_wait(sem) != 0)
    {
        if (errno == EINTR)
        {
            continue;
        }

        return errno;
    }

    return 0;
}

int create_thread_buffer(thread_buffer **dest, size_t type_size, int capacity, int type){
    if (dest == NULL)
    {
        return EFAULT;
    }

    *dest = NULL;

    thread_buffer *tb = calloc(1, sizeof(thread_buffer));
    if (tb == NULL)
    {
        return ENOMEM;
    }

    int err = create_cb(&tb->buf, type_size, capacity, type);
    if (err != 0)
    {
        free(tb);
        return err;
    }

    err = pthread_mutex_init(&tb->mutex, NULL);
    if (err != 0)
    {
        delete_cb(&tb->buf);
        free(tb);
        return err;
    }

    err = open_unlinked_semaphore("cb_empty", tb, (unsigned int)capacity, &tb->empty);
    if (err != 0)
    {
        pthread_mutex_destroy(&tb->mutex);
        delete_cb(&tb->buf);
        free(tb);
        return err;
    }

    err = open_unlinked_semaphore("cb_full", tb, 0U, &tb->full);
    if (err != 0)
    {
        sem_close(tb->empty);
        pthread_mutex_destroy(&tb->mutex);
        delete_cb(&tb->buf);
        free(tb);
        return err;
    }

    tb->capacity = capacity;
    *dest = tb;
    return 0;
}

int delete_thread_buffer(thread_buffer **dest){
    if (dest == NULL)
    {
        return EFAULT;
    }
    if (*dest == NULL)
    {
        return EFAULT;
    }

    thread_buffer *tb = *dest;
    int first_err = 0;

    if (tb->empty != NULL && sem_close(tb->empty) != 0 && first_err == 0)
    {
        first_err = errno;
    }
    if (tb->full != NULL && sem_close(tb->full) != 0 && first_err == 0)
    {
        first_err = errno;
    }

    int err = pthread_mutex_destroy(&tb->mutex);
    if (err != 0 && first_err == 0)
    {
        first_err = err;
    }

    if (tb->buf != NULL)
    {
        err = delete_cb(&tb->buf);
        if (err != 0 && first_err == 0)
        {
            first_err = err;
        }
    }

    free(tb);
    *dest = NULL;

    return first_err;
}

int push_thread_buffer(thread_buffer *tb, void *item){
    if (tb == NULL || item == NULL)
    {
        return EFAULT;
    }

    int err = wait_semaphore(tb->empty);
    if (err != 0)
    {
        return err;
    }

    err = pthread_mutex_lock(&tb->mutex);
    if (err != 0)
    {
        sem_post(tb->empty);
        return err;
    }

    err = push_value(tb->buf, item);
    int unlock_err = pthread_mutex_unlock(&tb->mutex);
    if (err != 0)
    {
        sem_post(tb->empty);
        return err;
    }
    if (unlock_err != 0)
    {
        return unlock_err;
    }
    if (sem_post(tb->full) != 0)
    {
        return errno;
    }

    return 0;
}

int pop_thread_buffer(thread_buffer *tb, void *out){
    if (tb == NULL || out == NULL)
    {
        return EFAULT;
    }

    int err = wait_semaphore(tb->full);
    if (err != 0)
    {
        return err;
    }

    err = pthread_mutex_lock(&tb->mutex);
    if (err != 0)
    {
        sem_post(tb->full);
        return err;
    }

    err = pop_value(tb->buf, out);
    int unlock_err = pthread_mutex_unlock(&tb->mutex);
    if (err != 0)
    {
        sem_post(tb->full);
        return err;
    }
    if (unlock_err != 0)
    {
        return unlock_err;
    }
    if (sem_post(tb->empty) != 0)
    {
        return errno;
    }

    return 0;
}

int foreach_thread_buffer(thread_buffer *tb, cb_iter_fn func){
    if (tb == NULL || func == NULL)
    {
        return EFAULT;
    }

    int err = pthread_mutex_lock(&tb->mutex);
    if (err != 0)
    {
        return err;
    }

    err = foreach_value(tb->buf, func);
    int unlock_err = pthread_mutex_unlock(&tb->mutex);
    if (err != 0)
    {
        return err;
    }
    if (unlock_err != 0)
    {
        return unlock_err;
    }

    return 0;
}

int get_size_thread_buffer(thread_buffer *tb, int *size){
    if (tb == NULL || size == NULL)
    {
        return EFAULT;
    }

    int err = pthread_mutex_lock(&tb->mutex);
    if (err != 0)
    {
        return err;
    }

    err = get_size(tb->buf, size);
    int unlock_err = pthread_mutex_unlock(&tb->mutex);
    if (err != 0)
    {
        return err;
    }
    if (unlock_err != 0)
    {
        return unlock_err;
    }

    return 0;
}

int get_capacity_thread_buffer(thread_buffer *tb, int *capacity){
    if (tb == NULL || capacity == NULL)
    {
        return EFAULT;
    }

    int err = pthread_mutex_lock(&tb->mutex);
    if (err != 0)
    {
        return err;
    }

    err = get_capacity(tb->buf, capacity);
    int unlock_err = pthread_mutex_unlock(&tb->mutex);
    if (err != 0)
    {
        return err;
    }
    if (unlock_err != 0)
    {
        return unlock_err;
    }

    return 0;
}

int is_empty_thread_buffer(thread_buffer *tb, int *result){
    if (tb == NULL || result == NULL)
    {
        return EFAULT;
    }

    int err = pthread_mutex_lock(&tb->mutex);
    if (err != 0)
    {
        return err;
    }

    err = is_empty(tb->buf, result);
    int unlock_err = pthread_mutex_unlock(&tb->mutex);
    if (err != 0)
    {
        return err;
    }
    if (unlock_err != 0)
    {
        return unlock_err;
    }

    return 0;
}

int is_full_thread_buffer(thread_buffer *tb, int *result){
    if (tb == NULL || result == NULL)
    {
        return EFAULT;
    }

    int err = pthread_mutex_lock(&tb->mutex);
    if (err != 0)
    {
        return err;
    }

    err = is_full(tb->buf, result);
    int unlock_err = pthread_mutex_unlock(&tb->mutex);
    if (err != 0)
    {
        return err;
    }
    if (unlock_err != 0)
    {
        return unlock_err;
    }

    return 0;
}
