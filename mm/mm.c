/********************************************
 * Unoptimized matrix matrix multiplication *
 ********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define NDIM 35

void mm(int, int, int, double a[NDIM][NDIM], double b[NDIM][NDIM], double c[NDIM][NDIM]);
void print_matrix(int);
void check_matrix(int);

double          a[NDIM][NDIM];
double          b[NDIM][NDIM];
double          c[NDIM][NDIM];

typedef struct
{
	int             id;
	int             noproc;
	int             dim;
	double	(*a)[NDIM][NDIM],(*b)[NDIM][NDIM],(*c)[NDIM][NDIM];
}               parm;

void worker(void *arg)
{
	parm           *p = (parm *) arg;
	mm(p->id, p->noproc, p->dim, *(p->a), *(p->b), *(p->c));
	//return NULL;
}

int main(int argc, char *argv[]) {
	// profile on
	int             noproc, me_no;

	parm           *arg;
	int             n, i, j;

	for (i = 0; i < NDIM; i++) {
		for (j = 0; j < NDIM; j++) { // unroll j 200
			a[i][j] = i + j;
			b[i][j] = i + j;
		} // end for
	}
	if (argc != 2)
	{
		printf("Usage: %s n\n  where n is no. of thread\n", argv[0]);
		exit(1);
	}
	n = atoi(argv[1]);

	arg=(parm *)malloc(sizeof(parm)*n);
	/* setup barrier */

	/* Start up thread */

	/* Spawn thread */
	
	for (i = 0; i < n; i++)
	{
		arg[i].id = i;
		arg[i].noproc = n;
		arg[i].dim = NDIM;
		arg[i].a = &a;
		arg[i].b = &b;
		arg[i].c = &c;
		//printf("HAHAHAHAHA\n");
		worker((void *)(arg+i));
	}

	/* print_matrix(NDIM); */
	check_matrix(NDIM);
	free(arg);
	return 0;
	// profile off
} // end main

void mm(int me_no, int noproc, int n, double a[NDIM][NDIM], double b[NDIM][NDIM], double c[NDIM][NDIM])
{
	int             i,j,k;
	double sum;
	i=me_no;
	//printf("HAHAHA\n");
	while (i<n) {

		for (j = 0; j < n; j++) { 
			sum = 0.0;
			for (k = 0; k < n; k++) { // unroll k 200
				sum = sum + a[i][k] * b[k][j];
			} // end for
			c[i][j] = sum;
		} 
		i+=noproc;
	}
}

void check_matrix(dim)
int dim;
{
	int i,j,k;
	int error=0;

	printf("Now checking the results\n");
	for(i=0;i<dim;i++) {
		for(j=0;j<dim;j++) {
			double e=0.0;

			for (k=0;k<dim;k++) { // unroll k 200
				e+=a[i][k]*b[k][j];
			} // end for

		}
	}
}