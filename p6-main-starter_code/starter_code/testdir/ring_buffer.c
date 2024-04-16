#include "ring_buffer.h"
#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>

pthread_cond_t not_full;
pthread_cond_t not_empty;
pthread_mutex_t mutex;

/*
 * Initialize the ring
 * @param r A pointer to the ring
 * @return 0 on success, negative otherwise - this negative value will be
 * printed to output by the client program
*/
int init_ring(struct ring *r) {
    memset(r, 0, sizeof(struct ring));
    r->p_head = 0;
    r->p_tail = 0;
    r->c_head = 0;
    r->c_tail = 0;

    memset(r, 0, sizeof(struct ring));
    pthread_cond_init(&not_full, NULL);
    pthread_cond_init(&not_empty, NULL);
    pthread_mutex_init(&mutex, NULL);
    return 0;
}

void destroy_ring(struct ring *r) {
    pthread_mutex_destroy(&r->mutex);
    pthread_cond_destroy(&r->not_full);
    pthread_cond_destroy(&r->not_empty);
}


/*
 * Submit a new item - should be thread-safe
 * This call will block the calling thread if there's not enough space
 * @param r The shared ring
 * @param bd A pointer to a valid buffer_descriptor - This pointer is only
 * guaranteed to be valid during the invocation of the function
*/
void ring_submit(struct ring *r, struct buffer_descriptor *bd) {
    pthread_mutex_lock(&r->mutex);

    int index = atomic_fetch_add(&r->p_head, 1) % RING_SIZE;

    // Wait while the ring is full
    while ((r->p_head - r->c_tail) >= RING_SIZE) {
        pthread_cond_wait(&r->not_full, &r->mutex);
    }

    // Make sure this spot is free to use
    while (r->buffer[index].ready != 0) {
        pthread_cond_wait(&r->not_full, &r->mutex);
    }

    // Copy data into the ring buffer
    memcpy(&r->buffer[index], bd, sizeof(struct buffer_descriptor));
    r->buffer[index].ready = 1;

    // Update the producer tail
    r->p_tail = r->p_head;

    // Signal to consumers that there is new data
    pthread_cond_signal(&r->not_empty);

    pthread_mutex_unlock(&r->mutex);
}


/*
 * Get an item from the ring - should be thread-safe
 * This call will block the calling thread if the ring is empty
 * @param r A pointer to the shared ring 
 * @param bd pointer to a valid buffer_descriptor to copy the data to
 * Note: This function is not used in the clinet program, so you can change
 * the signature.
*/
void ring_get(struct ring *r, struct buffer_descriptor *bd) {
    pthread_mutex_lock(&r->mutex);

    int index = atomic_fetch_add(&r->c_head, 1) % RING_SIZE;

    // Wait while the buffer is empty
    while (r->c_head >= r->p_tail) {
        pthread_cond_wait(&r->not_empty, &r->mutex);
    }

    // Wait for data to be ready to consume
    while (r->buffer[index].ready != 1) {
        pthread_cond_wait(&r->not_empty, &r->mutex);
    }

    // Copy data out of the buffer
    memcpy(bd, &r->buffer[index], sizeof(struct buffer_descriptor));
    r->buffer[index].ready = 0; // Reset ready status

    // Update consumer tail
    r->c_tail = r->c_head;

    // Signal to producers that there is now space
    pthread_cond_signal(&r->not_full);

    pthread_mutex_unlock(&r->mutex);
}

