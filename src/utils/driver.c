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
 * See the README.md and the initial comment in sts.c for more information.
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
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "defs.h"
#include "utilities.h"
#include "debug.h"
#include "stat_fncs.h"

extern long int debuglevel;	// -v lvl: defines the level of verbosity for debugging

/*
 * Driver interface - defines how each test is performed at each phase
 */
struct driver {
	void (*init) (struct state *state);			// Initialize the test and check input size recommendations
	void (*iterate) (struct thread_state * thread_state);	// Perform a single iteration test on the bitstream
	void (*print) (struct state *state);			// Log iteration info into stats.txt, data*.txt, results.txt if -s
	void (*metrics) (struct state *state);			// Uniformity and proportional analysis of a test
	void (*destroy) (struct state *state);			// Final test cleanup and memory de-allocation
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

	{			// TEST_DFT = 7, Discrete Fourier Transform test (discreteFourierTransform.c)
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
 * Forward static function declarations
 */
static void finishMetricTestsSentence(test_metric_result result, struct state *state);

/*
 * Init - initialize the variables needed for each test and check if the input size recommendations are respected
 *
 * given:
 *      state           // current processing state
 */
void
init(struct state *state)
{
	int test_count;		// Number of tests enabled after initialization
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(50, __func__, "state arg is NULL");
	}
	if (state->workDir == NULL) {
		err(50, __func__, "state->workDir is NULL");
	}
	dbg(DBG_LOW, "Start of init phase");

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
		if (state->promptFlag == true) {

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
	 * Open the output files
	 */
	if (state->legacy_output == true) {
		state->freqFilePath = filePathName(state->workDir, "freq.txt");
		dbg(DBG_MED, "Will use freq.txt file: %s", state->freqFilePath);
		state->freqFile = fopen(state->freqFilePath, "w");
		if (state->freqFile == NULL) {
			errp(50, __func__, "Could not open freq.txt file: %s", state->freqFilePath);
		}

		if (state->runMode == MODE_ITERATE_AND_ASSESS || state->runMode == MODE_ASSESS_ONLY) {
			state->finalReptPath = filePathName(state->workDir, "finalAnalysisReport.txt");
			dbg(DBG_MED, "Will use finalAnalysisReport.txt file: %s", state->finalReptPath);
			state->finalRept = fopen(state->finalReptPath, "w");
			if (state->finalRept == NULL) {
				errp(50, __func__, "Could not open finalAnalysisReport.txt file: %s", state->finalReptPath);
			}
		}

	} else {
		if (state->runMode == MODE_ITERATE_AND_ASSESS || state->runMode == MODE_ASSESS_ONLY) {
			state->finalReptPath = filePathName(state->workDir, "result.txt");
			dbg(DBG_MED, "Will use result.txt file: %s", state->finalReptPath);
			state->finalRept = fopen(state->finalReptPath, "w");
			if (state->finalRept == NULL) {
				errp(50, __func__, "Could not open result.txt file: %s", state->finalReptPath);
			}
		}
	}

	/*
	 * Indicate that the test constants have not been initialized yet
	 *
	 * NOTE: The initialization must be done after the command line arguments are parsed,
	 *       AND after any test parameters are established (by default or via interactive prompt),
	 *       AND before the individual test init functions are called
	 *       (i.e., before the initialize all active tests section below).
	 */
	state->cSetup = false;

	/*
	 * Check preconditions (firewall)
	 */
	if (state->tp.n <= 0) { // guard against taking the square root of a negative value
		err(50, __func__, "bogus n value: %ld should be > 0", state->tp.n);
	}

	/*
	 * Compute pure numerical constants
	 */
	state->c.sqrt2 = sqrt(2.0);
	state->c.log2 = log(2.0);

	/*
	 * Compute constants related to the value of n
	 */
	state->c.sqrtn = sqrt((double) state->tp.n);
	state->c.logn = log(state->tp.n);

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
		err(50, __func__, "no more tests enabled, nothing to do, aborting");
	} else {
		dbg(DBG_MED, "We have %d tests enabled and initialized", test_count);
	}

	/*
	 * Check that n is big enough
	 */
	if ((state->tp.n <= 0) || (state->tp.n < GLOBAL_MIN_BITCOUNT)) {
		err(50, __func__, "bogus value n: %ld, must be >= %d", state->tp.n, GLOBAL_MIN_BITCOUNT);
	}

	/*
	 * Set the number of iterations not done yet to be equal to the total numOfBitstreams
	 */
	state->iterationsMissing = state->tp.numOfBitStreams;

	/*
	 * Allocate the array for the bit streams copied to memory
	 */
	state->epsilon = calloc((size_t) state->numberOfThreads, sizeof(*state->epsilon));
	if (state->epsilon == NULL) {
		errp(50, __func__, "cannot calloc for epsilon: %ld elements of %lu bytes each", state->numberOfThreads,
		     sizeof(*state->epsilon));
	}

	/*
	 * Allocate the array for the bit stream copied to memory for each thread
	 */
	for (i = 0; i < state->numberOfThreads; i++) {
		state->epsilon[i] = calloc((size_t) state->tp.n, sizeof(BitSequence));
		if (state->epsilon == NULL) {
			errp(50, __func__, "cannot calloc for epsilon[%d]: %ld elements of %lu bytes each", i,
			     state->tp.n, sizeof(BitSequence));
		}
	}

	/*
	 * Report the end of the init phase
	 */
	dbg(DBG_LOW, "End of init phase\n");

	return;
}


/*
 * iterate - perform a single run of all the enabled tests on a bitstream
 *
 * given:
 *      state           // current processing state
 */
void
iterate(struct thread_state *thread_state)
{
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (thread_state == NULL) {
		err(51, __func__, "thread_state arg is NULL");
	}
	struct state *state = thread_state->global_state;
	if (state == NULL) {
		err(51, __func__, "state is NULL");
	}

	/*
	 * Perform an iteration for each test on the current bitstream
	 */
	for (i = 1; i <= NUMOFTESTS; ++i) {

		/*
		 * Call test iterate function if the test is enabled
		 */
		if (state->testVector[i] == true && testDriver[i].iterate != NULL) {
			testDriver[i].iterate(thread_state);
		}
	}

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
		err(52, __func__, "state arg is NULL");
	}

	/*
	 * Print results from each test
	 *
	 * or old code: partition results
	 */
	dbg(DBG_LOW, "Start of print phase");
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
	dbg(DBG_LOW, "End of print phase\n");

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
	bool is_first = false;
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(53, __func__, "state arg is NULL");
	}

	/*
	 * Print introductory information to the output files
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(state->finalRept,
				 "------------------------------------------------------------------------------\n");
		if (io_ret <= 0) {
			errp(53, __func__, "error in writing to finalRept");
		}
		io_ret = fprintf(state->finalRept,
				 "RESULTS FOR THE UNIFORMITY OF P-VALUES AND THE PROPORTION OF PASSING SEQUENCES\n");
		if (io_ret <= 0) {
			errp(53, __func__, "error in writing to finalRept");
		}
		io_ret = fprintf(state->finalRept,
				 "------------------------------------------------------------------------------\n");
		if (io_ret <= 0) {
			errp(53, __func__, "error in writing to finalRept");
		}
		if (state->randomDataPath == NULL) {
			io_ret = fprintf(state->finalRept, "   generator is <((--UNKNOWN--))>\n");
		} else {
			io_ret = fprintf(state->finalRept, "   generator is <%s>\n", state->randomDataPath);
		}
		if (io_ret <= 0) {
			errp(53, __func__, "error in writing to finalRept");
		}
		io_ret = fprintf(state->finalRept,
				 "------------------------------------------------------------------------------\n");
		if (io_ret <= 0) {
			errp(53, __func__, "error in writing to finalRept");
		}
		io_ret = fprintf(state->finalRept,
				 " C1  C2  C3  C4  C5  C6  C7  C8  C9 C10  P-VALUE  PROPORTION  STATISTICAL TEST\n");
		if (io_ret <= 0) {
			errp(53, __func__, "error in writing to finalRept");
		}
		io_ret = fprintf(state->finalRept,
				 "------------------------------------------------------------------------------\n");
		if (io_ret <= 0) {
			errp(53, __func__, "error in writing to finalRept");
		}

	}

	/*
	 * Perform metrics processing for each test and print each result to the output files
	 */
	dbg(DBG_LOW, "Start of assess phase");
	for (i = 1; i <= NUMOFTESTS; i++) {	// FOR EACH TEST

		// Check if the test is enabled
		if (state->testVector[i] == true) {

			// Process metrics of the test
			if (testDriver[i].metrics != NULL) {
				dbg(DBG_MED, "Start of assess metrics phase for test[%d]: %s", i,
				    ((state->testNames[i] == NULL) ? "((NULL test name))" : state->testNames[i]));
				testDriver[i].metrics(state);
				dbg(DBG_MED, "End of assess metrics phase for test[%d]: %s", i,
				    ((state->testNames[i] == NULL) ? "((NULL test name))" : state->testNames[i]));
			}
		}
	}

	/*
	 * Print additional information to output files
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(state->finalRept,
				 "\n\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
		if (io_ret <= 0) {
			errp(53, __func__, "error in writing to finalRept");
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
		if (case1 == true) {
			if (state->maxGeneralSampleSize == 0) {
				io_ret = fprintf(state->finalRept,
						 "The minimum pass rate for each statistical test with the exception of the\n");
				if (io_ret <= 0) {
					errp(53, __func__, "error in writing to finalRept");
				}
				io_ret = fprintf(state->finalRept, "random excursion (variant) test is undefined.\n\n");
				if (io_ret <= 0) {
					errp(53, __func__, "error in writing to finalRept");
				}
			} else {
				long double floorl_result =
						floorl((p_hat - 3.0 * sqrt((p_hat * state->tp.alpha) /
											   state->maxGeneralSampleSize)) *
											   state->maxGeneralSampleSize);
				if (floorl_result > (long double) LONG_MAX) {        // firewall
					err(50, __func__, "floorl result is too big for a long int");
				}
				passRate = (long) floorl_result;

				io_ret = fprintf(state->finalRept,
						 "The minimum pass rate for each statistical test with the exception of the\n");
				if (io_ret <= 0) {
					errp(53, __func__, "error in writing to finalRept");
				}
				io_ret = fprintf(state->finalRept, "random excursion (variant) test is approximately = %ld for a\n",
						 state->maxGeneralSampleSize ? passRate : 0);
				if (io_ret <= 0) {
					errp(53, __func__, "error in writing to finalRept");
				}
				io_ret = fprintf(state->finalRept, "sample size = %ld binary sequences.\n\n",
						 state->maxGeneralSampleSize);
				if (io_ret <= 0) {
					errp(53, __func__, "error in writing to finalRept");
				}
			}
		}
		if (case2 == true) {
			if (state->maxRandomExcursionSampleSize == 0) {
				io_ret = fprintf(state->finalRept,
						 "The minimum pass rate for the random excursion (variant) test is undefined.\n\n");
				if (io_ret <= 0) {
					errp(53, __func__, "error in writing to finalRept");
				}
			} else {
				long double floorl_result = floorl((p_hat - 3.0 * sqrt((p_hat * state->tp.alpha) /
										       state->maxRandomExcursionSampleSize)) *
								   state->maxRandomExcursionSampleSize);
				if (floorl_result > (long double) LONG_MAX) {        // firewall
					err(50, __func__, "floorl result is too big for a long int");
				}
				passRate = (long) floorl_result;

				if (state->legacy_output == true) {
					io_ret = fprintf(state->finalRept,
							 "The minimum pass rate for the random excursion (variant) test\n");
					if (io_ret <= 0) {
						errp(53, __func__, "error in writing to finalRept");
					}
					io_ret = fprintf(state->finalRept,
							 "is approximately = %ld for a sample size = %ld binary sequences.\n\n",
							 passRate, state->maxRandomExcursionSampleSize);
					if (io_ret <= 0) {
						errp(53, __func__, "error in writing to finalRept");
					}
				} else {
					io_ret = fprintf(state->finalRept, "The minimum pass rate for "
							"the RandomExcursions and RandomExcursionsVariant\n");
					if (io_ret <= 0) {
						errp(53, __func__, "error in writing to finalRept");
					}
					io_ret = fprintf(state->finalRept,
							 "tests is %ld for a sample size of %ld binary sequences.\n\n",
							 passRate, state->maxRandomExcursionSampleSize);
					if (io_ret <= 0) {
						errp(53, __func__, "error in writing to finalRept");
					}
				}
			}
		}

		io_ret = fprintf(state->finalRept,
				 "For further guidelines construct a probability table using the MAPLE program\n");
		if (io_ret <= 0) {
			errp(53, __func__, "error in writing to finalRept");
		}
		io_ret = fprintf(state->finalRept, "provided in the addendum section of the documentation.\n");
		if (io_ret <= 0) {
			errp(53, __func__, "error in writing to finalRept");
		}
		io_ret = fprintf(state->finalRept,
				 "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
		if (io_ret <= 0) {
			errp(53, __func__, "error in writing to finalRept");
		}

	} else {

		/*
		 * Count the total number of tests conducted to a single bitstreams
		 */
		long int total_number_of_tests = 0;
		for (i = 1; i <= NUMOFTESTS; i++) {
			if (state->testVector[i] == true) {
				total_number_of_tests += state->partitionCount[i];
			}
		}

		/*
		 * Print a brief summary of what file / generator tested and how
		 */
		io_ret = fprintf(state->finalRept, "A total of %ld tests (some of the %d tests actually consist of multiple "
						 "sub-tests)\nwere conducted to evaluate the randomness of %ld bitstreams of "
						 "%ld bits from:\n\n\t%s\n\n",
				 total_number_of_tests, NUMOFTESTS, state->tp.numOfBitStreams, state->tp.n,
				 state->stdinData == true ?
				     "((standard input))" :
				     (state->randomDataArg == true ?
					 ((state->randomDataPath != NULL && strcmp(state->randomDataPath, "/dev/null") != 0) ?
					     state->randomDataPath : "--UNKNOWN--") :
					 "--SOME_FILE--"));
		if (io_ret <= 0) {
			errp(5, __func__, "error in writing to finalRept");
		}

		/*
		 * If in distributed mode, specify from which files the p-values were taken
		 */
		if (state->runMode == MODE_ASSESS_ONLY) {
			io_ret = fprintf(state->finalRept, "using the p-values from the following files:\n\n\t%s/"
					"sts.*.*.%ld.pvalues\n\n", state->pvalues_dir, state->tp.n);
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
		}

		/*
		 * Print a brief explanation on the metric analyses that were conducted on the p-values
		 */
		io_ret = fprintf(state->finalRept, "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - "
						 "- - - - - - - - - - - - - - -\n\n"
						 "The numerous empirical results of these tests were then interpreted with\nan "
						 "examination of the proportion of sequences that pass a statistical test\n"
						 "(proportion analysis) and the distribution of p-values to check for uniformity\n"
						 "(uniformity analysis). The results were the following:\n\n"
						 "\t%ld/%ld tests passed successfully both the analyses.\n"
						 "\t%ld/%ld tests did not pass successfully both the analyses.\n\n"
						 "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - "
						 "- - - - - - - - - - - - - - -\n\n"
						 "Here are the results of the single tests:\n",
				 state->successful_tests, total_number_of_tests,
				 total_number_of_tests - state->successful_tests, total_number_of_tests);
		if (io_ret <= 0) {
			errp(5, __func__, "error in writing to finalRept");
		}

		/*
		 * Print details about the single tests
		 */
		if (state->testVector[TEST_FREQUENCY] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Frequency\" test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.frequency, state);
		}

		if (state->testVector[TEST_BLOCK_FREQUENCY] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Block Frequency\" test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.block_frequency, state);
		}

		if (state->testVector[TEST_CUSUM] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Cumulative Sums\" (forward) test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.cusum[0], state);

			io_ret = fprintf(state->finalRept, "   The \"Cumulative Sums\" (backward) test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.cusum[1], state);
		}

		if (state->testVector[TEST_RUNS] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Runs\" test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.runs, state);
		}

		if (state->testVector[TEST_LONGEST_RUN] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Longest Run of Ones\" test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.longest_run, state);
		}

		if (state->testVector[TEST_RANK] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Binary Matrix Rank\" test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.rank, state);
		}

		if (state->testVector[TEST_DFT] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Discrete Fourier Transform\" test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.dft, state);
		}

		if (state->testVector[TEST_NON_OVERLAPPING] == true) {
			is_first = true;

			if (state->metric_results.non_overlapping[PASSED_BOTH] > 0) {
				io_ret = fprintf(state->finalRept, "\n - ");
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				is_first = false;

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Non-overlapping Template Matching\" tests ",
						 state->metric_results.non_overlapping[PASSED_BOTH],
						 state->partitionCount[TEST_NON_OVERLAPPING]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(PASSED_BOTH, state);
			}

			if (state->metric_results.non_overlapping[FAILED_PROPORTION] > 0) {
				if (is_first == true) {
					io_ret = fprintf(state->finalRept, "\n - ");
					is_first = false;
				} else {
					io_ret = fprintf(state->finalRept, "   ");
				}

				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Non-overlapping Template Matching\" tests ",
						 state->metric_results.non_overlapping[FAILED_PROPORTION],
						 state->partitionCount[TEST_NON_OVERLAPPING]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(FAILED_PROPORTION, state);
			}

			if (state->metric_results.non_overlapping[FAILED_UNIFORMITY] > 0) {
				if (is_first == true) {
					io_ret = fprintf(state->finalRept, "\n - ");
					is_first = false;
				} else {
					io_ret = fprintf(state->finalRept, "   ");
				}

				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Non-overlapping Template Matching\" tests ",
						 state->metric_results.non_overlapping[FAILED_UNIFORMITY],
						 state->partitionCount[TEST_NON_OVERLAPPING]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(FAILED_UNIFORMITY, state);
			}

			if (state->metric_results.non_overlapping[FAILED_BOTH] > 0) {
				if (is_first == true) {
					io_ret = fprintf(state->finalRept, "\n - ");
				} else {
					io_ret = fprintf(state->finalRept, "   ");
				}

				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Non-overlapping Template Matching\" tests ",
						 state->metric_results.non_overlapping[FAILED_BOTH],
						 state->partitionCount[TEST_NON_OVERLAPPING]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(FAILED_BOTH, state);
			}
		}

		if (state->testVector[TEST_OVERLAPPING] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Overlapping Template Matching\" test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.overlapping, state);
		}

		if (state->testVector[TEST_UNIVERSAL] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Maurer's Universal Statistical\" test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.universal, state);
		}

		if (state->testVector[TEST_APEN] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Approximate Entropy\" test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.approximate_entropy, state);
		}

		if (state->testVector[TEST_RND_EXCURSION] == true) {
			is_first = true;

			if (state->metric_results.random_excursions[PASSED_BOTH] > 0) {
				io_ret = fprintf(state->finalRept, "\n - ");
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				is_first = false;

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Random Excursions\" tests ",
						 state->metric_results.random_excursions[PASSED_BOTH],
						 state->partitionCount[TEST_RND_EXCURSION]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(PASSED_BOTH, state);
			}

			if (state->metric_results.random_excursions[FAILED_PROPORTION] > 0) {
				if (is_first == true) {
					io_ret = fprintf(state->finalRept, "\n - ");
					is_first = false;
				} else {
					io_ret = fprintf(state->finalRept, "   ");
				}

				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Random Excursions\" tests ",
						 state->metric_results.random_excursions[FAILED_PROPORTION],
						 state->partitionCount[TEST_RND_EXCURSION]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(FAILED_PROPORTION, state);
			}

			if (state->metric_results.random_excursions[FAILED_UNIFORMITY] > 0) {
				if (is_first == true) {
					io_ret = fprintf(state->finalRept, "\n - ");
					is_first = false;
				} else {
					io_ret = fprintf(state->finalRept, "   ");
				}

				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Random Excursions\" tests ",
						 state->metric_results.random_excursions[FAILED_UNIFORMITY],
						 state->partitionCount[TEST_RND_EXCURSION]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(FAILED_UNIFORMITY, state);
			}

			if (state->metric_results.random_excursions[FAILED_BOTH] > 0) {
				if (is_first == true) {
					io_ret = fprintf(state->finalRept, "\n - ");
				} else {
					io_ret = fprintf(state->finalRept, "   ");
				}

				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Random Excursions\" tests ",
						 state->metric_results.random_excursions[FAILED_BOTH],
						 state->partitionCount[TEST_RND_EXCURSION]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(FAILED_BOTH, state);
			}
		}

		if (state->testVector[TEST_RND_EXCURSION_VAR] == true) {
			is_first = true;

			if (state->metric_results.random_excursions_var[PASSED_BOTH] > 0) {
				io_ret = fprintf(state->finalRept, "\n - ");
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				is_first = false;

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Random Excursions Variant\" tests ",
						 state->metric_results.random_excursions_var[PASSED_BOTH],
						 state->partitionCount[TEST_RND_EXCURSION_VAR]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(PASSED_BOTH, state);
			}

			if (state->metric_results.random_excursions_var[FAILED_PROPORTION] > 0) {
				if (is_first == true) {
					io_ret = fprintf(state->finalRept, "\n - ");
					is_first = false;
				} else {
					io_ret = fprintf(state->finalRept, "   ");
				}

				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Random Excursions Variant\" tests ",
						 state->metric_results.random_excursions_var[FAILED_PROPORTION],
						 state->partitionCount[TEST_RND_EXCURSION_VAR]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(FAILED_PROPORTION, state);
			}

			if (state->metric_results.random_excursions_var[FAILED_UNIFORMITY] > 0) {
				if (is_first == true) {
					io_ret = fprintf(state->finalRept, "\n - ");
					is_first = false;
				} else {
					io_ret = fprintf(state->finalRept, "   ");
				}

				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Random Excursions Variant\" tests ",
						 state->metric_results.random_excursions_var[FAILED_UNIFORMITY],
						 state->partitionCount[TEST_RND_EXCURSION_VAR]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(FAILED_UNIFORMITY, state);
			}

			if (state->metric_results.random_excursions_var[FAILED_BOTH] > 0) {
				if (is_first == true) {
					io_ret = fprintf(state->finalRept, "\n - ");
				} else {
					io_ret = fprintf(state->finalRept, "   ");
				}

				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}

				io_ret = fprintf(state->finalRept, "%ld/%d of the \"Random Excursions Variant\" tests ",
						 state->metric_results.random_excursions_var[FAILED_BOTH],
						 state->partitionCount[TEST_RND_EXCURSION_VAR]);
				if (io_ret <= 0) {
					errp(5, __func__, "error in writing to finalRept");
				}
				finishMetricTestsSentence(FAILED_BOTH, state);
			}
		}

		if (state->testVector[TEST_SERIAL] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Serial\" (first) test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.serial[0], state);

			io_ret = fprintf(state->finalRept, "   The \"Serial\" (second) test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.serial[1], state);
		}

		if (state->testVector[TEST_LINEARCOMPLEXITY] == true) {
			io_ret = fprintf(state->finalRept, "\n - The \"Linear Complexity\" test ");
			if (io_ret <= 0) {
				errp(5, __func__, "error in writing to finalRept");
			}
			finishMetricTestsSentence(state->metric_results.linear_complexity, state);
		}

		/*
		 * Print conclusion
		 */
		io_ret = fprintf(state->finalRept,
				 "\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - "
				 "- - - - - - - - - - - - - - -\n\n"
				"The missing tests (if any) were whether disabled manually by the user or disabled\n"
						 "at run time due to input size requirements not satisfied by this run.\n\n");
		if (io_ret <= 0) {
			errp(5, __func__, "error in writing to finalRept");
		}
	}

	/*
	 * Report the end of the metric phase
	 */
	dbg(DBG_LOW, "End of assess phase\n");
	return;
}

static void finishMetricTestsSentence(test_metric_result result, struct state *state) {
	int io_ret;		// I/O return status

	switch (result) {
	case PASSED_BOTH:
		io_ret = fprintf(state->finalRept, "passed both the analyses.\n");
		if (io_ret <= 0) {
			errp(5, __func__, "error in writing to finalRept");
		}
		break;
	case FAILED_UNIFORMITY:
		io_ret = fprintf(state->finalRept, "FAILED the uniformity analysis.\n");
		if (io_ret <= 0) {
			errp(5, __func__, "error in writing to finalRept");
		}
		break;
	case FAILED_PROPORTION:
		io_ret = fprintf(state->finalRept, "FAILED the proportion analysis.\n");
		if (io_ret <= 0) {
			errp(5, __func__, "error in writing to finalRept");
		}
		break;
	case FAILED_BOTH:
		io_ret = fprintf(state->finalRept, "FAILED both the analyses.\n");
		if (io_ret <= 0) {
			errp(5, __func__, "error in writing to finalRept");
		}
		break;
	}
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
	int io_ret;			// I/O return status
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(54, __func__, "state arg is NULL");
	}

	/*
	 * Close down the frequency file
	 */
	if (state->freqFile != NULL) {
		errno = 0;	// paranoia
		io_ret = fflush(state->freqFile);
		if (io_ret != 0) {
			errp(5, __func__, "error flushing freqFile");
		}
		errno = 0;	// paranoia
		io_ret = fclose(state->freqFile);
		if (io_ret != 0) {
			errp(5, __func__, "error closing freqFile");
		}
	}

	/*
	 * Flush the output file buffer and close the file
	 */
	if (state->finalRept != NULL) {
		errno = 0;                // paranoia
		io_ret = fflush(state->finalRept);
		if (io_ret != 0) {
			errp(5, __func__, "error flushing finalRept");
		}
		errno = 0;                // paranoia
		io_ret = fclose(state->finalRept);
		if (io_ret != 0) {
			errp(5, __func__, "error closing finalRept");
		}
	}

	/*
	 * Perform clean up for each test
	 */
	dbg(DBG_LOW, "Start of destroy phase");
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
	if (state->workDir != NULL && state->workDirFlag == true) {
		free(state->workDir);
		state->workDir = NULL;
	}
	if (state->tmpepsilon != NULL) {
		free(state->tmpepsilon);
		state->tmpepsilon = NULL;
	}
	for (i = 0; i < state->numberOfThreads; i++) {
		if (state->epsilon[i] != NULL) {
			free(state->epsilon[i]);
			state->epsilon[i] = NULL;
		}
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
	dbg(DBG_LOW, "End of destroy phase\n");
	return;
}
