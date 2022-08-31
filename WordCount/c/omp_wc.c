#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <omp.h>
#include "../htable.h"
#include <stdbool.h>

#define MAX_UNIQUES 5000000
#define TASK_SIZE 100

// Comparison function for qsort() ordering by count descending.
int cmp_count(const void* p1, const void* p2) {
    int c1 = ((COUNT*)p1)->count;
    int c2 = ((COUNT*)p2)->count;
    if (c1 == c2) return 0;
    if (c1 < c2) return 1;
    return -1;
}

int main(int argc, char** argv) {
    omp_set_dynamic(0);     // Explicitly disable dynamic teams
    omp_set_num_threads(2); // Use 2 threads for all consecutive parallel regions
    int NUM_THREADS = 2;

    // Used to find the file size
    struct stat sb;
    double start_time, file_time, exec_time, tolower_time, hashing_time, reduce_time, migrate_time, qsort_time;
    char filename[256];
    strcpy(filename, argv[1]);
    bool is_alpha;
    int i;

    start_time = omp_get_wtime();
    //////////////////////////////////// READ FILE ////////////////////////////////////
    stat(argv[1], &sb);

    char ** array_word = (char **)malloc(sizeof(char *) * NUM_THREADS);
    FILE *fp;
    #pragma omp parallel shared(array_word) private(i, fp) num_threads(2)
    {
        i = omp_get_thread_num();
        array_word[i] = malloc(sb.st_size/2 + 1);
        memset(array_word[i], 0, sb.st_size/2 + 1);

        fp = fopen(filename, "r");
        if (fp == NULL) {
            printf("Error opening data file\n");
        }

        if(i%2 != 0){
            fseek(fp, sb.st_size/2, SEEK_SET);
        }

        fread(array_word[i], sb.st_size/2, 1, fp);
        array_word[i][sb.st_size/2 + 1] = '\0';
        fclose(fp);
    }
    file_time = omp_get_wtime();
    printf("Complete reading files in the array_word[i]\n");

    //////////////////////////////////// CONVERT TO LOWERCASE ////////////////////////////////////
    char* p;
    #pragma omp parallel for shared(array_word) private(i, p) num_threads(2)
    for (i=0; i < NUM_THREADS; i++){
        // Convert word to lower case in place.
        for (p = array_word[i]; *p; p++) {
            *p = tolower(*p);
        }
    }
    tolower_time = omp_get_wtime();
    printf("Complete converting to lowercase\n");

    //////////////////////////////////// HASHING WORD TO HASH TABLE ////////////////////////////////////
    // Create hash table array [0, 1]
    HashTable **hash_tables;
    hash_tables = (struct HashTable **)malloc(sizeof(struct HashTable *) * NUM_THREADS);

    char *string, *free_string, *token, *start_string, *end_string;
    int j;
    #pragma omp parallel shared(array_word, hash_tables) private(i, start_string, end_string, string, token, j, is_alpha, free_string) num_threads(2)
    {
        i = omp_get_thread_num();
        hash_tables[i] = create_htable(MAX_UNIQUES);
        is_alpha = true;
        start_string = end_string = (char *)array_word[i];

        while( (end_string = strchr(start_string, '\n')) ){
            int size_string = end_string - start_string + 1;
            string = calloc(size_string, sizeof(char));
            strncpy(string, start_string, size_string-1);
            free_string = string;

            while(( token = strtok_r(string, " ", &string) )){
                for ( j=0; token[j] != '\0'; j++ ){
                    if ( isalpha(token[j]) == 0 ) {
                        is_alpha = false;
                        break;
                    }
                }
            
                if ( !is_alpha || strlen(token) >= 50 || token[0] =='\n'){
                    is_alpha = true;
                    continue;
                }

                ht_insert(hash_tables[i], token, 1);
            }
            free(free_string);
            start_string = end_string + 1;
        }
    }
    hashing_time = omp_get_wtime();
    printf("Complete hasing the word_array[i]\n");

    //////////////////////////////////// REDUCE HASHTABLE ////////////////////////////////////
    HashTable* final_table = create_htable(MAX_UNIQUES);

    #pragma omp parallel shared(hash_tables, final_table) private(i)
    {
        int id = omp_get_thread_num();
        int total_thread = omp_get_num_threads();
        int start = (MAX_UNIQUES * id) / total_thread;
        int end = (MAX_UNIQUES * (id + 1)) / total_thread;
        if(id == total_thread -1)
            end = MAX_UNIQUES;

        for (i = start; i < end; i++){
            reduce(hash_tables, final_table, total_thread, i);
        }
    }
    reduce_time = omp_get_wtime();
    printf("Complete redecing hashtable\n");

    //////////////////////////////////// MIGRATE WORD TO ARRAY ////////////////////////////////////
    COUNT* words = (COUNT*)calloc(final_table->count, sizeof(COUNT*));
    #pragma omp parallel shared(final_table, words) private(i)
    {
        int id = omp_get_thread_num();
        int total_thread = omp_get_num_threads();
        int h_start = (final_table->size * id) / total_thread;
        int h_end = (final_table->size * (id + 1)) / total_thread;
        int c_start = 0;
        int c_end = 0;

        if(id == total_thread -1)
            h_end = final_table->size;

        int c_count = 0; 
        for(i=h_start; i<h_end; i++){
            c_count += final_table->node_count[i]; 
        }

        if(id == 0){
            c_start = 0;
            c_end = c_count;
        } else {
            c_start = final_table->count - c_count;
            c_end = final_table->count;
        }

        if(c_start != c_end){
            migrate(words, final_table, h_start, h_end, c_start, c_end);
        }
    }
    migrate_time = omp_get_wtime();
    printf("Complete inserting populated data in hashtable to words[]\n");

    //////////////////////////////////// QUICK SORT ////////////////////////////////////
    // #pragma omp parallel
    // {
    //     #pragma omp single
    //     quicksort(words, 0, final_table->count);
    // }
    qsort(&words[0], final_table->count, sizeof(COUNT), cmp_count);
    qsort_time = omp_get_wtime();
    
    printf("Complete sorting words[]\n");

    //////////////////////////////////// PRINT RESULT ////////////////////////////////////
    // Iterate again to print output.
    // for (int i = 0; i < final_table->count; i++) {
    //     printf("%s %d\n", words[i].word, words[i].count);
    // }
    exec_time = omp_get_wtime();

    //////////////////////////////////// TIME ////////////////////////////////////
    printf("total_words : %d\n", final_table->count);
    printf("\nTotaltime = %.6f seconds\n", exec_time-start_time);
    printf("\nstart-file = %.6f seconds\n", file_time-start_time);
    printf("\nfile-tolower = %.6f seconds\n", tolower_time-file_time);
    printf("\ntolower-hashing = %.6f seconds\n", hashing_time-tolower_time);
    printf("\nhashing-reduce = %.6f seconds\n", reduce_time-hashing_time);
    printf("\nreduce-migrate = %.6f seconds\n", migrate_time-reduce_time);
    printf("\nmigrate-qsort = %.6f seconds\n", qsort_time-migrate_time);

    //////////////////////////////////// FREE ////////////////////////////////////
    // [TODO] : need to implemet hashtable free. (occuring invalid pointer)
    for(i=0; i<NUM_THREADS; i++){
        free(array_word[i]);
    }
    free(array_word);
    free(words);

    return 0;
}