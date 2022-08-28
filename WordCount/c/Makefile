CC=gcc
CFLAGS=-g -Wall

OBJS=wc.c ../htable.h
TARGET=wc.out

OBJS_TEST=wc_test.c ../htable.h
TARGET_TEST=wc_test.out

OBJS_OMP=omp_wc.c ../htable.h
TARGET_OMP=omp_wc.out

OBJS_OMP_TEST=omp_wc_test.c ../htable.h
TARGET_OMP_TEST=omp_wc_test.out

OBJS_MPI=mpi_wc.c ../htable.h
TARGET_MPI=mpi_wc.out

OBJS_MPI_TEST=mpi_wc_test.c ../htable.h
TARGET_MPI_TEST=mpi_wc_test.out

all: $(TARGET) $(TARGET_TEST) $(TARGET_MPI) $(TARGET_MPI_TEST) $(TARGET_OMP) $(TARGET_OMP_TEST)

clean:
	rm -f *.o
	rm -f *.out

$(TARGET) : $(OBJS)
	$(CC) -o $@ $(OBJS)

$(TARGET_TEST) : $(OBJS_TEST)
	$(CC) -o $@ $(OBJS_TEST)

$(TARGET_OMP) : $(OBJS_OMP)
	$(CC) -fopenmp -o $@ $(OBJS_OMP)

$(TARGET_OMP_TEST) : $(OBJS_OMP_TEST)
	$(CC) -fopenmp -o $@ $(OBJS_OMP_TEST)

$(TARGET_MPI) : $(OBJS_MPI)
	mpicc -o $@ $(OBJS_MPI)

$(TARGET_MPI_TEST) : $(OBJS_MPI_TEST)
	mpicc -o $@ $(OBJS_MPI_TEST)
