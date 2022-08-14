#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <mpi.h>
#include <sys/time.h>
#include <sys/stat.h>

// long getSpecificSize(char *name, int target_line){
//     FILE *ptr;
//     long size=0;
//     int line=0;
//     char c;

//     ptr=fopen(name,"r");
//     if (ptr == NULL) {
//         printf("Error opening data file\n");
//     }
    
//     while((c=fgetc(ptr))!=EOF){
//         if( line != target_line ) {
//             size ++;
//             if( c == '\n') {
//                 line ++;
//             }
//         } else {
//             break;
//         }
//     }

//     fseek(ptr, 0, SEEK_SET);
//     fclose(ptr);

//     return(size);
// }

int main(int argc, char** argv) {
    MPI_Init( &argc, &argv );
    int rank, size;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );

    const int ONEGB = 1024 * 1024 * 1024;

    struct stat sb;
    struct timeval start, end, execution_1, mpi, file, execution_2;
    double totaltime, start_file, file_exe1, exe1_mpi, mpi_exe2;
    char filename[256];
    char target[256];
    int ch = 0;
    int num_lines = 0;
    int num = 1;
    int received_num;
    int divider = 0;
    int *received_array_line;
    int total_line;
    long malloc_size;
    char * word;
    char * start_string, *end_string; // for strchr()

    strcpy(filename, argv[1]);
    strcpy(target, argv[3]);

    gettimeofday(&start, NULL);

    stat(argv[1], &sb);

    total_line = atoi(argv[2]);
    // line_size = getSpecificSize(filename, (int)total_line/2);

    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening data file\n");
    }

    // if (rank == 0) {
    //     word = malloc(line_size + 1);
    //     memset(word, 0, line_size + 1);
    
    //     fread(word, line_size, 1, fp);
    //     word[line_size + 1] = '\0';

    // } else {
    //     word = malloc(sb.st_size - line_size + 1);
    //     memset(word, 0, sb.st_size - line_size + 1);

    //     fseek(fp, line_size, SEEK_SET);

    //     fread(word, sb.st_size - line_size, 1, fp);
    //     word[sb.st_size - line_size + 1] = '\0';
    // }

    if (rank == 0) {
        divider = 0;
    } else {
        divider = 1;
    }

    fseek(fp, 0, SEEK_SET);
    word = malloc(sb.st_size/2 + 1);
    memset(word, 0, sb.st_size/2 + 1);
    malloc_size = sb.st_size/2 + 1;
    char* tmp_string;
    int i;
    // If it is not EOF but a specific number of lines
    for (i=0; i<total_line; i++){
        if (i%2 == divider){
            // If not EOF and fgets fails -> buffer is out of mem. 
            if (fgets(word+strlen(word), sb.st_size/2, fp) == NULL && !feof(fp)) {
                printf("out of buffer(word), Increase buffer size using realloc()\n");
                word = (char*)realloc(word, malloc_size+ONEGB);
                malloc_size += ONEGB;
                fgets(word+strlen(word), sb.st_size/2, fp);          
            }
        } else {
            fscanf(fp, "%s\n", tmp_string);
        }
        
    }

    printf("%s\n", word);

    gettimeofday(&file, NULL);

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
        num_lines++;

        char * token = strtok(string, " ");
        while( token != NULL ){
            if(strcmp(token, target) == 0){
                // printf("lines : %d\n", num_lines);
                array_line[num-1] = num_lines;
                num ++;
                if( NULL == (array_line = (int*)realloc(array_line, sizeof(int)*num)) ){
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
    gettimeofday(&execution_1, NULL);

    if (rank == 0){
        MPI_Recv(&received_num, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        received_array_line = calloc(received_num, sizeof(int));
    } else {
        MPI_Send(&num, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    // MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0){
        MPI_Recv(received_array_line, received_num, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Send(array_line, num, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    gettimeofday(&mpi, NULL);

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
    
    gettimeofday(&execution_2, NULL);
    gettimeofday(&end, NULL);

    if (rank == 0){
        start_file = (((file.tv_usec - start.tv_usec) / 1.0e6 + file.tv_sec - start.tv_sec) * 1000) / 1000;
        file_exe1 = (((execution_1.tv_usec - file.tv_usec) / 1.0e6 + execution_1.tv_sec - file.tv_sec) * 1000) / 1000;
        exe1_mpi = (((mpi.tv_usec - execution_1.tv_usec) / 1.0e6 + mpi.tv_sec - execution_1.tv_sec) * 1000) / 1000;
        mpi_exe2 = (((execution_2.tv_usec - mpi.tv_usec) / 1.0e6 + execution_2.tv_sec - mpi.tv_sec) * 1000) / 1000;
        totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;

        printf("start_file = %f seconds\n", start_file);
        printf("file_exe1 = %f seconds\n", file_exe1);
        printf("exe1_mpi = %f seconds\n", exe1_mpi);
        printf("mpi_exe2 = %f seconds\n", mpi_exe2);
        printf("\nTotaltime = %f seconds\n", totaltime);
    }

    fclose(fp);
    free(word);
    free(array_line);
    
    MPI_Finalize();

    return 0;
}