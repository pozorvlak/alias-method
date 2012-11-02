all: parallel sequential
	
parallel: alias.c Makefile
	gcc -g --openmp -o parallel alias.c

sequential: alias.c Makefile
	gcc -g -o sequential alias.c
