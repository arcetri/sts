/*****************************************************************************
                          F R E Q U E N C Y   T E S T
 *****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "externs.h"
#include "utilities.h"


static const enum test test_num = TEST_FREQUENCY;	// this test number


/*
 * Frequency_init - initalize the Frequency test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Frequency_init(struct state *state)
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
 * Frequency_iterate - interate one bit stream for Frequency test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Frequency_iterate(struct state *state)
{
	int n;		// Length of a single bit stream
	int		i;
	double	f, s_obs, p_value, sum, sqrt2 = 1.41421356237309504880;

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
	
	sum = 0.0;
	for ( i=0; i<n; i++ )
		sum += 2*(int)epsilon[i]-1;
	s_obs = fabs(sum)/sqrt(n);
	f = s_obs/sqrt2;
	p_value = erfc(f);

	fprintf(stats[test_num], "\t\t\t      FREQUENCY TEST\n");
	fprintf(stats[test_num], "\t\t---------------------------------------------\n");
	fprintf(stats[test_num], "\t\tCOMPUTATIONAL INFORMATION:\n");
	fprintf(stats[test_num], "\t\t---------------------------------------------\n");
	fprintf(stats[test_num], "\t\t(a) The nth partial sum = %d\n", (int)sum);
	fprintf(stats[test_num], "\t\t(b) S_n/n               = %f\n", sum/n);
	fprintf(stats[test_num], "\t\t---------------------------------------------\n");

	fprintf(stats[test_num], "%s\t\tp_value = %f\n\n", p_value < ALPHA ? "FAILURE" : "SUCCESS", p_value); fflush(stats[test_num]);
	fprintf(results[test_num], "%f\n", p_value); fflush(results[test_num]);
}
