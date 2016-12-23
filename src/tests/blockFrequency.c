/*****************************************************************************
		    B L O C K	F R E Q U E N C Y   T E S T
 *****************************************************************************/

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


// Exit codes: 20 thru 29

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "../constants/externs.h"
#include "../utils/cephes.h"
#include "../utils/utilities.h"
#include "../utils/debug.h"


/*
 * Private stats - stats.txt information for this test
 */
struct BlockFrequency_private_stats {
	bool success;		// Success or failure of iteration test
	double chi_squared;	// chi^2 of test results
};


/*
 * Static const variables declarations
 */
static const enum test test_num = TEST_BLOCK_FREQUENCY;	// This test number


/*
 * Forward static function declarations
 */
static bool BlockFrequency_print_stat(FILE * stream, struct state *state, struct BlockFrequency_private_stats *stat,
				      double p_value);
static bool BlockFrequency_print_p_value(FILE * stream, double p_value);
static void BlockFrequency_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin);


/*
 * BlockFrequency_init - initialize the Block Frequency test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
BlockFrequency_init(struct state *state)
{
	long int M;		// Length of each block to be tested
	long int n;		// Length of a single bit stream
	long int N;		// Number of independent M-bit blocks the bit stream is partitioned into

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(20, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->cSetup != true) {
		err(20, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_NULL && state->driver_state[test_num] != DRIVER_DESTROY) {
		err(20, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_NULL: %d and != DRIVER_DESTROY: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_NULL, DRIVER_DESTROY);
	}

	/*
	 * Collect parameters from state
	 */
	M = state->tp.blockFrequencyBlockLength;
	n = state->tp.n;
	N = n / M;

	/*
	 * Disable test if conditions do not permit this test from being run
	 */
	if (n < MIN_LENGTH_BLOCK_FREQUENCY) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires bitcount(n): %ld >= %d",
		     state->testNames[test_num], test_num, n, MIN_LENGTH_BLOCK_FREQUENCY);
		state->testVector[test_num] = false;
		return;
	} else if (M < MIN_M_BLOCK_FREQUENCY) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires block length(M): %ld >= %d",
		     state->testNames[test_num], test_num, M, MIN_M_BLOCK_FREQUENCY);
		state->testVector[test_num] = false;
		return;
	} else if (M <= MIN_RATIO_M_OVER_n_BLOCK_FREQUENCY * n) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires block length(M): %ld > %f * n, and here n = %ld",
		     state->testNames[test_num], test_num, M, MIN_RATIO_M_OVER_n_BLOCK_FREQUENCY, n);
		state->testVector[test_num] = false;
		return;
	} else if (N >= MAX_N_BLOCK_FREQUENCY) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires s %ld < %d",
		     state->testNames[test_num], test_num, N, MAX_N_BLOCK_FREQUENCY);
		state->testVector[test_num] = false;
		return;
	}

	/*
	 * Create working sub-directory if forming files such as results.txt and stats.txt
	 */
	if (state->resultstxtFlag == true) {
		state->subDir[test_num] = precheckSubdir(state, state->testNames[test_num]);
		dbg(DBG_HIGH, "test %s[%d] will use subdir: %s", state->testNames[test_num], test_num, state->subDir[test_num]);
	}

	/*
	 * Allocate dynamic arrays
	 */
	if (state->resultstxtFlag == true) {
		state->stats[test_num] = create_dyn_array(sizeof(struct BlockFrequency_private_stats),
							  DEFAULT_CHUNK, state->tp.numOfBitStreams, false);	// stats.txt
	}
	state->p_val[test_num] = create_dyn_array(sizeof(double),
						  DEFAULT_CHUNK, state->tp.numOfBitStreams, false);	// results.txt

	/*
	 * Determine format of data*.txt filenames based on state->partitionCount[test_num]
	 * NOTE: If we are not partitioning the p_values, no data*.txt filenames are needed
	 */
	state->datatxt_fmt[test_num] = data_filename_format(state->partitionCount[test_num]);
	dbg(DBG_HIGH, "%s[%d] will form data*.txt filenames with the following format: %s",
	    state->testNames[test_num], test_num, state->datatxt_fmt[test_num]);

	/*
	 * Set driver state to DRIVER_INIT
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_INIT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_INIT);
	state->driver_state[test_num] = DRIVER_INIT;

	return;
}


/*
 * BlockFrequency_iterate - iterate one bit stream for Block Frequency test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
BlockFrequency_iterate(struct state *state)
{
	struct BlockFrequency_private_stats stat;	// Stats for this iteration
	long int M;		// Length of each block to be tested
	long int n;		// Length of a single bit stream
	long int N;		// Number of independent M-bit blocks the bit stream is partitioned into
	long int blockSum;      // Number of ones in a block
	double p_value;		// p_value iteration test result(s)
	double sum;             // Term of the chi squared formula
	double pi;              // Proportion of ones in a block
	double v;               // Value used in chi squared formula
	long int i;
	long int j;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(21, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "iterate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	if (state->epsilon == NULL) {
		err(21, __FUNCTION__, "state->epsilon is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_INIT && state->driver_state[test_num] != DRIVER_ITERATE) {
		err(21, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_INIT: %d and != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_INIT, DRIVER_ITERATE);
	}

	/*
	 * Collect parameters from state
	 */
	M = state->tp.blockFrequencyBlockLength;
	n = state->tp.n;
	N = n / M;

	/*
	 * Step 1: partition the sequence into N independent blocks
	 */
	sum = 0.0;
	for (i = 0; i < N; i++) {

		/*
		 * Step 2: determine the proportion of ones in each M-bit block
		 */
		blockSum = 0;
		for (j = 0; j < M; j++) {
			if (state->epsilon[j + i * M]) {
				blockSum++;
			}
		}
		pi = (double) blockSum / (double) M;

		/*
        	 * Step 3a: prepare values required for computing the test statistic
		 */
		v = pi - 0.5;
		sum += v * v;
	}

	/*
	 * Step 3b: compute the test statistic
	 */
	stat.chi_squared = 4.0 * M * sum;

	/*
	 * Step 4: compute the test P-value
	 */
	p_value = cephes_igamc(N / 2.0, stat.chi_squared / 2.0);

	/*
	 * Record success or failure for this iteration
	 */
	state->count[test_num]++;	// Count this iteration
	state->valid[test_num]++;	// Count this valid iteration
	if (isNegative(p_value)) {
		state->failure[test_num]++;	// Bogus p_value < 0.0 treated as a failure
		stat.success = false;	        // FAILURE
		warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value: %f < 0.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value);
	} else if (isGreaterThanOne(p_value)) {
		state->failure[test_num]++;	// Bogus p_value > 1.0 treated as a failure
		stat.success = false;	        // FAILURE
		warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value: %f > 1.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value);
	} else if (p_value < state->tp.alpha) {
		state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
		state->failure[test_num]++;	// Valid p_value but too low is a failure
		stat.success = false;	        // FAILURE
	} else {
		state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
		state->success[test_num]++;	// Valid p_value not too low is a success
		stat.success = true;	        // SUCCESS
	}

	/*
	 * Record values computed during this iteration
	 */
	if (state->resultstxtFlag == true) {
		append_value(state->stats[test_num], &stat);
	}
	append_value(state->p_val[test_num], &p_value);

	/*
	 * Set driver state to DRIVER_ITERATE
	 */
	if (state->driver_state[test_num] != DRIVER_ITERATE) {
		dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_ITERATE: %d",
		    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_ITERATE);
		state->driver_state[test_num] = DRIVER_ITERATE;
	}

	return;
}


/*
 * BlockFrequency_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct BlockFrequency_private_stats for format and print
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
BlockFrequency_print_stat(FILE * stream, struct state *state, struct BlockFrequency_private_stats *stat, double p_value)
{
	long int M;		// Length of each block to be tested
	long int n;		// Length of a single bit stream
	long int N;		// Number of independent M-bit blocks the bit stream is partitioned into
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(22, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(22, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(22, __FUNCTION__, "stat arg is NULL");
	}
	if (p_value == NON_P_VALUE && stat->success == true) {
		err(22, __FUNCTION__, "p_value was set to NON_P_VALUE but stat->success == true");
	}

	/*
	 * Collect parameters from state
	 */
	M = state->tp.blockFrequencyBlockLength;
	n = state->tp.n;
	N = n / M;

	/*
	 * Print stat to a file
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t\tBLOCK FREQUENCY TEST\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t---------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\tCOMPUTATIONAL INFORMATION:\n");
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\t\tBlock Frequency test\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t---------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(a) Chi^2           = %f\n", stat->chi_squared);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(b) # of substrings = %ld\n", N);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(c) block length    = %ld\n", state->tp.blockFrequencyBlockLength);
	if (io_ret <= 0) {
		return false;
	}
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t(d) Note: %ld bits were discarded.\n", n % M);
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\t(d) bits discarded  = %ld\n", n % M);
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t---------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}

	/*
	 * Report success or failure
	 */
	if (stat->success == true) {
		io_ret = fprintf(stream, "SUCCESS\t\tp_value = %f\n\n", p_value);
		if (io_ret <= 0) {
			return false;
		}
	} else if (p_value == NON_P_VALUE) {
		io_ret = fprintf(stream, "FAILURE\t\tp_value = __INVALID__\n\n");
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "FAILURE\t\tp_value = %f\n\n", p_value);
		if (io_ret <= 0) {
			return false;
		}
	}

	/*
	 * All printing successful
	 */
	return true;
}


/*
 * BlockFrequency_print_p_value - print p_value information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      stat            // struct BlockFrequency_private_stats for format and print
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
BlockFrequency_print_p_value(FILE * stream, double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(23, __FUNCTION__, "stream arg is NULL");
	}

	/*
	 * Print p_value to a file
	 */
	if (p_value == NON_P_VALUE) {
		io_ret = fprintf(stream, "__INVALID__\n");
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "%f\n", p_value);
		if (io_ret <= 0) {
			return false;
		}
	}

	/*
	 * All printing successful
	 */
	return true;
}


/*
 * BlockFrequency_print - print to results.txt, data*.txt, stats.txt for all iterations
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for once to print dynamic arrays into
 * results.txt, data*.txt, stats.txt.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
BlockFrequency_print(struct state *state)
{
	struct BlockFrequency_private_stats *stat;	// Pointer to statistics of an iteration
	double p_value;			// p_value iteration test result(s)
	FILE *stats = NULL;		// Open stats.txt file
	FILE *results = NULL;		// Open results.txt file
	FILE *data = NULL;		// Open data*.txt file
	char *stats_txt = NULL;		// Pathname for stats.txt
	char *results_txt = NULL;	// Pathname for results.txt
	char *data_txt = NULL;		// Pathname for data*.txt
	char data_filename[BUFSIZ + 1];	// Basename for a given data*.txt pathname
	bool ok;			// true -> I/O was OK
	int snprintf_ret;		// snprintf return value
	int io_ret;			// I/O return status
	long int i;
	long int j;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(24, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_HIGH, "Print driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->resultstxtFlag == false) {
		dbg(DBG_HIGH, "Print driver interface for %s[%d] was not enabled with -s", state->testNames[test_num], test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(24, __FUNCTION__,
		    "print driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(24, __FUNCTION__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->datatxt_fmt[test_num] == NULL) {
		err(24, __FUNCTION__, "format for data0*.txt filename is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_ITERATE) {
		err(24, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_ITERATE);
	}

	/*
	 * Open stats.txt file
	 */
	stats_txt = filePathName(state->subDir[test_num], "stats.txt");
	dbg(DBG_HIGH, "about to open/truncate: %s", stats_txt);
	stats = openTruncate(stats_txt);

	/*
	 * Open results.txt file
	 */
	results_txt = filePathName(state->subDir[test_num], "results.txt");
	dbg(DBG_HIGH, "about to open/truncate: %s", results_txt);
	results = openTruncate(results_txt);

	/*
	 * Write results.txt and stats.txt files
	 */
	for (i = 0; i < state->stats[test_num]->count; ++i) {

		/*
		 * Locate stat for this iteration
		 */
		stat = addr_value(state->stats[test_num], struct BlockFrequency_private_stats, i);

		/*
		 * Get p_value for this iteration
		 */
		p_value = get_value(state->p_val[test_num], double, i);

		/*
		 * Print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = BlockFrequency_print_stat(stats, state, stat, p_value);
		if (ok == false) {
			errp(24, __FUNCTION__, "error in writing to %s", stats_txt);
		}

		/*
		 * Print p_value to results.txt
		 */
		errno = 0;	// paranoia
		ok = BlockFrequency_print_p_value(results, p_value);
		if (ok == false) {
			errp(24, __FUNCTION__, "error in writing to %s", results_txt);
		}
	}

	/*
	 * Flush and close stats.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(stats);
	if (io_ret != 0) {
		errp(24, __FUNCTION__, "error flushing to: %s", stats_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(stats);
	if (io_ret != 0) {
		errp(24, __FUNCTION__, "error closing: %s", stats_txt);
	}
	free(stats_txt);
	stats_txt = NULL;

	/*
	 * Flush and close results.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(results);
	if (io_ret != 0) {
		errp(24, __FUNCTION__, "error flushing to: %s", results_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(results);
	if (io_ret != 0) {
		errp(24, __FUNCTION__, "error closing: %s", results_txt);
	}
	free(results_txt);
	results_txt = NULL;

	/*
	 * Write data*.txt for each data file if we need to partition results
	 */
	if (state->partitionCount[test_num] > 1) {

		/*
		 * For each data file
		 */
		for (j = 0; j < state->partitionCount[test_num]; ++j) {

			/*
			 * Form the data*.txt basename
			 */
			errno = 0;	// paranoia
			snprintf_ret = snprintf(data_filename, BUFSIZ, state->datatxt_fmt[test_num], j + 1);
			data_filename[BUFSIZ] = '\0';	// paranoia
			if (snprintf_ret <= 0 || snprintf_ret >= BUFSIZ || errno != 0) {
				errp(24, __FUNCTION__, "snprintf failed for %d bytes for data%03ld.txt, returned: %d", BUFSIZ,
				     j + 1, snprintf_ret);
			}

			/*
			 * Form the data*.txt filename
			 */
			data_txt = filePathName(state->subDir[test_num], data_filename);
			dbg(DBG_HIGH, "about to open/truncate: %s", data_txt);
			data = openTruncate(data_txt);

			/*
			 * Write this particular data*.txt filename
			 */
			if (j < state->p_val[test_num]->count) {
				for (i = j; i < state->p_val[test_num]->count; i += state->partitionCount[test_num]) {

					/*
					 * Get p_value for an iteration belonging to this data*.txt filename
					 */
					p_value = get_value(state->p_val[test_num], double, i);

					/*
					 * Print p_value to results.txt
					 */
					errno = 0;	// paranoia
					ok = BlockFrequency_print_p_value(data, p_value);
					if (ok == false) {
						errp(24, __FUNCTION__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * Flush and close data*.txt, free pathname
			 */
			errno = 0;	// paranoia
			io_ret = fflush(data);
			if (io_ret != 0) {
				errp(24, __FUNCTION__, "error flushing to: %s", data_txt);
			}
			errno = 0;	// paranoia
			io_ret = fclose(data);
			if (io_ret != 0) {
				errp(24, __FUNCTION__, "error closing: %s", data_txt);
			}
			free(data_txt);
			data_txt = NULL;
		}
	}

	/*
	 * Set driver state to DRIVER_PRINT
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_PRINT);
	state->driver_state[test_num] = DRIVER_PRINT;

	return;
}


/*
 * BlockFrequency_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state           // run state to test under
 *      sampleCount             // Number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // uniformity frequency bins
 */
static void
BlockFrequency_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
{
	long int passCount;			// p_values that pass
	double p_hat;				// 1 - alpha
	double proportion_threshold_max;	// When passCount is too high
	double proportion_threshold_min;	// When passCount is too low
	double chi2;				// Sum of chi^2 for each tenth
	double uniformity;			// Uniformity of frequency bins
	double expCount;			// Sample size divided by frequency bin count
	int io_ret;				// I/O return status
	long int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(25, __FUNCTION__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(25, __FUNCTION__, "freqPerBin arg is NULL");
	}

	/*
	 * Determine the number tests that passed
	 */
	if ((sampleCount <= 0) || (sampleCount < toolow)) {
		passCount = 0;
	} else {
		passCount = sampleCount - toolow;
	}

	/*
	 * Determine proportion thresholds
	 */
	p_hat = 1.0 - state->tp.alpha;
	proportion_threshold_max = (p_hat + 3.0 * sqrt((p_hat * state->tp.alpha) / sampleCount)) * sampleCount;
	proportion_threshold_min = (p_hat - 3.0 * sqrt((p_hat * state->tp.alpha) / sampleCount)) * sampleCount;

	/*
	 * Compute uniformity p-value
	 */
	chi2 = 0.0;
	expCount = sampleCount / state->tp.uniformity_bins;
	if (expCount <= 0.0) {
		// Not enough samples for uniformity check
		uniformity = 0.0;
	} else {
		// Sum chi squared of the frequency bins
		for (i = 0; i < state->tp.uniformity_bins; ++i) {
			chi2 += (freqPerBin[i] - expCount) * (freqPerBin[i] - expCount) / expCount;
		}
		// Uniformity threshold level
		uniformity = cephes_igamc((state->tp.uniformity_bins - 1.0) / 2.0, chi2 / 2.0);
	}

	/*
	 * Save or print results
	 */
	if (state->legacy_output == true) {

		/*
		 * Output uniformity results in traditional format to finalAnalysisReport.txt
		 */
		for (i = 0; i < state->tp.uniformity_bins; ++i) {
			fprintf(state->finalRept, "%3ld ", freqPerBin[i]);
		}
		if (expCount <= 0.0) {
			// Not enough samples for uniformity check
			fprintf(state->finalRept, "    ----    ");
			dbg(DBG_HIGH, "too few iterations for uniformity check on %s", state->testNames[test_num]);
		} else if (uniformity < state->tp.uniformity_level) {
			// Uniformity failure (the uniformity p-value is smaller than the minimum uniformity_level (default 0.0001)
			fprintf(state->finalRept, " %8.6f * ", uniformity);
			dbg(DBG_HIGH, "metrics detected uniformity failure for %s", state->testNames[test_num]);
		} else {
			// Uniformity success
			fprintf(state->finalRept, " %8.6f   ", uniformity);
			dbg(DBG_HIGH, "metrics detected uniformity success for %s", state->testNames[test_num]);
		}

		/*
		 * Output proportional results in traditional format to finalAnalysisReport.txt
		 */
		if (sampleCount == 0) {
			// Not enough samples for proportional check
			fprintf(state->finalRept, " ------     %s\n", state->testNames[test_num]);
			dbg(DBG_HIGH, "too few samples for proportional check on %s", state->testNames[test_num]);
		} else if ((passCount < proportion_threshold_min) || (passCount > proportion_threshold_max)) {
			// Proportional failure
			fprintf(state->finalRept, "%4ld/%-4ld *	 %s\n", passCount, sampleCount, state->testNames[test_num]);
			dbg(DBG_HIGH, "metrics detected proportional failure for %s", state->testNames[test_num]);
		} else {
			// Proportional success
			fprintf(state->finalRept, "%4ld/%-4ld	 %s\n", passCount, sampleCount, state->testNames[test_num]);
			dbg(DBG_HIGH, "metrics detected proportional success for %s", state->testNames[test_num]);
		}

		/*
		 * Flush the output file buffer
		 */
		errno = 0;		// paranoia
		io_ret = fflush(state->finalRept);
		if (io_ret != 0) {
			errp(25, __FUNCTION__, "error flushing to: %s", state->finalReptPath);
		}

	} else {
		bool uniformity_passed = true;
		bool proportion_passed = true;

		/*
		 * Check uniformity results
		 */
		if (expCount <= 0.0 || uniformity < state->tp.uniformity_level) {
			// Uniformity failure or not enough samples for uniformity check
			uniformity_passed = false;
			dbg(DBG_HIGH, "metrics detected uniformity failure for %s", state->testNames[test_num]);
		}

		/*
		 * Check proportional results
		 */
		if (sampleCount == 0 || (passCount < proportion_threshold_min) || (passCount > proportion_threshold_max)) {
			// Proportional failure or not enough samples for proportional check
			proportion_passed = false;
			dbg(DBG_HIGH, "metrics detected proportional failure for %s", state->testNames[test_num]);
		}

		if (proportion_passed == false && uniformity_passed == false) {
			state->metric_results.block_frequency = FAILED_BOTH;
		} else if (proportion_passed == false) {
			state->metric_results.block_frequency = FAILED_PROPORTION;
		} else if (uniformity_passed == false) {
			state->metric_results.block_frequency = FAILED_UNIFORMITY;
		} else {
			state->metric_results.block_frequency = PASSED_BOTH;
			state->successful_tests++;
		}
	}

	return;
}


/*
 * BlockFrequency_metrics - uniformity and proportional analysis of a test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to complete the test analysis for all iterations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
BlockFrequency_metrics(struct state *state)
{
	long int sampleCount;	// Number of bitstreams in which we will count p_values
	long int toolow;	// p_values that were below alpha
	double p_value;		// p_value iteration test result(s)
	long int *freqPerBin;	// Uniformity frequency bins
	long int i;
	long int j;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(26, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(26, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(26, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with p_val length: %ld != bit streams: %ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->driver_state[test_num] != DRIVER_PRINT && state->resultstxtFlag == true) {
		err(26, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_PRINT: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_PRINT);
	} else if (state->driver_state[test_num] != DRIVER_ITERATE && state->resultstxtFlag == false) {
		err(26, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_ITERATE);
	}

	/*
	 * Allocate uniformity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(26, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
		     state->tp.uniformity_bins, sizeof(long int));
	}

	/*
	 * Print for each partition (or the whole set of p_values if partitionCount is 1)
	 */
	for (j = 0; j < state->partitionCount[test_num]; ++j) {

		/*
		 * Set counters to zero
		 */
		toolow = 0;
		sampleCount = 0;
		memset(freqPerBin, 0, state->tp.uniformity_bins * sizeof(freqPerBin[0]));

		/*
		 * Tally p_value
		 */
		for (i = j; i < state->p_val[test_num]->count; i += state->partitionCount[test_num]) {

			// Get the iteration p_value
			p_value = get_value(state->p_val[test_num], double, i);
			if (p_value == NON_P_VALUE) {
				continue;	// the test was not possible for this iteration
			}
			// Case: random excursion test
			if (state->is_excursion[test_num] == true) {
				// Random excursion tests only sample > 0 p_values
				if (p_value > 0.0) {
					++sampleCount;
				} else {
					// Ignore p_value of 0 for random excursion tests
					continue;
				}
			}
			// Case: general (non-random excursion) test
			else {
				// All other tests count all p_values
				++sampleCount;
			}

			// Count the number of p_values below alpha
			if (p_value < state->tp.alpha) {
				++toolow;
			}
			// Tally the p_value in a uniformity bin
			if (p_value >= 1.0) {
				++freqPerBin[state->tp.uniformity_bins - 1];
			} else if (p_value >= 0.0) {
				++freqPerBin[(int) floor(p_value * (double) state->tp.uniformity_bins)];
			} else {
				++freqPerBin[0];
			}
		}

		/*
		 * Print uniformity and proportional information for a tallied count
		 */
		BlockFrequency_metric_print(state, sampleCount, toolow, freqPerBin);

		/*
		 * Track maximum samples
		 */
		if (state->is_excursion[test_num] == true) {
			if (sampleCount > state->maxRandomExcursionSampleSize) {
				state->maxRandomExcursionSampleSize = sampleCount;
			}
		} else {
			if (sampleCount > state->maxGeneralSampleSize) {
				state->maxGeneralSampleSize = sampleCount;
			}
		}
	}

	/*
	 * Free allocated storage
	 */
	free(freqPerBin);
	freqPerBin = NULL;

	/*
	 * Set driver state to DRIVER_METRICS
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_METRICS);
	state->driver_state[test_num] = DRIVER_METRICS;

	return;
}


/*
 * BlockFrequency_destroy - post process results for this text
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to cleanup any storage or state
 * associated with this test.
 */
void
BlockFrequency_destroy(struct state *state)
{
	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(27, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "destroy function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}

	/*
	 * Free dynamic arrays
	 */
	if (state->stats[test_num] != NULL) {
		free_dyn_array(state->stats[test_num]);
		free(state->stats[test_num]);
		state->stats[test_num] = NULL;
	}
	if (state->p_val[test_num] != NULL) {
		free_dyn_array(state->p_val[test_num]);
		free(state->p_val[test_num]);
		state->p_val[test_num] = NULL;
	}

	/*
	 * Free other test storage
	 */
	if (state->datatxt_fmt[test_num] != NULL) {
		free(state->datatxt_fmt[test_num]);
		state->datatxt_fmt[test_num] = NULL;
	}
	if (state->subDir[test_num] != NULL) {
		free(state->subDir[test_num]);
		state->subDir[test_num] = NULL;
	}

	/*
	 * Set driver state to DRIVER_DESTROY
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_DESTROY);
	state->driver_state[test_num] = DRIVER_DESTROY;

	return;
}
