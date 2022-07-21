#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <mpi.h>
#include <sys/time.h>
#include <sys/stat.h>

int getTotalLine(char *name){
    FILE *ptr;
    int line=0;
    char c;

    ptr=fopen(name,"r");
    if (ptr == NULL) {
        printf("Error opening data file\n");
    }

    while((c=fgetc(ptr))!=EOF)
        if(c=='\n') line++;

    fseek(ptr, 0, SEEK_SET);
    fclose(ptr);

    return(line);
}

int getSpecificSize(char *name, int target_line){
    FILE *ptr;
    int size=0;
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

    struct stat sb;
    struct timeval start, end;
    double totaltime;
    char filename[256];
    char target[256];
    int ch = 0;
    int num_lines = 0;
    int num = 1;
    int received_num;
    int *received_array_line;
    int total_line;
    int line_size;
    char * word;
    char * start_string, *end_string; // for strchr()

    printf("debug 0\n");

    strcpy(filename, argv[1]);
    strcpy(target, argv[2]);
    printf("debug 01\n");

    gettimeofday(&start, NULL);
    printf("debug 02\n");

    stat(argv[1], &sb);
    printf("debug 03\n");

    total_line = getTotalLine(filename);
    printf("debug 04\n");
    line_size = getSpecificSize(filename, (int)total_line/2);
    printf("debug 05\n");
    printf("total_line : %d\n", total_line);
    printf("line_size : %d\n", line_size);

    FILE *fp;

    printf("debug 1\n");

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening data file\n");
    }

    printf("debug 2\n");

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

    printf("debug 3\n");

    printf("line_size : %d\n", line_size);
    printf("sb.st_size : %lu\n", sb.st_size);
    printf("word size : %lu\n", malloc_usable_size(word));

    // Convert word to lower case in place.
    for (char* p = word; *p; p++) {
        *p = tolower(*p);
    }

    printf("debug 4\n");

    int * array_line = calloc(num, sizeof(int));
    int * backup_ptr = array_line;

    start_string = end_string = (char *)word;

    printf("debug 5\n");

    while( (end_string = strchr(start_string, '\n')) ){
        int size_string = end_string - start_string + 1;
        char *string = calloc(size_string, sizeof(char));
        strncpy(string, start_string, size_string-1);
        // string[size_string] = '\0';
        // int size_string = end_string - start_string;
        // string = (char *)malloc(sizeof(char) * size_string + 1);
        // memset(string, 0, sizeof(char) * size_string + 1);
        // strncpy(string, start_string, size_string);
        // string[size_string+1] = '\0';
        num_lines++;

        char * token = strtok(string, " ");
        while( token != NULL ){
            if(strcmp(token, target) == 0){
                printf("lines : %d\n", num_lines);
                array_line[num-1] = num_lines;
                num ++;
                if( NULL == (array_line = (int*)realloc(array_line, num)) ){
                    free(backup_ptr);
                    fprintf(stderr, "Memory allocation is failed");
                    exit(1);
                }
                break;
            } 
            token = strtok(NULL, " ");
        }
        free(string);
        start_string = end_string + 1;
    }

    printf("debug 6\n");

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0){
        MPI_Recv(&received_num, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        received_array_line = calloc(received_num, sizeof(int));
    } else {
        MPI_Send(&num, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    printf("debug 7\n");

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0){
        MPI_Recv(&received_array_line, received_num, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Send(&array_line, num, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    printf("debug 8\n");

    if (rank == 0){
        for (int i=0; i<num; i++){
            printf("%d\n", array_line[i]);
        }

        for (int i=0; i<received_num-1; i++){
            printf("%d\n", num + received_array_line[i]);
        }
    }

    printf("debug 9\n");

    gettimeofday(&end, NULL);
    totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;

    printf("\nTotaltime = %f seconds\n", totaltime);

    fclose(fp);
    free(word);
    free(received_array_line);
    free(array_line);

    return 0;
}