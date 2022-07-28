all: ag mpi_ag

CC =  gcc

gag_test: ag.c
	$(CC) -o $@ $<

mpi_gag: mpi_ag.c
	mpicc mpi_ag.c -o mpi_ag

clean:
	rm ag
	rm mpi_ag
