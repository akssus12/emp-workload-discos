all: wc wc_test mpi_wc mpi_wc_test

CC = gcc

wc: wc.c
	gcc wc.c -o wc

wc_test: wc_test.c
	gcc wc_test.c -o wc_test

mpi_wc: mpi_wc.c
	mpicc mpi_wc.c -o mpi_wc

mpi_wc_test: mpi_wc_test
	mpicc mpi_wc_test -o mpi_wc_test

clean:
	rm wc
	rm wc_test
	rm mpi_wc
	rm mpi_wc_test
