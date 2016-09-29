/*****************************************************************************
            R A N D O M   E X C U R S I O N S   V A R I A N T   T E S T
 *****************************************************************************/

#include <stdio.h> 
#include <math.h> 
#include <string.h>
#include <stdlib.h>
#include "externs.h"
#include "cephes.h"
#include "utilities.h"


static const enum test test_num = TEST_RND_EXCURSION_VAR;	// this test number


/*
 * RandomExcursionsVariant_init - initalize the Random Excursions Variant test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
RandomExcursionsVariant_init(struct state *state)
{
	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "interate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}

	/*
	 * create working sub-directory if forming files such as results.txt and stats.txt
	 */
	if (state->resultstxtFlag == true) {
		state->subDir[test_num] = precheckSubdir(state, state->testNames[test_num]);
		dbg(DBG_HIGH, "test %s[%d] will use subdir: %s", state->testNames[test_num], test_num, state->subDir[test_num]);
	}
	return;
}


/*
 * RandomExcursionsVariant_iterate - interate one bit stream for Random Excursions Variant test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
RandomExcursionsVariant_iterate(struct state *state)
{
	int n;		// Length of a single bit stream
	int		i, p, J, x, constraint, count, *S_k;
	int		stateX[18] = { -9, -8, -7, -6, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	double	p_value;

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "interate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	n = state->tp.n;
	
	if ( (S_k = (int *)calloc(n, sizeof(int))) == NULL ) {
		fprintf(stats[test_num], "\t\tRANDOM EXCURSIONS VARIANT: Insufficent memory allocated.\n");
		return;
	}
	J = 0;
	S_k[0] = 2*(int)epsilon[0] - 1;
	for ( i=1; i<n; i++ ) {
		S_k[i] = S_k[i-1] + 2*epsilon[i] - 1;
		if ( S_k[i] == 0 )
			J++;
	}
	if ( S_k[n-1] != 0 )
		J++;

	fprintf(stats[test_num], "\t\t\tRANDOM EXCURSIONS VARIANT TEST\n");
	fprintf(stats[test_num], "\t\t--------------------------------------------\n");
	fprintf(stats[test_num], "\t\tCOMPUTATIONAL INFORMATION:\n");
	fprintf(stats[test_num], "\t\t--------------------------------------------\n");
	fprintf(stats[test_num], "\t\t(a) Number Of Cycles (J) = %d\n", J);
	fprintf(stats[test_num], "\t\t(b) Sequence Length (n)  = %d\n", n);
	fprintf(stats[test_num], "\t\t--------------------------------------------\n");

	constraint = (int)MAX(0.005*pow(n, 0.5), 500);
	if (J < constraint) {
		fprintf(stats[test_num], "\n\t\tWARNING:  TEST NOT APPLICABLE.  THERE ARE AN\n");
		fprintf(stats[test_num], "\t\t\t  INSUFFICIENT NUMBER OF CYCLES.\n");
		fprintf(stats[test_num], "\t\t---------------------------------------------\n");
		for ( i=0; i<18; i++ )
			fprintf(results[test_num], "%f\n", 0.0);
	}
	else {
		for ( p=0; p<=17; p++ ) {
			x = stateX[p];
			count = 0;
			for ( i=0; i<n; i++ )
				if ( S_k[i] == x )
					count++;
			p_value = erfc(fabs(count-J)/(sqrt(2.0*J*(4.0*fabs(x)-2))));

			if ( isNegative(p_value) || isGreaterThanOne(p_value) )
				fprintf(stats[test_num], "\t\t(b) WARNING: P_VALUE IS OUT OF RANGE.\n");
			fprintf(stats[test_num], "%s\t\t", p_value < ALPHA ? "FAILURE" : "SUCCESS");
			fprintf(stats[test_num], "(x = %2d) Total visits = %4d; p-value = %f\n", x, count, p_value);
			fprintf(results[test_num], "%f\n", p_value); fflush(results[test_num]);
		}
	}
	fprintf(stats[test_num], "\n"); fflush(stats[test_num]);
	free(S_k);
}
