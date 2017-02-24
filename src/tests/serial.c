/*****************************************************************************
			      S E R I A L   T E S T
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


// Exit codes: 190 thru 199

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "../utils/externs.h"
#include "../utils/cephes.h"
#include "../utils/utilities.h"
#include "../utils/debug.h"


/*
 * Private stats - stats.txt information for this test
 */
struct Serial_private_stats {
	bool success1;		// Success or failure of iteration test for 1st p_value
	bool success2;		// Success or failure of iteration test for 2nd p_value
	double psim0;		// 0th chi^2 type statistic
	double psim1;		// 1st chi^2 type statistic
	double psim2;		// 2nd chi^2 type statistic
	double del1;		// first delta chi^2 type statistic
	double del2;		// second delta chi^2 type statistic
};


/*
 * Static const variables declarations
 */
static const enum test test_num = TEST_SERIAL;	// This test number


/*
 * Forward static function declarations
 */
static double compute_psi2(struct thread_state *thread_state, long int blocksize);
static bool Serial_print_stat(FILE * stream, struct state *state, struct Serial_private_stats *stat, double p_value1,
			      double p_value2);
static bool Serial_print_p_value(FILE * stream, double p_value);
static void Serial_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin, int index);


/*
 * Serial_init - initialize the Serial test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Serial_init(struct state *state)
{
	long int m;		// Serial block length (state->tp.serialBlockLength)
	long int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(190, __func__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->cSetup != true) {
		err(190, __func__, "test constants not setup prior to calling %s for %s[%d]",
		    __func__, state->testNames[test_num], test_num);
	}

	/*
	 * Collect parameters from state
	 */
	m = state->tp.serialBlockLength;

	/*
	 * Disable test if conditions do not permit this test from being run
	 */
	if (m >= lround(floor((state->c.logn / state->c.log2) - 2.0))) {
		warn(__func__, "disabling test %s[%d]: requires block length(m): %ld >= %ld",
		     state->testNames[test_num], test_num, m, lround(floor(state->c.logn / state->c.log2 - 2.0)));
		state->testVector[test_num] = false;
		return;
	}

	/*
	 * Allocate frequency count v array
	 */
	if (m > (BITS_N_LONGINT - 1)) {	// firewall
		err(190, __func__, "m is too large, 1 << (m:%ld) can't be longer than %ld bits", m, BITS_N_LONGINT - 1);
	}
	state->serial_v_len = (long int) 1 << m;
	state->serial_v = malloc((size_t) state->numberOfThreads * sizeof(*state->serial_v));
	if (state->serial_v == NULL) {
		errp(190, __func__, "cannot malloc for serial_v: %ld elements of %ld bytes each", state->numberOfThreads,
		     sizeof(*state->serial_v));
	}
	for (i = 0; i < state->numberOfThreads; i++) {
		state->serial_v[i] = malloc(state->serial_v_len * sizeof(state->serial_v[i][0]));
		if (state->serial_v[i] == NULL) {
			errp(190, __func__, "cannot malloc of %ld elements of %ld bytes each for state->serial_v[%ld]",
			     state->serial_v_len, sizeof(state->universal_T[i][0]), i);
		}
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
		state->stats[test_num] = create_dyn_array(sizeof(struct Serial_private_stats),
							  DEFAULT_CHUNK, state->tp.numOfBitStreams, false);        // stats.txt
	}
	state->p_val[test_num] = create_dyn_array(sizeof(double),
						  DEFAULT_CHUNK, 2 * state->tp.numOfBitStreams, false);	// results.txt data

	/*
	 * Determine format of data*.txt filenames based on state->partitionCount[test_num]
	 * NOTE: If we are not partitioning the p_values, no data*.txt filenames are needed
	 */
	state->datatxt_fmt[test_num] = data_filename_format(state->partitionCount[test_num]);
	dbg(DBG_HIGH, "%s[%d] will form data*.txt filenames with the following format: %s",
	    state->testNames[test_num], test_num, state->datatxt_fmt[test_num]);

	return;
}


/*
 * Serial_iterate - iterate one bit stream for Serial test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Serial_iterate(struct thread_state *thread_state)
{
	struct Serial_private_stats stat;	// Stats for this iteration
	long int m;		// Serial block length (state->tp.serialBlockLength)
	double p_value1;	// p_value iteration test result(s) - #1
	double p_value2;	// p_value iteration test result(s) - #2

	/*
	 * Check preconditions (firewall)
	 */
	if (thread_state == NULL) {
		err(191, __func__, "thread_state arg is NULL");
	}
	struct state *state = thread_state->global_state;
	if (state == NULL) {
		err(191, __func__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "iterate function[%d] %s called when test vector was false", test_num, __func__);
		return;
	}

	/*
	 * Collect parameters from state
	 */
	m = state->tp.serialBlockLength;

	/*
	 * Perform the test
	 */
	stat.psim0 = compute_psi2(thread_state, m);
	stat.psim1 = compute_psi2(thread_state, m - 1);
	stat.psim2 = compute_psi2(thread_state, m - 2);

	/*
	 * Step 4: compute the test statistics
	 */
	stat.del1 = stat.psim0 - stat.psim1;
	stat.del2 = stat.psim0 - 2.0 * stat.psim1 + stat.psim2;

	/*
	 * Step 5: compute the test P-values
	 */
	p_value1 = cephes_igamc((double) ((long int) 1 << (m - 1)) / 2.0, stat.del1 / 2.0);
	p_value2 = cephes_igamc((double) ((long int) 1 << (m - 2)) / 2.0, stat.del2 / 2.0);

	/*
	 * Record success or failure for this iteration (1st test)
	 */
	state->count[test_num]++;	// Count this iteration
	state->valid[test_num]++;	// Count this valid iteration
	if (isNegative(p_value1)) {
		state->failure[test_num]++;	// Bogus p_value1 < 0.0 treated as a failure
		stat.success1 = false;		// FAILURE
		warn(__func__, "iteration %ld of test %s[%d] produced bogus p_value1: %f < 0.0\n",
		     thread_state->iteration_being_done + 1, state->testNames[test_num], test_num, p_value1);
	} else if (isGreaterThanOne(p_value1)) {
		state->failure[test_num]++;	// Bogus p_value1 > 1.0 treated as a failure
		stat.success1 = false;		// FAILURE
		warn(__func__, "iteration %ld of test %s[%d] produced bogus p_value1: %f > 1.0\n",
		     thread_state->iteration_being_done + 1, state->testNames[test_num], test_num, p_value1);
	} else if (p_value1 < state->tp.alpha) {
		state->valid_p_val[test_num]++;	// Valid p_value1 in [0.0, 1.0] range
		state->failure[test_num]++;	// Valid p_value1 but too low is a failure
		stat.success1 = false;		// FAILURE
	} else {
		state->valid_p_val[test_num]++;	// Valid p_value1 in [0.0, 1.0] range
		state->success[test_num]++;	// Valid p_value1 not too low is a success
		stat.success1 = true;		// SUCCESS
	}

	/*
	 * Lock mutex before making changes to the shared state
	 */
	if (thread_state->mutex != NULL) {
		pthread_mutex_lock(thread_state->mutex);
	}

	/*
	 * Record success or failure for this iteration (2nd test)
	 */
	state->count[test_num]++;	// Count this iteration
	state->valid[test_num]++;	// Count this valid iteration
	if (isNegative(p_value2)) {
		state->failure[test_num]++;	// Bogus p_value2 < 0.0 treated as a failure
		stat.success2 = false;		// FAILURE
		warn(__func__, "iteration %ld of test %s[%d] produced bogus p_value2: %f < 0.0\n",
		     thread_state->iteration_being_done + 1, state->testNames[test_num], test_num, p_value2);
	} else if (isGreaterThanOne(p_value2)) {
		state->failure[test_num]++;	// Bogus p_value2 > 1.0 treated as a failure
		stat.success2 = false;		// FAILURE
		warn(__func__, "iteration %ld of test %s[%d] produced bogus p_value2: %f > 1.0\n",
		     thread_state->iteration_being_done + 1, state->testNames[test_num], test_num, p_value2);
	} else if (p_value2 < state->tp.alpha) {
		state->valid_p_val[test_num]++;	// Valid p_value2 in [0.0, 1.0] range
		state->failure[test_num]++;	// Valid p_value2 but too low is a failure
		stat.success2 = false;		// FAILURE
	} else {
		state->valid_p_val[test_num]++;	// Valid p_value2 in [0.0, 1.0] range
		state->success[test_num]++;	// Valid p_value2 not too low is a success
		stat.success2 = true;		// SUCCESS
	}

	/*
	 * Record values computed during this iteration
	 */
	if (state->resultstxtFlag == true) {
		append_value(state->stats[test_num], &stat);
	}
	append_value(state->p_val[test_num], &p_value1);
	append_value(state->p_val[test_num], &p_value2);

	/*
	 * Unlock mutex after making changes to the shared state
	 */
	if (thread_state->mutex != NULL) {
		pthread_mutex_unlock(thread_state->mutex);
	}

	return;
}

/*
 * compute_psi2 - compute psi-squared for the given block size
 *
 * given:
 *      state           // run state to test under
 *      blocksize	// length of an overlapping sub-sequence
 *
 * This auxiliary function computes the psi-squared values needed for the
 * test statistic of the Serial test.
 */
static double
compute_psi2(struct thread_state *thread_state, long int blocksize)
{
	long int n;		// Length of a single bit stream
	long int powLen;	// Number of possible m-bit sub-sequences
	long int mask;		// Bit-mask used to discard the extra bits of a sequence
	long int dec;		// Decimal representation of an m-bit sub-sequence
	double sum;		// Sum of the squares of all the counters, needed to compute psi-squared
	long int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (thread_state == NULL) {
		err(192, __func__, "thread_state arg is NULL");
	}
	struct state *state = thread_state->global_state;
	if (state == NULL) {
		err(192, __func__, "state arg is NULL");
	}
	if (state->epsilon == NULL) {
		err(192, __func__, "state->epsilon is NULL");
	}
	if (state->epsilon[thread_state->thread_id] == NULL) {
		err(192, __func__, "state->epsilon[%ld] is NULL", thread_state->thread_id);
	}
	if ((blocksize == 0) || (blocksize == -1)) {
		return 0.0;
	}
	if (blocksize > (BITS_N_LONGINT - 1)) {	// firewall
		err(192, __func__, "m is too large, 1 << (m:%ld) can't be longer than %ld bits", blocksize, BITS_N_LONGINT - 1);
	}
	if (state->serial_v == NULL) {
		err(192, __func__, "state->serial_v is NULL");
	}
	if (state->serial_v[thread_state->thread_id] == NULL) {
		err(192, __func__, "state->serial_v[%ld] is NULL", thread_state->thread_id);
	}

	/*
	 * Collect parameters from state
	 */
	n = state->tp.n;

	/*
	 * Compute how many counters are needed, i.e. how many different possible
	 * sub-sequences of the given size can possibly exist
	 */
	powLen = (long int) 1 << blocksize;
	if (powLen > state->serial_v_len) {
		err(192, __func__, "powLen: %ld is too large, "
				"1 << blocksize: %ld > state->serial_v_len: %ld ", powLen, blocksize, state->serial_v_len);
	}

	/*
	 * Zeroize those counters in the array v
	 */
	memset(state->serial_v[thread_state->thread_id], 0, powLen * sizeof(state->serial_v[thread_state->thread_id][0]));

	/*
	 * Compute the mask that will be used by the algorithm
	 */
	mask = (1 << blocksize) - 1;

	/*
	 * Step 2: compute the frequency of all the overlapping sub-sequences
	 *
	 * This algorithm works by taking each consecutive overlapping sub-sequence
	 * of length blocksize from the original sequence epsilon of length n.
	 *
	 * For each sub-sequence found, the decimal representation is computed and
	 * the corresponding counter in the array v is incremented.
	 *
	 * It is convenient to use the decimal representation because we can more easily
	 * store and have access to the counters of each block in the array v with size 2^blocksize.
	 *
	 * NOTE: i % n is used to avoid appending blocksize-1 bits in the end (as indicated in the paper)
	 */
	for (dec = 0, i = 0; i < n + blocksize; i++) {

		/*
		 * Get the decimal representation of the current block.
		 * This line of code works by shifting the decimal representation of the
		 * previous number left by 1 bit, adding to it the following bit of epsilon,
		 * and then discarding the left-most bit by doing an AND with the mask (in fact,
		 * the mask is used to keep only the right-most blocksize bits of the number).
		 */
		dec = ((dec << 1) + (int) state->epsilon[thread_state->thread_id][i % n]) & mask;

		/*
		 * If we have already counted the first (blocksize - 1) bits of epsilon,
		 * count the occurrence of the current block in its corresponding counter.
		 *
		 * NOTE: this check is important because during the first (blocksize - 1) iterations
		 * of this loop, dec will be the decimal representation of blocks of length
		 * which is smaller than blocksize.
		 */
		if (i >= blocksize) {
			state->serial_v[thread_state->thread_id][dec]++;
		}
	}

	/*
	 * Compute the sum of the squares of all the frequencies (needed for step 3)
	 */
	sum = 0.0;
	for (i = 0; i < powLen; i++) {
		sum += (double) state->serial_v[thread_state->thread_id][i] * (double) state->serial_v[thread_state->thread_id][i];
	}

	/*
	 * Step 3: compute psi-squared
	 */
	return (sum * (double) ((long int) 1 << blocksize) / (double) n) - (double) n;
}


/*
 * Serial_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct Serial_private_stats for format and print
 *      p_value1        // p_value iteration test result(s) - #1
 *      p_value2        // p_value iteration test result(s) - #2
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
Serial_print_stat(FILE * stream, struct state *state, struct Serial_private_stats *stat, double p_value1, double p_value2)
{
	long int n;		// Length of a single bit stream
	long int m;		// Serial block length (state->tp.serialBlockLength)
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(193, __func__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(193, __func__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(193, __func__, "stat arg is NULL");
	}
	if (p_value1 == NON_P_VALUE && stat->success1 == true) {
		err(193, __func__, "p_value1 was set to NON_P_VALUE but stat->success1 == true");
	}
	if (p_value2 == NON_P_VALUE && stat->success2 == true) {
		err(193, __func__, "p_value2 was set to NON_P_VALUE but stat->success2 == true");
	}

	/*
	 * Collect parameters from state
	 */
	n = state->tp.n;
	m = state->tp.serialBlockLength;

	/*
	 * Print stat to a file
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t\t       SERIAL TEST\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t---------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t COMPUTATIONAL INFORMATION:		  \n");
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\t\t       Serial test\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t---------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(a) Block length    (m) = %ld\n", m);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(b) Sequence length (n) = %ld\n", n);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(c) Psi_m               = %f\n", stat->psim0);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(d) Psi_m-1             = %f\n", stat->psim1);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(e) Psi_m-2             = %f\n", stat->psim2);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(f) Del_1               = %f\n", stat->del1);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(g) Del_2               = %f\n", stat->del2);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t---------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	if (state->legacy_output == true) {
		if (stat->success1 == true) {
			io_ret = fprintf(stream, "SUCCESS\t\tp_value1 = %f\n", p_value1);
			if (io_ret <= 0) {
				return false;
			}
		} else if (p_value1 == NON_P_VALUE) {
			io_ret = fprintf(stream, "FAILURE\t\tp_value1 = __INVALID__\n");
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret = fprintf(stream, "FAILURE\t\tp_value1 = %f\n", p_value1);
			if (io_ret <= 0) {
				return false;
			}
		}
		if (stat->success2 == true) {
			io_ret = fprintf(stream, "SUCCESS\t\tp_value2 = %f\n\n", p_value2);
			if (io_ret <= 0) {
				return false;
			}
		} else if (p_value2 == NON_P_VALUE) {
			io_ret = fprintf(stream, "FAILURE\t\tp_value2 = __INVALID__\n\n");
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret = fprintf(stream, "FAILURE\t\tp_value2 = %f\n\n", p_value2);
			if (io_ret <= 0) {
				return false;
			}
		}
	} else {
		if (stat->success1 == true) {
			io_ret = fprintf(stream, "SUCCESS\t\tp_value = %f\n", p_value1);
			if (io_ret <= 0) {
				return false;
			}
		} else if (p_value1 == NON_P_VALUE) {
			io_ret = fprintf(stream, "FAILURE\t\tp_value = __INVALID__\n");
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret = fprintf(stream, "FAILURE\t\tp_value = %f\n", p_value1);
			if (io_ret <= 0) {
				return false;
			}
		}
		if (stat->success2 == true) {
			io_ret = fprintf(stream, "SUCCESS\t\tp_value = %f\n\n", p_value2);
			if (io_ret <= 0) {
				return false;
			}
		} else if (p_value2 == NON_P_VALUE) {
			io_ret = fprintf(stream, "FAILURE\t\tp_value = __INVALID__\n\n");
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret = fprintf(stream, "FAILURE\t\tp_value = %f\n\n", p_value2);
			if (io_ret <= 0) {
				return false;
			}
		}
	}

	/*
	 * All printing successful
	 */
	return true;
}


/*
 * Serial_print_p_value - print p_value information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      stat            // struct Serial_private_stats for format and print
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
Serial_print_p_value(FILE * stream, double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(194, __func__, "stream arg is NULL");
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
 * Serial_print - print to results.txt, data*.txt, stats.txt for all iterations
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
Serial_print(struct state *state)
{
	struct Serial_private_stats *stat;	// Pointer to statistics of an iteration
	double p_value;				// Generic p_value iteration
	double p_value1;			// p_value iteration test result(s) - #1
	double p_value2;			// p_value iteration test result(s) - #2
	FILE *stats = NULL;			// Open stats.txt file
	FILE *results = NULL;			// Open results.txt file
	FILE *data = NULL;			// Open data*.txt file
	char *stats_txt = NULL;			// Pathname for stats.txt
	char *results_txt = NULL;		// Pathname for results.txt
	char *data_txt = NULL;			// Pathname for data*.txt
	char data_filename[BUFSIZ + 1];		// Basename for a given data*.txt pathname
	bool ok;				// true -> I/O was OK
	int snprintf_ret;			// snprintf return value
	int io_ret;				// I/O return status
	long int i;
	long int j;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(195, __func__, "state arg is NULL");
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
		err(195, __func__,
		    "print driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(195, __func__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->datatxt_fmt[test_num] == NULL) {
		err(195, __func__, "format for data0*.txt filename is NULL");
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
		stat = addr_value(state->stats[test_num], struct Serial_private_stats, i);

		/*
		 * Get both p_values for this iteration
		 */
		p_value1 = get_value(state->p_val[test_num], double, i * 2);
		p_value2 = get_value(state->p_val[test_num], double, (i * 2) + 1);

		/*
		 * Print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = Serial_print_stat(stats, state, stat, p_value1, p_value2);
		if (ok == false) {
			errp(195, __func__, "error in writing to %s", stats_txt);
		}

		/*
		 * Print 1st p_value to results.txt
		 */
		errno = 0;	// paranoia
		ok = Serial_print_p_value(results, p_value1);
		if (ok == false) {
			errp(195, __func__, "error in writing to %s", results_txt);
		}

		/*
		 * Print 2nd p_value to results.txt
		 */
		errno = 0;	// paranoia
		ok = Serial_print_p_value(results, p_value2);
		if (ok == false) {
			errp(195, __func__, "error in writing to %s", results_txt);
		}
	}

	/*
	 * Flush and close stats.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(stats);
	if (io_ret != 0) {
		errp(195, __func__, "error flushing to: %s", stats_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(stats);
	if (io_ret != 0) {
		errp(195, __func__, "error closing: %s", stats_txt);
	}
	free(stats_txt);
	stats_txt = NULL;

	/*
	 * Flush and close results.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(results);
	if (io_ret != 0) {
		errp(195, __func__, "error flushing to: %s", results_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(results);
	if (io_ret != 0) {
		errp(195, __func__, "error closing: %s", results_txt);
	}
	free(results_txt);
	results_txt = NULL;

	/*
	 * Write data*.txt for each data file if we need to partition results
	 */
	if (state->partitionCount[test_num] > 1) {
		for (j = 0; j < state->partitionCount[test_num]; ++j) {

			/*
			 * Form the data*.txt basename
			 */
			errno = 0;	// paranoia
			snprintf_ret = snprintf(data_filename, BUFSIZ, state->datatxt_fmt[test_num], j + 1);
			data_filename[BUFSIZ] = '\0';	// paranoia
			if (snprintf_ret <= 0 || snprintf_ret >= BUFSIZ || errno != 0) {
				errp(195, __func__,
				     "snprintf failed for %d bytes for data%03ld.txt, returned: %d", BUFSIZ, j + 1, snprintf_ret);
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
					ok = Serial_print_p_value(data, p_value);
					if (ok == false) {
						errp(195, __func__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * Flush and close data*.txt, free pathname
			 */
			errno = 0;	// paranoia
			io_ret = fflush(data);
			if (io_ret != 0) {
				errp(195, __func__, "error flushing to: %s", data_txt);
			}
			errno = 0;	// paranoia
			io_ret = fclose(data);
			if (io_ret != 0) {
				errp(195, __func__, "error closing: %s", data_txt);
			}
			free(data_txt);
			data_txt = NULL;
		}
	}

	return;
}


/*
 * Serial_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state           // run state to test under
 *      sampleCount             // Number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // uniformity frequency bins
 */
static void
Serial_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin, int index)
{
	long int passCount;	// p_values that pass
	double p_hat;		// 1 - alpha
	double proportion_threshold_max;	// When passCount is too high
	double proportion_threshold_min;	// When passCount is too low
	double chi2;		// Sum of chi^2 for each tenth
	double uniformity;	// Uniformity of frequency bins
	double expCount;	// Sample size divided by frequency bin count
	int io_ret;		// I/O return status
	long int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(196, __func__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(196, __func__, "freqPerBin arg is NULL");
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
		errno = 0;                // paranoia
		io_ret = fflush(state->finalRept);
		if (io_ret != 0) {
			errp(196, __func__, "error flushing to: %s", state->finalReptPath);
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
			state->metric_results.serial[index] = FAILED_BOTH;
		} else if (proportion_passed == false) {
			state->metric_results.serial[index] = FAILED_PROPORTION;
		} else if (uniformity_passed == false) {
			state->metric_results.serial[index] = FAILED_UNIFORMITY;
		} else {
			state->metric_results.serial[index] = PASSED_BOTH;
			state->successful_tests++;
		}
	}

	return;
}


/*
 * Serial_metrics - uniformity and proportional analysis of a test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to complete the test analysis for all iterations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
Serial_metrics(struct state *state)
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
		err(197, __func__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(197, __func__,
		    "metrics driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(197, __func__,
		    "metrics driver interface for %s[%d] called with p_val length: %ld != bit streams: %ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}

	/*
	 * Allocate uniformity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(197, __func__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
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
		Serial_metric_print(state, sampleCount, toolow, freqPerBin, j);

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

	return;
}


/*
 * Serial_destroy - post process results for this text
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to cleanup any storage or state
 * associated with this test.
 */
void
Serial_destroy(struct state *state)
{
	long int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(198, __func__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "destroy function[%d] %s called when test vector was false", test_num, __func__);
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
	for (i = 0; i < state->numberOfThreads; i++) {
		if (state->serial_v[i] != NULL) {
			free(state->serial_v[i]);
			state->serial_v[i] = NULL;
		}
	}
	if (state->serial_v != NULL) {
		free(state->serial_v);
		state->serial_v = NULL;
	}

	return;
}
