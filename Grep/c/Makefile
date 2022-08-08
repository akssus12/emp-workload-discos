all: grep grep_test mpi_grep mpi_grep_test

CC = gcc

grep: grep.c
	gcc grep.c -o grep

grep_test: grep_test.c
	gcc grep_test.c -o grep_test

mpi_grep: mpi_grep.c
	mpicc mpi_grep.c -o mpi_grep

mpi_grep_test: mpi_grep_test.c
	mpicc mpi_grep_test.c -o mpi_grep_test

clean:
	rm grep
	rm grep_test
	rm mpi_grep
	rm mpi_grep_test
