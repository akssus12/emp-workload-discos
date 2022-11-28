#! /bin/bash

# $1 : target file neme
# line : lines of target file

line=$(wc -l < $1)
echo "${line}"

mpirun -np 2 -hostfile ./my_hostfile ./mpi_ag $2 $line