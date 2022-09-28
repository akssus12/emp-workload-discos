#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <omp.h>
#include <sys/time.h>
#include <sys/stat.h>

int main(int argc, char** argv) {
    omp_set_dynamic(0);     // Explicitly disable dynamic teams
    omp_set_num_threads(2); // Use 2 threads for all consecutive parallel regions

    double start_time, file_time, tolower_time, search_time, end_time, free_time; 
    char filename[256];
    char target[256];
    int i;
    
    strcpy(filename, argv[1]);
    strcpy(target, argv[3]);
    int total_line = atoi(argv[2]);

    start_time = omp_get_wtime();
    //////////////////////////////////// Insert String to Array //////////////////////////////////// 
    
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

    //////////////////////////////////// SEARCH TARGET WORD ////////////////////////////////////

    char *token; // for strchr()
    char **free_array = (char **)calloc(total_line, sizeof(char*));

    int* result_arr = (int *)calloc(total_line, sizeof(int));
    #pragma omp parallel for schedule(dynamic) shared(array_word1, free_array, result_arr) private(i, token)
    for (i=0; i<total_line; i++){
        free_array[i] = array_word1[i];

        while((token = strtok_r(array_word1[i], " ", &array_word1[i]))) {
            if(strcmp(token, target) == 0){
                result_arr[i] = 1;
                break;
            }
        }
    }
    search_time = omp_get_wtime();
    printf("Complete searching target word\n");

    //////////////////////////////////// PRINT RESULT ////////////////////////////////////

    for(i = 0; i<total_line; i++){
        if(result_arr[i] == 1){
            // printf("line : %d\n", i+1);
        }
    }
    end_time = omp_get_wtime();

    //////////////////////////////////// FREE ////////////////////////////////////
    for(i=0; i<total_line; i++){
        free(free_array[i]);
    }
    free(array_word1);
    free(result_arr);
    printf("finish free\n");
    free_time = omp_get_wtime();

    //////////////////////////////////// TIME ////////////////////////////////////
    printf("\nTotaltime = %.6f seconds\n", free_time-start_time);
    printf("\nstart_file = %f seconds\n", file_time-start_time);
    printf("\nfile-tolower = %.6f seconds\n", tolower_time-file_time);
    printf("\ntolower-search  = %.6f seconds\n", search_time-tolower_time);
    printf("\nsearch-end  = %.6f seconds\n", end_time-search_time);
    printf("\nend-free = %f seconds\n", free_time-end_time);

    

    return 0;
}

// size_t getSpecificSize_getline(char *name, int target_line){
//     FILE *ptr;
//     size_t size =0;
//     int line=0;
//     char *tmp_line = NULL;
//     size_t len = 0;
//     ssize_t read;
//     //char c;

//     ptr=fopen(name, "r");
//     if (ptr == NULL) {
//         printf("Error opening data file\n");
//     }
    
//     while((read = getline(&tmp_line, &len, ptr)) != -1) {
//         if( line != target_line ){
//             size += (size_t)read;
//             line++;
//         } else { 
//             break;
//         }
//     }
//     free(tmp_line);

//     fseek(ptr, 0, SEEK_SET);
//     fclose(ptr);

//     return(size);
// }

// int main(int argc, char** argv) {
//     omp_set_dynamic(0);     // Explicitly disable dynamic teams
//     omp_set_num_threads(2); // Use 2 threads for all consecutive parallel regions
//     int NUM_THREADS = 2;

//     double start_time, getsize_time, file_time, tolower_time, search_time, end_time; 
//     char filename[256];
//     char target[256];
//     int i, j;
    
//     strcpy(filename, argv[1]);
//     strcpy(target, argv[3]);
//     int total_line = atoi(argv[2]);

//     start_time = omp_get_wtime();
//     //////////////////////////////////// GET THE FILE SIZE UP TO A SPECIFIC LINE //////////////////////////////////// 
    
//     struct stat sb;
//     stat(argv[1], &sb);

//     size_t line_size = getSpecificSize_getline(filename, (int)total_line/2);
//     getsize_time = omp_get_wtime();
//     //////////////////////////////////// READ FILE ////////////////////////////////////
//     FILE *fp;
//     char ** array_word = (char **)malloc(sizeof(char *) * NUM_THREADS);

//     #pragma omp parallel shared(array_word) private(i, fp)
//     {
//         i = omp_get_thread_num();

//         fp = fopen(filename, "r");
//         if (fp == NULL) {
//             printf("Error opening data file\n");
//         }

//         if (i%2 == 0){
//             array_word[i] = malloc(line_size + 1);
//             memset(array_word[i], 0, line_size + 1);

//             fread(array_word[i], line_size, 1, fp);
//             array_word[i][line_size + 1] = '\0';
//         } else {
//             array_word[i] = malloc(sb.st_size - line_size + 1);
//             memset(array_word[i], 0, sb.st_size - line_size + 1);

//             fseek(fp, line_size, SEEK_SET);

//             fread(array_word[i], sb.st_size - line_size, 1, fp);
//             array_word[i][sb.st_size - line_size + 1] = '\0';
//         }
        
//         fclose(fp);
//     }
//     file_time = omp_get_wtime();
//     printf("Complete reading files in the array_word[i]\n");

//     //////////////////////////////////// CONVERT TO LOWERCASE ////////////////////////////////////
//     char* p;
//     #pragma omp parallel for shared(array_word) private(i, p)
//     for (i=0; i < NUM_THREADS; i++){
//         // Convert word to lower case in place.
//         for (p = array_word[i]; *p; p++) {
//             *p = tolower(*p);
//         }
//     }
//     tolower_time = omp_get_wtime();
//     printf("Complete converting to lowercase\n");

//     //////////////////////////////////// SEARCHING TARGET WORD ////////////////////////////////////
//     char * start_string, *end_string, *token, *free_string, *string; // for strchr()
//     int num[2] = {1, 1};
//     int num_lines = 0;
//     int ** array_lines = (int **) calloc(NUM_THREADS, sizeof(int*));

//     #pragma omp parallel shared(array_word, array_lines, num) private(i, num_lines, token, start_string, end_string, string, free_string)
//     {
//         i = omp_get_thread_num();
//         array_lines[i] = calloc(num[i], sizeof(int));
//         int * backup_ptr = array_lines[i];
//         num_lines = 0;

//         start_string = end_string = (char *)array_word[i];

//         while( (end_string = strchr(start_string, '\n')) ){
//             int size_string = end_string - start_string + 1;
//             string = calloc(size_string, sizeof(char));
//             strncpy(string, start_string, size_string-1);
//             free_string = string;
//             num_lines++;

//             while((token = strtok_r(string, " ", &string))){
//                 if(strcmp(token, target) == 0){
//                     // printf("lines : %d\n", num_lines);
//                     array_lines[i][num[i]-1] = num_lines;
//                     num[i] ++;
//                     if((array_lines[i] = (int*)realloc(array_lines[i], sizeof(int)*num[i])) == NULL){
//                         free(backup_ptr);
//                         fprintf(stderr, "Memory allocation is failed");
//                         exit(1);
//                     }
//                     break;
//                 } 
//             }
//             free(free_string);
//             start_string = end_string + 1;
//         }
//     }

//     search_time = omp_get_wtime();
//     printf("Complete searching target word\n");

//     //////////////////////////////////// PRINT RESULT ////////////////////////////////////

//     for (i = 0; i<NUM_THREADS; i++){
//         for (j = 0; j < num[i]; j++){
//             // Uncomment it if you want to print the target line
//             // printf("%d\n", array_lines[i][j]);
//         }
//     }
//     end_time = omp_get_wtime();

//     //////////////////////////////////// TIME ////////////////////////////////////
//     printf("\nTotaltime = %.6f seconds\n", end_time-start_time);
//     printf("start_getsize = %f seconds\n", getsize_time-start_time);
//     printf("getsize_file = %f seconds\n", file_time-getsize_time);
//     printf("\nfile-tolower = %.6f seconds\n", tolower_time-file_time);
//     printf("\ntolower-search  = %.6f seconds\n", search_time-tolower_time);
//     printf("\nsearch-end  = %.6f seconds\n", end_time-search_time);

//     //////////////////////////////////// FREE ////////////////////////////////////
//     for(i=0; i<NUM_THREADS; i++){
//         free(array_lines[i]);
//         free(array_word[i]);
//     }
//     free(array_word);
//     free(array_lines);

//     return 0;
// }
