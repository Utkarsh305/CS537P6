#include "ring_buffer.h"
#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>

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

    int falseWriteHead = atomic_fetch_add(&r->p_head, 1);
    int realWriteHead = falseWriteHead % RING_SIZE;

    while(falseWriteHead - r->c_tail >= RING_SIZE) {
        sleep(0);
    } 
    r->buffer[realWriteHead] = *bd;
    r->buffer[realWriteHead].ready = 1;

    // increment the writer_tail
    int temp = falseWriteHead;
    while(r->buffer[falseWriteHead%RING_SIZE].ready == 1 && atomic_compare_exchange_strong(&r->p_tail, &temp, falseWriteHead + 1) == 1) {
        falseWriteHead++;
        temp = falseWriteHead;
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
    
        int falseReadIndex = atomic_fetch_add(&r->c_head, 1);
        int realReadIndex = falseReadIndex % RING_SIZE;
    
        while(falseReadIndex >= r->p_tail) {
            sleep(0);
        }

        while(r->buffer[realReadIndex].ready != 1) {
            sleep(0);
        }
        *bd = r->buffer[realReadIndex];
        r->buffer[realReadIndex].ready = 2;

        // increment the reader_tail
        int temp = falseReadIndex;
        while(r->buffer[falseReadIndex%RING_SIZE].ready == 2 && atomic_compare_exchange_strong(&r->c_tail, &temp, falseReadIndex + 1) == 1) {
            r->buffer[falseReadIndex%RING_SIZE].ready = 0;
            falseReadIndex++;
            temp = falseReadIndex;
        }
}
