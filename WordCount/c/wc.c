#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdbool.h>

#define MAX_UNIQUES 5000000
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

void reduce(HashTable **hash_tables, HashTable *final_table, int num_hashtables, int location);
void migrate(COUNT* words, HashTable* final_table, int h_start, int h_end, int c_start, int c_end);

int cmp_count(const void* p1, const void* p2);

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

// Comparison function for qsort() ordering by count descending.
int cmp_count(const void* p1, const void* p2) {
    int c1 = ((COUNT*)p1)->count;
    int c2 = ((COUNT*)p2)->count;
    if (c1 == c2) return 0;
    if (c1 < c2) return 1;
    return -1;
}

//////////////////////////////////// MAIN ////////////////////////////////////

int main(int argc, char** argv) {
    // Used to find the file size
    struct stat sb;

    struct timeval start,end, file_time, tolower_time, hashing_time, migrate_time,qsort_time;
    char filename[256];
    strcpy(filename, argv[1]);
    bool is_alpha = true;
    int i;
    char *start_string, *end_string; // for strchr()

    gettimeofday(&start, NULL);
    //////////////////////////////////// READ FILE ////////////////////////////////////
    stat(argv[1], &sb);
    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening data file\n");
    }

    char* word = malloc(sb.st_size+1);
    memset(word, 0, sb.st_size+1);
    word[sb.st_size+1] = '\0';

    fread(word, sb.st_size+1, 1, fp);
    word[sb.st_size+1] = '\0';

    fclose(fp);
    gettimeofday(&file_time, NULL);

    //////////////////////////////////// CONVERT TO LOWERCASE ////////////////////////////////////
    char* p;
    // Convert word to lower case in place.
    for (p = word; *p; p++) {
        *p = tolower(*p);
    }
    gettimeofday(&tolower_time, NULL);

    //////////////////////////////////// HASHING WORD TO HASH TABLE ////////////////////////////////////

    HashTable* hash_table = create_htable(MAX_UNIQUES);
    start_string = end_string = (char *)word;
    char *string, *free_string, *token;

    while( (end_string = strchr(start_string, '\n')) ){
        int size_string = end_string - start_string + 1;
        string = calloc(size_string, sizeof(char));
        strncpy(string, start_string, size_string-1);
        free_string = string;

        while(( token = strtok_r(string, " ", &string) )){
            for ( i=0; token[i] != '\0'; i++ ){
                if ( isalpha(token[i]) == 0 ) {
                    is_alpha = false;
                    break;
                }
            }
            
            if ( !is_alpha || strlen(token) >= 50 || token[0] =='\n'){
                // token = strtok(NULL, " ");
                is_alpha = true;
                continue;
            }

            ht_insert(hash_table, token, 1);
        }
        free(free_string);
        start_string = end_string + 1;
    }
    gettimeofday(&hashing_time, NULL);

    //////////////////////////////////// MIGRATE WORD TO ARRAY ////////////////////////////////////

    COUNT* words = calloc(hash_table->count, sizeof(COUNT));

    migrate(words, hash_table, 0, hash_table->size, 0, hash_table->count);
    gettimeofday(&migrate_time, NULL);

    //////////////////////////////////// QUICK SORT ////////////////////////////////////

    qsort(&words[0], hash_table->count, sizeof(COUNT), cmp_count); 
    gettimeofday(&qsort_time, NULL);

    //////////////////////////////////// PRINT RESULT ////////////////////////////////////
    // Iterate again to print output.
    // for (i = 0; i < hash_table->count; i++) {
    //     printf("%s %d\n", words[i].word, words[i].count);
    // }
    gettimeofday(&end, NULL);

    //////////////////////////////////// TIME ////////////////////////////////////

    double totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;
    double start_file = (((file_time.tv_usec - start.tv_usec) / 1.0e6 + file_time.tv_sec - start.tv_sec) * 1000) / 1000;
    double file_tolower = (((tolower_time.tv_usec - file_time.tv_usec) / 1.0e6 + tolower_time.tv_sec - file_time.tv_sec) * 1000) / 1000;
    double tolower_hashing = (((hashing_time.tv_usec - tolower_time.tv_usec) / 1.0e6 + hashing_time.tv_sec - tolower_time.tv_sec) * 1000) / 1000;
    double hashing_migrate = (((migrate_time.tv_usec - hashing_time.tv_usec) / 1.0e6 + migrate_time.tv_sec - hashing_time.tv_sec) * 1000) / 1000;
    double migrate_qsort = (((qsort_time.tv_usec - migrate_time.tv_usec) / 1.0e6 + qsort_time.tv_sec - migrate_time.tv_sec) * 1000) / 1000;

    printf("total_words : %d\n", hash_table->count);
    printf("\nTotaltime = %f seconds\n", totaltime);
    printf("\nstart_file = %f seconds\n", start_file);
    printf("\nfile_tolower = %f seconds\n", file_tolower);
    printf("\ntolower_hashing = %f seconds\n", tolower_hashing);
    printf("\nhashing_migrate = %f seconds\n", hashing_migrate);
    printf("\nmigrate_qsort = %f seconds\n", migrate_qsort);

    //////////////////////////////////// FREE ////////////////////////////////////
    free(word);
    free(words);

    return 0;
}