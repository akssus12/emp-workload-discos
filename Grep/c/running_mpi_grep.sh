#! /bin/bash

# $1 : number of using cores
# $2 : target file neme
# $3 : target word
# line : lines of target file

line=$(wc -l < $2)
echo "${line}"

mpirun -np $1 ./mpi_grep -hostfile ./my_hostfile $2 $line $3