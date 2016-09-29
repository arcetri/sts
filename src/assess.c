/* --------------------------------------------------------------------------
   Title       :  The NIST Statistical Test Suite

   Date        :  December 1999

   Programmer  :  Juan Soto

   Summary     :  For use in the evaluation of the randomness of bitstreams
                  produced by cryptographic random number generators.

   Package     :  Version 1.0

   Copyright   :  (c) 1999 by the National Institute Of Standards & Technology

   History     :  Version 1.0 by J. Soto, October 1999
                  Revised by J. Soto, November 1999
                  Revised by Larry Bassham, March 2008

   Keywords    :  Pseudorandom Number Generator (PRNG), Randomness, Statistical 
                  Tests, Complementary Error functions, Incomplete Gamma 
                  Function, Random Walks, Rank, Fast Fourier Transform, 
                  Template, Cryptographically Secure PRNG (CSPRNG),
                  Approximate Entropy (ApEn), Secure Hash Algorithm (SHA-1), 
                  Blum-Blum-Shub (BBS) CSPRNG, Micali-Schnorr (MS) CSPRNG, 

   Source      :  David Banks, Elaine Barker, James Dray, Allen Heckert, 
                  Stefan Leigh, Mark Levenson, James Nechvatal, Andrew Rukhin, 
                  Miles Smid, Juan Soto, Mark Vangel, and San Vo.

   Technical
   Assistance  :  Larry Bassham, Ron Boisvert, James Filliben, Daniel Lozier,
                  and Bert Rust.

   Warning     :  Portability Issues.

   Limitation  :  Amount of memory allocated for workspace.

   Restrictions:  Permission to use, copy, and modify this software without 
                  fee is hereby granted, provided that this entire notice is 
                  included in all copies of any software which is or includes
                  a copy or modification of this software and in all copies 
                  of the supporting documentation for such software.
   -------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "defs.h"
#include "cephes.h"  
#include "utilities.h"
#include "stat_fncs.h"

// sts_version-edit_number edit_year-edit_Month-edit_day
const char * const version = "sts version 2.1.2 Cisco edit 1 2015-Feb-08";

// our name
char * program = "assess";

// do not debug by default
long int debuglevel = DBG_NONE;

//pasted from decls.h
/*****************************************************************************
                   G L O B A L   D A T A  S T R U C T U R E S
 *****************************************************************************/

BitSequence	*epsilon;		// BIT STREAM
FILE		*stats[NUMOFTESTS+1];	// FILE OUTPUT STREAM
FILE		*results[NUMOFTESTS+1];	// FILE OUTPUT STREAM
FILE		*freqfp;		// FILE OUTPUT STREAM
FILE		*summary;		// FILE OUTPUT STREAM


/*
 * test - setup a given test, interate on bitsteams, analyze test results
 */

const struct driver test[NUMOFTESTS+1] = {

	{ // TEST_ALL = 0, converence for indicating run all tests
		NULL,
	},

	{ // TEST_FREQUENCY = 1, Frequency test (frequency.c)
		Frequency_init,
		Frequency_iterate,
	},

	{ // TEST_BLOCK_FREQUENCY = 2, Block Frequency test (blockFrequency.c)
		BlockFrequency_init,
		BlockFrequency_iterate,
	},

	{ // TEST_CUSUM = 3, Cumluative Sums test (cusum.c)
		CumulativeSums_init,
		CumulativeSums_iterate,
	},

	{ // TEST_RUNS = 4, Runs test (runs.c)
		Runs_init,
		Runs_iterate,
	},

	{ // TEST_LONGEST_RUN = 5, Longest Runs test (longestRunOfOnes.c)
		LongestRunOfOnes_init,
		LongestRunOfOnes_iterate,
	},

	{ // TEST_RANK = 6, Rank test (rank.c)
		Rank_init,
		Rank_iterate,
	},

	{ // TEST_FFT = 7, Discrete Fourier Transform test (discreteFourierTransform.c)
		DiscreteFourierTransform_init,
		DiscreteFourierTransform_iterate,
	},

	{ // TEST_NONPERIODIC = 8, Nonoverlapping Template test (nonOverlappingTemplateMatchings.c)
		NonOverlappingTemplateMatchings_init,
		NonOverlappingTemplateMatchings_iterate,
	},

	{ // TEST_OVERLAPPING = 9, Overlapping Template test (overlappingTemplateMatchings.c)
		OverlappingTemplateMatchings_init,
		OverlappingTemplateMatchings_iterate,
	},

	{ // TEST_UNIVERSAL = 10, Universal test (universal.c)
		Universal_init,
		Universal_iterate,
	},

	{ // TEST_APEN = 11, Aproximate Entrooy test (approximateEntropy.c)
		ApproximateEntropy_init,
		ApproximateEntropy_iterate,
	},

	{ // TEST_RND_EXCURSION = 12, Random Excursions test (randomExcursions.c)
		RandomExcursions_init,
		RandomExcursions_iterate,
	},

	{ // TEST_RND_EXCURSION_VAR = 13, Random Excursions Variant test (randomExcursionsVariant.c)
		RandomExcursionsVariant_init,
		RandomExcursionsVariant_iterate,
	},

	{ // TEST_SERIAL = 14, Serial test (serial.c)
		Serial_init,
		Serial_iterate,
	},

	{ // TEST_LINEARCOMPLEXITY = 15, Linear Complexity test (linearComplexity.c)
		LinearComplexity_init,
		LinearComplexity_iterate,
	},
};

static void	partitionResultFile(struct state *state, int numOfFiles, int numOfSequences, int generator, int testNameID);
static void	postProcessResults(struct state *state, int generator);
static int	cmp(const void *a, const void *b);
static int	computeMetrics(struct state *state, char *s, int test);

int
main(int argc, char *argv[])
{
	struct state run_state;		/* options set and dynamic arrays for this run */
	int	i;

	/*
	 * set default test parameters and parse command line
	 */
	Parse_args(&run_state, argc, argv);

	/*
	 * prep the top of the working directory
	 */
	precheckPath(&run_state, run_state.workDir);

	/*
	 * in classic mode (no -b), ask for test parameters
	 * 	if reading from file (-g 0), we open that file
	 * in batch mode (-b) we just
	 * 	if reading from file (-g 0), we open that file
	 */
	generatorOptions(&run_state);

	/*
	 * in classic mode (no -b), ask for test parameters
	 */
	if (run_state.batchmode == false) {
		chooseTests(&run_state);
		if (run_state.promptFlag == false) {
			fixParameters(&run_state);
		}
	}

	/*
	 * initialize all active tests
	 */
	for( i=1; i<=NUMOFTESTS; i++ ) {
		if (run_state.testVector[i] == true) {
			test[i].init(&run_state);
		}
	}

	/*
	 * open up all those output file descriptors
	 */
	openOutputStreams(&run_state);

	/*
	 * run test suite interations
	 */
	invokeTestSuite(&run_state);

	/*
	 * close down all of those output files
	 *
	 * XXX - remove when tests use dynatic arrays
	 */
	if (freqfp != NULL) {
		fclose(freqfp);
	}
	for( i=1; i<=NUMOFTESTS; i++ ) {
		if ( stats[i] != NULL ) {
			fclose(stats[i]);
		}
		if ( results[i] != NULL ) {
			fclose(results[i]);
		}
	}

	/*
	 * partition results - XXX - use numOfFiles in state
	 */
	if ( (run_state.testVector[0] == true) || (run_state.testVector[TEST_CUSUM] == true) ) 
		partitionResultFile(&run_state, 2, run_state.tp.numOfBitStreams, run_state.generator, TEST_CUSUM);
	if ( (run_state.testVector[0] == true) || (run_state.testVector[TEST_NONPERIODIC] == true) ) 
		partitionResultFile(&run_state, MAXNUMOFTEMPLATES, run_state.tp.numOfBitStreams, run_state.generator, TEST_NONPERIODIC);
	if ( (run_state.testVector[0] == true) || (run_state.testVector[TEST_RND_EXCURSION] == true) )
		partitionResultFile(&run_state, 8, run_state.tp.numOfBitStreams, run_state.generator, TEST_RND_EXCURSION);
	if ( (run_state.testVector[0] == true) || (run_state.testVector[TEST_RND_EXCURSION_VAR] == true) )
		partitionResultFile(&run_state, 18, run_state.tp.numOfBitStreams, run_state.generator, TEST_RND_EXCURSION_VAR);
	if ( (run_state.testVector[0] == true) || (run_state.testVector[TEST_SERIAL] == true) )
		partitionResultFile(&run_state, 2, run_state.tp.numOfBitStreams, run_state.generator, TEST_SERIAL);

	/*
	 * final result processing
	 */
 	fprintf(summary, "------------------------------------------------------------------------------\n");
	fprintf(summary, "RESULTS FOR THE UNIFORMITY OF P-VALUES AND THE PROPORTION OF PASSING SEQUENCES\n");
	fprintf(summary, "------------------------------------------------------------------------------\n");
	fprintf(summary, "   generator is: %s\n", run_state.generatorDir[run_state.generator]);
	fprintf(summary, "------------------------------------------------------------------------------\n");
	fprintf(summary, " C1  C2  C3  C4  C5  C6  C7  C8  C9 C10  P-VALUE  PROPORTION  STATISTICAL TEST\n");
	fprintf(summary, "------------------------------------------------------------------------------\n");
	postProcessResults(&run_state, run_state.generator);
	fclose(summary);

	// All Done!!! -- Jessica Noll, Age 2
	exit(0);
}


static void
partitionResultFile(struct state *state, int numOfFiles, int numOfSequences, int generator, int testNameID)
{ 
	int		i, k, m, j, start, end, num, numread;
	float	c;
	FILE	**fp = (FILE **)calloc(numOfFiles+1, sizeof(FILE *));
	char	*s[MAXFILESPERMITTEDFORPARTITION];
	char	resultsDir[200];

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->subDir[testNameID] == NULL) {
		err(10, __FUNCTION__, "state->subDir[%d] is NULL", testNameID);
	}
	
	for ( i=0; i<MAXFILESPERMITTEDFORPARTITION; i++ )
		s[i] = (char*)calloc(200, sizeof(char));
	
	sprintf(resultsDir, "%s/results.txt", state->subDir[testNameID]);
	dbg(DBG_HIGH, "in %s, generator %s[%d] test %s[%d] uses results.txt file: %s", 
		      __FUNCTION__, state->generatorDir[generator], generator, 
		      state->testNames[testNameID], testNameID, resultsDir);
	
	if ( (fp[numOfFiles] = fopen(resultsDir, "r")) == NULL ) {
		errp(1, __FUNCTION__, "cannot open for reading: %s", resultsDir);
	}
	
	for ( i=0; i<numOfFiles; i++ ) {
		if ( i < 10 )
			sprintf(s[i], "%s/data%1d.txt", state->subDir[testNameID], i+1);
		else if (i < 100)
			sprintf(s[i], "%s/data%2d.txt", state->subDir[testNameID], i+1);
		else
			sprintf(s[i], "%s/data%3d.txt", state->subDir[testNameID], i+1);
	}
	numread = 0;
	m = numOfFiles/20;
	if ( (numOfFiles%20) != 0 )
		m++;
	for ( i=0; i<numOfFiles; i++ ) {
		if ( (fp[i] = fopen(s[i], "w")) == NULL ) {
			errp(1, __FUNCTION__, "cannot open for writing: %s", s[i]);
		}
		fclose(fp[i]);
	}
	for ( num=0; num<numOfSequences; num++ ) {
		for ( k=0; k<m; k++ ) { 			/* FOR EACH BATCH */
			
			start = k*20;		/* BOUNDARY SEGMENTS */
			end   = k*20+19;
			if ( k == (m-1) )
				end = numOfFiles-1;
			
			for ( i=start; i<=end; i++ ) {		/* OPEN FILE */
				if ( (fp[i] = fopen(s[i], "a")) == NULL ) {
					errp(1, __FUNCTION__, "cannot open for appending: %s", s[i]);
				}
			}
			
			for ( j=start; j<=end; j++ ) {		/* POPULATE FILE */
				fscanf(fp[numOfFiles], "%f", &c);
				fprintf(fp[j], "%f\n", c);
				numread++;
			}

			for ( i=start; i<=end; i++ )		/* CLOSE FILE */
				fclose(fp[i]);
		}
	}
	fclose(fp[numOfFiles]);
	for ( i=0; i<MAXFILESPERMITTEDFORPARTITION; i++ )
		free(s[i]);

	return;
}

static int
cmp(const void *a, const void *b)
{
	if ( (double *)a < (double *)b )
		return -1;
	if ( (double *)a == (double *)b )
		return 0;
	return 1;
}

static void
postProcessResults(struct state *state, int generator)
{
	int		i, k, randomExcursionSampleSize, generalSampleSize;
	int		passRate, case1, case2, numOfFiles = 2;
	double	p_hat;
	char	s[200];

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	
	for ( i=1; i<=NUMOFTESTS; i++ ) {		// FOR EACH TEST
		if ( state->testVector[i] ) {
			// SPECIAL CASES -- HANDLING MULTIPLE FILES FOR A SINGLE TEST
			if ( ((i == TEST_CUSUM) && state->testVector[TEST_CUSUM] ) ||
				 ((i == TEST_NONPERIODIC) && state->testVector[TEST_NONPERIODIC] ) ||
				 ((i == TEST_RND_EXCURSION) && state->testVector[TEST_RND_EXCURSION]) ||
				 ((i == TEST_RND_EXCURSION_VAR) && state->testVector[TEST_RND_EXCURSION_VAR]) || 
				 ((i == TEST_SERIAL) && state->testVector[TEST_SERIAL]) ) {
				
				if ( (i == TEST_NONPERIODIC) && state->testVector[TEST_NONPERIODIC] )  
					numOfFiles = MAXNUMOFTEMPLATES;
				else if ( (i == TEST_RND_EXCURSION) && state->testVector[TEST_RND_EXCURSION] ) 
					numOfFiles = 8;
				else if ( (i == TEST_RND_EXCURSION_VAR) && state->testVector[TEST_RND_EXCURSION_VAR] ) 
					numOfFiles = 18;
				else
					numOfFiles = 2;
				for ( k=0; k<numOfFiles; k++ ) {
					if ( k < 10 )
						sprintf(s, "%s/data%1d.txt", state->subDir[i], k+1);
					else if ( k < 100 )
						sprintf(s, "%s/data%2d.txt", state->subDir[i], k+1);
					else
						sprintf(s, "%s/data%3d.txt", state->subDir[i], k+1);
					if ( (i == TEST_RND_EXCURSION) || (i == TEST_RND_EXCURSION_VAR) ) 
						randomExcursionSampleSize = computeMetrics(state,s,i);
					else
						generalSampleSize = computeMetrics(state,s,i);
				}
			}
			else {
				sprintf(s, "%s/results.txt", state->subDir[i]);
				dbg(DBG_HIGH, "in %s, generator %s[%d] test %s[%d] uses results.txt file: %s", 
					      __FUNCTION__, state->generatorDir[generator], generator, 
					      state->testNames[i], i, s);
				generalSampleSize = computeMetrics(state,s,i);
			}
		}
	}

	fprintf(summary, "\n\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
	case1 = 0;
	case2 = 0;
	if ( state->testVector[TEST_RND_EXCURSION] || state->testVector[TEST_RND_EXCURSION_VAR] ) 
		case2 = 1;
	for ( i=1; i<=NUMOFTESTS; i++ ) {
		if ( state->testVector[i] && (i != TEST_RND_EXCURSION) && (i != TEST_RND_EXCURSION_VAR) ) {
			case1 = 1;
			break;
		}
	}
	p_hat = 1.0 - ALPHA;
	if ( case1 ) {
		if ( generalSampleSize == 0 ) {
			fprintf(summary, "The minimum pass rate for each statistical test with the exception of the\n");
			fprintf(summary, "random excursion (variant) test is undefined.\n\n");
		}
		else {
			passRate = (p_hat - 3.0 * sqrt((p_hat*ALPHA)/generalSampleSize)) * generalSampleSize;
			fprintf(summary, "The minimum pass rate for each statistical test with the exception of the\n");
			fprintf(summary, "random excursion (variant) test is approximately = %d for a\n", generalSampleSize ? passRate : 0);
			fprintf(summary, "sample size = %d binary sequences.\n\n", generalSampleSize);
		}
	}
	if ( case2 ) {
		if ( randomExcursionSampleSize == 0 )
			fprintf(summary, "The minimum pass rate for the random excursion (variant) test is undefined.\n\n");
		else {
			passRate = (p_hat - 3.0 * sqrt((p_hat*ALPHA)/randomExcursionSampleSize)) * randomExcursionSampleSize;
			fprintf(summary, "The minimum pass rate for the random excursion (variant) test\n");
			fprintf(summary, "is approximately = %d for a sample size = %d binary sequences.\n\n", passRate, randomExcursionSampleSize);
		}
	}
	fprintf(summary, "For further guidelines construct a probability table using the MAPLE program\n");
	fprintf(summary, "provided in the addendum section of the documentation.\n");
	fprintf(summary, "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
}

static int
computeMetrics(struct state *state, char *s, int test)
{
	int		j, pos, count, passCount, sampleSize, expCount, proportion_threshold_min, proportion_threshold_max;
	int		freqPerBin[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	double	*A, *T, chi2, uniformity, p_hat;
	float	c;
	FILE	*fp;

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	
	if ( (fp = fopen(s, "r")) == NULL ) {
		printf("%s",s);
		printf(" -- file not found. Exiting program.\n");
		exit(-1);
	}
	
	if ( (A = (double *)calloc(state->tp.numOfBitStreams, sizeof(double))) == NULL ) {
		printf("Final Analysis Report aborted due to insufficient workspace\n");
		return 0;
	}
	
	/* Compute Metric 1: Proportion of Passing Sequences */
	
	count = 0; 		
	sampleSize = state->tp.numOfBitStreams;
	
	if ( (test == TEST_RND_EXCURSION) || (test == TEST_RND_EXCURSION_VAR) ) { /* Special Case: Random Excursion Tests */
		if ( (T = (double *)calloc(state->tp.numOfBitStreams, sizeof(double))) == NULL ) {
			printf("Final Analysis Report aborted due to insufficient workspace\n");
			return 0;
		}
		for ( j=0; j<sampleSize; j++ ) {
			fscanf(fp, "%f", &c);
			if ( c > 0.000000 )
				T[count++] = c;
		}
		
		if ( (A = (double *)calloc(count, sizeof(double))) == NULL ) {
			printf("Final Analysis Report aborted due to insufficient workspace\n");
			return 0;
		}
		
		for ( j=0; j<count; j++ )
			A[j] = T[j];
		
		sampleSize = count;
		count = 0;
		for ( j=0; j<sampleSize; j++ )
			if ( A[j] < ALPHA )
				count++;
		free(T);
	}
	else {
		if ( (A = (double *)calloc(sampleSize, sizeof(double))) == NULL ) {
			printf("Final Analysis Report aborted due to insufficient workspace\n");
			return 0;
		}
		for ( j=0; j<sampleSize; j++ ) {
			fscanf(fp, "%f", &c);
			if ( c < ALPHA )
				count++;
			A[j] = c;
		}
	}
	if ( sampleSize == 0 )
		passCount = 0;
	else
		passCount = sampleSize - count;
	
	p_hat = 1.0 - ALPHA;
	proportion_threshold_max = (p_hat + 3.0 * sqrt((p_hat*ALPHA)/sampleSize)) * sampleSize;
	proportion_threshold_min = (p_hat - 3.0 * sqrt((p_hat*ALPHA)/sampleSize)) * sampleSize;
	
	/* Compute Metric 2: Histogram */
	
	qsort((void *)A, sampleSize, sizeof(double), cmp);
	for ( j=0; j<sampleSize; j++ ) {
		pos = (int)floor(A[j]*10);
		if ( pos == 10 )
			pos--;
		freqPerBin[pos]++;
	}
	chi2 = 0.0;
	expCount = sampleSize/10;
	if ( expCount == 0 )
		uniformity = 0.0;
	else {
		for ( j=0; j<10; j++ )
			chi2 += pow(freqPerBin[j]-expCount, 2)/expCount;
		uniformity = cephes_igamc(9.0/2.0, chi2/2.0);
	}
	
	for ( j=0; j<10; j++ )			/* DISPLAY RESULTS */
		fprintf(summary, "%3d ", freqPerBin[j]);
	
	if ( expCount == 0 )
		fprintf(summary, "    ----    ");
	else if ( uniformity < 0.0001 )
		fprintf(summary, " %8.6f * ", uniformity);
	else
		fprintf(summary, " %8.6f   ", uniformity);
	
	if ( sampleSize == 0 )
		fprintf(summary, " ------     %s\n", state->testNames[test]);
	//	else if ( proportion < 0.96 )
	else if ( (passCount < proportion_threshold_min) || (passCount > proportion_threshold_max))
		fprintf(summary, "%4d/%-4d *  %s\n", passCount, sampleSize, state->testNames[test]);
	else
		fprintf(summary, "%4d/%-4d    %s\n", passCount, sampleSize, state->testNames[test]);
	
	fclose(fp);
	free(A);
	
	return sampleSize;
}
