CC=gcc
CFLAGS=-g -Wall

OBJS=gag.c
TARGET=gag.out

OBJS_TEST=gag_test.c
TARGET_TEST=gag_test.out

OBJS_OMP=omp_gag.c
TARGET_OMP=omp_gag.out

OBJS_OMP_TEST=omp_gag_test.c
TARGET_OMP_TEST=omp_gag_test.out

OBJS_MPI=mpi_gag.c
TARGET_MPI=mpi_gag.out

OBJS_MPI_TEST=mpi_gag_test.c
TARGET_MPI_TEST=mpi_gag_test.out

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
