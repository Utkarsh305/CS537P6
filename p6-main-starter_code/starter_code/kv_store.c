#include "ring_buffer.h"
#include "common.h"

struct ring *ring = NULL;

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

void get(struct hashtable *H, int k) {
    //TODO
    int bucket = hash_function(k, H->size);
    List_Lookup(&H->lists[bucket], k);
}

int main(int argc, char * argv[]){
    //TODO
    return 0;
}