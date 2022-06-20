#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MAX_UNIQUES 6000000

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
    struct timeval start,end;
    double totaltime;
    char filename[256];
    strcpy(filename, argv[1]);
    // The hcreate hash table doesn't provide a way to iterate, so
    // store the words in an array too (also used for sorting).
    count* words = calloc(MAX_UNIQUES, sizeof(count));
    int num_words = 0;

    gettimeofday(&start, NULL);

    // Allocate hash table.
    if (hcreate(MAX_UNIQUES) == 0) {
        fprintf(stderr, "error creating hash table\n");
        return 1;
    }

    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening data file\n");
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);

    printf("size : %ld\n", size+1);

    //char word[101]; // 100-char word plus NUL byte
    char* word = malloc(size + 1);
    if(NULL == word) { 
        printf("size exceed\n");
        return 0;
    }
    memset(word, 0, size+1);

    fseek(fp, 0, SEEK_SET);

    while (!feof(fp)) {
        //fscanf(fp,"%100s", word);
        fread(word, size, 1, fp);
        // Convert word to lower case in place.
        for (char* p = word; *p; p++) {
            *p = tolower(*p);
        }

        char * token = strtok(word, " ");

        while( token != NULL ) {
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
    }

    // Iterate once to add counts to words list, then sort.
    for (int i = 0; i < num_words; i++) {
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
    for (int i = 0; i < num_words; i++) {
        // printf("%s %d\n", words[i].word, words[i].count);
    }

    gettimeofday(&end, NULL);
    totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;

    printf("\nTotaltime = %f seconds\n", totaltime);

    fclose(fp);
    free(word);

    return 0;
}
