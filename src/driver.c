/*
 * testDriver
 *
 * For each test, we declare the functions that are needed by the
 * driver interface to perform the complete test.
 *
 * For historical reasons and the way the classical interactive code
 * prompted the user, test 0 is not a real test but rather an alias
 * for running all tests.  Therefore testDriver[0] is a driver that
 * is filled with NULL pointers.
 */

/*
 * This code has been heavily modified by the following people:
 *
 *      Landon Curt Noll
 *      Tom Gilgan
 *      Riccardo Paccagnella
 *
 * See the README.txt and the initial comment in assess.c for more information.
 *
 * WE (THOSE LISTED ABOVE WHO HEAVILY MODIFIED THIS CODE) DISCLAIM ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL WE (THOSE LISTED ABOVE
 * WHO HEAVILY MODIFIED THIS CODE) BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * chongo (Landon Curt Noll, http://www.isthe.com/chongo/index.html) /\oo/\
 *
 * Share and enjoy! :-)
 */


// Exit codes: 50 thru 59

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "defs.h"
#include "utilities.h"
#include "debug.h"
#include "stat_fncs.h"
#include "externs.h"


/*
 * Driver interface - defines how each test is performed at each phase
 */
struct driver {
	void (*init) (struct state * state);		// Initialize the test and check input size recommendations
	void (*iterate) (struct state * state);		// Perform a single iteration test on the bitstream
	void (*print) (struct state * state);		// Log iteration info into stats.txt, data*.txt, results.txt unless -n
	void (*metrics) (struct state * state);		// Uniformity and proportional analysis of a test
	void (*destroy) (struct state * state);		// Final test cleanup and memory de-allocation
};
static const struct driver testDriver[NUMOFTESTS + 1] = {

	{			// TEST_ALL = 0, convention for indicating run all tests
	 NULL,
	 NULL,
	 NULL,
	 NULL,
	 NULL,
	 },

	{			// TEST_FREQUENCY = 1, Frequency test (frequency.c)
	 Frequency_init,
	 Frequency_iterate,
	 Frequency_print,
	 Frequency_metrics,
	 Frequency_destroy,
	 },

	{			// TEST_BLOCK_FREQUENCY = 2, Block Frequency test (blockFrequency.c)
	 BlockFrequency_init,
	 BlockFrequency_iterate,
	 BlockFrequency_print,
	 BlockFrequency_metrics,
	 BlockFrequency_destroy,
	 },

	{			// TEST_CUSUM = 3, Cumulative Sums test (cusum.c)
	 CumulativeSums_init,
	 CumulativeSums_iterate,
	 CumulativeSums_print,
	 CumulativeSums_metrics,
	 CumulativeSums_destroy,
	 },

	{			// TEST_RUNS = 4, Runs test (runs.c)
	 Runs_init,
	 Runs_iterate,
	 Runs_print,
	 Runs_metrics,
	 Runs_destroy,
	 },

	{			// TEST_LONGEST_RUN = 5, Longest Runs test (longestRunOfOnes.c)
	 LongestRunOfOnes_init,
	 LongestRunOfOnes_iterate,
	 LongestRunOfOnes_print,
	 LongestRunOfOnes_metrics,
	 LongestRunOfOnes_destroy,
	 },

	{			// TEST_RANK = 6, Rank test (rank.c)
	 Rank_init,
	 Rank_iterate,
	 Rank_print,
	 Rank_metrics,
	 Rank_destroy,
	 },

	{			// TEST_FFT = 7, Discrete Fourier Transform test (discreteFourierTransform.c)
	 DiscreteFourierTransform_init,
	 DiscreteFourierTransform_iterate,
	 DiscreteFourierTransform_print,
	 DiscreteFourierTransform_metrics,
	 DiscreteFourierTransform_destroy,
	 },

	{			// TEST_NON_OVERLAPPING = 8, Non-overlapping Template test (nonOverlappingTemplateMatchings.c)
	 NonOverlappingTemplateMatchings_init,
	 NonOverlappingTemplateMatchings_iterate,
	 NonOverlappingTemplateMatchings_print,
	 NonOverlappingTemplateMatchings_metrics,
	 NonOverlappingTemplateMatchings_destroy,
	 },

	{			// TEST_OVERLAPPING = 9, Overlapping Template test (overlappingTemplateMatchings.c)
	 OverlappingTemplateMatchings_init,
	 OverlappingTemplateMatchings_iterate,
	 OverlappingTemplateMatchings_print,
	 OverlappingTemplateMatchings_metrics,
	 OverlappingTemplateMatchings_destroy,
	 },

	{			// TEST_UNIVERSAL = 10, Universal test (universal.c)
	 Universal_init,
	 Universal_iterate,
	 Universal_print,
	 Universal_metrics,
	 Universal_destroy,
	 },

	{			// TEST_APEN = 11, Approximate Entropy test (approximateEntropy.c)
	 ApproximateEntropy_init,
	 ApproximateEntropy_iterate,
	 ApproximateEntropy_print,
	 ApproximateEntropy_metrics,
	 ApproximateEntropy_destroy,
	 },

	{			// TEST_RND_EXCURSION = 12, Random Excursions test (randomExcursions.c)
	 RandomExcursions_init,
	 RandomExcursions_iterate,
	 RandomExcursions_print,
	 RandomExcursions_metrics,
	 RandomExcursions_destroy,
	 },

	{			// TEST_RND_EXCURSION_VAR = 13, Random Excursions Variant test (randomExcursionsVariant.c)
	 RandomExcursionsVariant_init,
	 RandomExcursionsVariant_iterate,
	 RandomExcursionsVariant_print,
	 RandomExcursionsVariant_metrics,
	 RandomExcursionsVariant_destroy,
	 },

	{			// TEST_SERIAL = 14, Serial test (serial.c)
	 Serial_init,
	 Serial_iterate,
	 Serial_print,
	 Serial_metrics,
	 Serial_destroy,
	 },

	{			// TEST_LINEARCOMPLEXITY = 15, Linear Complexity test (linearComplexity.c)
	 LinearComplexity_init,
	 LinearComplexity_iterate,
	 LinearComplexity_print,
	 LinearComplexity_metrics,
	 LinearComplexity_destroy,
	 },
};


/*
 * Init - initialize the variables needed for each test and check if the input size recommendations are respected
 *
 * given:
 *      state           // current processing state
 */
void
init(struct state *state)
{
	int r;			// Row count to consider
	double product;		// Probability product
	int test_count;		// Number of tests enabled after initialization
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(50, __FUNCTION__, "state arg is NULL");
	}
	if (state->workDir == NULL) {
		err(50, __FUNCTION__, "state->workDir is NULL");
	}
	dbg(DBG_LOW, "start of init phase");

	/*
	 * Prepare the top of the working directory
	 */
	precheckPath(state, state->workDir);

	/*
	 * Get number generators to be tested
	 */
	generatorOptions(state);

	/*
	 * Get which tests to perform
	 */
	if (state->batchmode == false) {
		chooseTests(state);
		if (state->promptFlag == false) {

			/*
			 * Check if the user wants to adjust the parameters
			 */
			fixParameters(state);
		}
	}

	/*
	 * Create the path if required
	 */
	if (state->workDirFlag == true) {
		makePath(state->workDir);
	}

	/*
	 * Open the freq.txt file
	 */
	state->freqFilePath = filePathName(state->workDir, "freq.txt");
	dbg(DBG_MED, "will use freq.txt file: %s", state->freqFilePath);
	state->freqFile = fopen(state->freqFilePath, "w");
	if (state->freqFile == NULL) {
		errp(50, __FUNCTION__, "Could not open freq.txt file: %s", state->freqFilePath);
	}

	/*
	 * Open the finalAnalysisReport.txt file
	 */
	if (state->runMode != MODE_WRITE_ONLY) {
		state->finalReptPath = filePathName(state->workDir, "finalAnalysisReport.txt");
		dbg(DBG_MED, "will use finalAnalysisReport.txt file: %s", state->finalReptPath);
		state->finalRept = fopen(state->finalReptPath, "w");
		if (state->finalRept == NULL) {
			errp(50, __FUNCTION__, "Could not open finalAnalysisReport file: %s", state->finalReptPath);
		}
	}

	/*
	 * Initialize test constants // TODO consider moving
	 *
	 * NOTE: This must be done after the command line arguments are parsed,
	 *       AND after any test parameters are established (by default or via interactive prompt),
	 *       AND before the individual test init functions are called
	 *       (i.e., before the initialize all active tests section below).
	 */
	state->cSetup = false;	// note that the test constants are not yet initialized
	// firewall - guard against taking the square root of a negative value
	if (state->tp.n <= 0) {
		err(50, __FUNCTION__, "bogus n value: %ld should be > 0", state->tp.n);
	}
	// compute pure numerical constants
	state->c.sqrt2 = sqrt(2.0);
	state->c.log2 = log(2.0);
	// compute constants related to the value of n
	state->c.sqrtn = sqrt((double) state->tp.n);
	state->c.sqrtn4_095_005 = sqrt((double) state->tp.n / 4.0 * 0.95 * 0.05);
	state->c.sqrt_log20_n = sqrt(log(20.0) * (double) state->tp.n);	// 2.995732274 * n
	state->c.sqrt2n = sqrt(2.0 * (double) state->tp.n);
	if (state->c.sqrtn == 0.0) {	// paranoia
		state->c.two_over_sqrtn = 0.0;
	} else {
		state->c.two_over_sqrtn = 2.0 / state->c.sqrtn;
	}
	// Probability of rank NUMBER_OF_ROWS_RANK
	r = NUMBER_OF_ROWS_RANK;
	product = 1.0;
	for (i = 0; i <= r - 1; i++) {
		product *= ((1.0 - pow(2.0, i - NUMBER_OF_ROWS_RANK))
			    * (1.0 - pow(2.0, i - NUMBER_OF_COLS_RANK))) / (1.0 - pow(2.0, i - r));
	}
	state->c.p_32 = pow(2.0, r * (NUMBER_OF_ROWS_RANK + NUMBER_OF_COLS_RANK - r)
				 - NUMBER_OF_ROWS_RANK * NUMBER_OF_COLS_RANK) * product;
	if (state->c.p_32 <= 0.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_32 value: %f should be > 0.0", state->c.p_32);
	}
	if (state->c.p_32 >= 1.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_32 value: %f should be < 1.0", state->c.p_32);
	}
	// Probability of rank NUMBER_OF_ROWS_RANK-1
	r = NUMBER_OF_ROWS_RANK - 1;
	product = 1.0;
	for (i = 0; i <= r - 1; i++) {
		product *= ((1.0 - pow(2.0, i - NUMBER_OF_ROWS_RANK))
			    * (1.0 - pow(2.0, i - NUMBER_OF_COLS_RANK))) / (1.0 - pow(2.0, i - r));
	}
	state->c.p_31 = pow(2.0, r * (NUMBER_OF_ROWS_RANK + NUMBER_OF_COLS_RANK - r)
				 - NUMBER_OF_ROWS_RANK * NUMBER_OF_COLS_RANK) * product;
	if (state->c.p_31 <= 0.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_31 value: %f should be > 0.0", state->c.p_31);
	}
	if (state->c.p_31 >= 1.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_31 value: %f should be < 1.0", state->c.p_31);
	}
	// Probability of rank < NUMBER_OF_ROWS_RANK-1
	state->c.p_30 = 1.0 - (state->c.p_32 + state->c.p_31);
	if (state->c.p_30 <= 0.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_30 value: %f == (1.0 - p32: %f - p_31: %f) should be > 0.0",
		    state->c.p_30, state->c.p_31, state->c.p_32);
	}
	if (state->c.p_30 >= 1.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_30 value: %f == (1.0 - p32: %f - p_31: %f) should be < 1.0",
		    state->c.p_30, state->c.p_31, state->c.p_32);
	}
	// Total possible matrix for a given bit stream length - used by RANK_TEST
	if (NUMBER_OF_ROWS_RANK <= 0) {	// paranoia
		err(50, __FUNCTION__, "NUMBER_OF_ROWS_RANK: %d must be > 0", NUMBER_OF_ROWS_RANK);
	}
	if (NUMBER_OF_COLS_RANK <= 0) {	// paranoia
		err(50, __FUNCTION__, "NUMBER_OF_COLS_RANK: %d must be > 0", NUMBER_OF_COLS_RANK);
	}
	if (((long int) NUMBER_OF_ROWS_RANK * (long int) NUMBER_OF_COLS_RANK) > (long int) LONG_MAX) {	// paranoia
		err(50, __FUNCTION__, "NUMBER_OF_ROWS_RANK: %d * NUMBER_OF_COLS_RANK: %d cannot fit into an int because"
				    "the product is > %ld", NUMBER_OF_ROWS_RANK, NUMBER_OF_COLS_RANK, LONG_MAX);
	}
	if ((NUMBER_OF_ROWS_RANK * NUMBER_OF_COLS_RANK) == 0) {	// paranoia
		err(50, __FUNCTION__, "NUMBER_OF_ROWS_RANK: %d * NUMBER_OF_COLS_RANK: %d == 0, perhaps due to overflow",
		    NUMBER_OF_ROWS_RANK, NUMBER_OF_COLS_RANK);
	}
	state->c.logn = log(state->tp.n);
	state->c.matrix_count = state->tp.n / (NUMBER_OF_ROWS_RANK * NUMBER_OF_COLS_RANK);

	/*
	 * Compute the minimum number of zero crossings required by the random excursions test
	 */
	state->c.min_zero_crossings = (long int) MAX(0.005 * state->c.sqrtn, 500.0);

	/*
	 * Indicate that the test constants have been initialized
	 */
	state->cSetup = true;

	/*
	 * Initialize all active tests
	 */
	for (i = 1; i <= NUMOFTESTS; i++) {
		if (state->testVector[i] == true && testDriver[i].init != NULL) {

			// Initialize a test
			testDriver[i].init(state);
		}
	}

	/*
	 * Some tests may have disabled themselves, be sure we have at least one enabled test
	 */
	test_count = 0;
	for (i = 1; i <= NUMOFTESTS; i++) {
		if (state->testVector[i] == true) {
			++test_count;
			dbg(DBG_HIGH, "test %s[%d] is still enabled", state->testNames[i], i);
		}
	}
	if (test_count <= 0) {
		err(50, __FUNCTION__, "no more tests enabled, nothing to do, aborting");
	} else {
		dbg(DBG_MED, "We have %d tests enabled and initialized", test_count);
	}

	/*
	 * Allocate bit stream
	 */
	if ((state->tp.n <= 0) || (state->tp.n < MIN_BITCOUNT)) {
		err(50, __FUNCTION__, "bogus value n: %ld, must be >= %d", state->tp.n, MIN_BITCOUNT);
	}
	state->epsilon = calloc((size_t) state->tp.n, sizeof(BitSequence));
	if (state->epsilon == NULL) {
		errp(50, __FUNCTION__, "cannot calloc for epsilon: %ld elements of %lu bytes each", state->tp.n,
		     sizeof(BitSequence));
	}

	/*
	 * Report the end of the init phase
	 */
	dbg(DBG_LOW, "end of init phase");

	return;
}


/*
 * iterate - perform a single iteration of a bitstream
 *
 * given:
 *      state           // current processing state
 */
void
iterate(struct state *state)
{
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(51, __FUNCTION__, "state arg is NULL");
	}

	/*
	 * Perform an iteration for each test on the current bitstream
	 */
	dbg(DBG_VHIGH, "before an iteration: %ld", state->curIteration);
	for (i = 1; i <= NUMOFTESTS; ++i) {

		/*
		 * Call test iterate function if the test is enabled
		 */
		if (state->testVector[i] == true && testDriver[i].iterate != NULL) {
			testDriver[i].iterate(state);
		}
	}
	dbg(DBG_VHIGH, "after an iteration: %ld", state->curIteration);

	return;
}


/*
 * Print - print to results.txt, data*.txt, stats.txt for all iterations
 *
 * given:
 *      state           // current processing state
 */
void
print(struct state *state)
{
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(52, __FUNCTION__, "state arg is NULL");
	}

	/*
	 * Print results from each test
	 *
	 * or old code: partition results
	 */
	dbg(DBG_LOW, "start of print phase");
	for (i = 1; i <= NUMOFTESTS; i++) {
		if (state->testVector[i] == true) {
			if (testDriver[i].print != NULL) {
				testDriver[i].print(state);	// print results of a test
			}
		}
	}

	/*
	 * Report the end of the print phase
	 */
	dbg(DBG_LOW, "end of print phase");

	return;
}


/*
 * metrics - uniformity and proportional analysis
 *
 * given:
 *      state           // current processing state
 */
void
metrics(struct state *state)
{
	long int passRate;
	bool case1;
	bool case2;
	double p_hat;
	int io_ret;		// I/O return status
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(53, __FUNCTION__, "state arg is NULL");
	}

	/*
	 * Perform metrics processing for each test
	 */
	dbg(DBG_LOW, "start of metrics phase");
	for (i = 1; i <= NUMOFTESTS; i++) {	// FOR EACH TEST

		// Check if the test is enabled
		if (state->testVector[i] == true) {

			// Process metrics of the test
			if (testDriver[i].metrics != NULL) {
				testDriver[i].metrics(state);
			}
		}
	}

	io_ret = fprintf(state->finalRept,
			 "\n\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
	if (io_ret <= 0) {
		errp(53, __FUNCTION__, "error in writing to finalRept");
	}
	case1 = false;
	case2 = false;
	if (state->testVector[TEST_RND_EXCURSION] || state->testVector[TEST_RND_EXCURSION_VAR]) {
		case2 = true;
	}
	for (i = 1; i <= NUMOFTESTS; i++) {
		if (state->testVector[i] && (i != TEST_RND_EXCURSION) && (i != TEST_RND_EXCURSION_VAR)) {
			case1 = true;
			break;
		}
	}
	p_hat = 1.0 - state->tp.alpha;
	passRate = 0;
	if (case1 == true) {
		if (state->maxGeneralSampleSize == 0) {
			io_ret =
			    fprintf(state->finalRept,
				    "The minimum pass rate for each statistical test with the exception of the\n");
			if (io_ret <= 0) {
				errp(53, __FUNCTION__, "error in writing to finalRept");
			}
			io_ret = fprintf(state->finalRept, "random excursion (variant) test is undefined.\n\n");
			if (io_ret <= 0) {
				errp(53, __FUNCTION__, "error in writing to finalRept");
			}
		} else {
			long double floorl_result =
			    floorl((p_hat -
				    3.0 * sqrt((p_hat * state->tp.alpha) / state->maxGeneralSampleSize)) *
				   state->maxGeneralSampleSize);
			if (floorl_result > (long double) LONG_MAX) {	// firewall
				err(50, __FUNCTION__, "floorl result is too big for a long int");
			}
			passRate = (long) floorl_result;

			io_ret =
			    fprintf(state->finalRept,
				    "The minimum pass rate for each statistical test with the exception of the\n");
			if (io_ret <= 0) {
				errp(53, __FUNCTION__, "error in writing to finalRept");
			}
			io_ret = fprintf(state->finalRept, "random excursion (variant) test is approximately = %ld for a\n",
					 state->maxGeneralSampleSize ? passRate : 0);
			if (io_ret <= 0) {
				errp(53, __FUNCTION__, "error in writing to finalRept");
			}
			io_ret = fprintf(state->finalRept, "sample size = %ld binary sequences.\n\n", state->maxGeneralSampleSize);
			if (io_ret <= 0) {
				errp(53, __FUNCTION__, "error in writing to finalRept");
			}
		}
	}
	if (case2 == true) {
		if (state->maxRandomExcursionSampleSize == 0) {
			io_ret =
			    fprintf(state->finalRept,
				    "The minimum pass rate for the random excursion (variant) test is undefined.\n\n");
			if (io_ret <= 0) {
				errp(53, __FUNCTION__, "error in writing to finalRept");
			}
		} else {
			long double floorl_result =
			    floorl((p_hat - 3.0 * sqrt((p_hat * state->tp.alpha) / state->maxRandomExcursionSampleSize)) *
				   state->maxRandomExcursionSampleSize);
			if (floorl_result > (long double) LONG_MAX) {	// firewall
				err(50, __FUNCTION__, "floorl result is too big for a long int");
			}
			passRate = (long) floorl_result;

			if (state->legacy_output == true) {
				io_ret =
				    fprintf(state->finalRept, "The minimum pass rate for the random excursion (variant) test\n");
				if (io_ret <= 0) {
					errp(53, __FUNCTION__, "error in writing to finalRept");
				}
				io_ret = fprintf(state->finalRept,
						 "is approximately = %ld for a sample size = %ld binary sequences.\n\n",
						 passRate, state->maxRandomExcursionSampleSize);
				if (io_ret <= 0) {
					errp(53, __FUNCTION__, "error in writing to finalRept");
				}
			} else {
				io_ret =
				    fprintf(state->finalRept,
					    "The minimum pass rate for the RandomExcursions and RandomExcursionsVariant\n");
				if (io_ret <= 0) {
					errp(53, __FUNCTION__, "error in writing to finalRept");
				}
				io_ret = fprintf(state->finalRept,
						 "tests is %ld for a sample size of %ld binary sequences.\n\n",
						 passRate, state->maxRandomExcursionSampleSize);
				if (io_ret <= 0) {
					errp(53, __FUNCTION__, "error in writing to finalRept");
				}
			}
		}
	}
	if (state->legacy_output == true) {
		io_ret =
		    fprintf(state->finalRept, "For further guidelines construct a probability table using the MAPLE program\n");
		if (io_ret <= 0) {
			errp(53, __FUNCTION__, "error in writing to finalRept");
		}
		io_ret = fprintf(state->finalRept, "provided in the addendum section of the documentation.\n");
		if (io_ret <= 0) {
			errp(53, __FUNCTION__, "error in writing to finalRept");
		}
		io_ret =
		    fprintf(state->finalRept,
			    "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
		if (io_ret <= 0) {
			errp(53, __FUNCTION__, "error in writing to finalRept");
		}
	}

	/*
	 * Report the end of the metric phase
	 */
	dbg(DBG_LOW, "end of metrics phase");
	return;
}

/*
 * destroy - free memory, undo init operations and other post processing
 *
 * given:
 *      state           // current processing state
 */
void
destroy(struct state *state)
{
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(54, __FUNCTION__, "state arg is NULL");
	}

	/*
	 * Perform clean up for each test
	 */
	dbg(DBG_LOW, "start of destroy phase");
	for (i = 1; i <= NUMOFTESTS; i++) {

		// Check if the test is enabled
		if (state->testVector[i] == true && testDriver[i].destroy != NULL) {

			// Perform destroy of the test
			testDriver[i].destroy(state);
		}
	}

	/*
	 * Free global allocated storage
	 */
	if (state->workDir != NULL) {
		free(state->workDir);
		state->workDir = NULL;
	}
	if (state->tmpepsilon != NULL) {
		free(state->tmpepsilon);
		state->tmpepsilon = NULL;
	}
	if (state->epsilon != NULL) {
		free(state->epsilon);
		state->epsilon = NULL;
	}
	if (state->freqFilePath != NULL) {
		free(state->freqFilePath);
		state->freqFilePath = NULL;
	}
	if (state->finalReptPath != NULL) {
		free(state->finalReptPath);
		state->finalReptPath = NULL;
	}

	/*
	 * Report the end of the metric phase
	 */
	dbg(DBG_LOW, "end of destroy phase");
	return;
}
