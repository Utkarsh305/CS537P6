#include "ring_buffer.h"
#include <stdatomic.h>

/*
 * Initialize the ring
 * @param r A pointer to the ring
 * @return 0 on success, negative otherwise - this negative value will be
 * printed to output by the client program
*/
int init_ring(struct ring *r) {
    r->writer_head = 0;
    r->writer_tail = 0;
    r->reader_head = 0;
    r->reader_tail = 0;
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

    int falseWriteHead = atomic_fetch_add(&r->writer_head, 1);
    int realWriteHead = falseWriteHead % RING_SIZE;

    int readIndexTail;
    do {
        readIndexTail = atomic_load(&r->reader_tail);
        // Also, maybe sleep/yield here, but seems unnecessary
    } while(falseWriteHead - readIndexTail >= RING_SIZE); // TODO: check case for when the falseWriteIndex and/or readIndex have overflowed
    r->buffer[realWriteHead] = *bd;
    r->buffer[realWriteHead].ready = 1;

    // increment the writer_tail
    while(atomic_compare_exchange_strong(&r->writer_tail, &falseWriteHead, falseWriteHead + 1) == 0) {
        falseWriteHead++;
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
    
        int falseReadIndex = atomic_fetch_add(&r->reader_head, 1);
        int realReadIndex = falseReadIndex % RING_SIZE;
    
        int writeIndexTail;
        do {
            writeIndexTail = atomic_load(&r->writer_tail);
        } while(writeIndexTail == falseReadIndex);

        while(r->buffer[realReadIndex].ready == 0) {
            // maybe yield/sleep here
        }
        *bd = r->buffer[realReadIndex];
        r->buffer[realReadIndex].ready = 0;

        // increment the reader_tail
        while(atomic_compare_exchange_strong(&r->reader_tail, &falseReadIndex, falseReadIndex + 1) == 0) {
            falseReadIndex++;
        }
}
