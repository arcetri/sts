/*****************************************************************************
                              S E R I A L   T E S T 
 *****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "externs.h"
#include "cephes.h"  
#include "utilities.h"


static const enum test test_num = TEST_SERIAL;	// this test number


/*
 * forward static function declarations
 */
static double psi2(int m, int n);


/*
 * Serial_init - initalize the Serial test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Serial_init(struct state *state)
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
 * Serial_iterate - interate one bit stream for Serial test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Serial_iterate(struct state *state)
{
	int m;		// Serial Test - block length
	int n;		// Length of a single bit stream
	double	p_value1, p_value2, psim0, psim1, psim2, del1, del2;

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "interate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	m = state->tp.serialBlockLength;
	n = state->tp.n;
	
	psim0 = psi2(m, n);
	psim1 = psi2(m-1, n);
	psim2 = psi2(m-2, n);
	del1 = psim0 - psim1;
	del2 = psim0 - 2.0*psim1 + psim2;
	p_value1 = cephes_igamc(pow(2, m-1)/2, del1/2.0);
	p_value2 = cephes_igamc(pow(2, m-2)/2, del2/2.0);
	
	fprintf(stats[test_num], "\t\t\t       SERIAL TEST\n");
	fprintf(stats[test_num], "\t\t---------------------------------------------\n");
	fprintf(stats[test_num], "\t\t COMPUTATIONAL INFORMATION:		  \n");
	fprintf(stats[test_num], "\t\t---------------------------------------------\n");
	fprintf(stats[test_num], "\t\t(a) Block length    (m) = %d\n", m);
	fprintf(stats[test_num], "\t\t(b) Sequence length (n) = %d\n", n);
	fprintf(stats[test_num], "\t\t(c) Psi_m               = %f\n", psim0);
	fprintf(stats[test_num], "\t\t(d) Psi_m-1             = %f\n", psim1);
	fprintf(stats[test_num], "\t\t(e) Psi_m-2             = %f\n", psim2);
	fprintf(stats[test_num], "\t\t(f) Del_1               = %f\n", del1);
	fprintf(stats[test_num], "\t\t(g) Del_2               = %f\n", del2);
	fprintf(stats[test_num], "\t\t---------------------------------------------\n");

	fprintf(stats[test_num], "%s\t\tp_value1 = %f\n", p_value1 < ALPHA ? "FAILURE" : "SUCCESS", p_value1);
	fprintf(results[test_num], "%f\n", p_value1);

	fprintf(stats[test_num], "%s\t\tp_value2 = %f\n\n", p_value2 < ALPHA ? "FAILURE" : "SUCCESS", p_value2); fflush(stats[test_num]);
	fprintf(results[test_num], "%f\n", p_value2); fflush(results[test_num]);
}


static double
psi2(int m, int n)
{
	int				i, j, k, powLen;
	double			sum, numOfBlocks;
	unsigned int	*P;
	
	if ( (m == 0) || (m == -1) )
		return 0.0;
	numOfBlocks = n;
	powLen = (int)pow(2, m+1)-1;
	if ( (P = (unsigned int*)calloc(powLen,sizeof(unsigned int)))== NULL ) {
		fprintf(stats[test_num], "Serial Test:  Insufficient memory available.\n");
		fflush(stats[test_num]);
		return 0.0;
	}
	for ( i=1; i<powLen-1; i++ )
		P[i] = 0;	  /* INITIALIZE NODES */
	for ( i=0; i<numOfBlocks; i++ ) {		 /* COMPUTE FREQUENCY */
		k = 1;
		for ( j=0; j<m; j++ ) {
			if ( epsilon[(i+j)%n] == 0 )
				k *= 2;
			else if ( epsilon[(i+j)%n] == 1 )
				k = 2*k+1;
		}
		P[k-1]++;
	}
	sum = 0.0;
	for ( i=(int)pow(2, m)-1; i<(int)pow(2, m+1)-1; i++ )
		sum += pow(P[i], 2);
	sum = (sum * pow(2, m)/(double)n) - (double)n;
	free(P);
	
	return sum;
}
