all: parallel sequential
	
parallel: alias.c Makefile
	gcc -O3 -g --openmp -o parallel alias.c

sequential: alias.c Makefile
	gcc -O3 -g -o sequential alias.c
