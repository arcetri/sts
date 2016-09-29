/*****************************************************************************
                              R A N K   T E S T
 *****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "externs.h"
#include "cephes.h"
#include "matrix.h"
#include "utilities.h"


static const enum test test_num = TEST_RANK;	// this test number


/*
 * Rank_init - initalize the Rank test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Rank_init(struct state *state)
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
 * Rank_iterate - interate one bit stream for Rank test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Rank_iterate(struct state *state)
{
	int n;		// Length of a single bit stream
	int			N, i, k, r;
	double		p_value, product, chi_squared, arg1, p_32, p_31, p_30, R, F_32, F_31, F_30;
	BitSequence	**matrix = create_matrix(32, 32);

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "interate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	n = state->tp.n;
	
	N = n/(32*32);
	if ( isZero(N) ) {
		fprintf(stats[test_num], "\t\t\t\tRANK TEST\n");
		fprintf(stats[test_num], "\t\tError: Insuffucient # Of Bits To Define An 32x32 (%dx%d) Matrix\n", 32, 32);
		p_value = 0.00;
	}
	else {
		r = 32;					/* COMPUTE PROBABILITIES */
		product = 1;
		for ( i=0; i<=r-1; i++ )
			product *= ((1.e0-pow(2, i-32))*(1.e0-pow(2, i-32)))/(1.e0-pow(2, i-r));
		p_32 = pow(2, r*(32+32-r)-32*32) * product;
		
		r = 31;
		product = 1;
		for ( i=0; i<=r-1; i++ )
			product *= ((1.e0-pow(2, i-32))*(1.e0-pow(2, i-32)))/(1.e0-pow(2, i-r));
		p_31 = pow(2, r*(32+32-r)-32*32) * product;
		
		p_30 = 1 - (p_32+p_31);
		
		F_32 = 0;
		F_31 = 0;
		for ( k=0; k<N; k++ ) {			/* FOR EACH 32x32 MATRIX   */
			def_matrix(32, 32, matrix, k);
#if (DISPLAY_MATRICES == 1)
			display_matrix(32, 32, matrix);
#endif
			R = computeRank(32, 32, matrix);
			if ( R == 32 )
				F_32++;			/* DETERMINE FREQUENCIES */
			if ( R == 31 )
				F_31++;
		}
		F_30 = (double)N - (F_32+F_31);
		
		chi_squared =(pow(F_32 - N*p_32, 2)/(double)(N*p_32) +
					  pow(F_31 - N*p_31, 2)/(double)(N*p_31) +
					  pow(F_30 - N*p_30, 2)/(double)(N*p_30));
		
		arg1 = -chi_squared/2.e0;

		fprintf(stats[test_num], "\t\t\t\tRANK TEST\n");
		fprintf(stats[test_num], "\t\t---------------------------------------------\n");
		fprintf(stats[test_num], "\t\tCOMPUTATIONAL INFORMATION:\n");
		fprintf(stats[test_num], "\t\t---------------------------------------------\n");
		fprintf(stats[test_num], "\t\t(a) Probability P_%d = %f\n", 32,p_32);
		fprintf(stats[test_num], "\t\t(b)             P_%d = %f\n", 31,p_31);
		fprintf(stats[test_num], "\t\t(c)             P_%d = %f\n", 30,p_30);
		fprintf(stats[test_num], "\t\t(d) Frequency   F_%d = %d\n", 32,(int)F_32);
		fprintf(stats[test_num], "\t\t(e)             F_%d = %d\n", 31,(int)F_31);
		fprintf(stats[test_num], "\t\t(f)             F_%d = %d\n", 30,(int)F_30);
		fprintf(stats[test_num], "\t\t(g) # of matrices    = %d\n", N);
		fprintf(stats[test_num], "\t\t(h) Chi^2            = %f\n", chi_squared);
		fprintf(stats[test_num], "\t\t(i) NOTE: %d BITS WERE DISCARDED.\n", n%(32*32));
		fprintf(stats[test_num], "\t\t---------------------------------------------\n");

		p_value = exp(arg1);
		if ( isNegative(p_value) || isGreaterThanOne(p_value) )
			fprintf(stats[test_num], "WARNING:  P_VALUE IS OUT OF RANGE.\n");

		for ( i=0; i<32; i++ )				/* DEALLOCATE MATRIX  */
			free(matrix[i]);
		free(matrix);
	}
	fprintf(stats[test_num], "%s\t\tp_value = %f\n\n", p_value < ALPHA ? "FAILURE" : "SUCCESS", p_value); fflush(stats[test_num]);
	fprintf(results[test_num], "%f\n", p_value); fflush(results[test_num]);
}
