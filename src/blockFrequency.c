/*****************************************************************************
                    B L O C K   F R E Q U E N C Y   T E S T
 *****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "externs.h"
#include "cephes.h"
#include "utilities.h"


static const enum test test_num = TEST_BLOCK_FREQUENCY;	// this test number


/*
 * BlockFrequency_init - initalize the Block Frequency test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
BlockFrequency_init(struct state *state)
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
 * BlockFrequency_iterate - interate one bit stream for Block Frequency test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
BlockFrequency_iterate(struct state *state)
{
	int M;		// Block Frequency Test - block length
	int n;		// Length of a single bit stream
	int		i, j, N, blockSum;
	double	p_value, sum, pi, v, chi_squared;

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
	M = state->tp.blockFrequencyBlockLength;
	n = state->tp.n;
	
	N = n/M; 		/* # OF SUBSTRING BLOCKS      */
	sum = 0.0;
	
	for ( i=0; i<N; i++ ) {
		blockSum = 0;
		for ( j=0; j<M; j++ )
			blockSum += epsilon[j+i*M];
		pi = (double)blockSum/(double)M;
		v = pi - 0.5;
		sum += v*v;
	}
	chi_squared = 4.0 * M * sum;
	p_value = cephes_igamc(N/2.0, chi_squared/2.0);

	fprintf(stats[test_num], "\t\t\tBLOCK FREQUENCY TEST\n");
	fprintf(stats[test_num], "\t\t---------------------------------------------\n");
	fprintf(stats[test_num], "\t\tCOMPUTATIONAL INFORMATION:\n");
	fprintf(stats[test_num], "\t\t---------------------------------------------\n");
	fprintf(stats[test_num], "\t\t(a) Chi^2           = %f\n", chi_squared);
	fprintf(stats[test_num], "\t\t(b) # of substrings = %d\n", N);
	fprintf(stats[test_num], "\t\t(c) block length    = %d\n", M);
	fprintf(stats[test_num], "\t\t(d) Note: %d bits were discarded.\n", n % M);
	fprintf(stats[test_num], "\t\t---------------------------------------------\n");

	fprintf(stats[test_num], "%s\t\tp_value = %f\n\n", p_value < ALPHA ? "FAILURE" : "SUCCESS", p_value); fflush(stats[test_num]);
	fprintf(results[test_num], "%f\n", p_value); fflush(results[test_num]);
}
