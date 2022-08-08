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
    int i, j;
    struct Node* remove;
    for (i=0; i<max_key; i++){
        if (list_node[i].num == 0) {
            continue;
        } else {
            while(list_node[i].next != NULL){
                remove = list_node[i].next;
                list_node[i].next = remove->next;
                // remove->next=NULL;
                // printf("free node | key: %d, value: %d\n", i, remove->value);
                free(remove);
                // remove = NULL;
            }
            // for (j=0; j<list_node[i].num && list_node[i].next != NULL; j++){
            //     remove = list_node[i].next;
            //     list_node[i].next = remove->next;
            //     printf("free node | key: %d, value: %d\n", i, remove->value);
            //     free(remove);
            //     remove = NULL;
            // }
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
    // printf("size of created new_node : %lu\n", malloc_usable_size(new_node));
    // printf("size of created new_node/sizeof(Node) : %lu\n", malloc_usable_size(new_node)/sizeof(struct Node));
    // printf("addr of new_node's pointer : %p\n", new_node);
    // If it is first node
    if (list_node[key].num == 0){
        //Init the start
        // printf("first node! \n");
        list_node[key].next = new_node;
        list_node[key].num++;
    } else {
        // Insert the node in the end
        // printf("insert node! \n");
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
            printf("key: %d | num: %d | sum: %d | avg: %lf\n", i, num, sum, list_node[i].avg);
        }
        
    }
}

int main(int agrc, char** argv){
    struct stat sb;
    struct timeval start, end;
    char filename[256];
    int key, value;
    double totaltime;

    strcpy(filename, argv[1]);

    gettimeofday(&start, NULL);

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
    max = 1;                    // FOR AG
    printf("max: %d\n", max);

    fread(word, sb.st_size+1, 1, fp);
    word[sb.st_size+1] = '\0';
    printf("sb.st_size : %lu\n", sb.st_size);
    printf("word size : %lu\n", malloc_usable_size(word));

    init(max);
    // printf("size of list_node[] : %lu\n", malloc_usable_size(list_node)/sizeof(struct Front));
    // printf("size of list_node : %lu\n", malloc_usable_size(list_node));
    // printf("addr of list_node's pointer : %p\n", list_node);

    char * token = strtok(word, "\n");

    while( token != NULL ){
        sscanf(token, "<%d,%d>", &key, &value);
        // printf("%s\n", token);
        // printf("insert node key: %d, value: %d\n", key, value);
        key = 0;                // FOR AG
        create(key, value);

        token = strtok(NULL, "\n");
    }
    printf("finish insert node\n");
    aggregate(max);
    printf("finish aggregation\n");
    destroy(max);
    printf("finish destroy\n");

    free(word);
    free(list_node);

    gettimeofday(&end, NULL);
    totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;

    printf("\nTotaltime = %f seconds\n", totaltime);

    return 0;
}