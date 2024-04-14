#include "ring_buffer.h"
#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
 * Initialize the ring
 * @param r A pointer to the ring
 * @return 0 on success, negative otherwise - this negative value will be
 * printed to output by the client program
*/
int init_ring(struct ring *r) {
    r->p_head = 0;
    r->p_tail = 0;
    r->c_head = 0;
    r->c_tail = 0;
    memset(r->buffer, 0, sizeof(struct buffer_descriptor) * RING_SIZE);
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

    int producerHead = atomic_fetch_add(&r->p_head, 1);
    int realProducerHead = producerHead % RING_SIZE;

    while(producerHead - r->c_tail >= RING_SIZE) {
        sleep(0);
    } 
    while(r->buffer[realProducerHead].ready != 0) {
        sleep(0);
    }
    r->buffer[realProducerHead] = *bd;
    r->buffer[realProducerHead].ready = 1;
 
    // increment the p_tail
    int temp = producerHead;
    while(r->buffer[producerHead%RING_SIZE].ready == 1 && atomic_compare_exchange_strong(&r->p_tail, &temp, producerHead + 1) == 1) {
        r->buffer[producerHead%RING_SIZE].ready = 2;
        producerHead++;
        temp = producerHead;
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
    
        int consumerHead = atomic_fetch_add(&r->c_head, 1);
        int realConsumerHead = consumerHead % RING_SIZE;
    
        while(consumerHead >= r->p_tail) {
            sleep(0);
        }

        while(r->buffer[realConsumerHead].ready != 2) {
            sleep(0);
        }
        *bd = r->buffer[realConsumerHead];
        r->buffer[realConsumerHead].ready = 3;

        // increment the c_tail
        int temp = consumerHead;
        while(r->buffer[consumerHead%RING_SIZE].ready == 3 && atomic_compare_exchange_strong(&r->c_tail, &temp, consumerHead + 1) == 1) {
            r->buffer[consumerHead%RING_SIZE].ready = 0;
            consumerHead++;
            temp = consumerHead;
        }
}
