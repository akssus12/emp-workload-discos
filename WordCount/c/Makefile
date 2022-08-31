CC=gcc
CFLAGS=-g -Wall

OBJS=wc.c
TARGET=wc.out

OBJS_OMP=omp_wc.c
TARGET_OMP=omp_wc.out

OBJS_MPI=mpi_wc.c
TARGET_MPI=mpi_wc.out

all: $(TARGET) $(TARGET_MPI) $(TARGET_OMP)

clean:
	rm -f *.o
	rm -f *.out

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(TARGET_OMP) : $(OBJS_OMP)
	$(CC) $(CFLAGS) -fopenmp -o $@ $(OBJS_OMP)

$(TARGET_MPI) : $(OBJS_MPI)
	mpicc $(CFLAGS) -o $@ $(OBJS_MPI)