#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/stat.h>

typedef struct {
    char* word;
    int count;
} count;

int main(int argc, char** argv) {
    struct stat sb;
    struct timeval start, end;
    double totaltime;
    char filename[256];
    char target[256];
    int ch = 0;
    int num_lines = 0;

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

    char* word = malloc(sb.st_size+1);
    memset(word, 0, sb.st_size+1);
    word[sb.st_size+1] = '\0';
    //char word[101]; // 100-char word plus NUL byte

    //fscanf(fp,"%100s", word);
    fread(word, sb.st_size+1, 1, fp);
    word[sb.st_size+1] = '\0';
    printf("sb.st_size : %lu\n", sb.st_size);
    printf("word size : %lu\n", malloc_usable_size(word));

    // Convert word to lower case in place.
    for (char* p = word; *p; p++) {
        *p = tolower(*p);
    }

    while(!feof(fp)){
        ch = fgetc(fp);
        if(ch == '\n'){
            num_lines++;
        }
    }

    char * lines = strtok(word, '\n');

    while( lines != NULL ) {
        num_lines++;
        char * token = strtok(lines, " ");
        if(strcmp(token, target) == 0){

        } else {
            
        }
        
    }
    gettimeofday(&end, NULL);
    totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;

    printf("\nTotaltime = %f seconds\n", totaltime);

    fclose(fp);
    free(word);

    return 0;
}
