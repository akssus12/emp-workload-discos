all: grep grep_test mpi_grep

CC = gcc

grep: grep.c
	$(CC) -o $@ $<

grep_test: grep_test.c
	$(CC) -o $@ $<

mpi_grep: mpi_grep.c
	mpicc mpi_grep.c -o mpi_grep

clean:
	rm grep
	rm grep_test
	rm mpi_grep
