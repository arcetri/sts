/*****************************************************************************
                A P P R O X I M A T E   E N T R O P Y    T E S T
 *****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "externs.h"
#include "utilities.h"
#include "cephes.h"  


static const enum test test_num = TEST_APEN;	// this test number


/*
 * ApproximateEntropy_init - initalize the Aproximate Entrooy test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
ApproximateEntropy_init(struct state *state)
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
 * ApproximateEntropy_iterate - interate one bit stream for Aproximate Entrooy test
 *
 * given:
 * 	state		// run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
ApproximateEntropy_iterate(struct state *state)
{
	int m;		// Approximate Entropy Test - block length
	int n;		// Length of a single bit stream
	int				i, j, k, r, blockSize, seqLength, powLen, index;
	double			sum, numOfBlocks, ApEn[2], apen, chi_squared, p_value;
	unsigned int	*P;

	/*
	 * firewall
	 */
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
	m = state->tp.approximateEntropyBlockLength;
	n = state->tp.n;
	
	fprintf(stats[test_num], "\t\t\tAPPROXIMATE ENTROPY TEST\n");
	fprintf(stats[test_num], "\t\t--------------------------------------------\n");
	fprintf(stats[test_num], "\t\tCOMPUTATIONAL INFORMATION:\n");
	fprintf(stats[test_num], "\t\t--------------------------------------------\n");
	fprintf(stats[test_num], "\t\t(a) m (block length)    = %d\n", m);

	seqLength = n;
	r = 0;
	
	for ( blockSize=m; blockSize<=m+1; blockSize++ ) {
		if ( blockSize == 0 ) {
			ApEn[0] = 0.00;
			r++;
		}
		else {
			numOfBlocks = (double)seqLength;
			powLen = (int)pow(2, blockSize+1)-1;
			if ( (P = (unsigned int*)calloc(powLen,sizeof(unsigned int)))== NULL ) {
				fprintf(stats[test_num], "ApEn:  Insufficient memory available.\n");
				return;
			}
			for ( i=1; i<powLen-1; i++ )
				P[i] = 0;
			for ( i=0; i<numOfBlocks; i++ ) { /* COMPUTE FREQUENCY */
				k = 1;
				for ( j=0; j<blockSize; j++ ) {
					k <<= 1;
					if ( (int)epsilon[(i+j) % seqLength] == 1 )
						k++;
				}
				P[k-1]++;
			}
			/* DISPLAY FREQUENCY */
			sum = 0.0;
			index = (int)pow(2, blockSize)-1;
			for ( i=0; i<(int)pow(2, blockSize); i++ ) {
				if ( P[index] > 0 )
					sum += P[index]*log(P[index]/numOfBlocks);
				index++;
			}
			sum /= numOfBlocks;
			ApEn[r] = sum;
			r++;
			free(P);
		}
	}
	apen = ApEn[0] - ApEn[1];
	
	chi_squared = 2.0*seqLength*(log(2) - apen);
	p_value = cephes_igamc(pow(2, m-1), chi_squared/2.0);
	
	fprintf(stats[test_num], "\t\t(b) n (sequence length) = %d\n", seqLength);
	fprintf(stats[test_num], "\t\t(c) Chi^2               = %f\n", chi_squared);
	fprintf(stats[test_num], "\t\t(d) Phi(m)	       = %f\n", ApEn[0]);
	fprintf(stats[test_num], "\t\t(e) Phi(m+1)	       = %f\n", ApEn[1]);
	fprintf(stats[test_num], "\t\t(f) ApEn                = %f\n", apen);
	fprintf(stats[test_num], "\t\t(g) Log(2)              = %f\n", log(2.0));
	fprintf(stats[test_num], "\t\t--------------------------------------------\n");

	if ( m > (int)(log(seqLength)/log(2)-5) ) {
		fprintf(stats[test_num], "\t\tNote: The blockSize = %d exceeds recommended value of %d\n", m,
			MAX(1, (int)(log(seqLength)/log(2)-5)));
		fprintf(stats[test_num], "\t\tResults are inaccurate!\n");
		fprintf(stats[test_num], "\t\t--------------------------------------------\n");
	}
	
	fprintf(stats[test_num], "%s\t\tp_value = %f\n\n", p_value < ALPHA ? "FAILURE" : "SUCCESS", p_value); fflush(stats[test_num]);
	fprintf(results[test_num], "%f\n", p_value); fflush(results[test_num]);
}
