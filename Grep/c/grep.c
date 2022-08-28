#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/stat.h>

int main(int argc, char** argv) {
    struct stat sb;
    struct timeval start, end, file_t, tolower_t;
    
    char filename[256];
    char target[256];
    int num_lines = 0;
    
    strcpy(filename, argv[1]);
    strcpy(target, argv[2]);

    gettimeofday(&start, NULL);
    //////////////////////////////////// READ FILE ////////////////////////////////////

    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening data file\n");
    }

    stat(argv[1], &sb);
    char* word = malloc(sb.st_size+1);
    memset(word, 0, sb.st_size+1);
    word[sb.st_size+1] = '\0';

    //fscanf(fp,"%100s", word);
    fread(word, sb.st_size+1, 1, fp);
    word[sb.st_size+1] = '\0';
    
    fclose(fp);

    gettimeofday(&file_t, NULL);
    printf("Complete reading files in the array_word[i]\n");

    //////////////////////////////////// CONVERT TO LOWERCASE ////////////////////////////////////

    // Convert word to lower case in place.
    char* p;
    for (p = word; *p; p++) {
        *p = tolower(*p);
    }
    gettimeofday(&tolower_t, NULL);
    printf("Complete converting to lowercase\n");

    //////////////////////////////////// SEARCHING TARGET WORD ////////////////////////////////////
    char * start_string, *end_string; // for strchr()
    start_string = end_string = (char *)word;

    while( (end_string = strchr(start_string, '\n')) ){
        int size_string = end_string - start_string + 1;
        char *string = calloc(size_string, sizeof(char));
        strncpy(string, start_string, size_string-1);
        num_lines++;

        char * token = strtok(string, " ");
        while( token != NULL ){
            if(strcmp(token, target) == 0){
                // Uncomment it if you want to print the target line
                // printf("lines : %d\n", num_lines);
                break;
            } 
            token = strtok(NULL, " ");
        }
        free(string);
        start_string = end_string + 1;
    }
    gettimeofday(&end, NULL);
    printf("Complete searching target word\n");

    //////////////////////////////////// TIME ////////////////////////////////////
    double totaltime, file_time, tolower_time, search_time;
    totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;
    file_time = (((file_t.tv_usec - start.tv_usec) / 1.0e6 + file_t.tv_sec - start.tv_sec) * 1000) / 1000;
    tolower_time = (((tolower_t.tv_usec - file_t.tv_usec) / 1.0e6 + tolower_t.tv_sec - file_t.tv_sec) * 1000) / 1000;
    search_time = (((end.tv_usec - tolower_t.tv_usec) / 1.0e6 + end.tv_sec - tolower_t.tv_sec) * 1000) / 1000;

    printf("\nTotaltime = %f seconds\n", totaltime);
    printf("\nstart-file = %f seconds\n", file_time);
    printf("\nfile-tolower = %f seconds\n", tolower_time);
    printf("\ntolower-search&end  = %f seconds\n", search_time);

    //////////////////////////////////// FREE ////////////////////////////////////
    free(word);

    return 0;
}
