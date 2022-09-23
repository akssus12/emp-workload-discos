#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <omp.h>
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
    int value;
    Node *next;
};

// Define the Hash Table here
struct HashTable {
    // Contains an array of pointers to items
    Node** nodes;
    omp_lock_t *lock_table;
    int* node_count;
    int size;
    int count;
};

struct Count{
    char* word;
    int count;
};

//////////////////////////////////////////////// HASH TABLE FUNCTIONS ////////////////////////////////////////////////

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

void free_node(Node* node) {
    // Frees an item
    free(node->key);
    free(node);
}

HashTable* create_htable(int size) {
    // Creates a new HashTable
    HashTable* table = (HashTable*) malloc (sizeof(HashTable));
    table->size = size;
    table->count = 0;
    table->nodes = (Node**) calloc (table->size, sizeof(Node*));
    table->node_count = (int *) calloc (table->size, sizeof(int));
    table->lock_table = (omp_lock_t *) calloc(table->size, sizeof(omp_lock_t));
    int i;
    for (i=0; i < table->size; i++){
        table->nodes[i] = NULL;
        omp_init_lock(&table->lock_table[i]);
    }

    return table;
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
        omp_destroy_lock(&table->lock_table[i]);
    }

    free(table->lock_table);
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
    omp_set_dynamic(0);     // Explicitly disable dynamic teams
    omp_set_num_threads(2); // Use 2 threads for all consecutive parallel regions

    // Used to find the file size
    struct stat sb;
    double start_time, file_time, exec_time, tolower_time, hashing_time, migrate_time, qsort_time;
    char filename[256];
    strcpy(filename, argv[1]);
    int total_line = atoi(argv[2]);
    int i;

    start_time = omp_get_wtime();
    //////////////////////////////////// READ FILE ////////////////////////////////////
    stat(argv[1], &sb);

    char **array_word1 = (char **)calloc(total_line, sizeof(char *));

    FILE *ptr;
    char *tmp_line = NULL;
    size_t len = 0;
    ssize_t read;

    ptr = fopen(filename, "r");
    if (ptr == NULL) {
        printf("Error opening data file\n");
    }

    i = 0;
    while((read = getline(&tmp_line, &len, ptr)) != -1) {
        array_word1[i] = (char *)calloc(read+1, 1);
        memset(array_word1[i], 0, (size_t)read+1);
    
        array_word1[i] = strncpy(array_word1[i], tmp_line, read);
        array_word1[i][read] = '\0';
        i++;
    }
    free(tmp_line);
    fclose(ptr);

    file_time = omp_get_wtime();
    printf("Complete reading files in the array_word[i]\n");

    //////////////////////////////////// CONVERT TO LOWERCASE ////////////////////////////////////
    char *p;
    #pragma omp parallel for schedule(dynamic) shared(array_word1) private(i, p)
    for(i=0; i < total_line; i++){
        p = array_word1[i];
        while( *p ){
            *p = tolower(*p);
            p++;
        }
    }
    tolower_time = omp_get_wtime();
    printf("Complete converting to lowercase\n");

    //////////////////////////////////// HASHING WORD TO HASH TABLE ////////////////////////////////////
    // Create hash table array
    HashTable* hash_table = create_htable(MAX_UNIQUES);
    char **free_array = (char **)calloc(total_line, sizeof(char*));

    bool is_alpha;
    char *token;
    int j;

    #pragma omp parallel for schedule(dynamic) shared(array_word1, free_array, hash_table) private(i, j, token)
    for (i=0; i<total_line; i++){
        free_array[i] = array_word1[i];
        is_alpha = true;

        while(( token = strtok_r(array_word1[i], " ", &array_word1[i] ))) {
            for( j=0; token[j] != '\0'; j++ ){
               if ( isalpha(token[j]) == 0 ) {
                    is_alpha = false;
                    break;
               }
            }

            if ( !is_alpha || strlen(token) >= 50 || token[0] =='\n'){
                is_alpha = true;
                continue;
            }

            unsigned long index = hash_function(token);
            omp_set_lock(&hash_table->lock_table[index]);
            ht_insert(hash_table, token, 1);
            omp_unset_lock(&hash_table->lock_table[index]);
        }
    }
    hashing_time = omp_get_wtime();
    printf("Complete hasing the word_array[i]\n");

    //////////////////////////////////// MIGRATE WORD TO ARRAY ////////////////////////////////////
    COUNT* words = (COUNT*)calloc(hash_table->count, sizeof(COUNT));
    int *len_words = (int *)calloc(hash_table->size, sizeof(int));

    #pragma omp parallel for reduction(+:len_words[:MAX_UNIQUES])
    for (i = 1; i<hash_table->size; i++){
        len_words[i] += len_words[i-1] + hash_table->node_count[i];
    }

    #pragma omp parallel shared(hash_table, words, len_words) private(i)
    {
        int id = omp_get_thread_num();
        int total_thread = omp_get_num_threads();
        int h_start = (hash_table->size * id) / total_thread;
        int h_end = (hash_table->size * (id + 1)) / total_thread;
        int c_start = 0;
        int c_end = 0;

        if(id == 0) {
            c_start = 0;
            c_end = len_words[MAX_UNIQUES/total_thread];
        } else {
            c_start = len_words[MAX_UNIQUES/total_thread];
            c_end = hash_table->count;
        }

        migrate(words, hash_table, h_start, h_end, c_start, c_end);
    }

    migrate_time = omp_get_wtime();
    printf("Complete inserting populated data in hashtable to words[]\n");

    //////////////////////////////////// QUICK SORT ////////////////////////////////////
    qsort(&words[0], hash_table->count, sizeof(COUNT), cmp_count);
    qsort_time = omp_get_wtime();
    
    printf("Complete sorting words[]\n");

    //////////////////////////////////// PRINT RESULT ////////////////////////////////////
    // Iterate again to print output.
    // for (int i = 0; i < hash_table->count; i++) {
    //     printf("%s %d\n", words[i].word, words[i].count);
    // }
    exec_time = omp_get_wtime();

    //////////////////////////////////// TIME ////////////////////////////////////
    printf("total_words : %d\n", hash_table->count);
    printf("\nTotaltime = %.6f seconds\n", exec_time-start_time);
    printf("\nstart-file = %.6f seconds\n", file_time-start_time);
    printf("\nfile-tolower = %.6f seconds\n", tolower_time-file_time);
    printf("\ntolower-hashing = %.6f seconds\n", hashing_time-tolower_time);
    printf("\nhashing-migrate = %.6f seconds\n", migrate_time-hashing_time);
    printf("\nmigrate-qsort = %.6f seconds\n", qsort_time-migrate_time);

    //////////////////////////////////// FREE ////////////////////////////////////
    // [TODO] : need to implemet hashtable free. (occuring invalid pointer)
    for(i=0; i<total_line; i++){
        free(free_array[i]);
    }
    free(free_array);
    free(array_word1);
    free(words);
    free(len_words);
    free_htable(hash_table);

    return 0;
}


// LEGACY

// #include <ctype.h>
// #include <search.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/time.h>
// #include <sys/stat.h>
// #include <omp.h>
// #include "../htable.h"
// #include <stdbool.h>

// #define MAX_UNIQUES 5000000
// #define TASK_SIZE 100

// // Comparison function for qsort() ordering by count descending.
// int cmp_count(const void* p1, const void* p2) {
//     int c1 = ((COUNT*)p1)->count;
//     int c2 = ((COUNT*)p2)->count;
//     if (c1 == c2) return 0;
//     if (c1 < c2) return 1;
//     return -1;
// }

// int main(int argc, char** argv) {
//     omp_set_dynamic(0);     // Explicitly disable dynamic teams
//     omp_set_num_threads(2); // Use 2 threads for all consecutive parallel regions
//     int NUM_THREADS = 2;

//     // Used to find the file size
//     struct stat sb;
//     double start_time, file_time, exec_time, tolower_time, hashing_time, reduce_time, migrate_time, qsort_time;
//     char filename[256];
//     strcpy(filename, argv[1]);
//     bool is_alpha;
//     int i;

//     start_time = omp_get_wtime();
//     //////////////////////////////////// READ FILE ////////////////////////////////////
//     stat(argv[1], &sb);

//     char ** array_word = (char **)malloc(sizeof(char *) * NUM_THREADS);
//     FILE *fp;
//     #pragma omp parallel shared(array_word) private(i, fp) num_threads(2)
//     {
//         i = omp_get_thread_num();
//         array_word[i] = malloc(sb.st_size/2 + 1);
//         memset(array_word[i], 0, sb.st_size/2 + 1);

//         fp = fopen(filename, "r");
//         if (fp == NULL) {
//             printf("Error opening data file\n");
//         }

//         if(i%2 != 0){
//             fseek(fp, sb.st_size/2, SEEK_SET);
//         }

//         fread(array_word[i], sb.st_size/2, 1, fp);
//         array_word[i][sb.st_size/2 + 1] = '\0';
//         fclose(fp);
//     }
//     file_time = omp_get_wtime();
//     printf("Complete reading files in the array_word[i]\n");

//     //////////////////////////////////// CONVERT TO LOWERCASE ////////////////////////////////////
//     char* p;
//     #pragma omp parallel for shared(array_word) private(i, p) num_threads(2)
//     for (i=0; i < NUM_THREADS; i++){
//         // Convert word to lower case in place.
//         for (p = array_word[i]; *p; p++) {
//             *p = tolower(*p);
//         }
//     }
//     tolower_time = omp_get_wtime();
//     printf("Complete converting to lowercase\n");

//     //////////////////////////////////// HASHING WORD TO HASH TABLE ////////////////////////////////////
//     // Create hash table array [0, 1]
//     HashTable **hash_tables;
//     hash_tables = (struct HashTable **)malloc(sizeof(struct HashTable *) * NUM_THREADS);

//     char *string, *free_string, *token, *start_string, *end_string;
//     int j;
//     #pragma omp parallel shared(array_word, hash_tables) private(i, start_string, end_string, string, token, j, is_alpha, free_string) num_threads(2)
//     {
//         i = omp_get_thread_num();
//         hash_tables[i] = create_htable(MAX_UNIQUES);
//         is_alpha = true;
//         start_string = end_string = (char *)array_word[i];

//         while( (end_string = strchr(start_string, '\n')) ){
//             int size_string = end_string - start_string + 1;
//             string = calloc(size_string, sizeof(char));
//             strncpy(string, start_string, size_string-1);
//             free_string = string;

//             while(( token = strtok_r(string, " ", &string) )){
//                 for ( j=0; token[j] != '\0'; j++ ){
//                     if ( isalpha(token[j]) == 0 ) {
//                         is_alpha = false;
//                         break;
//                     }
//                 }
            
//                 if ( !is_alpha || strlen(token) >= 50 || token[0] =='\n'){
//                     is_alpha = true;
//                     continue;
//                 }

//                 ht_insert(hash_tables[i], token, 1);
//             }
//             free(free_string);
//             start_string = end_string + 1;
//         }
//     }
//     hashing_time = omp_get_wtime();
//     printf("Complete hasing the word_array[i]\n");

//     //////////////////////////////////// REDUCE HASHTABLE ////////////////////////////////////
//     HashTable* final_table = create_htable(MAX_UNIQUES);

//     #pragma omp parallel shared(hash_tables, final_table) private(i)
//     {
//         int id = omp_get_thread_num();
//         int total_thread = omp_get_num_threads();
//         int start = (MAX_UNIQUES * id) / total_thread;
//         int end = (MAX_UNIQUES * (id + 1)) / total_thread;
//         if(id == total_thread -1)
//             end = MAX_UNIQUES;

//         for (i = start; i < end; i++){
//             reduce(hash_tables, final_table, total_thread, i);
//         }
//     }
//     reduce_time = omp_get_wtime();
//     printf("Complete redecing hashtable\n");

//     //////////////////////////////////// MIGRATE WORD TO ARRAY ////////////////////////////////////
//     COUNT* words = (COUNT*)calloc(final_table->count, sizeof(COUNT));
//     #pragma omp parallel shared(final_table, words) private(i)
//     {
//         int id = omp_get_thread_num();
//         int total_thread = omp_get_num_threads();
//         int h_start = (final_table->size * id) / total_thread;
//         int h_end = (final_table->size * (id + 1)) / total_thread;
//         int c_start = 0;
//         int c_end = 0;

//         if(id == total_thread -1)
//             h_end = final_table->size;

//         int c_count = 0; 
//         for(i=h_start; i<h_end; i++){
//             c_count += final_table->node_count[i]; 
//         }

//         if(id == 0){
//             c_start = 0;
//             c_end = c_count;
//         } else {
//             c_start = final_table->count - c_count;
//             c_end = final_table->count;
//         }

//         if(c_start != c_end){
//             migrate(words, final_table, h_start, h_end, c_start, c_end);
//         }
//     }
//     migrate_time = omp_get_wtime();
//     printf("Complete inserting populated data in hashtable to words[]\n");

//     //////////////////////////////////// QUICK SORT ////////////////////////////////////
//     // #pragma omp parallel
//     // {
//     //     #pragma omp single
//     //     quicksort(words, 0, final_table->count);
//     // }
//     qsort(&words[0], final_table->count, sizeof(COUNT), cmp_count);
//     qsort_time = omp_get_wtime();
    
//     printf("Complete sorting words[]\n");

//     //////////////////////////////////// PRINT RESULT ////////////////////////////////////
//     // Iterate again to print output.
//     // for (int i = 0; i < final_table->count; i++) {
//     //     printf("%s %d\n", words[i].word, words[i].count);
//     // }
//     exec_time = omp_get_wtime();

//     //////////////////////////////////// TIME ////////////////////////////////////
//     printf("total_words : %d\n", final_table->count);
//     printf("\nTotaltime = %.6f seconds\n", exec_time-start_time);
//     printf("\nstart-file = %.6f seconds\n", file_time-start_time);
//     printf("\nfile-tolower = %.6f seconds\n", tolower_time-file_time);
//     printf("\ntolower-hashing = %.6f seconds\n", hashing_time-tolower_time);
//     printf("\nhashing-reduce = %.6f seconds\n", reduce_time-hashing_time);
//     printf("\nreduce-migrate = %.6f seconds\n", migrate_time-reduce_time);
//     printf("\nmigrate-qsort = %.6f seconds\n", qsort_time-migrate_time);

//     //////////////////////////////////// FREE ////////////////////////////////////
//     // [TODO] : need to implemet hashtable free. (occuring invalid pointer)
//     for(i=0; i<NUM_THREADS; i++){
//         free(array_word[i]);
//     }
//     free(array_word);
//     free(words);

//     return 0;
// }