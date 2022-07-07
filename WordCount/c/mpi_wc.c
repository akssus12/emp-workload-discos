#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <sys/time.h>
#include <sys/stat.h>

#define MAX_UNIQUES 20000000
#define Max_length 600

typedef struct {
    int count;
    char word[Max_length];
} count;

// Comparison function for qsort() ordering by count descending.
int cmp_count(const void* p1, const void* p2) {
    int c1 = ((count*)p1)->count;
    int c2 = ((count*)p2)->count;
    if (c1 == c2) return 0;
    if (c1 < c2) return 1;
    return -1;
}

int main(int argc, char** argv) {
    // Used to find the file size
    struct stat sb;
    
    MPI_Init( &argc, &argv );
    int rank, size;
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );

    // Create the Datatype for MPI
    MPI_Datatype countMPI;
    int lenghts[2] = {1, Max_length};

    MPI_Aint displacements[2];
    count dummy_count;
    MPI_Aint base_address;
    MPI_Get_address(&dummy_count, &base_address);
    MPI_Get_address(&dummy_count.count, &displacements[0]);
    MPI_Get_address(&dummy_count.word[0], &displacements[1]);
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);

    MPI_Datatype types[2] = { MPI_INT, MPI_CHAR };
    MPI_Type_create_struct(2, lenghts, displacements, types, &countMPI);
    MPI_Type_commit(&countMPI);
    
    struct timeval start,end;
    double totaltime;
    int lines;

    gettimeofday(&start, NULL);

    char filename[256];
    strcpy(filename, argv[1]);
    // The hcreate hash table doesn't provide a way to iterate, so
    // store the words in an array too (also used for sorting).
    count* words = calloc(MAX_UNIQUES, sizeof(count));
    count* receive_words;
    count* final_words;
    int num_words = 0;
    int total_words = 0;
    int received_num_words[2];

    char limit_word[Max_length];

    // Allocate hash table.
    if (hcreate(MAX_UNIQUES) == 0) {
        fprintf(stderr, "error creating hash table\n");
        return 1;
    }

    stat(argv[1], &sb);

    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening data file\n");
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //char word[101]; // 100-char word plus NUL byte
    char* word = malloc(sb.st_size/2 + 1);
    memset(word, 0, sb.st_size/2+1);

    fseek(fp, 0, SEEK_SET);

    if (rank != 0){
        fseek(fp, sb.st_size/2, SEEK_SET);
    }

    fread(word, sb.st_size/2, 1, fp);

    // Convert word to lower case in place.
    for (char* p = word; *p; p++) {
        *p = tolower(*p);
    }

    char * token = strtok(word, " ");

    while( token != NULL ) {
        // Search for word in hash table.
        strncpy(limit_word, token, Max_length-1);
        limit_word[Max_length] = '\0';

        ENTRY item = {token, NULL};
        ENTRY* found = hsearch(item, FIND);
        if (found != NULL) {
            // Word already in table, increment count.
            int* pn = (int*)found->data;
            (*pn)++;

            token = strtok(NULL, " ");
        } else {
            // Word not in table, insert it with count 1.
            item.key = strdup(limit_word); // need to copy word
            if (item.key == NULL) {
                fprintf(stderr, "out of memory in strdup\n");
                return 1;
            }
            int* pn = malloc(sizeof(int));
            if (pn == NULL) {
                fprintf(stderr, "out of memory in malloc\n");
                return 1;
            }
            *pn = 1;
            item.data = pn;
            ENTRY* entered = hsearch(item, ENTER);
            if (entered == NULL) {
                fprintf(stderr, "table full, increase MAX_UNIQUES\n");
                return 1;
            }

            // And add to words list for iterating.
            words[num_words].word = item.key;
            num_words++;
            // strncpy(words[num_words].word, item.key, Max_length-1);
            // words[num_words].word[Max_length] = '\0';
            // num_words++;

            token = strtok(NULL, " ");
        }
    }

    // Iterate once to add counts to words list, then sort.
    for (int i = 0; i < num_words; i++) {
        ENTRY item = {words[i].word, NULL};
        ENTRY* found = hsearch(item, FIND);
        if (found == NULL) { // shouldn't happen
            fprintf(stderr, "key not found12: %s\n", item.key);
            return 1;
        }
        words[i].count = *(int*)found->data;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Gather(&num_words, 1, MPI_INT, received_num_words, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // for (int i=0 ; i<num_words; i++){
    //     printf("%s %d\n", words[i].word, words[i].count);
    // }
    // printf("total num : %d | %d\n", num_words, rank);

    MPI_Barrier(MPI_COMM_WORLD);
    
    if(rank == 0){
        // printf("received_num_words[0] : %d\n", received_num_words[0]);
        // printf("received_num_words[1] : %d\n", received_num_words[1]);
        total_words = received_num_words[0] + received_num_words[1];
        receive_words = calloc(total_words, sizeof(count));
        int counts[2] = {received_num_words[0], received_num_words[1]};
        int displs[2] = {0, received_num_words[0]};
        MPI_Gatherv(words, num_words, countMPI, receive_words, counts, displs, countMPI, 0, MPI_COMM_WORLD);

    } else {
        MPI_Gatherv(words, num_words, countMPI, NULL, NULL, NULL, countMPI, 0, MPI_COMM_WORLD);
    }


    MPI_Barrier(MPI_COMM_WORLD);
    
    if(rank==0){
        // for (int i=0 ; i<total_words; i++){
        //     printf("%s %d\n", receive_words[i].word, receive_words[i].count);
        // }
        // printf("total num : %d\n", total_words);
        // // final_words = calloc(total_words, sizeof(count));
        // // memcpy(final_words, words, num_words * sizeof(count));

        
        for (int i=num_words; i<total_words; i++){
            // Search for word in hash table.
            ENTRY item = {receive_words[i].word, NULL};
            ENTRY* found = hsearch(item, FIND);
            // printf("item : %s\n", item.key);
            if (found != NULL) {
                // Word already in table, increment count.
                int* pn = (int*)found->data;
                (*pn) += receive_words[i].count;

            } else {
                // Word not in table, insert it with count 1.
                item.key = strdup(receive_words[i].word); // need to copy word
                if (item.key == NULL) {
                    fprintf(stderr, "out of memory in strdup\n");
                    return 1;
                }
                int* pn = malloc(sizeof(int));
                if (pn == NULL) {
                    fprintf(stderr, "out of memory in malloc\n");
                    return 1;
                }
                *pn = receive_words[i].count;
                item.data = pn;
                ENTRY* entered = hsearch(item, ENTER);
                if (entered == NULL) {
                    fprintf(stderr, "table full, increase MAX_UNIQUES\n");
                    return 1;
                }

                // And add to words list for iterating.
                words[num_words].word = item.key;
                num_words++;
                // strncpy(words[num_words].word, item.key, Max_length-1);
                // words[num_words].word[Max_length] = '\0';
                // num_words++;
            }
        }    

        for (int i = 0; i < num_words; i++) {
                 ENTRY item = {words[i].word, NULL};
                 ENTRY* found = hsearch(item, FIND);
                 if (found == NULL) { // shouldn't happen
                     fprintf(stderr, "key not found: %s\n", item.key);
                     return 1;
                 }
                 words[i].count = *(int*)found->data;
        }
        qsort(&words[0], num_words, sizeof(count), cmp_count);

        // for(int i=0; i<num_words; i++){
        //     printf("%s, %d\n", words[i].word, words[i].count);
        // }

        gettimeofday(&end, NULL);
        totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;

        printf("num_words : %d\n", num_words);
        printf("\nTotaltime = %f seconds\n", totaltime);
        free(receive_words);
        
        

    }   
    fclose(fp);
    free(words);
    free(word);

    MPI_Finalize();

    return 0;
}

