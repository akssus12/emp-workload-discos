all: hash_wc wc mpi_wc ori_wc

CC = gcc

wc: wc.c
        $(CC) -o $@ $<

mpi_wc: mpi_wc.c
        mpicc mpi_wc.c -o mpi_wc

clean:
        rm hash_wc
        rm wc
        rm mpi_wc
        rm ori_wc
