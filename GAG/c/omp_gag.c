#include <ctype.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <omp.h>
#include <sys/time.h>
#include <sys/stat.h>

struct Node {
    struct Node* next;
    int value;
};

struct Front {
    struct Node* next;
    int num;
    int sum;
    float avg;
    omp_lock_t lock;
};

// Empty List of Node
struct Front* list_node;

// Function to initialize list_node(struct Front) FOR OMP
void init(int max_key, int index){
    list_node[index].next = NULL;
    list_node[index].num = 0;
    list_node[index].sum = 0;
    list_node[index].avg = 0;
    omp_init_lock(&list_node[index].lock);
}

// Function to free Node linked list_node
void destroy(int max_key){
    int i;
    struct Node* remove;
    for (i=0; i<max_key; i++){
        omp_destroy_lock(&list_node[i].lock);
        if (list_node[i].num == 0) {
            continue;
        } else {
            while(list_node[i].next != NULL){
                remove = list_node[i].next;
                list_node[i].next = remove->next;
                free(remove);
                remove = NULL;
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
        node = list_node[key].next;
        list_node[key].next = new_node;
        new_node->next = node;

        list_node[key].num++;
    }
}

void aggregate(int max_key, int index){
    int i;
    int num, sum;

    if(list_node[index].next != NULL){
        num = list_node[index].num;
        sum = 0;
        struct Node* pt_node = list_node[index].next;
        for(i=0; i<num; i++){
            sum += pt_node->value;
            pt_node = pt_node->next;
        }
        list_node[index].sum = sum;
        list_node[index].avg = (float)sum / num;
    }
}

int main(int argc, char** argv) {
    omp_set_dynamic(0);     // Explicitly disable dynamic teams
    omp_set_num_threads(2); // Use 2 threads for all consecutive parallel regions

    double start_time, file_time, create_time, agg_time, end_time; 
    char filename[256];
    int total_line;
    int i;

    strcpy(filename, argv[1]);
    total_line = atoi(argv[2]);

    start_time = omp_get_wtime();
    //////////////////////////////////// Insert String to Array //////////////////////////////////// 
    struct stat sb;
    stat(argv[1], &sb);

    char **array_word1 = calloc(total_line, sizeof(char *));

    FILE *ptr;
    char *tmp_line = NULL;
    size_t len = 0;
    ssize_t read;

    ptr = fopen(filename, "r");
    if (ptr == NULL) {
        printf("Error opening data file\n");
    }
    
    i = 0;
    while((read = getline(&tmp_line, &len, ptr)) != -1) {
        array_word1[i] = malloc((size_t)read+1);
        memset(array_word1[i], 0, (size_t)read+1);

        strncpy(array_word1[i], tmp_line, (size_t)read);
        array_word1[i][(size_t)read] = '\0';
        i++;
    }
    free(tmp_line);
    fclose(ptr);

    printf("Complete reading files in the array_word[i]\n");
    file_time = omp_get_wtime();

    //////////////////////////////////// INSERT DATA INTO NODE ////////////////////////////////////
    int max;
    sscanf(array_word1[0], "%d\n", &max);
    printf("max : %d\n", max);
    
    list_node = calloc(max, sizeof(struct Front));

    #pragma omp parallel for schedule(dynamic) shared(list_node)
    for (i = 0; i<max; i++){
        init(max, i);
    }

    int key, value;
    #pragma omp parallel for schedule(dynamic) shared(list_node) private(key, value)
    // first line is max value
    for (i=1; i<total_line; i++){
        sscanf(array_word1[i], "<%d, %d>", &key, &value);
        omp_set_lock(&list_node[key].lock);
        create(key, value);
        omp_unset_lock(&list_node[key].lock);
    }
    printf("finish insert node\n");
    create_time = omp_get_wtime();

    //////////////////////////////////// AGGREGATE ////////////////////////////////////
    #pragma omp parallel for schedule(dynamic) shared(list_node)
    for(i=0; i<max; i++){
        aggregate(max, i);
    }
    printf("finish aggregation\n");
    agg_time = omp_get_wtime();

    //////////////////////////////////// PRINT RESULT ////////////////////////////////////
    int num, sum;
    float avg;
    for(i=0; i<max; i++){
        if(list_node[i].num == 0)
            continue;
        else {
            num = list_node[i].num;
            sum = list_node[i].sum;
            avg = list_node[i].avg;

            // Uncomment it if you want to print the result
            // printf("final key: %d | num: %d | sum: %d | avg: %f\n", i, num, sum, avg);   
        }
    }
    printf("finish print result\n");
    end_time = omp_get_wtime();

    //////////////////////////////////// TIME ////////////////////////////////////
    printf("\nTotaltime = %f seconds\n", end_time-start_time);
    printf("\nstart_file = %f seconds\n", file_time-start_time);
    printf("\nfile-create = %f seconds\n", create_time-file_time);
    printf("\ncreate-aggregation = %f seconds\n", agg_time-create_time);
    printf("\naggregation-end = %f seconds\n", end_time-agg_time);

    //////////////////////////////////// FREE ////////////////////////////////////
    destroy(max);
    free(list_node);
    for(i = 0; i<max; i++){
        free(array_word1[i]);
    }
    free(array_word1);

    return 0;
}

// size_t* getSpecificSize_getline(char *name, int target_line){
//     FILE *ptr;
//     size_t* size = calloc(2, sizeof(size_t));
//     int line=0;
//     char *tmp_line = NULL;
//     size_t len = 0;
//     ssize_t read;

//     ptr=fopen(name,"r");
//     if (ptr == NULL) {
//         printf("Error opening data file\n");
//     }
    
//     while((read = getline(&tmp_line, &len, ptr)) != -1) {
//         size[1] += (size_t)read;
//         line ++;
        
//         if (line >= 1){
//             if( line != target_line ) {
//                 size[0] += (size_t)read;
//             } else
//                 break;
//         } 
//     }
//     free(tmp_line);

//     fseek(ptr, 0, SEEK_SET);
//     fclose(ptr);

//     return(size);
// }

// // for MPI, separate Front struct to Front(not included sum, avg) struct & int sum, num array
// struct Node {
//     struct Node* next;
//     int value;
// };

// struct Front {
//     struct Node* next;
//     int num;
// };

// // Empty List of Node
// struct Front** list_node;
// int** sum_array;
// int** num_array;
// int* reduce_sum_array;
// int* reduce_num_array;

// // Function to initialize list_node(struct Front) & sum_array, num_array
// void init(int max_key, int num_thread, int id){
//     int i;

//     list_node[id] = calloc(max_key, sizeof(struct Front));
//     sum_array[id] = calloc(max_key, sizeof(int));
//     num_array[id] = calloc(max_key, sizeof(int));

//     for (i = 0; i<max_key; i++){
//         list_node[id][i].next = NULL;
//         list_node[id][i].num = 0;
//     }
// }

// // Function to free Node linked list_node
// void destroy(int max_key, int id){
//     int i;
//     struct Node* remove;
//     for (i=0; i<max_key; i++){
//         if (list_node[id][i].num == 0) {
//             continue;
//         } else {
//             while(list_node[id][i].next != NULL){
//                 remove = list_node[id][i].next;
//                 list_node[id][i].next = remove->next;
//                 free(remove);
//             }
//         }
//     }
// }

// // Function to create a header linked list
// void create(int key, int data, int id){
//     // Create a new node
//     struct Node *new_node, *node;
//     new_node = calloc(1, sizeof(struct Node));
//     new_node->value = data;
//     new_node->next = NULL;
//     // If it is first node
//     if (list_node[id][key].num == 0){
//         list_node[id][key].next = new_node;
//         list_node[id][key].num++;
//     } else {
//         // Insert the node in the end
//         // printf("insert node! \n");
//         node = list_node[id][key].next;
//         list_node[id][key].next = new_node;
//         new_node->next = node;

//         list_node[id][key].num++;
//     }
// }

// void aggregate(int max_key, int id){
//     int i, j;
//     int sum, num;

//     for(i=0; i<max_key; i++){
//         if (list_node[id][i].next == NULL){
//             continue;
//         } else {
//             num = list_node[id][i].num;
//             sum = 0;
//             struct Node* pt_node = list_node[id][i].next;
//             for(j=0; j<num; j++){
//                 sum += pt_node->value;
//                 pt_node = pt_node->next;
//             }
//             sum_array[id][i] = sum;
//             num_array[id][i] = num;
//         }
//     }
// }

// int main(int argc, char** argv) {
//     omp_set_dynamic(0);     // Explicitly disable dynamic teams
//     omp_set_num_threads(2); // Use 2 threads for all consecutive parallel regions
//     int NUM_THREADS = 2;

//     double start_time, getsize_time, file_time, create_time, agg_time, reduce_time, end_time; 
//     char filename[256];
//     int total_line;
//     int max = 0;
//     int i, j;

//     strcpy(filename, argv[1]);
//     total_line = atoi(argv[2]);
//     total_line -= 1;

//     start_time = omp_get_wtime();
//     //////////////////////////////////// GET THE FILE SIZE UP TO A SPECIFIC LINE //////////////////////////////////// 
//     struct stat sb;
//     stat(argv[1], &sb);

//     size_t* line_size = getSpecificSize_getline(filename, (int)total_line/2);
//     getsize_time = omp_get_wtime();

//     //////////////////////////////////// READ FILE ////////////////////////////////////
//     FILE * fp;
//     char ** array_word = (char **)malloc(sizeof(char *) * NUM_THREADS);
//     char* free_word[2];

//     #pragma omp parallel shared(array_word) private(i, fp)
//     {
//         i = omp_get_thread_num();

//         fp = fopen(filename, "r");
//         if (fp == NULL){
//             printf("Error opening data file\n");
//         }
    
//         if (i%2 == 0){
//             array_word[i] = malloc(line_size[i] + 1);
//             memset(array_word[i], 0, line_size[i] + 1);

//             fscanf(fp, "%d\n", &max);

//             fread(array_word[i], line_size[i], 1, fp);
//             array_word[i][line_size[i] + 1] = '\0';
//         } else {
//             array_word[i] = malloc(sb.st_size - line_size[i] + 1);
//             memset(array_word[i], 0, sb.st_size - line_size[i] + 1);

//             fseek(fp, line_size[i], SEEK_CUR);

//             fread(array_word[i], sb.st_size - line_size[i], 1, fp);
//             array_word[i][sb.st_size - line_size[i] + 1] = '\0';
//         }
//         free_word[i] = array_word[i];
//         fclose(fp);
//     }
//     free(line_size);
//     file_time = omp_get_wtime();
//     printf("Complete reading files in the array_word[i]\n");

//     //////////////////////////////////// INSERT DATA INTO NODE ////////////////////////////////////

//     list_node = (struct Front **)calloc(NUM_THREADS, sizeof(struct Front *));
//     sum_array = (int **)calloc(NUM_THREADS, sizeof(int *));
//     num_array = (int **)calloc(NUM_THREADS, sizeof(int *));

//     char * token;
//     int key, value;
//     #pragma omp parallel private(i, token, key, value)
//     {
//         i = omp_get_thread_num();
//         init(max, NUM_THREADS, i);

//         while( (token = strtok_r(array_word[i], "\n", &array_word[i])) ){
//             sscanf(token, "<%d,%d>", &key, &value);
//             create(key, value, i);
//         }
//     }
//     printf("finish insert node\n");
//     create_time = omp_get_wtime();

//     //////////////////////////////////// AGGREGATE ////////////////////////////////////
//     reduce_num_array = calloc(max, sizeof(int));
//     reduce_sum_array = calloc(max, sizeof(int));

//     #pragma omp parallel private(i)
//     {
//         i = omp_get_thread_num();
//         aggregate(max, i);
//     }
//     printf("finish aggregation\n");
//     agg_time = omp_get_wtime();

//     //////////////////////////////////// REDUCE ////////////////////////////////////

//     #pragma omp parallel for reduction(+:reduce_sum_array[:max], reduce_num_array[:max]) private(j)
//     for(i=0; i<max; i++){
//         for(j=0; j<NUM_THREADS; j++){
//             reduce_sum_array[i] += sum_array[j][i];
//             reduce_num_array[i] += num_array[j][i];
//         }
//     }
//     printf("finish reduction\n");
//     reduce_time = omp_get_wtime();

//     //////////////////////////////////// PRINT RESULT ////////////////////////////////////

//     int num;
//     float sum;
//     for(i=0; i<max; i++){
//         if(reduce_num_array[i] == 0)
//             continue;
//         else {
//             num = reduce_num_array[i];
//             sum = reduce_sum_array[i];

//             // Uncomment it if you want to print the result
//             // printf("final key: %d | num: %d | sum: %f | avg: %f\n", i, num, sum, sum/num);   
//         }
//     }
//     printf("finish print result\n");
//     end_time = omp_get_wtime();

//     //////////////////////////////////// TIME ////////////////////////////////////
//     printf("\nTotaltime = %f seconds\n", end_time-start_time);
//     printf("start_getsize = %f seconds\n", getsize_time-start_time);
//     printf("getsize_file = %f seconds\n", file_time-getsize_time);
//     printf("\nfile-create = %f seconds\n", create_time-file_time);
//     printf("\ncreate-aggregation = %f seconds\n", agg_time-create_time);
//     printf("\naggregation-reduction = %f seconds\n", reduce_time-agg_time);
//     printf("\nreduction-end = %f seconds\n", end_time-reduce_time);

//     //////////////////////////////////// FREE ////////////////////////////////////

//     #pragma omp parallel private(i)
//     {
//         i = omp_get_thread_num();
//         destroy(max, i);

//         free(list_node[i]);
//         free(free_word[i]);
//         free(sum_array[i]);
//         free(num_array[i]);
//     }
//     free(list_node);
//     free(array_word);
//     free(sum_array);
//     free(num_array);
//     free(reduce_num_array);
//     free(reduce_sum_array);

//     return 0;
// }