#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdbool.h>

#define MAX_UNIQUES 10000000

typedef struct {
    char* word;
    int count;
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

    struct timeval start,end;
    double totaltime;
    char filename[256];
    strcpy(filename, argv[1]);
    // The hcreate hash table doesn't provide a way to iterate, so
    // store the words in an array too (also used for sorting).
    count* words = calloc(MAX_UNIQUES, sizeof(count));
    int num_words = 0;
    bool is_alpha = true;
    int i;

    char *start_string, *end_string; // for strchr()

    gettimeofday(&start, NULL);

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

    char* word = malloc(sb.st_size+1);
    memset(word, 0, sb.st_size+1);
    word[sb.st_size+1] = '\0';
    //char word[101]; // 100-char word plus NUL byte

    //fscanf(fp,"%100s", word);
    fread(word, sb.st_size+1, 1, fp);
    word[sb.st_size+1] = '\0';
    printf("sb.st_size : %lu\n", sb.st_size);
    printf("word size : %lu\n", malloc_usable_size(word));
    char* p;
    // Convert word to lower case in place.
    for (p = word; *p; p++) {
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
                words[num_words].word = item.key;
                num_words++;
                token = strtok(NULL, " ");
            }
        }
        free(string);
        start_string = end_string + 1;
    }

    // Iterate once to add counts to words list, then sort.
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
    // Iterate again to print output.
    // for (int i = 0; i < num_words; i++) {
    //     printf("%s %d\n", words[i].word, words[i].count);
    // }

    gettimeofday(&end, NULL);
    totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;

    printf("total_words : %d\n", num_words);
    printf("\nTotaltime = %f seconds\n", totaltime);

    fclose(fp);
    free(word);
    free(words);

    return 0;
}