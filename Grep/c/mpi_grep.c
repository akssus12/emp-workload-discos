#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <mpi.h>
#include <sys/time.h>
#include <sys/stat.h>

long getSpecificSize(char *name, int target_line){
    FILE *ptr;
    long size=0;
    int line=0;
    char c;

    ptr=fopen(name,"r");
    if (ptr == NULL) {
        printf("Error opening data file\n");
    }
    
    while((c=fgetc(ptr))!=EOF){
        if( line != target_line ) {
            size ++;
            if( c == '\n') {
                line ++;
            }
        } else {
            break;
        }
    }

    fseek(ptr, 0, SEEK_SET);
    fclose(ptr);

    return(size);
}

int main(int argc, char** argv) {
    MPI_Init( &argc, &argv );
    int rank, size;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );

    double start_time, file_time, tolower_time, search_time, communicate_time, end_time;
    char filename[256];
    char target[256];

    strcpy(filename, argv[1]);
    strcpy(target, argv[3]);

    int total_line = atoi(argv[2]);
    long line_size = getSpecificSize(filename, (int)total_line/2);

    start_time = MPI_Wtime();

    //////////////////////////////////// READ FILE ////////////////////////////////////
    char * word;
    struct stat sb;
    stat(argv[1], &sb);

    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening data file\n");
    }

    if (rank == 0) {
        word = malloc(line_size + 1);
        memset(word, 0, line_size + 1);
    
        fread(word, line_size, 1, fp);
        word[line_size + 1] = '\0';

    } else {
        word = malloc(sb.st_size - line_size + 1);
        memset(word, 0, sb.st_size - line_size + 1);

        fseek(fp, line_size, SEEK_SET);

        fread(word, sb.st_size - line_size, 1, fp);
        word[sb.st_size - line_size + 1] = '\0';
    }
    fclose(fp);

    file_time = MPI_Wtime();
    printf("Complete reading files in the word\n");

    //////////////////////////////////// CONVERT TO LOWERCASE ////////////////////////////////////
    // Convert word to lower case in place.
    char* p;
    for (p = word; *p; p++) {
        *p = tolower(*p);
    }
    tolower_time = MPI_Wtime();
    printf("Complete converting to lowercase\n");

    //////////////////////////////////// SEARCHING TARGET WORD ////////////////////////////////////
    int num_lines = 0;
    int num = 1;
    int * array_line = calloc(num, sizeof(int));
    int * backup_ptr = array_line;
    char * start_string, *end_string, *token; // for strchr()

    start_string = end_string = (char *)word;

    while( (end_string = strchr(start_string, '\n')) ){
        int size_string = end_string - start_string + 1;
        char *string = calloc(size_string, sizeof(char));
        strncpy(string, start_string, size_string-1);
        num_lines++;

        while( token = strtok_r(string, " ", &string) ){
            if(strcmp(token, target) == 0){
                // printf("lines : %d\n", num_lines);
                array_line[num-1] = num_lines;
                num ++;
                if( (array_line = (int*)realloc(array_line, sizeof(int)*num)) == NULL){
                    free(backup_ptr);
                    fprintf(stderr, "Memory allocation is failed");
                    exit(1);
                }
                break;
            } 
        }
        free(string);
        start_string = end_string + 1;
    }
    search_time = MPI_Wtime();
    printf("Complete searching target word\n");

    //////////////////////////////////// MPI ////////////////////////////////////
    int received_num;
    int *received_array_line;

    if (rank == 0){
        MPI_Recv(&received_num, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        received_array_line = calloc(received_num, sizeof(int));
    } else {
        MPI_Send(&num, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    if (rank == 0){
        MPI_Recv(received_array_line, received_num, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Send(array_line, num, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    communicate_time = MPI_Wtime();
    printf("Complete MPI\n");

    //////////////////////////////////// PRINT RESULT ////////////////////////////////////

    if (rank == 0){
        // Uncomment it if you want to print the target line
        // for (int i=0; i<num-1; i++){
        //     printf("%d\n", array_line[i]);
        // }

        // for (int i=0; i<received_num-1; i++){
        //     printf("%d\n", total_line/2 + received_array_line[i]);
        // }
        free(received_array_line);
    }
    
    end_time = MPI_Wtime();

    //////////////////////////////////// TIME ////////////////////////////////////

    if (rank == 0){
        printf("\nTotaltime = %f seconds\n", end_time - start_time);
        printf("start_file = %f seconds\n", file_time-start_time);
        printf("file_tolower = %f seconds\n", tolower_time-file_time);
        printf("tolower_search = %f seconds\n", search_time-tolower_time);
        printf("search-MPI = %f seconds\n", communicate_time-search_time);
        printf("MPI-end = %f seconds\n", end_time-communicate_time);
    }

    free(word);
    free(array_line);
    
    MPI_Finalize();
    return 0;
}