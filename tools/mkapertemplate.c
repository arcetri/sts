/*
 * mkapertemplate - make an Aperiodic Template
 *
 * NOTE: This function was based on, in part, code from NIST Special Publication 800-22 Revision 1a:
 *
 *      http://csrc.nist.gov/groups/ST/toolkit/rng/documents/SP800-22rev1a.pdf
 *
 * In particular see section F.2 (page F-4) of the document revised April 2010.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>

#if __STDC_VERSION__ >= 199901L
#   include <stdint.h>
#else				// __STDC_VERSION__ >= 199901L
#   ifndef int64_t
typedef unsigned long long int64_t;	/* 64-bit unsigned word */
#   endif
#endif				// __STDC_VERSION__ >= 199901L

#include "debug.h"

#define B (8*sizeof(unsigned long long))

int debuglevel = 0;
char *program = NULL;
const char *const version = "mkapertemplate-2.1.2-2.cisco";
static const char *const usage = "bits template dataInfo";
static void displayBits(FILE * fp, unsigned int *A, unsigned long long value, long int count, long int *nonPeriodic);

int
main(int argc, char *argv[])
{
	char *template;		// filename of template
	FILE *fp1;		// open stream for template
	char *dataInfo;		// filenane of dataInfo
	FILE *fp2;		// open stream for dataInfo
	long int M;		// bit size of template
	long int count;
	unsigned long long num;
	long int nonPeriodic;
	unsigned int *A;
	unsigned long long i;

	/*
	 * parse args
	 */
	program = argv[0];
	if (argc != 4) {
		usage_err(usage, 1, __FUNCTION__, "expected 4 argumentsi, found only %d", argc);
	}
	errno = 0;
	M = strtol(argv[1], NULL, 0);
	if (errno != 0) {
		errp(2, __FUNCTION__, "error in parsing bits argument: %s", argv[1]);
	}
	template = argv[2];
	dataInfo = argv[3];
	if (M < 1) {
		err(3, __FUNCTION__, "bits argument: %ld must be > 1", M);
	}
	if (M >= B) {
		err(3, __FUNCTION__, "bits argument: %ld must be < %ld", M, B);
	}

	/*
	 * allocation of bit as byte array
	 */
	A = calloc(B, sizeof(unsigned int));
	if (A == NULL) {
		errp(1, __FUNCTION__, "cannot calloc %ld unsigned int of %ld bytes each", B, sizeof(unsigned int));
	}

	/*
	 * open files for output
	 */
	fp1 = fopen(template, "w");
	if (fp1 == NULL) {
		errp(2, __FUNCTION__, "cannot open for writing, %s", template);
	}
	fp2 = fopen(dataInfo, "a");
	if (fp2 == NULL) {
		errp(2, __FUNCTION__, "cannot open for appending, %s", dataInfo);
	}

	/*
	 * setup to generate templates
	 */
	/*
	 * NOTE: Mathematical expression code rewrite, old code commented out below:
	 *
	 * num = pow(2, M);
	 * count = log(num) / log(2);
	 */
	num = (unsigned long long) 1 << M;
	count = M;
	nonPeriodic = 0;

	/*
	 * form non-periodic bits
	 */
	for (i = 1; i < num; i++) {
		displayBits(fp1, A, i, count, &nonPeriodic);
	}

	/*
	 * report
	 */
	fprintf(fp2, "M = %ld\n", M);
	fprintf(fp2, "# of nonperiodic templates = %ld\n", nonPeriodic);
	fprintf(fp2, "# of all possible templates = %lld\n", num);
	fprintf(fp2, "{# nonperiodic}/{# templates} = %f\n", (double) nonPeriodic / num);
	fprintf(fp2, "==========================================================\n");

	/*
	 * cleanup
	 */
	fclose(fp1);
	fclose(fp2);
	free(A);
	A = NULL;
	exit(0);
}

static void
displayBits(FILE * fp, unsigned int *A, unsigned long long value, long int count, long int *nonPeriodic)
{
	unsigned long long displayMask;
	int match;
	int c;
	int i;

	displayMask = (unsigned long long) 1 << (B - 1);
	for (i = 0; i < B; i++)
		A[i] = 0;
	for (c = 1; c <= B; c++) {
		if (value & displayMask)
			A[c - 1] = 1;
		else
			A[c - 1] = 0;
		value <<= (unsigned long long) 1;
	}
	for (i = 1; i < count; i++) {
		match = 1;
		if ((A[B - count] != A[B - 1]) && ((A[B - count] != A[B - 2]) || (A[B - count + 1] != A[B - 1]))) {
			for (c = B - count; c <= (B - 1) - i; c++) {
				if (A[c] != A[c + i]) {
					match = 0;
					break;
				}
			}
		}
		if (match) {
			/*
			 * printf("\nPERIODIC TEMPLATE: SHIFT = %d\n",i);
			 */
			break;
		}
	}
	if (!match) {
		for (c = B - count; c < (B - 1); c++)
			fprintf(fp, "%u ", A[c]);
		fprintf(fp, "%u\n", A[B - 1]);
		(*nonPeriodic)++;
	}
	return;
}
