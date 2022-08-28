CC=gcc
CFLAGS=-g -Wall

OBJS=grep.c
TARGET=grep.out

OBJS_TEST=grep_test.c
TARGET_TEST=grep_test.out

OBJS_OMP=omp_grep.c
TARGET_OMP=omp_grep.out

OBJS_OMP_TEST=omp_grep_test.c
TARGET_OMP_TEST=omp_grep_test.out

OBJS_MPI=mpi_grep.c
TARGET_MPI=mpi_grep.out

OBJS_MPI_TEST=mpi_grep_test.c
TARGET_MPI_TEST=mpi_grep_test.out

all: $(TARGET) $(TARGET_TEST) $(TARGET_MPI) $(TARGET_MPI_TEST) $(TARGET_OMP) $(TARGET_OMP_TEST)

clean:
	rm -f *.o
	rm -f *.out

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(TARGET_TEST) : $(OBJS_TEST)
	$(CC) $(CFLAGS) -o $@ $(OBJS_TEST)

$(TARGET_OMP) : $(OBJS_OMP)
	$(CC) $(CFLAGS) -fopenmp -o $@ $(OBJS_OMP)

$(TARGET_OMP_TEST) : $(OBJS_OMP_TEST)
	$(CC) $(CFLAGS) -fopenmp -o $@ $(OBJS_OMP_TEST)

$(TARGET_MPI) : $(OBJS_MPI)
	mpicc $(CFLAGS) -o $@ $(OBJS_MPI)

$(TARGET_MPI_TEST) : $(OBJS_MPI_TEST)
	mpicc $(CFLAGS) -o $@ $(OBJS_MPI_TEST)
