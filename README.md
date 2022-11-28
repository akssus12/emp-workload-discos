# emp-workload-discos
This project is Evaluation of cyclomatic complexity(CCN) between EMP and distributed computing. We use Cyclomatic Complexity Analyzer, [Lizard](https://github.com/terryyin/lizard) to get CCN. Three simple big data applications are used to compare CCN. 

- **Grep**

    Grep is a string matching application that prints lines containing a matching keyword from the input file.

- **GAG(Group By Aggregation)**

    GAG is an application that generates statistics based on key values of <key, value> pairs.

- **AG(Aggregation)**
    
    AG is an application that operates in a similar way to GAG, but has the same key value.

  
Each application is implemented in three versions.
- **[workload].out** runs with single thread in single machine.
- **mpi_[workload].out** runs in one thread on each of the two machines using Open MPI(v4.1.1).
- **omp_[workload].out** runs with two threads in single machine using OpenMP(v4.5).

---
# How to compile
- **Grep**
    ```bash
    $ cd Grep/c
    $ make
    ```
 - **GAG**
    ```bash
    $ cd GAG/c
    $ make
    ```   

- **AG**
    ```bash
    $ cd AG/c
    $ make
    ```
---
# How to run
MPI and OMP version use a shell script to execute. When running the MPI version, it should create a hostfile called ***my_hostfile***
- **Grep**
    - grep.out
        ```bash
        $ grep.out [file] [target word] 
        ```
    - mpi_grep.out
        ```bash
        $ running_mpi_grep.sh [file] [target word] 
        ```
    - omp_grep.out
        ```bash
        $ running_omp_grep.sh [file] [target word] 
        ```

<br/>

- **GAG**
    - gag.out
        ```bash
        $ gag.out [file] 
        ```
    - mpi_gag.out
        ```bash
        $ running_mpi_gag.sh [file] 
        ```
    - omp_gag.out
        ```bash
        $ running_omp_grep.sh [file] 
        ```

<br/>

- **AG**
    - ag.out
        ```bash
        $ ag.out [file]
        ```
    - mpi_ag.out
        ```bash
        $ running_mpi_ag.sh [file]
        ```
    - omp_ag.out
        ```bash
        $ running_omp_ag.sh [file]
        ```
---
# Constructing Dataset
When running GAG and AG applicaiton, you can construct dataset using our dataset generator. generator is simple python code that create dataset based on specified the maximum key value and the number of data in python code. generator is located in the AG, GAG workload path.<br/>
The Grep application can use any text file. 