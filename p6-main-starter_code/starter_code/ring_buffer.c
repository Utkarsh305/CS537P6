#include "ring_buffer.h"
#include <stdatomic.h>

/*
 * Initialize the ring
 * @param r A pointer to the ring
 * @return 0 on success, negative otherwise - this negative value will be
 * printed to output by the client program
*/
int init_ring(struct ring *r) {
    r->writer_index = 0;
    r->reader_index = 0;
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

    int falseWriteIndex = atomic_fetch_add(&r->writer_index, 1);
    int realWriteIndex = falseWriteIndex % RING_SIZE;

    int readIndex;
    do {
        readIndex = atomic_load(&r->reader_index);
        // Also, maybe sleep/yield here, but seems unnecessary
    } while(falseWriteIndex - readIndex >= RING_SIZE); // TODO: check case for when the falseWriteIndex and/or readIndex have overflowed
    r->buffer[realWriteIndex] = *bd;
    r->buffer[realWriteIndex].ready = 1;

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
    
        int falseReadIndex = atomic_fetch_add(&r->reader_index, 1);
        int realReadIndex = falseReadIndex % RING_SIZE;
    
        int writeIndex;
        do {
            writeIndex = atomic_load(&r->writer_index);
        } while(writeIndex == falseReadIndex);

        while(r->buffer[realReadIndex].ready == 0) {
            // maybe yield/sleep here
        }
        *bd = r->buffer[realReadIndex];
}
