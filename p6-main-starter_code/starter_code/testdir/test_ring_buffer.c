#include <stdio.h>
#include <stdlib.h>
#include "ring_buffer.h"
#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>

double randDouble() {
    return (double)rand() / (double)RAND_MAX;
}

void createVal(struct buffer_descriptor *bd , int value) {
   //  printf("Creating %d\n", value);
    // bd->req_type = PUT;
    // bd->k = 0;
    bd->v = value;
    // bd->res_off = 0;
    bd->ready = 0;
}


void printRing(struct ring *r) {
    printf("Ring: ");
    printf("Writer Head: %d, Writer Tail: %d, Reader Head: %d, Reader Tail: %d\n", r->p_head, r->p_tail, r->c_head, r->c_tail);
    for(int i = 0; i < RING_SIZE; i++) {
        printf("%d ", r->buffer[i].ready);
    }
    printf("\n");
}

void test1() {
    printf("Ring Buffer Test 1:\n");

    struct ring r;
    init_ring(&r);

    struct buffer_descriptor bd;
    for(int i = 0; i < 10; i++) {
        createVal(&bd, i);
        ring_submit(&r, &bd);
    }

    // printRing(&r);

    for(int i = 0; i < 10; i++) {
        struct buffer_descriptor bd;
        ring_get(&r, &bd);
        if(bd.v != i){
            printf("Error: Expected %d, but got %d\n", i, bd.v);
        }
    }

    printf("Ring Buffer Test 1 Finished\n");

}

struct worker_args {
    struct ring *r;
    int num;
    int *increment;
    int id;
};

void producer(struct worker_args *args) {
    struct buffer_descriptor bd;
    for(int i = 0; i < args->num; i++) {
        createVal(&bd, i);
        ring_submit(args->r, &bd);
        // printf("Producer %d: %d\n", args->id, bd.v);
        atomic_fetch_add(args->increment + bd.v, 1);

        if(randDouble() < 0.1) {
            sleep(0);
        }
    }
}

void consumer(struct worker_args *args) {
    for(int i = 0; i < args->num; i++) {
        struct buffer_descriptor bd;
        ring_get(args->r, &bd);
        // printf("Consumer %d %d: %d\n", args->id, i, bd.v);
        atomic_fetch_add(args->increment + bd.v, 1);

        if(randDouble() < 0.1) {
            sleep(0);
        }
    }
}

struct ring r;

int test2() {
   // printf("Ring Buffer Test 2:\n");

    init_ring(&r);

    #define n 100
    #define to_produce 200000

    pthread_t producers[n];
    struct worker_args producer_args[n];
    int prodIncrement[to_produce] = {0};


    pthread_t consumers[n];
    struct worker_args consumer_args[n];
    int consIncrement[to_produce] = {0};

    for(int i = 0; i < n; i++) {
        consumer_args[i].r = &r;
        consumer_args[i].num = to_produce;
        consumer_args[i].id = i;
        consumer_args[i].increment = consIncrement;
        pthread_create(&consumers[i], NULL, (void * (*)(void *))consumer, &consumer_args[i]);
    
        producer_args[i].r = &r;
        producer_args[i].num = to_produce;
        producer_args[i].id = i;
        producer_args[i].increment = prodIncrement;
        pthread_create(&producers[i], NULL, (void * (*)(void *))producer, &producer_args[i]);
    
    }
    // sleep(2);
   // printRing(&r);
  
    for(int i = 0; i < n; i++) {
        pthread_join(producers[i], NULL);
    }
    for(int i = 0; i < n; i++) {
        pthread_join(consumers[i], NULL);
    }

   //  printf("Checking Results\n");
    for(int i = 0; i < to_produce; i++) {
        if(prodIncrement[i] != n) {
            printf("Error: Expected %d, but got %d\n", n, prodIncrement[i]);
        }
        if(consIncrement[i] != n) {
            printf("Error: Expected %d, but got %d\n", n, consIncrement[i]);
        }
    }
   // printf("Ring Buffer Test 2 Finished\n");

    return 0;
}



int main() {
    // test1();
    for(int i = 0; i < 1000; i++) { 
        printf("Running test 2 #%d\n", i);
        test2();
    }
    return 0;
}
