#! /bin/bash

# $1 : num of processes
# $2 : target file neme
# line : lines of target file

line=$(wc -l < $2)
echo "${line}"

mpirun -np $1 -hostfile ./my_hostfile ./mpi_wc $2 $line