CC=gcc
CFLAGS=-g -Wall

OBJS=gag.c
TARGET=gag.out

OBJS_OMP=omp_gag.c
TARGET_OMP=omp_gag.out

OBJS_MPI=mpi_gag.c
TARGET_MPI=mpi_gag.out

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
