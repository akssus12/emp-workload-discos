main: main.c Lab4_IO.c
	mpicc main.c  Lab4_IO.c -o main -g -w -std=c99 -lm

trim: datatrim.c
	gcc datatrim.c -o datatrim

test: serialtester.c Lab4_IO.c
	gcc serialtester.c Lab4_IO.c -o serialtester -lm

run:
	mpirun -np 4 ./main

rundatatrim:
	./datatrim -b 1000
