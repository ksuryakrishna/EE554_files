#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#define NUM 800

void triad(int *a,int *b, int *c, int s){

  int i;
  for(i=0;i<NUM;i++)
    c[i] = a[i] + s*b[i];
}

int main(){
	int *a, *b, *c;
    a = (int *) malloc (sizeof(int) * NUM);
    b = (int *) malloc (sizeof(int) * NUM);
    c = (int *) malloc (sizeof(int) * NUM);
	int i;
  srand(8650341L);
	for(i=0; i<NUM; i++){
		c[i] = 0;
		a[i] = rand();
		b[i] = rand();
	}
	triad(&a[0],&b[0],&c[0],3);

	return 0;
}
