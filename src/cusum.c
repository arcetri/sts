/*****************************************************************************
		    C U M U L A T I V E   S U M S   T E S T
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "externs.h"
#include "cephes.h"
#include "utilities.h"


static const enum test test_num = TEST_CUSUM;	// this test number


/*
 * CumulativeSums_init - initalize the Cumluative Sums test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
CumulativeSums_init(struct state *state)
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
 * CumulativeSums_iterate - interate one bit stream for Cumluative Sums test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
CumulativeSums_iterate(struct state *state)
{
	int n;		// Length of a single bit stream
	int		S, sup, inf, z, zrev, k;
	double	sum1, sum2, p_value;

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "interate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}

	/*
	 * collect parameters from state
	 */
	n = state->tp.n;

	S = 0;
	sup = 0;
	inf = 0;
	for ( k=0; k<n; k++ ) {
		epsilon[k] ? S++ : S--;
		if ( S > sup )
			sup++;
		if ( S < inf )
			inf--;
		z = (sup > -inf) ? sup : -inf;
		zrev = (sup-S > S-inf) ? sup-S : S-inf;
	}

	// forward
	sum1 = 0.0;
	for ( k=(-n/z+1)/4; k<=(n/z-1)/4; k++ ) {
		sum1 += cephes_normal(((4*k+1)*z)/sqrt(n));
		sum1 -= cephes_normal(((4*k-1)*z)/sqrt(n));
	}
	sum2 = 0.0;
	for ( k=(-n/z-3)/4; k<=(n/z-1)/4; k++ ) {
		sum2 += cephes_normal(((4*k+3)*z)/sqrt(n));
		sum2 -= cephes_normal(((4*k+1)*z)/sqrt(n));
	}

	p_value = 1.0 - sum1 + sum2;

	fprintf(stats[test_num], "\t\t      CUMULATIVE SUMS (FORWARD) TEST\n");
	fprintf(stats[test_num], "\t\t-------------------------------------------\n");
	fprintf(stats[test_num], "\t\tCOMPUTATIONAL INFORMATION:\n");
	fprintf(stats[test_num], "\t\t-------------------------------------------\n");
	fprintf(stats[test_num], "\t\t(a) The maximum partial sum = %d\n", z);
	fprintf(stats[test_num], "\t\t-------------------------------------------\n");

	if ( isNegative(p_value) || isGreaterThanOne(p_value) )
		fprintf(stats[test_num], "\t\tWARNING:  P_VALUE IS OUT OF RANGE\n");

	fprintf(stats[test_num], "%s\t\tp_value = %f\n\n", p_value < ALPHA ? "FAILURE" : "SUCCESS", p_value);
	fprintf(results[test_num], "%f\n", p_value);

	// backwards
	sum1 = 0.0;
	for ( k=(-n/zrev+1)/4; k<=(n/zrev-1)/4; k++ ) {
		sum1 += cephes_normal(((4*k+1)*zrev)/sqrt(n));
		sum1 -= cephes_normal(((4*k-1)*zrev)/sqrt(n));
	}
	sum2 = 0.0;
	for ( k=(-n/zrev-3)/4; k<=(n/zrev-1)/4; k++ ) {
		sum2 += cephes_normal(((4*k+3)*zrev)/sqrt(n));
		sum2 -= cephes_normal(((4*k+1)*zrev)/sqrt(n));
	}
	p_value = 1.0 - sum1 + sum2;

	fprintf(stats[test_num], "\t\t      CUMULATIVE SUMS (REVERSE) TEST\n");
	fprintf(stats[test_num], "\t\t-------------------------------------------\n");
	fprintf(stats[test_num], "\t\tCOMPUTATIONAL INFORMATION:\n");
	fprintf(stats[test_num], "\t\t-------------------------------------------\n");
	fprintf(stats[test_num], "\t\t(a) The maximum partial sum = %d\n", zrev);
	fprintf(stats[test_num], "\t\t-------------------------------------------\n");

	if ( isNegative(p_value) || isGreaterThanOne(p_value) )
		fprintf(stats[test_num], "\t\tWARNING:  P_VALUE IS OUT OF RANGE\n");

	fprintf(stats[test_num], "%s\t\tp_value = %f\n\n", p_value < ALPHA ? "FAILURE" : "SUCCESS", p_value); fflush(stats[test_num]);
	fprintf(results[test_num], "%f\n", p_value); fflush(results[test_num]);
}
