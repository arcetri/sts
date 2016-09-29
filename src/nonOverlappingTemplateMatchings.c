/*****************************************************************************
          N O N O V E R L A P P I N G   T E M P L A T E   T E S T
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "externs.h"
#include "utilities.h"
#include "cephes.h"  


static const enum test test_num = TEST_NONPERIODIC;	// this test number


/*
 * NonOverlappingTemplateMatchings_init - initalize the Nonoverlapping Template test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
NonOverlappingTemplateMatchings_init(struct state *state)
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
 * NonOverlappingTemplateMatchings_iterate - interate one bit stream for Nonoverlapping Template test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
NonOverlappingTemplateMatchings_iterate(struct state *state)
{
	int m;		// NonOverlapping Template Test - block length
	int n;		// Length of a single bit stream
	int		numOfTemplates[100] = {0, 0, 2, 4, 6, 12, 20, 40, 74, 148, 284, 568, 1116,
						2232, 4424, 8848, 17622, 35244, 70340, 140680, 281076, 562152};
	/*----------------------------------------------------------------------------
	NOTE:  Should additional templates lengths beyond 21 be desired, they must 
	first be constructed, saved into files and then the corresponding 
	number of nonperiodic templates for that file be stored in the m-th 
	position in the numOfTemplates variable.
	----------------------------------------------------------------------------*/
	unsigned int	bit, W_obs, nu[6], *Wj = NULL; 
	FILE			*fp = NULL;
	double			sum, chi2, p_value, lambda, pi[6], varWj;
	int				i, j, jj, k, match, SKIP, M, N, K = 5;
	char			templateFilename[200];
	BitSequence		*sequence = NULL;

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->testVector[test_num] == false) {
		dbg(DBG_LOW, "interate function[%d] %s called when testVector was false", test_num, __FUNCTION__);
		return;
	}
	if (state->testNames[test_num] == NULL) {
		dbg(DBG_LOW, "interate function[%d] %s called when testNames was NULL", test_num, __FUNCTION__);
		return;
	}
	if (state->dataDir == NULL) {
		dbg(DBG_LOW, "interate function[%d] %s called when dataDir was NULL", test_num, __FUNCTION__);
		return;
	}
	m = state->tp.nonOverlappingTemplateBlockLength;
	n = state->tp.n;

	N = 8;
	M = n/N;

	Wj = (unsigned int*)calloc(N, sizeof(unsigned int));
	if (Wj == NULL) {
		warnp(__FUNCTION__, "aborting %s, cannot calloc for Wj: %d * %d bytes",
				    state->testNames[test_num], N, sizeof(unsigned int));
		return;
	}
	lambda = (M-m+1)/pow(2, m);
	varWj = M*(1.0/pow(2.0, m) - (2.0*m-1.0)/pow(2.0, 2.0*m));
	sprintf(templateFilename, "%s/templates/template%d", state->dataDir, m);

	/*
	 * lambda must be > 0.0
	 */
	if (isNegative(lambda)) {
		warn(__FUNCTION__, "aborting %s, Lambda < 0.0: %f", state->testNames[test_num], lambda);
		free(Wj);
		return;
	}
	if (isZero(lambda)) {
		warn(__FUNCTION__, "aborting %s, Lambda == 0.0: %f", state->testNames[test_num], lambda);
		free(Wj);
		return;
	}

	/*
	 * open our template
	 */
	fp = fopen(templateFilename, "r");
	if (fp == NULL) {
		warnp(__FUNCTION__, "aborting %s, cannot open template file: %s", state->testNames[test_num], templateFilename);
		free(Wj);
		return;
	}

	/*
	 * calloc our special BitSequence for this test
	 */
	sequence = (BitSequence *) calloc(m, sizeof(BitSequence));
	if (sequence == NULL) {
		warnp(__FUNCTION__, "aborting %s, cannot calloc for sequence: %d * %d bytes",
				    state->testNames[test_num], m, sizeof(BitSequence));
		free(Wj);
		fclose(fp);
		return;
	}

	// rest of the test follows
	fprintf(stats[test_num], "\t\t  NONPERIODIC TEMPLATES TEST\n");
	fprintf(stats[test_num], "-------------------------------------------------------------------------------------\n");
	fprintf(stats[test_num], "\t\t  COMPUTATIONAL INFORMATION\n");
	fprintf(stats[test_num], "-------------------------------------------------------------------------------------\n");
	fprintf(stats[test_num], "\tLAMBDA = %f\tM = %d\tN = %d\tm = %d\tn = %d\n", lambda, M, N, m, n);
	fprintf(stats[test_num], "-------------------------------------------------------------------------------------\n");
	fprintf(stats[test_num], "\t\tF R E Q U E N C Y\n");
	fprintf(stats[test_num], "Template   W_1  W_2  W_3  W_4  W_5  W_6  W_7  W_8    Chi^2   P_value Assignment Index\n");
	fprintf(stats[test_num], "-------------------------------------------------------------------------------------\n");

	if ( numOfTemplates[m] < MAXNUMOFTEMPLATES )
		SKIP = 1;
	else
		SKIP = (int)(numOfTemplates[m]/MAXNUMOFTEMPLATES);
	numOfTemplates[m] = (int)numOfTemplates[m]/SKIP;
	
	sum = 0.0;
	for ( i=0; i<2; i++ ) {                      /* Compute Probabilities */
		pi[i] = exp(-lambda+i*log(lambda)-cephes_lgam(i+1));
		sum += pi[i];
	}
	pi[0] = sum;
	for ( i=2; i<=K; i++ ) {                      /* Compute Probabilities */
		pi[i-1] = exp(-lambda+i*log(lambda)-cephes_lgam(i+1));
		sum += pi[i-1];
	}
	pi[K] = 1 - sum;

	for( jj=0; jj<MIN(MAXNUMOFTEMPLATES, numOfTemplates[m]); jj++ ) {
		sum = 0;

		for ( k=0; k<m; k++ ) {
			fscanf(fp, "%u", &bit);
			sequence[k] = bit;
			fprintf(stats[test_num], "%d", sequence[k]);
		}
		fprintf(stats[test_num], " ");
		for ( k=0; k<=K; k++ )
			nu[k] = 0;
		for ( i=0; i<N; i++ ) {
			W_obs = 0;
			for ( j=0; j<M-m+1; j++ ) {
				match = 1;
				for ( k=0; k<m; k++ ) {
					if ( (int)sequence[k] != (int)epsilon[i*M+j+k] ) {
						match = 0;
						break;
					}
				}
				if ( match == 1 ) {
					W_obs++;
					j += m-1;
				}
			}
			Wj[i] = W_obs;
		}
		sum = 0;
		chi2 = 0.0;                                   /* Compute Chi Square */
		for ( i=0; i<N; i++ ) {
			if ( m == 10 )
				fprintf(stats[test_num], "%3d  ", Wj[i]);
			else
				fprintf(stats[test_num], "%4d ", Wj[i]);
			chi2 += pow(((double)Wj[i] - lambda)/pow(varWj, 0.5), 2);
		}
		p_value = cephes_igamc(N/2.0, chi2/2.0);
	
		if ( isNegative(p_value) || isGreaterThanOne(p_value) )
			fprintf(stats[test_num], "\t\tWARNING:  P_VALUE IS OUT OF RANGE.\n");

		fprintf(stats[test_num], "%9.6f %f %s %3d\n", chi2, p_value, p_value < ALPHA ? "FAILURE" : "SUCCESS", jj);
		if ( SKIP > 1 )
			fseek(fp, (long)(SKIP-1)*2*m, SEEK_CUR);
		fprintf(results[test_num], "%f\n", p_value); fflush(results[test_num]);
	}
	
	fprintf(stats[test_num], "\n"); fflush(stats[test_num]);
	if ( sequence != NULL )
		free(sequence);

	if ( Wj != NULL )
		free(Wj);
	if ( fp != NULL )
		fclose(fp);
}
