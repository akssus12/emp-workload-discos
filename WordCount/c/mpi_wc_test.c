#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdbool.h>

#define MAX_UNIQUES 8000000
#define Max_length 50

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
    struct timeval start,end, execution_1, mpi, file, execution_2;
    double totaltime, start_file, file_exe1, exe1_mpi, mpi_exe2;
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

    bool is_alpha = true;
    int i;

    char *start_string, *end_string; // for strchr()

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

    char* word = malloc(sb.st_size/2 + 1);
    memset(word, 0, sb.st_size/2+1);

    fseek(fp, 0, SEEK_SET);

    if (rank != 0){
        fseek(fp, sb.st_size/2, SEEK_SET);
    }

    fread(word, sb.st_size/2, 1, fp);

    gettimeofday(&file, NULL);

    // Convert word to lower case in place.
    for (char* p = word; *p; p++) {
        *p = tolower(*p);
    }

    start_string = end_string = (char *)word;

    while( (end_string = strchr(start_string, '\n')) ){
        int size_string = end_string - start_string + 1;
        char *string = calloc(size_string, sizeof(char));
        strncpy(string, start_string, size_string-1);

        char * token = strtok(string, " ");
        while( token != NULL ){
            for ( i=0; token[i] != '\0'; i++ ){
                if ( isalpha(token[i]) == 0 ) {
                    is_alpha = false;
                    break;
                }
            }
            
            if ( !is_alpha || strlen(token) >= 50 ){
                token = strtok(NULL, " ");
                is_alpha = true;
                continue;
            }

            // Search for word in hash table.
            ENTRY item = {token, NULL};
            ENTRY* found = hsearch(item, FIND);
            if (found != NULL) {
                // Word already in table, increment count.
                int* pn = (int*)found->data;
                (*pn)++;
                token = strtok(NULL, " ");
            } else {
                // Word not in table, insert it with count 1.
                item.key = strdup(token); // need to copy word
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
                // words[num_words].word = item.key; // error : assignment to expression with array type
                strcpy(words[num_words].word, item.key);
                
                num_words++;
                token = strtok(NULL, " ");
            }
        }
        free(string);
        start_string = end_string + 1;
    }

    // Iterate once to add counts to words list (not sort)
    for (i = 0; i < num_words; i++) {
        ENTRY item = {words[i].word, NULL};
        ENTRY* found = hsearch(item, FIND);
        if (found == NULL) { // shouldn't happen
            fprintf(stderr, "key not found12: %s\n", item.key);
            return 1;
        }
        words[i].count = *(int*)found->data;
    }

    gettimeofday(&execution_1, NULL);

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Gather(&num_words, 1, MPI_INT, received_num_words, 1, MPI_INT, 0, MPI_COMM_WORLD);


    MPI_Barrier(MPI_COMM_WORLD);
    
    if(rank == 0){
        total_words = received_num_words[0] + received_num_words[1];
        receive_words = calloc(total_words, sizeof(count));
        int counts[2] = {received_num_words[0], received_num_words[1]};
        int displs[2] = {0, received_num_words[0]};
        MPI_Gatherv(words, num_words, countMPI, receive_words, counts, displs, countMPI, 0, MPI_COMM_WORLD);

    } else {
        MPI_Gatherv(words, num_words, countMPI, NULL, NULL, NULL, countMPI, 0, MPI_COMM_WORLD);
    }


    MPI_Barrier(MPI_COMM_WORLD);

    gettimeofday(&mpi, NULL);
    
    if(rank==0){
        for (i=num_words; i<total_words; i++){
            // Search for word in hash table.
            ENTRY item = {receive_words[i].word, NULL};
            ENTRY* found = hsearch(item, FIND);
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
                strncpy(words[num_words].word, item.key, Max_length-1);
                words[num_words].word[Max_length] = '\0';
                num_words++;
            }
        }    

        for (i = 0; i < num_words; i++) {
                 ENTRY item = {words[i].word, NULL};
                 ENTRY* found = hsearch(item, FIND);
                 if (found == NULL) { // shouldn't happen
                     fprintf(stderr, "key not found: %s\n", item.key);
                     return 1;
                 }
                 words[i].count = *(int*)found->data;
        }
        qsort(&words[0], num_words, sizeof(count), cmp_count);

        gettimeofday(&execution_2, NULL);
        gettimeofday(&end, NULL);
        start_file = (((file.tv_usec - start.tv_usec) / 1.0e6 + file.tv_sec - start.tv_sec) * 1000) / 1000;
        file_exe1 = (((execution_1.tv_usec - file.tv_usec) / 1.0e6 + execution_1.tv_sec - file.tv_sec) * 1000) / 1000;
        exe1_mpi = (((mpi.tv_usec - execution_1.tv_usec) / 1.0e6 + mpi.tv_sec - execution_1.tv_sec) * 1000) / 1000;
        mpi_exe2 = (((execution_2.tv_usec - mpi.tv_usec) / 1.0e6 + execution_2.tv_sec - mpi.tv_sec) * 1000) / 1000;
        totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;

        printf("num_words : %d\n", num_words);
        printf("start_file = %f seconds\n", start_file);
        printf("file_exe1 = %f seconds\n", file_exe1);
        printf("exe1_mpi = %f seconds\n", exe1_mpi);
        printf("mpi_exe2 = %f seconds\n", mpi_exe2);
        printf("\nTotaltime = %f seconds\n", totaltime);
        
        free(receive_words);
        
    }   
    fclose(fp);
    free(words);
    free(word);

    MPI_Finalize();

    return 0;
}

