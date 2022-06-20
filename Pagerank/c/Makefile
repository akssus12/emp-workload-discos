main: mpi_pr.c Lab4_IO.c
	mpicc mpi_pr.c  Lab4_IO.c -o mpi_pr -g -w -std=c99 -lm

trim: datatrim.c
	gcc datatrim.c -o datatrim

serial: pr.c Lab4_IO.c
	gcc pr.c Lab4_IO.c -o pr -lm

run:
	mpirun -np 4 ./mpi_pr

rundatatrim:
	./datatrim -b 1000
