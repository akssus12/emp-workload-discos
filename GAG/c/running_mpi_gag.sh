#! /bin/bash

# $1 : number of using cores
# $2 : target file neme
# line : lines of target file

line=$(wc -l < $2)
echo "${line}"

mpirun -np $1 ./mpi_gag -hostfile ./my_hostfile $2 $line