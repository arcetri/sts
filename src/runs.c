/*****************************************************************************
                              R U N S   T E S T 
 *****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "externs.h"
#include "cephes.h"
#include "utilities.h"


static const enum test test_num = TEST_RUNS;	// this test number


/*
 * Runs_init - initalize the Runs test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Runs_init(struct state *state)
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
 * Runs_iterate - interate one bit stream for Runs test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Runs_iterate(struct state *state)
{
	int n;		// Length of a single bit stream
	int		S, k;
	double	pi, V, erfc_arg, p_value;

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "interate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	n = state->tp.n;

	S = 0;
	for ( k=0; k<n; k++ )
		if ( epsilon[k] )
			S++;
	pi = (double)S / (double)n;

	if ( fabs(pi - 0.5) > (2.0 / sqrt(n)) ) {
		fprintf(stats[TEST_RUNS], "\t\t\t\tRUNS TEST\n");
		fprintf(stats[TEST_RUNS], "\t\t------------------------------------------\n");
		fprintf(stats[TEST_RUNS], "\t\tPI ESTIMATOR CRITERIA NOT MET! PI = %f\n", pi);
		p_value = 0.0;
	}
	else {

		V = 1;
		for ( k=1; k<n; k++ )
			if ( epsilon[k] != epsilon[k-1] )
				V++;
	
		erfc_arg = fabs(V - 2.0 * n * pi * (1-pi)) / (2.0 * pi * (1-pi) * sqrt(2*n));
		p_value = erfc(erfc_arg);
		
		fprintf(stats[TEST_RUNS], "\t\t\t\tRUNS TEST\n");
		fprintf(stats[TEST_RUNS], "\t\t------------------------------------------\n");
		fprintf(stats[TEST_RUNS], "\t\tCOMPUTATIONAL INFORMATION:\n");
		fprintf(stats[TEST_RUNS], "\t\t------------------------------------------\n");
		fprintf(stats[TEST_RUNS], "\t\t(a) Pi                        = %f\n", pi);
		fprintf(stats[TEST_RUNS], "\t\t(b) V_n_obs (Total # of runs) = %d\n", (int)V);
		fprintf(stats[TEST_RUNS], "\t\t(c) V_n_obs - 2 n pi (1-pi)\n");
		fprintf(stats[TEST_RUNS], "\t\t    -----------------------   = %f\n", erfc_arg);
		fprintf(stats[TEST_RUNS], "\t\t      2 sqrt(2n) pi (1-pi)\n");
		fprintf(stats[TEST_RUNS], "\t\t------------------------------------------\n");
		if ( isNegative(p_value) || isGreaterThanOne(p_value) )
			fprintf(stats[TEST_RUNS], "WARNING:  P_VALUE IS OUT OF RANGE.\n");

		fprintf(stats[TEST_RUNS], "%s\t\tp_value = %f\n\n", p_value < ALPHA ? "FAILURE" : "SUCCESS", p_value); fflush(stats[TEST_RUNS]);
	}

	fprintf(results[TEST_RUNS], "%f\n", p_value); fflush(results[TEST_RUNS]);
}
