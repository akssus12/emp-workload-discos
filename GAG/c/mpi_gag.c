#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <mpi.h>
#include <sys/time.h>
#include <sys/stat.h>

#if SIZE_MAX == UCHAR_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
   #error "what is happening here?"
#endif

size_t* getSpecificSize_getline(char *name, int target_line){
    FILE *ptr;
    size_t* size = calloc(2, sizeof(size_t));
    int line=0;
    char *tmp_line = NULL;
    size_t len = 0;
    ssize_t read;

    ptr=fopen(name,"r");
    if (ptr == NULL) {
        printf("Error opening data file\n");
    }
    
    while((read = getline(&tmp_line, &len, ptr)) != -1) {
        size[1] += (size_t)read;
        line ++;
        
        if (line >= 1){
            if( line != target_line ) {
                size[0] += (size_t)read;
            } else
                break;
        } 
    }
    free(tmp_line);

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
            sum_array[i] = sum;
            num_array[i] = num;
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init( &argc, &argv );
    int rank, size;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );

    double start_time, getsize_time, file_time, create_time, agg_time, reduce_time, end_time, free_time;
    char filename[256];
    int key, value;
    int total_line;
    
    char* word;
    int max = 0;

    strcpy(filename, argv[1]);
    total_line = atoi(argv[2]);
    total_line -= 1;

    start_time = MPI_Wtime();
    //////////////////////////////////// GET THE FILE SIZE UP TO A SPECIFIC LINE //////////////////////////////////// 
    struct stat sb;
    stat(argv[1], &sb);

    size_t* line_size = calloc(2, sizeof(size_t));
    size_t* received_line_size = calloc(2, sizeof(size_t));

    if ( rank == 0 ){
        line_size = getSpecificSize_getline(filename, (int)total_line/2);
        MPI_Send(line_size, 2, my_MPI_SIZE_T, 1, 0, MPI_COMM_WORLD);
    } else {
        MPI_Recv(received_line_size, 2, my_MPI_SIZE_T, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    getsize_time = MPI_Wtime();
    //////////////////////////////////// READ FILE ////////////////////////////////////

    FILE * fp;
    fp = fopen(filename, "r");
    if (fp == NULL){
        printf("Error opening data file\n");
    }

    if (rank == 0){
        word = malloc(line_size[rank] + 1);
        memset(word, 0, line_size[rank] + 1);

        fseek(fp, 0, SEEK_SET);
        fscanf(fp, "%d\n", &max);

        fread(word, line_size[rank] + 1, 1, fp);
        word[line_size[rank]] = '\0';

        MPI_Send(&max, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    } else {
        word = malloc(sb.st_size - received_line_size[rank] + 1);
        memset(word, 0, sb.st_size - received_line_size[rank] + 1);

        fseek(fp, 0, SEEK_SET);
        fseek(fp, received_line_size[rank], SEEK_SET);

        fread(word, sb.st_size - received_line_size[rank], 1, fp);
        word[sb.st_size - received_line_size[rank]] = '\0';

        int received_max;
        MPI_Recv(&received_max, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        max = received_max;
    }
    free(line_size);
    free(received_line_size);
    fclose(fp);

    file_time = MPI_Wtime();
    printf("%d : Complete reading files in the word\n", rank);
    //////////////////////////////////// INSERT DATA INTO NODE ////////////////////////////////////
    init(max);

    char * token = strtok(word, "\n");

    while( token != NULL ){
        sscanf(token, "<%d,%d>", &key, &value);
        create(key, value);

        token = strtok(NULL, "\n");
    }
    printf("%d : finish insert node\n", rank);
    create_time = MPI_Wtime();
    //////////////////////////////////// AGGREGATE ////////////////////////////////////

    aggregate(max);
    printf("%d : finish aggregation\n", rank);

    agg_time = MPI_Wtime();

    //////////////////////////////////// REDUCE ////////////////////////////////////
    MPI_Reduce(sum_array, received_sum_array, max, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(num_array, received_num_array, max, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    printf("%d : finish reduction\n", rank);
    reduce_time = MPI_Wtime();

    //////////////////////////////////// PRINT RESULT ////////////////////////////////////

    if (rank == 0) {
        int i;
        float sum;
        int num;
        for(i=0; i<max; i++){
            if (received_num_array[i] == 0){
                continue;
            } else {
                num = received_num_array[i];
                sum = received_sum_array[i];

                // Uncomment it if you want to print the result

                // printf("final key: %d | num: %d | sum: %f | avg: %f\n", i, num, sum, sum/num);
            }
        }

        printf("finish print result\n");
        end_time = MPI_Wtime();
    }

    //////////////////////////////////// FREE ////////////////////////////////////

    destroy(max);
    printf("%d : finish destroy\n", rank);

    free(word);
    free(list_node);
    free(sum_array);
    free(num_array);
    free(received_num_array);
    free(received_sum_array);

    printf("finish free\n");
    free_time = MPI_Wtime();

    if (rank == 0) {
        //////////////////////////////////// TIME ////////////////////////////////////
        printf("\nTotaltime = %f seconds\n", free_time-start_time);
        printf("\nstart_getsize = %f seconds\n", getsize_time-start_time);
        printf("\ngetsize_file = %f seconds\n", file_time-getsize_time);
        printf("\nfile-create = %f seconds\n", create_time-file_time);
        printf("\ncreate-aggregation = %f seconds\n", agg_time-create_time);
        printf("\naggregation-reduction = %f seconds\n", reduce_time-agg_time);
        printf("\nreduction-end = %f seconds\n", end_time-reduce_time);
        printf("\nend-free = %f seconds\n", free_time-end_time);
    }

    MPI_Finalize();
    return 0;
}