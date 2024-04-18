#include "ring_buffer.h"
#include "common.h"
#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <string.h>

char shm_file[] = "shmem_file";

struct node {
    value_type value;
    key_type key;
    struct node *next;
};

struct list {
    struct node *head;
    pthread_mutex_t lock;
} list;

struct hashtable {
    int size;
    struct list *lists;
};

struct server_thread_args {
    struct hashtable *hashtable;
    void *memStart;
};


void List_Init(struct list *L) {
    L->head = NULL;
    pthread_mutex_init(&L->lock, NULL);
}

// NOTE: Not thread safe
void List_Insert(struct list *L, key_type key, value_type value) {
    struct node *n = malloc(sizeof(struct node));
    if (n == NULL) {
        perror("malloc");
        return;
    }
    n->key = key;
    n->value = value;

    // pthread_mutex_lock(&L->lock);
    n->next = L->head;
    L->head = n;
    // pthread_mutex_unlock(&L->lock);
    return;
}

value_type List_Lookup(struct list *L, key_type key) {
    value_type val = 0;
    // pthread_mutex_lock(&L->lock);
    struct node *curr = L->head;
    while(curr) {
        if (curr->key == key) {
            val = curr->value;
            break;
        }
        curr = curr->next;
    }
    // pthread_mutex_unlock(&L->lock);
    return val;
}

void Hash_Init(struct hashtable *H, int size) {
    H->size = size;
    H->lists = malloc(sizeof(list) * size);
    for(int i=0; i<size; i++) {
        List_Init(&H->lists[i]);
    }
}

// THREAD SAFE
void put(struct hashtable *H,  key_type k, value_type v) {
    //TODO
    index_t bucket = hash_function(k, H->size);

    struct list *list = &H->lists[bucket];

    pthread_mutex_lock(&list->lock);
    struct node *curr = list->head;
    while(curr) {
        if (curr->key == k) {
            curr->value = v;
            goto finish;
        }
        curr = curr->next;
    }
    List_Insert(list, k, v);

    finish:
    pthread_mutex_unlock(&list->lock);
}

int get(struct hashtable *H, key_type k) {
    //TODO
    index_t bucket = hash_function(k, H->size);
    return List_Lookup(&H->lists[bucket], k);
}

void server_main_loop(struct server_thread_args *args) {

    struct ring *ring = (struct ring *)args->memStart;
    struct hashtable *hashtable = args->hashtable;
    void *memStart = args->memStart;
    while (1) {
        struct buffer_descriptor bd;
        ring_get(ring, &bd);
        int offset = bd.res_off;
        if (bd.req_type == PUT) {
            put(hashtable, bd.k, bd.v);
        } else if (bd.req_type == GET) {
            bd.v = get(hashtable, bd.k);
        }
        bd.ready = 0;
        struct buffer_descriptor *result = (struct buffer_descriptor *)(memStart + offset);
        *result = bd;
        result->ready = 1;
    }
}

int main(int argc, char * argv[]){
    //TODO
    if(argc != 5){
        printf("Not enough arguments");
        exit(1);
    }

    int num_threads = 0;
    int hash_size = 0;

    int op;
    while ((op = getopt(argc, argv, "n:s:")) != -1) {
        switch (op) {
            case 'n':
            num_threads = atoi(optarg);
            break;

            case 's':
		    hash_size = atoi(optarg);
		    break;
        
        }
    }

    // printf("%i\n", num_threads);
    // printf("%i\n", hash_size);


    // int shm_size = sizeof(struct ring) + num_threads * win_size * sizeof(struct buffer_descriptor);
    printf("Opening shared region\n");
    int fd = open(shm_file, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd < 0)
		perror("open");

    printf("Shared region opened\n");
    struct stat buf;
    if(fstat(fd, &buf) == -1) {
        printf("Failed to fstat\n");
        // exit(0);
    }
    int size = buf.st_size;

    char *mem = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (mem == (void *)-1) 
		perror("mmap");

    printf("Mmap succesful\n");
    close(fd);
    
    
    // initialize necessary values for server threads
    struct server_thread_args args;

    args.hashtable = malloc(sizeof(struct hashtable));
    Hash_Init(args.hashtable, hash_size);

    args.memStart = mem;

    pthread_t workers[num_threads];
    // Launch worker threads
    printf("Creating Thread\n");
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&workers[i], NULL, (void *(*)(void *))server_main_loop, &args) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    printf("Finished creating threads\n");

    // Wait for threads to finish
    for(int i = 0; i < num_threads; i++) {
        if (pthread_join(workers[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }

    free(args.hashtable);
    close(fd);
    return 0;
}
