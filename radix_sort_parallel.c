#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <omp.h>

#define DIGITS_AMOUNT 	10    /* number of all possible keys (bucket's size) */

uint32_t tenPowers[10] = {
	1,		10,		100,
	1000,		10000,		100000,
	1000000,	10000000,	100000000,
	1000000000,
};

/*
 * Gives power of 10 at certain exp
 */
uint32_t p(int exp)
{
	return tenPowers[exp];
}

/*
 * Parallel Radix sorting
 */
void radixsort_parallel(uint32_t *input_array, uint32_t input_array_size,
				uint32_t max_digits_size, int debug, int chunksize)
{
	uint32_t digit;
	int f, i, j;
	double start_time;

	if (debug > 0)
		start_time = omp_get_wtime();

	/* 1.1) allocate temporary array */
	uint32_t *tmp = (uint32_t *)calloc(input_array_size, sizeof(uint32_t));
	if (!tmp) {
		fprintf(stderr, "ERROR: not enough memory!\n");
		abort();
	}
	/* 1.2) Shared buckets definition */
	int shared_buckets[max_digits_size][DIGITS_AMOUNT];
	for (j=0; j<max_digits_size; j++) {
		for(f=0; f<DIGITS_AMOUNT; f++)
			shared_buckets[j][f]=0;
	}

	if (debug > 0) {
		printf("  1) ALLOCATE MEM, INIT BUCKETS\t\t%lf\n",
						    omp_get_wtime()-start_time);
	}

	if (debug > 0)
		start_time = omp_get_wtime();
	/* Least Significant Digit */
	#pragma omp parallel for private(digit, j, i, f) schedule(dynamic, chunksize)
	for (i=0; i<input_array_size; i++) {

		/* each thread make these step to its pool of values */
		for (j=0; j<max_digits_size; j++) {
			digit = p(j);

			/* 2) each bucket is how many times that digit is read
			 *    so we put in shared bucket all infos
			 */
			f=input_array[i]/digit%DIGITS_AMOUNT;
			#pragma omp atomic
			shared_buckets[j][f]++;
		}
	}
	if (debug > 0) {
		printf("  2) CREATION OF BUCKETS\t\t%lf\n",
						    omp_get_wtime()-start_time);
	}

	/* 3) sum precedent bucket */
	if (debug > 0)
		start_time = omp_get_wtime();
	for (j=0; j<max_digits_size; j++) {
		for (i=1; i<DIGITS_AMOUNT; i++)
			shared_buckets[j][i]+=shared_buckets[j][i-1];
	}
	if (debug > 2) {
		printf("   [0]\t[1]\t\[2]\t[3]\t[4]\t[5]\t[6]\t[7]\t[8]\t[9]\n");
		for (j=0; j<max_digits_size; j++) {
			printf("%i  ", j);
			for (i=0; i<DIGITS_AMOUNT; i++)
				printf(" %i\t", shared_buckets[j][i]);
			printf("\n");
		}
	}
	if (debug > 0) {
		printf("  3) SUMMING BUCKETS\t\t\t%lf\n",
						    omp_get_wtime()-start_time);
	}

	if (debug > 0)
		start_time = omp_get_wtime();
	for (j=0; j<max_digits_size; j++) {
		digit = p(j);

		/* 4) order tmp list as shared buckets array says */
		for(i=input_array_size-1;i>=0;i--) {
			int unit = input_array[i]/digit%DIGITS_AMOUNT;
			int pos = --shared_buckets[j][unit];
			tmp[pos]=input_array[i];
		}
		/* 5) copy into input list */
		for(i=0;i<input_array_size;i++)
			input_array[i]=tmp[i];
		if (debug > 2) {
			char layout[80];
			sprintf(layout, "%%%iu\n", max_digits_size+1);
			for (i=0; i<input_array_size; i++)
				printf(layout, input_array[i]);
			printf("***\n");
		}
	}
	if (debug > 0) {
		printf("4-5) USING BUCKETS AND COPYING\t\t%lf\n",
						    omp_get_wtime()-start_time);
	}

	cfree(tmp);
}

/*
 * sorts elements separated by space and/or new line in an input file
 */
int main(int argc, char **argv)
{
	uint32_t i, *input, num_of_values = 1000, max_value = 0, tmp_value = 0;
	int value_lenght = 5, max_digits_size = 0;
	double start_time;
	char str[10] = {0};
	int chunksize=1;
	int debug=0;
	char layout[80];

	/*
	 *  1st opt param size of input list (default 1000)		[N]
	 *  2nd opt param max digits of the elements' list (default 5)	[K]
	 *  3rd opt debug level(num) (default level 0)
	 *  4rd opt chunk size (default is 1)
	 */
	if (argc >= 2) {
		num_of_values = strtoul(argv[1], NULL, 10);
		if (num_of_values < 0 || num_of_values > UINT32_MAX)
			num_of_values = 1000;
	}
	if (argc >= 3) {
		value_lenght = atoi(argv[2]);
		if (value_lenght < 0 || value_lenght > 10 )
			value_lenght = 5;
	}
	if (argc >= 4) {
		int tmp_debug;
		if ((tmp_debug=atoi(argv[3]))>0) {
			debug=tmp_debug;
		}
	}
	if (argc >= 5) {
		int tmp_chunk;
		if ((tmp_chunk=atoi(argv[4]))>0) {
			chunksize=tmp_chunk;
		}
	}

        if (debug > 0) {
		printf("DEBUG LEVEL: %i\n", debug);
		printf("CHUNKSIZE  : %i\n", chunksize);
		printf("THREADS    : %i\n", omp_get_num_procs());
	}

	/* allocation of memory */
	if (debug > 0)
		start_time = omp_get_wtime();
	input = (uint32_t *)calloc(num_of_values, sizeof(uint32_t));
	if (!input) {
		fprintf(stderr, "ERROR: not enough memory!\n");
		return 1;
	}
	if (debug > 0) {
		printf("INPUT ALLOCATION\t\t\t%lf\n", omp_get_wtime() -
								    start_time);
	}

	/* generate random input list */
	if (debug > 0)
		start_time = omp_get_wtime();
	for(i=0;i<value_lenght;i++)
		str[i] = '9';
	srand(time(NULL));
	for(i=0;i<num_of_values; i++) {
		input[i] = rand() % strtoul(str,NULL,10) + 1;
		if (max_value < input[i])
			max_value = input[i];
	}
	if (debug > 0) {
		printf("RANDOM GENERATION\t\t\t%lf\n", omp_get_wtime() -
								    start_time);
	}

        /* calculates max number of digit in max_value [K] */
	if (debug > 0)
		start_time = omp_get_wtime();
        tmp_value = max_value;
        while((tmp_value/=10)>0)
                max_digits_size++;
        max_digits_size++;
	if (debug > 0) {
		printf("CALCULATION OF K\t\t\t%lf\n",
						    omp_get_wtime()-start_time);
	}


	/* print unordered input list */
	if (debug > 1) {
		sprintf(layout, "%%%iu\n", max_digits_size+1);
		for (i=0; i<num_of_values; i++)
			printf(layout, input[i]);
	}

	/* sort input list */
	if (debug > 0)
		printf("START RADIX SORTING\n");
	start_time = omp_get_wtime();
	radixsort_parallel(input, num_of_values, max_digits_size, debug, chunksize);
	if (debug > 0)
		printf("K\t\tN\tTIME\n");
	printf("%i\t%8u\t%f\n", max_digits_size, num_of_values,
						  omp_get_wtime() - start_time);

	/* print ordered input list */
	if (debug > 1) {
		sprintf(layout, "%%%iu\n", max_digits_size+1);
		for (i=0; i<num_of_values; i++)
			printf(layout, input[i]);
	}

	/* check values order */
	int not_ordered = 0;
	for (i=1; i<num_of_values; i++) {
		if (input[i]<input[i-1]) {
			not_ordered = 1;
			break;
		}
	}
	if (not_ordered) {
		printf("ORDERED CHECK: FAIL!\n");
	} else {
		if (debug > 0)
			printf("ORDERED CHECK: PASS\n");
	}

	/* free memory */
	cfree(input);

	return 0;
}

