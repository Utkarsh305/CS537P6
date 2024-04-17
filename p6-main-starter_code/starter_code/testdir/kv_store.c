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

typedef struct node_t{

    int k;
    int v;

    struct node_t * next;
} node;

typedef struct list_t{
    node * head;
    pthread_mutex_t lock;
    int size;
}list;

typedef struct hashtable_t{
    int size;
    list * lists;
}hashtable;

void List_Init(list *L){
    L -> head = NULL;
    pthread_mutex_init(&L->lock, NULL);
}

int List_Insert(list *L, int key, int val) {
    pthread_mutex_lock(&L->lock);

    node *new_node = malloc(sizeof(node));
    if (new_node == NULL) {
        pthread_mutex_unlock(&L->lock);
        printf("Error: Unable to allocate memory for new node\n");
        return -1; // Indicate failure
    }

    new_node->k = key;
    new_node->v = val;
    new_node->next = L->head;
    L->head = new_node;

    L->size++;

    pthread_mutex_unlock(&L->lock);
    return 0; // Success
}


node *List_Lookup(list *L, int key) {
    pthread_mutex_lock(&L->lock);  // Locking for thread safety
    node *curr = L->head;
    while (curr != NULL) {
        if (curr->k == key) {
            break;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&L->lock);  // Unlocking after operation
    return curr;
}


int Hash_Init(hashtable *H, int size){
    H -> size = size;
    H -> lists = malloc(sizeof(list)*size);

    if(H -> lists == NULL){
        printf("Can't alloclate lists for bucket");
        return -1;
    }

    for(int i = 0; i < size; i++){
        List_Init(&(H -> lists[i]));
    }
}

int put(hashtable *H,  int k, int v){
    int bucket = hash_function(k, H->size);
    return List_Insert(&(H -> lists[bucket]), k, v);
}

node * get(hashtable *H,  int k){
    int bucket = hash_function(k, H->size);
    return List_Lookup(&(H -> lists[bucket]), k);
}

void main_loop(hashtable *H) {
    while(1) {
        struct buffer_descriptor bd;
        ring_get(ring, &bd); // Safely fetch a request from the ring buffer

        if (bd.req_type == PUT) {
            put(H, bd.k, bd.v);
        } else if (bd.req_type == GET) {
            node* result_node = get(H, bd.k);
            if (result_node != NULL) {
                bd.v = result_node->v;
            } else {
                bd.v = -1; // Or some other sentinel value indicating 'not found'
            }
        }

        struct buffer_descriptor *results = (struct buffer_descriptor *)(mem_start + bd.res_off);
        memcpy(results, &bd, sizeof(struct buffer_descriptor));
        __sync_synchronize(); // Memory barrier to ensure all writes are done
        results->ready = 1; // Signal that the request has been processed
    }
}


int main(int argc, char * argv[]){
    hashtable *H = malloc(sizeof(hashtable));

    if(H == NULL){
        printf("Hashtable not initlaized");
        return -1;
    }

    int num_threads = 0;
    int hash_size = 0;

    printf("I work until here 1(Malloc H)\n");

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

    printf("I work until here 2(Got arguments)\n");

    Hash_Init(H, hash_size);

    printf("I work until here 3(Hash Init)\n");

    int shm_size = 0;
    int fd = open(shm_file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd < 0){
        printf("Can't open file");
        return -1;
    }

    // struct stat file_status;
    // if (stat((char *)fd, &file_status) < 0) {
    //     return -1;
    // }
    //shm_size = file_status.st_size;
    shm_size = lseek(fd, 0, SEEK_END);

    char *mem = mmap(NULL, shm_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

    printf("I work until here 4(mmap)\n");
    //printf("I work until here %i, %i\n", num_threads, hash_size);

    if (mem == (void *)-1) {
        printf("Can't mmap");
        close(fd);
        return -1;
    }

    mem_start = mem;

    ring = (struct ring *)mem;

    close(fd);

    pthread_t thread_list[num_threads];

    for (int i = 0; i < num_threads; i++) {
        //pthread_t tid;
        if (pthread_create(&thread_list[i], NULL, (void *(*)(void *))main_loop, H) != 0) {
            printf("Can't pthread create");
            return -1;
        }
    }

    printf("I work until here 5(made threads)\n");

    for(int i = 0; i < num_threads; i++){
        printf("thread %i\n", i);
        pthread_join(thread_list[i], NULL);
    }

    printf("I work until here 6(join threads)\n");

    // Wait for threads to finish
    while (1) {
        sched_yield();
    }


	   

    return 0;
}