#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/stat.h>

struct Node {
    struct Node* next;
    int value;
};

struct Front {
    struct Node* next;
    int sum;
    int num;
    double avg;
};

// Empty List of Node
struct Front* list_node;

void init(int max_key){
    int i;
    list_node = calloc(max_key, sizeof(struct Front));
    for (i = 0; i<max_key; i++){
        list_node[i].next = NULL;
        list_node[i].sum = 0;
        list_node[i].num = 0;
        list_node[i].avg = 0;
    }
}

void destroy(int max_key){
    int i;
    struct Node* remove;
    for (i=0; i<max_key; i++){
        if (list_node[i].num == 0) {
            continue;
        } else {
            while(list_node[i].next != NULL){
                remove = list_node[i].next;
                list_node[i].next = remove->next;
                free(remove);
            }
        }
    }
}

// Function to create a header linked list
void create(int key, int data){
    // Create a new node
    struct Node *new_node, *node;
    new_node = calloc(1, sizeof(struct Node));
    new_node->value = data;
    new_node->next = NULL;
    // If it is first node
    if (list_node[key].num == 0){
        //Init the start
        list_node[key].next = new_node;
        list_node[key].num++;
    } else {
        // Insert the node in the end
        node = list_node[key].next;
        list_node[key].next = new_node;
        new_node->next = node;

        list_node[key].num++;
    }
}

void aggregate(int max_key){
    int i, j;
    int sum, num;
    for(i=0; i<max_key; i++){
        if (list_node[i].next == NULL){
            continue;
        } else {
            num = list_node[i].num;
            sum = 0;
            struct Node* pt_node = list_node[i].next;
            for(j=0; j<num; j++){
                sum += pt_node->value;
                pt_node = pt_node->next;
            }
            list_node[i].sum = sum;
            list_node[i].avg = sum/num;
        }
        
    }
}

int main(int agrc, char** argv){
    struct stat sb;
    struct timeval start, end, file_t, create_t, agg_t;
    char filename[256];
    int key, value;
    int i;

    strcpy(filename, argv[1]);

    gettimeofday(&start, NULL);
    //////////////////////////////////// READ FILE ////////////////////////////////////

    FILE * fp;
    fp = fopen(filename, "r");
    if (fp == NULL){
        printf("Error opening data file\n");
    }

    stat(argv[1], &sb);
    char* word = malloc(sb.st_size+1);
    memset(word, 0, sb.st_size+1);
    word[sb.st_size+1] = '\0';

    // find max_key
    int max;
    fscanf(fp, "%d\n", &max);
    max = 1;                        // FOR AG
    printf("max: %d\n", max);

    fread(word, sb.st_size+1, 1, fp);
    word[sb.st_size+1] = '\0';

    printf("Complete reading files\n");
    gettimeofday(&file_t, NULL);
    //////////////////////////////////// INSERT DATA INTO NODE ////////////////////////////////////
    init(max);
    char * token = strtok(word, "\n");

    while( token != NULL ){
        sscanf(token, "<%d,%d>", &key, &value);
        key = 0;                    // FOR AG
        create(key, value);

        token = strtok(NULL, "\n");
    }

    printf("finish insert node\n");
    gettimeofday(&create_t, NULL);
    //////////////////////////////////// AGGREGATE ////////////////////////////////////
    aggregate(max);
    printf("finish aggregation\n");
    gettimeofday(&agg_t, NULL);

    //////////////////////////////////// PRINT RESULT ////////////////////////////////////
    float sum;
    int num;
    for(i=0; i<max; i++){
        if (list_node[i].num == 0){
            continue;
        } else {
            sum = list_node[i].sum;
            num = list_node[i].num;

            // Uncomment it if you want to print the result
            // printf("final key: %d | num: %d | sum: %f | avg: %f\n", i, num, sum, sum/num);  
        }
    }

    printf("finish print result\n");
    gettimeofday(&end, NULL);

    //////////////////////////////////// TIME ////////////////////////////////////
    double totaltime, file_time, create_time, agg_time, end_time;
    totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;
    file_time = (((file_t.tv_usec - start.tv_usec) / 1.0e6 + file_t.tv_sec - start.tv_sec) * 1000) / 1000;
    create_time = (((create_t.tv_usec - file_t.tv_usec) / 1.0e6 + create_t.tv_sec - file_t.tv_sec) * 1000) / 1000;
    agg_time = (((agg_t.tv_usec - create_t.tv_usec) / 1.0e6 + agg_t.tv_sec - create_t.tv_sec) * 1000) / 1000;
    end_time = (((end.tv_usec - agg_t.tv_usec) / 1.0e6 + end.tv_sec - agg_t.tv_sec) * 1000) / 1000;

    printf("\nTotaltime = %f seconds\n", totaltime);
    printf("\n start-file = %f seconds\n", file_time);
    printf("\n file-create = %f seconds\n", create_time);
    printf("\n create-aggregate = %f seconds\n", agg_time);
    printf("\n aggregate-end = %f seconds\n", end_time);

    destroy(max);
    printf("finish destroy\n");

    free(word);
    free(list_node);

    return 0;
}