#ifndef HTABLE_H
#define HTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define CAPACITY 5000000 // Size of the Hash Table

typedef struct Node Node;
typedef struct HashTable HashTable;
typedef struct Count COUNT;
unsigned long hash_function (const char *s_);

Node* create_node(char* key, int value);
HashTable* create_htable(int size);

void free_node(Node* node);
void free_htable(HashTable* table);

Node* ht_insert(HashTable* table, char* key, int value);
Node* ht_search(HashTable* table, char* key);
void ht_print(HashTable *h);

void reduce(HashTable **hash_tables, HashTable *final_table, int num_hashtables, int location);
void migrate(COUNT* words, HashTable* final_table, int h_start, int h_end, int c_start, int c_end);

struct Node {
    char *key;
    // int *value;
    int value;
    Node *next;
};

// Define the Hash Table here
struct HashTable {
    // Contains an array of pointers to items
    Node** nodes;
    int* node_count;
    int size;
    int count;
};

struct Count{
    char* word;
    int count;
};
//////////////////////////////////////////////// HASH TABLE ////////////////////////////////////////////////

/* Fowler-Noll-Vo hash constants, for 32-bit word sizes. */
#define FNV_32_PRIME 16777619u
#define FNV_32_BASIS 2166136261u

/* Returns a hash of string S. */
unsigned long hash_function (const char *s_) 
{
  const unsigned char *s = (const unsigned char *) s_;
  unsigned hash;

  hash = FNV_32_BASIS;
  while (*s != '\0')
    hash = (hash * FNV_32_PRIME) ^ *s++;

  return hash % CAPACITY;
}

Node* create_node(char* key, int value) {
    // Creates a pointer to a new hash table item
    Node* node = (Node*) malloc (sizeof(Node));
    node->key = (char*) malloc (strlen(key) + 1);
    // node->value = (int*) malloc (sizeof(int));
    node->next = NULL;
    
    strcpy(node->key, key);
    // *node->value = value;
    node->value = value;

    return node;
}

HashTable* create_htable(int size) {
    // Creates a new HashTable
    HashTable* table = (HashTable*) malloc (sizeof(HashTable));
    table->size = size;
    table->count = 0;
    table->nodes = (Node**) calloc (table->size, sizeof(Node*));
    table->node_count = (int *) calloc (table->size, sizeof(int));
    int i;
    for (i=0; i < table->size; i++)
        table->nodes[i] = NULL;

    return table;
}

void free_node(Node* node) {
    // Frees an item
    free(node->key);
    free(node);
}

void free_htable(HashTable* table) {
    // Frees the table
    Node* tmp = NULL;
    int i;

    for (i=0; i < table->size; i++){
        while (table->nodes[i] != NULL){
            tmp = table->nodes[i];
            table->nodes[i] = table->nodes[i]->next;
            free_node(tmp);
        }
    }

    free(table->node_count);
    free(table->nodes);
    free(table);
}

Node* ht_insert(HashTable* table, char* key, int value) {
    // Compute the index
    unsigned long index = hash_function(key);

    Node* current_node = table->nodes[index];

    while (current_node != NULL){
        if (strcmp(key, current_node->key) == 0){
            current_node->value += value;
            return current_node;
        }
        current_node = current_node->next;
    }

    // Create the item
    Node* node = create_node(key, value);
    node->next = table->nodes[index];
    table->nodes[index] = node;
    table->node_count[index] += 1;
    table->count += 1;
    return node;
}

Node* ht_search(HashTable* table, char* key) {
    // Searches the key in the hashtable
    // and returns NULL if it doesn't exist
    int index = hash_function(key);
    Node* node = table->nodes[index];

    // Ensure that we move to items which are not NULL
    while (node != NULL) {
        if (strcmp(node->key, key) == 0)
            return node;
        node = node->next;
    }
    return NULL;
}

void ht_print(HashTable *h)
{
    /* Set all pointers to NULL */
    Node *current = NULL;
    int i;

    for (i = 0; i < h->size; i++)
    {
        current = h->nodes[i];
        if (current == NULL)
            continue;
        /* Deallocate memory of every node in the table */
        int spaces = 0;
        while (current != NULL)
        {
            int j;
            for (j = 0; j < spaces; j++)
            {
                printf("\t");
            }
            printf("i: %d, key: %s, value: %d\n", i, current->key, current->value);
            current = current->next;
            spaces++;
        }
    }

    for (i=0; i < h->size; i++){
        if (h->node_count[i] != 0)
            printf("i: %d, count : %d\n", i, h->node_count[i]);
    }
}

void reduce(HashTable **hash_tables, HashTable *final_table, int num_hashtables, int location)
{
    Node *node = NULL;
    int i;
    for (i = 0; i < num_hashtables; i++)
    {
        if (hash_tables[i] == NULL || hash_tables[i]->nodes[location] == NULL)
        {
            continue;
        }

        Node *current = hash_tables[i]->nodes[location];
        if (current == NULL)
            continue;

        while (current != NULL)
        {
            node = ht_insert(final_table, current->key, 0);
            node->value += current->value;
            current = current->next;
        }
    }
}

void migrate(COUNT* words, HashTable* final_table, int h_start, int h_end, int c_start, int c_end){
    Node *current = NULL;

    int i;
    int j = c_start;

    for (i = h_start; i < h_end; i++){
        current = final_table->nodes[i];
        if (current == NULL)
            continue;

        if (j >= c_end){
            return;
        }
        
        while(current != NULL){
            words[j].word =  strdup(current->key);
            words[j].count = current->value;
            current = current->next;
            j++;
        }
    }
}

#endif