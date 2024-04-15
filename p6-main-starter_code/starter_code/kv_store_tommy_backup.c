#include "common.h"
#include "ring_buffer.h"
#include <pthread.h>

#define BUCKETS (101)

typedef struct __node_t {
    key_type key;
    value_type value;
    struct __node_t *next;
} node_t;

typedef struct __list_t {
    node_t *head;
    pthread_mutex_t lock;
} list_t;

// Initialize a list
void List_Init(list_t *L) {
    L->head = NULL;
    pthread_mutex_init(&L->lock, NULL);
}

// Insert a key-value pair into the list, return 1 on success, 0 otherwise
int List_Insert(list_t *L, key_type key, value_type value) {
    pthread_mutex_lock(&L->lock);
    // Check if the key already exists, update value if it does
    node_t *current = L->head;
    while (current) {
        if (current->key == key) {
            current->value = value; // Update value
            pthread_mutex_unlock(&L->lock);
            return 1;
        }
        current = current->next;
    }
    // If not, create a new node and add it to the list
    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node) {
        pthread_mutex_unlock(&L->lock);
        return 0; // Return 0 if malloc fails
    }
    new_node->key = key;
    new_node->value = value;
    new_node->next = L->head;
    L->head = new_node;
    pthread_mutex_unlock(&L->lock);
    return 1;
}

// Lookup a key in the list, if found set value and return 1, otherwise return 0
int List_Lookup(list_t *L, key_type key, value_type *value) {
    pthread_mutex_lock(&L->lock);
    node_t *current = L->head;
    while (current) {
        if (current->key == key) {
            *value = current->value;
            pthread_mutex_unlock(&L->lock);
            return 1;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&L->lock);
    return 0; // Return 0 if not found
}

// Free all elements of the list
void List_Free(list_t *L) {
    pthread_mutex_lock(&L->lock);
    node_t *current = L->head;
    node_t *temp;

    while (current != NULL) {
        temp = current;
        current = current->next;
        free(temp);
    }

    L->head = NULL; // After freeing the nodes, set the head to NULL.
    pthread_mutex_unlock(&L->lock);
    pthread_mutex_destroy(&L->lock); // Destroy the lock after we're done with it.
}


typedef struct __hash_t {
    list_t lists[BUCKETS];
} hash_t;

hash_t kv_store;

// Initialize the hash table.
void Hash_Init(hash_t *H) {
    for (int i = 0; i < BUCKETS; i++) {
        List_Init(&H->lists[i]);
    }
}

// Insert a key-value pair into the hash table.
int Hash_Insert(hash_t *H, key_type key, value_type v) {
    int bucket = key % BUCKETS;
    return List_Insert(&H->lists[bucket], key, v);
}

// Lookup a key in the hash table and retrieve the value.
int Hash_Lookup(hash_t *H, key_type key, value_type* v) {
    int bucket = key % BUCKETS;
    return List_Lookup(&H->lists[bucket], key, v);
}

void put(key_type k, value_type v) {
    // Call the Hash_Insert function to insert the key-value pair.
    Hash_Insert(&kv_store, k, v);
}

value_type get(key_type k) {
    value_type v = 0;
    // Call the Hash_Lookup function to retrieve the value for the key.
    if (Hash_Lookup(&kv_store, k, &v)) {
        return v; // Key found, return the value.
    }
    return 0; // Key not found, return 0.
}

// Define and initialize mutexes or other synchronization primitives.

// The function to be executed by the server threads.
void* server_thread_func(void* arg) {
    // Loop to process requests from the Ring Buffer.
    while (true) {
        // Fetch a request.
        // Process the request using 'put' or 'get'.
        // Write the result to the Request-status Board.
        // Set the 'ready' flag.
    }
    return NULL;
}

// The server main function.
int main(int argc, char* argv[]) {
    // Parse command-line arguments.
    // int num_threads = /* parsed -n argument */;
    // int table_size = /* parsed -s argument */;
    
    // // Initialize shared memory and the Ring Buffer.
    
    // // Start server threads.
    // pthread_t threads[num_threads];
    // for (int i = 0; i < num_threads; ++i) {
    //     pthread_create(&threads[i], NULL, server_thread_func, NULL);
    // }
    
    // // Join threads before exit.
    // for (int i = 0; i < num_threads; ++i) {
    //     pthread_join(threads[i], NULL);
    // }
    
    // Clean-up and shutdown logic.
    return 0;
}
