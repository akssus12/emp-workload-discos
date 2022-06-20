#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <math.h>
#include "Lab4_IO.h"
#include "timer.h"

#define LAB4_EXTEND
#define EPSILON 0.00001
#define DAMPING_FACTOR 0.85

int number_of_processes;
int my_rank;
int total_nodes;
int my_size;
int my_start;
int my_end;
double *ranks;
double *last_ranks;
double *local_ranks;
int *sizes;
int *displacements;
double delta;
//JGH
int iterations = 0;

struct node{
    int *inlinks;
    int num_in_links;
    int num_out_links;
};

struct node *nodehead;

int get_end(){
    FILE* op;
    if ((op = fopen("data_input_meta","r")) == NULL) {
        printf("Error opening the input file.\n");
        exit(1);
    }

    char fileText[100];
    fgets(fileText, 100, op);
    fclose(op);

    return atoi(fileText);
}

void init(int argc, char * argv[]){
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &number_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int nodes_per_process;
    int i;

    total_nodes = get_end();
    nodes_per_process = total_nodes / number_of_processes;
    delta = 0;

    if (my_rank != number_of_processes -1){
        my_start = nodes_per_process * my_rank;
        my_end = nodes_per_process * (my_rank + 1) - 1;
    }
    else{
        my_start = nodes_per_process * my_rank;
        my_end = total_nodes - 1;
    }

    my_size = my_end - my_start + 1;

    node_init(&nodehead, 0, total_nodes);

    ranks = malloc(total_nodes * sizeof(double));
    last_ranks = malloc(total_nodes * sizeof(double));
    local_ranks = malloc(my_size * sizeof(double));
    sizes = malloc(number_of_processes * sizeof(int));
    displacements = malloc(number_of_processes * sizeof(int));

    MPI_Allgather(&my_size, 1, MPI_INT, sizes, 1, MPI_INT, MPI_COMM_WORLD);
    MPI_Allgather(&my_start, 1, MPI_INT, displacements, 1, MPI_INT, MPI_COMM_WORLD);

    for (i = 0; i < total_nodes; i++){
        last_ranks[i] = 0.0;
        ranks[i] = 1.0 / total_nodes;
        if (i < my_size){
            local_ranks[i] = ranks[i];
        }
    }
}

void finish(){
    MPI_Finalize();
}

void do_iteration(){
    int i, j, current_node_pos, in_node_pos;
    for (i = 0; i < my_size; i++){
        current_node_pos = i + my_start;
        local_ranks[i] = 0.0;
        local_ranks[i] += (1.0 - DAMPING_FACTOR) / total_nodes;
        for (j = 0; j < nodehead[current_node_pos].num_in_links; j++){
            in_node_pos = nodehead[current_node_pos].inlinks[j];
            local_ranks[i] += (ranks[in_node_pos] / nodehead[in_node_pos].num_out_links) * DAMPING_FACTOR;
        }
    }
}

void update_ranks(){
    MPI_Allgatherv(local_ranks, my_size, MPI_DOUBLE, ranks, sizes, displacements, MPI_DOUBLE, MPI_COMM_WORLD);
}

void get_delta(){
    double numerator, denominator;
    int i;
    for (i = 0; i < total_nodes; i++){
        numerator += pow(ranks[i]-last_ranks[i], 2);
        denominator += pow(ranks[i], 2);
    }
    numerator = sqrt(numerator);
    denominator = sqrt(denominator);
    delta = numerator / denominator;
}

void update_last_ranks(){
    memcpy(last_ranks, ranks, total_nodes * sizeof(double));
}

int main(int argc, char * argv[]){
    init(argc, argv);
    double start, end;
    GET_TIME(start);

    do{
        do_iteration();
        update_ranks();
        get_delta();
        update_last_ranks();
	iterations++;
    //} while (delta >= EPSILON);
    } while(iterations < 100);

    GET_TIME(end);
    if (my_rank == 0){
        Lab4_saveoutput(ranks, total_nodes, end-start);
        printf("Total time taken %f\n", end-start);
    }
    finish();
    return 0;
}
