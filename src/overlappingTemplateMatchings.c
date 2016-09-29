/*****************************************************************************
               O V E R L A P P I N G   T E M P L A T E   T E S T
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "externs.h"
#include "utilities.h"
#include "cephes.h"  


static const enum test test_num = TEST_OVERLAPPING;	// this test number


/*
 * forward static function declarations
 */
static double Pr(int u, double eta);


/*
 * OverlappingTemplateMatchings_init - initalize the Overlapping Template test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
OverlappingTemplateMatchings_init(struct state *state)
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
 * OverlappingTemplateMatchings_iterate - interate one bit stream for Overlapping Template test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
OverlappingTemplateMatchings_iterate(struct state *state)
{
	int m;		// Overlapping Template Test - block length
	int n;		// Length of a single bit stream
	int				i, k, match;
	double			W_obs, eta, sum, chi2, p_value, lambda;
	int				M, N, j, K = 5;
	unsigned int	nu[6] = { 0, 0, 0, 0, 0, 0 };
	//double			pi[6] = { 0.143783, 0.139430, 0.137319, 0.124314, 0.106209, 0.348945 };
	double			pi[6] = { 0.364091, 0.185659, 0.139381, 0.100571, 0.0704323, 0.139865 };
	BitSequence		*sequence;

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "interate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	m = state->tp.overlappingTemplateBlockLength;
	n = state->tp.n;

	M = 1032;
	N = n/M;
	
	if ( (sequence = (BitSequence *) calloc(m, sizeof(BitSequence))) == NULL ) {
		fprintf(stats[test_num], "\t\t    OVERLAPPING TEMPLATE OF ALL ONES TEST\n");
		fprintf(stats[test_num], "\t\t---------------------------------------------\n");
		fprintf(stats[test_num], "\t\tTEMPLATE DEFINITION:  Insufficient memory, Overlapping Template Matchings test aborted!\n");
	}
	else
		for ( i=0; i<m; i++ )
			sequence[i] = 1;
	
	lambda = (double)(M-m+1)/pow(2,m);
	eta = lambda/2.0;
	sum = 0.0;
	for ( i=0; i<K; i++ ) {			/* Compute Probabilities */
		pi[i] = Pr(i, eta);
		sum += pi[i];
	}
	pi[K] = 1 - sum;

	for ( i=0; i<N; i++ ) {
		W_obs = 0;
		for ( j=0; j<M-m+1; j++ ) {
			match = 1;
			for ( k=0; k<m; k++ ) {
				if ( sequence[k] != epsilon[i*M+j+k] )
					match = 0;
			}
			if ( match == 1 )
				W_obs++;
		}
		if ( W_obs <= 4 )
			nu[(int)W_obs]++;
		else
			nu[K]++;
	}
	sum = 0;
	chi2 = 0.0;                                   /* Compute Chi Square */
	for ( i=0; i<K+1; i++ ) {
		chi2 += pow((double)nu[i] - (double)N*pi[i], 2)/((double)N*pi[i]);
		sum += nu[i];
	}
	p_value = cephes_igamc(K/2.0, chi2/2.0);

	fprintf(stats[test_num], "\t\t    OVERLAPPING TEMPLATE OF ALL ONES TEST\n");
	fprintf(stats[test_num], "\t\t-----------------------------------------------\n");
	fprintf(stats[test_num], "\t\tCOMPUTATIONAL INFORMATION:\n");
	fprintf(stats[test_num], "\t\t-----------------------------------------------\n");
	fprintf(stats[test_num], "\t\t(a) n (sequence_length)      = %d\n", n);
	fprintf(stats[test_num], "\t\t(b) m (block length of 1s)   = %d\n", m);
	fprintf(stats[test_num], "\t\t(c) M (length of substring)  = %d\n", M);
	fprintf(stats[test_num], "\t\t(d) N (number of substrings) = %d\n", N);
	fprintf(stats[test_num], "\t\t(e) lambda [(M-m+1)/2^m]     = %f\n", lambda);
	fprintf(stats[test_num], "\t\t(f) eta                      = %f\n", eta);
	fprintf(stats[test_num], "\t\t-----------------------------------------------\n");
	fprintf(stats[test_num], "\t\t   F R E Q U E N C Y\n");
	fprintf(stats[test_num], "\t\t  0   1   2   3   4 >=5   Chi^2   P-value  Assignment\n");
	fprintf(stats[test_num], "\t\t-----------------------------------------------\n");
	fprintf(stats[test_num], "\t\t%3d %3d %3d %3d %3d %3d  %f ",
		nu[0], nu[1], nu[2], nu[3], nu[4], nu[5], chi2);

	if ( isNegative(p_value) || isGreaterThanOne(p_value) )
		fprintf(stats[test_num], "WARNING:  P_VALUE IS OUT OF RANGE.\n");

	free(sequence);
	fprintf(stats[test_num], "%f %s\n\n", p_value, p_value < ALPHA ? "FAILURE" : "SUCCESS"); fflush(stats[test_num]);
	fprintf(results[test_num], "%f\n", p_value); fflush(results[test_num]);
}

static double
Pr(int u, double eta)
{
	int		l;
	double	sum, p;
	
	if ( u == 0 )
		p = exp(-eta);
	else {
		sum = 0.0;
		for ( l=1; l<=u; l++ )
			sum += exp(-eta-u*log(2)+l*log(eta)-cephes_lgam(l+1)+cephes_lgam(u)-cephes_lgam(l)-cephes_lgam(u-l+1));
		p = sum;
	}
	return p;
}
