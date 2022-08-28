#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <omp.h>
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
    omp_set_dynamic(0);     // Explicitly disable dynamic teams
    omp_set_num_threads(2); // Use 2 threads for all consecutive parallel regions
    int NUM_THREADS = 2;

    double start_time, file_time, tolower_time, search_time, end_time; 
    char filename[256];
    char target[256];
    int i, j;
    
    strcpy(filename, argv[1]);
    strcpy(target, argv[3]);
    int total_line = atoi(argv[2]);
    long line_size = getSpecificSize(filename, (int)total_line/2);

    start_time = omp_get_wtime();
    //////////////////////////////////// READ FILE ////////////////////////////////////
    struct stat sb;
    stat(argv[1], &sb);
    FILE *fp;
    char ** array_word = (char **)malloc(sizeof(char *) * NUM_THREADS);

    #pragma omp parallel shared(array_word) private(i, fp)
    {
        i = omp_get_thread_num();

        fp = fopen(filename, "r");
        if (fp == NULL) {
            printf("Error opening data file\n");
        }

        if (i%2 == 0){
            array_word[i] = malloc(line_size + 1);
            memset(array_word[i], 0, line_size + 1);

            fread(array_word[i], line_size, 1, fp);
            array_word[i][line_size + 1] = '\0';
        } else {
            array_word[i] = malloc(sb.st_size - line_size + 1);
            memset(array_word[i], 0, sb.st_size - line_size + 1);

            fseek(fp, line_size, SEEK_SET);

            fread(array_word[i], sb.st_size - line_size, 1, fp);
            array_word[i][sb.st_size - line_size + 1] = '\0';
        }
        
        fclose(fp);
    }
    file_time = omp_get_wtime();
    printf("Complete reading files in the array_word[i]\n");

    //////////////////////////////////// CONVERT TO LOWERCASE ////////////////////////////////////
    char* p;
    #pragma omp parallel for shared(array_word) private(i, p)
    for (i=0; i < NUM_THREADS; i++){
        // Convert word to lower case in place.
        for (p = array_word[i]; *p; p++) {
            *p = tolower(*p);
        }
    }
    tolower_time = omp_get_wtime();
    printf("Complete converting to lowercase\n");

    //////////////////////////////////// SEARCHING TARGET WORD ////////////////////////////////////
    char * start_string, *end_string, *token; // for strchr()
    int num[2] = {1, 1};
    int num_lines = 0;
    int ** array_lines = (int **) calloc(NUM_THREADS, sizeof(int*));

    #pragma omp parallel shared(array_word, array_lines, num) private(i, num_lines, token)
    {
        i = omp_get_thread_num();
        int * array_lines[i] = calloc(num[i], sizeof(int));
        int * backup_ptr = array_lines[i];

        start_string = end_string = (char *)array_word[i];

        while( (end_string = strchr(start_string, '\n')) ){
            int size_string = end_string - start_string + 1;
            char *string = calloc(size_string, sizeof(char));
            strncpy(string, start_string, size_string-1);
            num_lines++;

            while((token = strtok_r(string, " ", &string))){
                if(strcmp(token, target) == 0){
                    // printf("lines : %d\n", num_lines);
                    array_lines[i][num-1] = num_lines;
                    num[i] ++;
                    if((array_lines[i] = (int*)realloc(array_lines[i], sizeof(int)*num[i])) == NULL){
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
    }

    search_time = omp_get_wtime();
    printf("Complete searching target word\n");

    //////////////////////////////////// PRINT RESULT ////////////////////////////////////

    for (i = 0; i<NUM_THREADS; i++){
        for (j = 0; j < num[i]; j++){
            // Uncomment it if you want to print the target line
            // printf("%d\n", array_lines[i][j]);
        }
    }
    end_time = omp_get_wtime();

    //////////////////////////////////// TIME ////////////////////////////////////
    printf("\nTotaltime = %.6f seconds\n", end_time-start_time);
    printf("\nstart-file = %.6f seconds\n", file_time-start_time);
    printf("\nfile-tolower = %.6f seconds\n", tolower_time-file_time);
    printf("\ntolower-search  = %.6f seconds\n", search_time-tolower_time);
    printf("\nsearch-end  = %.6f seconds\n", end_time-search_time);

    //////////////////////////////////// FREE ////////////////////////////////////
    for(i=0; i<NUM_THREADS; i++){
        free(array_lines[i]);
        free(array_word[i]);
    }
    free(array_word);
    free(array_lines);

    return 0;
}
