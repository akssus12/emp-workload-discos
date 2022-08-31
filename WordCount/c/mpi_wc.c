#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "../htable.h"

#define MAX_UNIQUES 5000000
#define Max_length 50

typedef struct {
    int count;
    char word[Max_length];
} MAX_COUNT;

void migrate_MPI(MAX_COUNT* words, HashTable* final_table, int h_start, int h_end, int c_start, int c_end){
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
            strncpy(words[j].word, current->key, Max_length);
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

int main(int argc, char** argv) {
    // Used to find the file size
    struct stat sb;
    double start_time, file_time, tolower_time, hashing_time, end_time;
    double migrate_comm_time, communicate_time, after_comm_time;
    MPI_Init( &argc, &argv );
    int rank, size;
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );

    char filename[256];
    strcpy(filename, argv[1]);
    
    bool is_alpha = true;
    int i, j;

    start_time = MPI_Wtime();
    //////////////////////////////////// CREATE THE DATATYPE FOR MPI ////////////////////////////////////

    // Create the Datatype for MPI
    MPI_Datatype countMPI;
    int lenghts[2] = {1, Max_length};

    MPI_Aint displacements[2];
    MAX_COUNT dummy_count;
    MPI_Aint base_address;
    MPI_Get_address(&dummy_count, &base_address);
    MPI_Get_address(&dummy_count.count, &displacements[0]);
    MPI_Get_address(&dummy_count.word[0], &displacements[1]);
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);

    MPI_Datatype types[2] = { MPI_INT, MPI_CHAR };
    MPI_Type_create_struct(2, lenghts, displacements, types, &countMPI);
    MPI_Type_commit(&countMPI);

    //////////////////////////////////// READ FILE ////////////////////////////////////
    stat(argv[1], &sb);
    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening data file\n");
    }

    char* word = malloc(sb.st_size/2 + 1);
    memset(word, 0, sb.st_size/2+1);

    fseek(fp, 0, SEEK_SET);
    if (rank != 0){
        fseek(fp, sb.st_size/2, SEEK_SET);
    }

    fread(word, sb.st_size/2, 1, fp);
    file_time = MPI_Wtime();
    printf("%d rank : Complete reading files\n", rank);
    fclose(fp);

    //////////////////////////////////// CONVERT TO LOWERCASE ////////////////////////////////////
    char* p;
    // Convert word to lower case in place.
    for (p = word; *p; p++) {
        *p = tolower(*p);
    }
    tolower_time = MPI_Wtime();
    printf("%d rank : Complete converting to lowercase\n", rank);

    //////////////////////////////////// HASHING WORD TO HASH TABLE ////////////////////////////////////
    HashTable* hash_table = create_htable(MAX_UNIQUES);
    char *start_string, *end_string, *free_string, *token; // for strchr()
    start_string = end_string = (char *)word;

    while( (end_string = strchr(start_string, '\n')) ){
        int size_string = end_string - start_string + 1;
        char *string = calloc(size_string, sizeof(char));
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

            ht_insert(hash_table, token, 1);
        }
        free(free_string);
        start_string = end_string + 1;
    }
    printf("%d : frequency of words %d\n", rank, hash_table->count);

    hashing_time = MPI_Wtime();
    printf("%d rank : Complete hashing to words\n", rank);

    //////////////////////////////////// MIGRATE WORD TO ARRAY FOR COMMUNICATION ////////////////////////////////////
    MAX_COUNT* words = calloc(hash_table->count, sizeof(MAX_COUNT));
    migrate_MPI(words, hash_table, 0, hash_table->size, 0, hash_table->count);

    migrate_comm_time = MPI_Wtime();
    printf("%d rank : Complete migrate words to array for MPI\n", rank);

    //////////////////////////////////// TRANSFER ARRAY ////////////////////////////////////
    int num_words = hash_table->count;
    int received_num_words[2];
    int total_words;
    MAX_COUNT* receive_words;
    MPI_Gather(&num_words, 1, MPI_INT, received_num_words, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if(rank == 0){
        printf("received_num_words[0] : %d\n", received_num_words[0]);
        printf("received_num_words[1] : %d\n", received_num_words[1]);
        total_words = received_num_words[0] + received_num_words[1];
        printf("total_words : %d\n", total_words);
        receive_words = calloc(total_words, sizeof(MAX_COUNT));
        int counts[2] = {received_num_words[0], received_num_words[1]};
        int displs[2] = {0, received_num_words[0]};
        MPI_Gatherv(words, num_words, countMPI, receive_words, counts, displs, countMPI, 0, MPI_COMM_WORLD);

    } else {
        MPI_Gatherv(words, num_words, countMPI, NULL, NULL, NULL, countMPI, 0, MPI_COMM_WORLD);
    }

    communicate_time = MPI_Wtime();
    printf("%d rank : Complete MPI\n", rank);

    //////////////////////////////////// AFTER MPI ////////////////////////////////////
    if ( rank == 0 ) {
        for (i=num_words; i<total_words; i++) {
            ht_insert(hash_table, receive_words[i].word, receive_words[i].count);
        }
        
        COUNT* final_words = calloc(hash_table->count, sizeof(COUNT));
        printf("num : %d\n", hash_table->count);
        migrate(final_words, hash_table, 0, hash_table->size, 0, hash_table->count);

        qsort(&final_words[0], hash_table->count, sizeof(COUNT), cmp_count);
        after_comm_time = MPI_Wtime();
        printf("%d rank : Complete tasks after MPI\n", rank);

        //////////////////////////////////// PRINT RESULT ////////////////////////////////////
        // Iterate again to print output.
        // for (i = 0; i < hash_table->count; i++) {
        //     printf("%s %d\n", final_words[i].word, final_words[i].count);
        // }
        end_time = MPI_Wtime();

        //////////////////////////////////// TIME ////////////////////////////////////

        printf(" number of words : %d\n", hash_table->count);
        printf("\n Totaltime = %.6f seconds\n", end_time-start_time);
        printf("\n start-file = %.6f seconds\n", file_time-start_time);
        printf("\n file-tolower = %.6f seconds\n", tolower_time-file_time);
        printf("\n tolower-hashing = %.6f seconds\n", hashing_time-tolower_time);
        printf("\n hashing-migrate_comm = %.6f seconds\n", migrate_comm_time-hashing_time);
        printf("\n migrate_comm-communication = %.6f seconds\n", communicate_time-migrate_comm_time);
        printf("\n communication-after_comm = %.6f seconds\n", after_comm_time-communicate_time);
        printf("\n after_comm-end_time = %.6f seconds\n", end_time-after_comm_time);

        //////////////////////////////////// FREE ////////////////////////////////////

        free(final_words);
        free(receive_words);
    }
    
    // [TODO] : need to implemet hashtable free. (occuring invalid pointer)
    free(words);
    free(word);

    MPI_Finalize();
    return 0;
}