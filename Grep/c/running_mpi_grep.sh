#! /bin/bash

# $1 : target file neme
# $2 : target word
# line : lines of target file

line=$(wc -l < $1)
echo "${line}"

mpirun -np 2 -hostfile ./my_hostfile ./mpi_grep $2 $line $3