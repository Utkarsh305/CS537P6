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

char* mem_start = NULL;
struct ring *ring = NULL;
char shm_file[] = "shmem_file";
int win_size = 1;
struct buffer_descriptor *results;

struct node {
    int value;
    struct node *next;
};

struct list {
    struct node *head;
    pthread_mutex_t lock;
} list;

struct hashtable {
    int size;
    struct list * lists;
};


void List_Init(struct list *L) {
    L->head = NULL;
    pthread_mutex_init(&L->lock, NULL);
}

void List_Insert(struct list *L, int key) {
    struct node *n = malloc(sizeof(struct node));
    if (n == NULL) {
    perror("malloc");
    return;
    }
    n->value = key;
    pthread_mutex_lock(&L->lock);
    n->next = L->head;
    L->head = n;
    pthread_mutex_unlock(&L->lock);
    return;
}

int List_Lookup(struct list *L, int key) {
    int rv = -1;
    pthread_mutex_lock(&L->lock);
    struct node *curr = L->head;
    while(curr) {
    if (curr->value == key) {
    rv = 0;
    break;
    }
    curr = curr->next;
    }
    pthread_mutex_unlock(&L->lock);
    return rv;
}

void Hash_Init(struct hashtable *H, int size) {
    H->size = size;
    H->lists = malloc(sizeof(list)  * size);
    int i=0;
    for(i=0;i<size;i++) {
        List_Init(&H->lists[i]);
    }
}

void put(struct hashtable *H,  int k, int v) {
    //TODO
    int bucket = hash_function(k, H->size);
    List_Insert(&H->lists[bucket],v);
}

int get(struct hashtable *H, int k) {
    //TODO
    int bucket = hash_function(k, H->size);
   return List_Lookup(&H->lists[bucket], k);
}

void server_main_loop(struct hashtable *H) {
    while (1) {
        struct buffer_descriptor bd;
        ring_get(ring, &bd);
        if (bd.req_type == PUT) {
            put(H, bd.k, bd.v);
            bd.ready = 1;
        } else if (bd.req_type == GET) {
            int value = get(H, bd.k);
            bd.v = value;
            bd.ready = 0;
        }
        ring_submit(ring, &bd);
    }
}

int main(int argc, char * argv[]){
    //TODO
    if(argc != 5){
        printf("Not enough arguments");
        exit(1);
    }

    struct hashtable *H = malloc(sizeof(struct hashtable));

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

    //printf("%i\n", num_threads);
    //printf("%i\n", hash_size);

    Hash_Init(H, hash_size);

    int shm_size = sizeof(struct ring) + num_threads * win_size * sizeof(struct buffer_descriptor);
    int fd = open(shm_file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd < 0)
		perror("open");
    
    char *mem = mmap(NULL, shm_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (mem == (void *)-1) 
		perror("mmap");
    
    mem_start = mem;

    close(fd);



    //TODO
    ring = (struct ring *)mem;
    init_ring(ring);
    results = (struct buffer_descriptor *)(mem + sizeof(struct ring));

    // Launch worker threads
    for (int i = 0; i < num_threads; i++) {
        pthread_t tid;
        if (pthread_create(&tid, NULL, (void *(*)(void *))server_main_loop, H) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    // Wait for threads to finish
    while (1) {
        sleep(1);
    }


    return 0;
}