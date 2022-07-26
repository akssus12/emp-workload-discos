all: gag_test mpi_gag

CC =  gcc

gag_test: gag_test.c
	$(CC) -o $@ $<

mpi_gag: mpi_gag.c
	mpicc mpi_gag.c -o mpi_gag

clean:
	rm gag_test
