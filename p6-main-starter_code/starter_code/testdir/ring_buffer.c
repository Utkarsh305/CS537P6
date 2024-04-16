#include "ring_buffer.h"
#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>

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
    return 0;
}

/*
 * Submit a new item - should be thread-safe
 * This call will block the calling thread if there's not enough space
 * @param r The shared ring
 * @param bd A pointer to a valid buffer_descriptor - This pointer is only
 * guaranteed to be valid during the invocation of the function
*/
void ring_submit(struct ring *r, struct buffer_descriptor *bd) {

    int index = atomic_fetch_add(&r->p_head, 1);
    int realIndex = index % RING_SIZE;

    while(index - r->c_tail >= RING_SIZE) {
        // buffer is full
        sleep(0);
    } 
    
    while(r->buffer[realIndex].ready != 0) {
        // this spot still has data which was not consumed
        sleep(0);
    }

    bd->ready = 0;
    r->buffer[realIndex] = *bd;
    r->buffer[realIndex].ready = 1;
 
    // increment the p_tail

/*
    int temp = index;
    while(atomic_compare_exchange_strong(&r->p_tail, &temp, index + 1) == 0) {
        sleep(0);
        temp = index;
    }
    r->buffer[realIndex].ready = 2;
    */
    
    int temp = index;

    while(r->buffer[index % RING_SIZE].ready == 1 && atomic_compare_exchange_strong(&r->p_tail, &temp, index + 1) == 1) {
        if(r->p_tail > r->p_head) {
            printf("Error 2: p_tail > p_head\n");
        }
        r->buffer[index % RING_SIZE].ready = 2;
        index++;
        temp = index;
    }
    
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

    
    int index = atomic_fetch_add(&r->c_head, 1);
    int realIndex = index % RING_SIZE;
    
    
    while(index >= r->p_tail) {
        // buffer is empty
        sleep(0);
    }
    

    while(r->buffer[realIndex].ready != 2) {
        // this spot doesn't contain valid data ready to consume yet
        sleep(0);
    }
    *bd = r->buffer[realIndex];
    r->buffer[realIndex].ready = 3;

    // increment the c_tail

/*
    int temp = index;
    while(atomic_compare_exchange_strong(&r->c_tail, &temp, index + 1) == 0) {
        sleep(0);
        temp = index;
    }
    r->buffer[realIndex].ready = 0;
    */


    
    int temp = index;
    while(r->buffer[index % RING_SIZE].ready == 3 && atomic_compare_exchange_strong(&r->c_tail, &temp, index + 1) == 1) {

        
        if(r->c_tail > r->c_head) {
            printf("Error 2: c_tail > c_head\n");
        }
        r->buffer[index % RING_SIZE].ready = 0;
        index++;
        temp = index;
    }
    
}
