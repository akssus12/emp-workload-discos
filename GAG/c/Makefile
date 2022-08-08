all: gag gag_test mpi_gag mpi_gag_test

CC =  gcc

gag: gag.c
	gcc gag.c -o gag

gag_test: gag_test.c
	gcc gag_test.c -o gag_test

mpi_gag: mpi_gag.c
	mpicc mpi_gag.c -o mpi_gag

mpi_gag_test: mpi_gag_test.c
	mpicc mpi_gag_test.c -o mpi_gag_test

clean:
	rm gag
	rm gag_test
	rm mpi_gag
	rm mpi_gag_test
