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
    FILE *fp;
    int line=0;
    char c;

    fp=fopen(name,"r");

    while((c=fgetc(fp))!=EOF)
        if(c=='\n') line++;
    fclose(fp);

    return(line);
}

int getSpecificSize(char *name, int target_line){
    FILE *fp;
    int size=0;
    int line=0;
    char c;

    fp=fopen(name,"r");
    
    while((c=fgetc(fp))!=EOF){
        if( line != target_line ) {
            size ++;
            if( c == '\n') {
                line ++;
            }
        } else {
            break;
        }
    }    
    fclose(fp);

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

    strcpy(filename, argv[1]);
    strcpy(target, argv[2]);

    gettimeofday(&start, NULL);

    stat(argv[1], &sb);

    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening data file\n");
    }

    total_line = getTotalLine(filename);
    line_size = getSpecificSize(filename, (int)total_line/2);
    if (rank == 0) {
        word = malloc(line_size + 1);
        memset(word, 0, line_size + 1);
    
        fread(word, line_size, 1, fp);
        word[line_size + 1] = '\0';

    } else {
        word = malloc(sb.st_size - line_size + 1);
        memset(word, 0, sb.st_size - line_size + 1);

        fread(word, sb.st_size - line_size, 1, fp);
        word[sb.st_size - line_size + 1] = '\0';
    }

    printf("line_size : %d\n", line_size);
    printf("sb.st_size : %lu\n", sb.st_size);
    printf("word size : %lu\n", malloc_usable_size(word));

    // Convert word to lower case in place.
    for (char* p = word; *p; p++) {
        *p = tolower(*p);
    }

    int * array_line = calloc(num, sizeof(int));
    int * backup_ptr = array_line;

    start_string = end_string = (char *)word;

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

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0){
        MPI_Recv(&received_num, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        received_array_line = calloc(received_num, sizeof(int));
    } else {
        MPI_Send(&num, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0){
        MPI_Recv(&received_array_line, received_num, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Send(&array_line, num, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    for (int i=0; i<num; i++){
        printf("%d\n", array_line[i]);
    }

    for (int i=0; i<received_num-1; i++){
        printf("%d\n", num + received_array_line[i]);
    }

    gettimeofday(&end, NULL);
    totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;

    printf("\nTotaltime = %f seconds\n", totaltime);

    fclose(fp);
    free(word);

    return 0;
}