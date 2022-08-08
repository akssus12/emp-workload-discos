all: ag ag_test mpi_ag mpi_ag_test

CC = gcc

ag: ag.c
	gcc ag.c -o ag

ag_test: ag_test.c
	gcc ag_test.c -o ag_test

mpi_ag: mpi_ag.c
	mpicc mpi_ag.c -o mpi_ag

mpi_ag_test: mpi_ag_test.c
	mpicc mpi_ag_test.c -o mpi_ag_test

clean:
	rm ag
	rm ag_test
	rm mpi_ag
	rm mpi_ag_test
