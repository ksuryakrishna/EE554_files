/* ***********************************************************************
  This program is part of the
	OpenMP Source Code Repository

	http://www.pcg.ull.es/ompscr/
	e-mail: ompscr@etsii.ull.es

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License 
  (LICENSE file) along with this program; if not, write to
  the Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
  Boston, MA  02111-1307  USA
	
	FILE:              c_qsort.c
  VERSION:           1.0
  DATE:              May 2004
  AUTHOR:            F. de Sande
  COMMENTS TO:       sande@csi.ull.es
  DESCRIPTION:       Parallel implementation of Quicksort using OpenMP
	                   Sorts an integer array
  COMMENTS:          The code requires nested Parallelism.
  REFERENCES:        C. A. R. Hoare,
                     ACM Algorithm 64}: Quicksort",
                     Communications of the ACM",
                     vol. 4, no. 7, pg. 321. Jul 1961
                     http://en.wikipedia.org/wiki/Quicksort
  BASIC PRAGMAS:     parallel for
  USAGE:             ./c_qsort.par 2000000
  INPUT:             The size (in K) of the vector to sort 
  OUTPUT:            The code tests that the vector is sorted
	FILE FORMATS:      -
	RESTRICTIONS:      -
	REVISION HISTORY:
**************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#define NUM_STEPS 1             /* No. of iterations (number of vectors to sort) */

int SIZE;
int array[1000000];      


/* -----------------------------------------------------------------------
                          PROTOTYPES
 * ----------------------------------------------------------------------- */

void initialize(int *v, int seed);
void testit(int *v);
void qs(int *v, int first, int last);

/* -----------------------------------------------------------------------
                          IMPLEMENTATION
 * ----------------------------------------------------------------------- */
/* -----------------------------------------------------------------------
   Sets randomly the values for the array 
 * ----------------------------------------------------------------------- */
void initialize(int *v, int seed) {
  unsigned i;

   srandom(seed);
   for(i = 0; i < SIZE; i++)
     v[i] = (int)random();
}
/* -----------------------------------------------------------------------
   Tests the result 
 * ----------------------------------------------------------------------- */
void testit(int *v) {
  register int k;
	int not_sorted = 0;

  for (k = 0; k < SIZE - 1; k++)
    if (v[k] > v[k + 1]) {
      not_sorted = 1;
      break;
    }
  if (not_sorted)
    ;//printf("Array NOT sorted.\n");
	else
    printf("Array sorted.\n");
}
/* ----------------------------------------------------------------------- */
void qs(int *v, int first, int last) {
  int start[2], end[2], pivot, i, temp;

  if (first < last) {
     start[1] = first;
     end[0] = last;
     pivot = v[(first + last) / 2];
     while (start[1] <= end[0]) {
       while (v[start[1]] < pivot)
         start[1]++;
       while (pivot < v[end[0]])
         end[0]--;
       if (start[1] <= end[0]) {
         temp = v[start[1]];
         v[start[1]] = v[end[0]];
         v[end[0]] = temp;
         start[1]++;
         end[0]--;
       }
     }
     start[0] = first; 
     end[1]   = last;  
{
     for(i = 0; i <= 1; i++) {
       qs(v, start[i], end[i]);
     }
}
		 
   } else {
   	i = 0;
   	temp = i;
   	pivot = temp + i;
   	start[0] = 1;
   	start[1] = 2;
   	end[0] = 3;
   	end[1] = 4;
   }
}
/* ----------------------------------------------------------------------- */
int main(int argc, char *argv[]) {
  int STEP;


  SIZE = atoi(argv[1]);
  if (SIZE > 1000000) {
    //printf("Size: %d Maximum size: %d\n", SIZE, 100);
    exit(-1);
  }
	/* Default: DEFAULT_SIZE */
  for (STEP = 0; STEP < NUM_STEPS; STEP++) {
    initialize(array, STEP);
    qs(array, 0, SIZE-1);
    testit(array);
  }
	printf("\nSIZE \tSTEPS \n");
	printf("%d \t%d \n", SIZE, NUM_STEPS);

} /* main */


/*
 * vim:ts=2:sw=2:
 */
