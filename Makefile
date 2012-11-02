all: parallel sequential
	
parallel: alias.c
	gcc --openmp -o parallel alias.c

sequential: alias.c
	gcc -o sequential alias.c
