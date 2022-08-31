CC=gcc
CFLAGS=-g -Wall

OBJS=ag.c
TARGET=ag.out

OBJS_OMP=omp_ag.c
TARGET_OMP=omp_ag.out

OBJS_MPI=mpi_ag.c
TARGET_MPI=mpi_ag.out

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
