#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <omp.h>

#define DIGITS_AMOUNT 	10    /* number of all possible keys (bucket's size) */
#define DEBUG 		 0

/* max number of digits of the elements' list (calculated) */
int maxlen = 0;

/* calculate power */
uint32_t p(uint32_t base, uint32_t exp)
{
	return (exp==0 ? 1 : p(base, exp-1)*base);
}

/*
 * Radix sorting
 */
void radixsort(uint32_t *in, uint32_t n)
{
	uint32_t digit, t, max=0;
	int i, j;

	uint32_t *tmp = (uint32_t *)calloc(n, sizeof(uint32_t));

	if (!tmp) {
		fprintf(stderr, "ERROR: not enough memory!\n");
		abort();
	}

	/* 0.1) caclculate max */
	#pragma omp parallel for private(i)
	for (i=0; i<n; i++) {
		if (in[i]>max) {
			#pragma omp critical
			max=in[i];
		}
	}
	#if (DEBUG>1)
	printf("END MAX @ %lf\n", omp_get_wtime());
	#endif

	/* 0.2) calculate max number of digit [K] */
	t = max;
	while((t/=10) >0)
		maxlen++;
	maxlen++;

	#if (DEBUG>1)
	printf("END MAXLEN @ %lf\n", omp_get_wtime());
	#endif

	/* 0.3) allocate bucket and init to 0 */
	int **bucket;
    	bucket = (int **)calloc(maxlen, sizeof(int *));

	if (!bucket) {
		fprintf(stderr, "ERROR: not enough memory!\n");
		abort();
	}

	for(i=0; i<maxlen+1; i++) {
        	*(bucket + i) = (int *)calloc(DIGITS_AMOUNT, sizeof(int));
		if (!*(bucket +i)) {
			fprintf(stderr, "ERROR: not enough memory!\n");
			abort();
		}
	}

	//max [10][10]
	for(i=0; i<maxlen; i++) {
		for(j=0; j<DIGITS_AMOUNT; j++)
			bucket[i][j] = 0;
	}
	#if (DEBUG>1)
	printf("END BUCKET INIT  @ %lf\n", omp_get_wtime());
	#endif

	/* 2) each bucket is how many times that digit is read */
	#pragma omp parallel for private(i, digit) schedule(static)
	for (i=0; i<maxlen; i++) {
		digit = p(10,i);

		#pragma omp parallel for private(j) schedule(static)
		for (j=0; j<n; j++) {
			bucket[i][in[j]/digit%DIGITS_AMOUNT]++;
		}
	}

	/* 3) sum precedent bucket */
	for(i=1; i<DIGITS_AMOUNT; i++) {
		for(j=0; j<maxlen; j++)
			bucket[j][i]+=bucket[j][i-1];
	}

	#if (DEBUG>1)
		printf("\t[0]\t[1]\t\[2]\t[3]\t[4]\t[5]\t[6]\t[7]\t[8]\t[9]\n");
		for (i=0; i<maxlen; i++) {
			printf("[%i]\t", i);
			for (j=0; j<DIGITS_AMOUNT; j++)
				printf(" %i\t", bucket[i][j]);
			printf("\n");
		}
		printf("\n");
	#endif

	for(j=0; j<maxlen; j++) {
		digit = p(10,j);

		/* 4) order tmp list as bucket-array says */
		#pragma omp parallel for ordered private(i) schedule(static)
		for(i=n-1;i>=0;i--) {
			#pragma omp ordered
			tmp[--bucket[j][in[i]/digit%DIGITS_AMOUNT]]=in[i];
		}

		/* 5) copy into input list */
		#pragma omp parallel for private(i) schedule(static)
		for(i=0;i<n;i++)
			in[i]=tmp[i];

		#if (DEBUG>1)
			char layout[80];
			int c;
			sprintf(layout, "%%%iu\n", maxlen+1);
			for (c=0; c<n; c++)
				printf(layout, in[c]);
			printf(" ***\n");
		#endif
	}

	cfree(tmp);
}

/*
 * sorts elements separated by space and/or new line in an input file
 */
int main(int argc, char **argv)
{
	uint32_t i, *input, n = 1000;
	int k=5;

	double start_time;
	char str[10] = {0};

	/*
	 *  1st opt param size of input list (default 1000)		[N]
	 *  2nd opt param max digits of the elements' list (default 5)	[K]
	 */
	if (argc >= 2) {
		n = strtoul(argv[1], NULL, 10);
		if (n < 0 || n > UINT32_MAX)
			n = 1000;
	}
	if (argc >= 3) {
		k = atoi(argv[2]);
		if (k < 0 || k > 10 )
			k = 5;
		for(i=0;i<k;i++)
			str[i] = '9';
	}

	#if (DEBUG>0)
		printf("Debug Level: %i\n", DEBUG);
	#endif

	/* allocation of memory */
	input = (uint32_t *)calloc(n, sizeof(uint32_t));
	if (!input) {
		fprintf(stderr, "ERROR: not enough memory!\n");
		return 1;
	}

	/* generate random input list */
	#if (DEBUG>0)
		printf("Random Creation\n");
	#endif
	srand(0);
	for(i=0;i<n; i++)
		input[i] = rand() % strtoul(str,NULL,10) + 1;

	/* print unordered input list */
	#if (DEBUG>0)
		char layout[80];
		sprintf(layout, "%%%iu\n", k+1);
		for (i=0; i<n; i++)
			printf(layout, input[i]);
	#endif

	/* sort input list */
	#if (DEBUG>0)
		printf("Radix Sorting\n");
	#endif
	start_time = omp_get_wtime();
	radixsort(input, n);
	#if (DEBUG>0)
		printf("K\t\tN\t\tTIME\n");
	#endif
	printf("%i\t%8u\t\t%f\n", maxlen, n, omp_get_wtime()-start_time);

	/* print ordered input list */
	#if (DEBUG>0)
		sprintf(layout, "%%%iu\n", maxlen+1);
		printf(layout, input[0]);
		for (i=1; i<n; i++) {
			printf(layout, input[i]);
			if (input[i]<input[i-1])
				printf("ERRORE\n");
		}
	#endif

	/* free memory */
	cfree(input);

	return 0;
}
