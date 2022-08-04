#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <mpi.h>
#include <sys/time.h>
#include <sys/stat.h>

int getTotalLine(char *name){
    FILE *ptr;
    int line=0;
    char c;

    ptr=fopen(name,"r");
    if (ptr == NULL) {
        printf("Error opening data file\n");
    }

    while((c=fgetc(ptr))!=EOF)
        if(c=='\n') line++;

    fseek(ptr, 0, SEEK_SET);
    fclose(ptr);

    return(line);
}

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

// for MPI, separate Front struct to Front(not included sum, avg) struct & int sum, num array
struct Node {
    struct Node* next;
    int value;
};

struct Front {
    struct Node* next;
    int num;
};

// Empty List of Node
struct Front* list_node;
int* sum_array;
int* num_array;
int* received_sum_array;
int* received_num_array;

// Function to initialize list_node(struct Front) & sum_array, num_array
void init(int max_key){
    int i;

    list_node = calloc(max_key, sizeof(struct Front));
    sum_array = calloc(max_key, sizeof(int));
    num_array = calloc(max_key, sizeof(int));
    received_sum_array = calloc(max_key, sizeof(int));
    received_num_array = calloc(max_key, sizeof(int));

    for (i = 0; i<max_key; i++){
        list_node[i].next = NULL;
        list_node[i].num = 0;
    }
}

// Function to free Node linked list_node
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
                printf("free node | key: %d, value: %d\n", i, remove->value);
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
    printf("size of created new_node : %lu\n", malloc_usable_size(new_node));
    printf("size of created new_node/sizeof(Node) : %lu\n", malloc_usable_size(new_node)/sizeof(struct Node));
    printf("addr of new_node's pointer : %p\n", new_node);
    // If it is first node
    if (list_node[key].num == 0){
        //Init the start
        printf("first node! \n");
        list_node[key].next = new_node;
        list_node[key].num++;
    } else {
        //Insert the node in the end
        printf("insert node! \n");
        node = list_node[key].next;
        while(node->next != NULL){
            node = node->next;
        }
        node->next = new_node;
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
            sum_array[i] = sum;
            num_array[i] = num;
            printf("key: %d | num: %d | sum: %d | \n", i, num, sum);
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init( &argc, &argv );
    int rank, size;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );

    struct stat sb;
    struct timeval start, end;
    char filename[256];
    int key, value;
    double totaltime;
    int total_line;
    long line_size;
    char* word;
    int max = 0;

    strcpy(filename, argv[1]);

    gettimeofday(&start, NULL);

    total_line = getTotalLine(filename);
    total_line -= 1;
    line_size = getSpecificSize(filename, (int)total_line/2);
    printf("total_line : %d\n", total_line);
    printf("line_size : %lu\n", line_size);

    FILE * fp;
    fp = fopen(filename, "r");
    if (fp == NULL){
        printf("Error opening data file\n");
    }

    stat(argv[1], &sb);

    if (rank == 0){
        word = malloc(line_size + 1);
        memset(word, 0, line_size + 1);

        fseek(fp, 0, SEEK_SET);

        fscanf(fp, "%d\n", &max);

        fread(word, line_size + 1, 1, fp);
        word[line_size + 1] = '\0';

        // find max_key
        // sscanf(word, "%d\n", &max);
        printf("(rank 0)max: %d\n", max);

        MPI_Send(&max, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

    } else {
        word = malloc(sb.st_size - line_size + 1);
        memset(word, 0, sb.st_size - line_size + 1);

        fseek(fp, 0, SEEK_SET);
        fseek(fp, line_size, SEEK_SET);

        fread(word, sb.st_size - line_size, 1, fp);
        word[sb.st_size - line_size + 1] = '\0';

        int received_max;
        MPI_Recv(&received_max, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("(rank 1)max : %d\n", received_max);
        max = received_max;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    printf("line_size : %lu\n", line_size);
    printf("sb.st_size : %lu\n", sb.st_size);
    printf("word size : %lu\n", malloc_usable_size(word));

    init(max);
    printf("size of list_node[] : %lu\n", malloc_usable_size(list_node)/sizeof(struct Front));
    printf("size of list_node : %lu\n", malloc_usable_size(list_node));
    printf("addr of list_node's pointer : %p\n", list_node);

    char * token = strtok(word, "\n");

    while( token != NULL ){
        sscanf(token, "<%d,%d>", &key, &value);
        printf("%s\n", token);
        printf("insert node key: %d, value: %d\n", key, value);
        create(key, value);

        token = strtok(NULL, "\n");
    }
    printf("finish insert node\n");
    aggregate(max);
    printf("finish aggregation\n");

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Reduce(sum_array, received_sum_array, max, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(num_array, received_num_array, max, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        int i, j;
        float sum;
        int num;
        for(i=0; i<max; i++){
            if (received_num_array[i] == 0){
                continue;
            } else {
                num = received_num_array[i];
                sum = received_sum_array[i];
                printf("final key: %d | num: %d | sum: %f | avg: %f\n", i, num, sum, sum/num);
            }
        }
    }

    destroy(max);
    printf("finish destroy\n");

    free(word);
    free(list_node);
    free(sum_array);
    free(num_array);
    free(received_num_array);
    free(received_sum_array);

    gettimeofday(&end, NULL);
    totaltime = (((end.tv_usec - start.tv_usec) / 1.0e6 + end.tv_sec - start.tv_sec) * 1000) / 1000;

    printf("\nTotaltime = %f seconds\n", totaltime);

    MPI_Finalize();

    return 0;
}