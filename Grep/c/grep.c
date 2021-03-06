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
    struct timeval start, end;
    double totaltime;
    char filename[256];
    char target[256];
    int ch = 0;
    int num_lines = 0;
    char * start_string, *end_string; // for strchr()

    strcpy(filename, argv[1]);
    strcpy(target, argv[2]);

    gettimeofday(&start, NULL);

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
    printf("sb.st_size : %lu\n", sb.st_size);
    printf("word size : %lu\n", malloc_usable_size(word));

    // Convert word to lower case in place.
    for (char* p = word; *p; p++) {
        *p = tolower(*p);
    }

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
                break;
            } 
            token = strtok(NULL, " ");
        }
        free(string);
        start_string = end_string + 1;
    }
    gettimeofday(&end, NULL);
    totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;

    printf("\nTotaltime = %f seconds\n", totaltime);

    fclose(fp);
    free(word);

    return 0;
}
