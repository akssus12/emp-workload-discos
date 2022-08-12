#! /bin/bash

mpirun -np $1 -hostfile ./my_hostfile ./mpi_wc $2