/*****************************************************************************
			      R A N K	T E S T
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


// Exit codes: 170 thru 179

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "../constants/externs.h"
#include "../utils/cephes.h"
#include "../utils/matrix.h"
#include "../utils/utilities.h"
#include "../utils/debug.h"


/*
 * Private stats - stats.txt information for this test
 */
struct Rank_private_stats {
	bool success;		// Success or failure of iteration test
	double chi_squared;	// Chi squared for rank frequencies
	long int F_M;		// Frequency of rank NUMBER_OF_ROWS_RANK fpr this iteration
	long int F_M_minus_one;	// Frequency of rank NUMBER_OF_ROWS_RANK-1 fpr this iteration
	long int F_remaining;	// Frequency of rank < NUMBER_OF_ROWS_RANK-1 fpr this iteration
};


/*
 * Static const variables declarations
 */
static const enum test test_num = TEST_RANK;	// This test number


/*
 * Static variables declarations
 */
static double p_32;			// Probability of rank NUMBER_OF_ROWS_RANK
static double p_31;			// Probability of rank NUMBER_OF_ROWS_RANK - 1
static double p_30;			// Probability of rank < NUMBER_OF_ROWS_RANK - 1
static long int matrix_count;		// Total possible matrix for a given bit stream length


/*
 * Forward static function declarations
 */
static bool Rank_print_stat(FILE * stream, struct state *state, struct Rank_private_stats *stat, double p_value);
static bool Rank_print_p_value(FILE * stream, double p_value);
static void Rank_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin);


/*
 * Rank_init - initialize the Rank test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Rank_init(struct state *state)
{
	double product;			// Probability product, used when computing values of static variables
	int r;				// Row count to consider, used when computing values of static variables
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(170, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->cSetup != true) {
		err(170, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_NULL && state->driver_state[test_num] != DRIVER_DESTROY) {
		err(170, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_NULL: %d and != DRIVER_DESTROY: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_NULL, DRIVER_DESTROY);
	}
	if (((long int) NUMBER_OF_ROWS_RANK * (long int) NUMBER_OF_COLS_RANK) > (long int) LONG_MAX) {	// paranoia
		err(170, __FUNCTION__, "NUMBER_OF_ROWS_RANK: %d * NUMBER_OF_COLS_RANK: %d cannot fit into a long int because"
				"the product is > %ld", NUMBER_OF_ROWS_RANK, NUMBER_OF_COLS_RANK, LONG_MAX);
	}

	/*
	 * Collect parameters from state
	 */
	matrix_count = state->tp.n / (NUMBER_OF_ROWS_RANK * NUMBER_OF_COLS_RANK);;

	/*
	 * Disable test if conditions do not permit this test from being run
	 */
	if (matrix_count < MIN_NUMBER_OF_MATRICES_RANK) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires number of matrixes(matrix_count): %ld >= %d",
		     state->testNames[test_num], test_num, matrix_count, MIN_NUMBER_OF_MATRICES_RANK);
		state->testVector[test_num] = false;
		return;
	}

	/*
	 * Compute probability of rank NUMBER_OF_ROWS_RANK
	 */
	r = NUMBER_OF_ROWS_RANK;
	product = 1.0;
	for (i = 0; i <= r - 1; i++) {
		product *= ((1.0 - pow(2.0, i - NUMBER_OF_ROWS_RANK))
			    * (1.0 - pow(2.0, i - NUMBER_OF_COLS_RANK))) / (1.0 - pow(2.0, i - r));
	}
	p_32 = pow(2.0, r * (NUMBER_OF_ROWS_RANK + NUMBER_OF_COLS_RANK - r)
			- NUMBER_OF_ROWS_RANK * NUMBER_OF_COLS_RANK) * product;
	if (p_32 <= 0.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_32 value: %f should be > 0.0", p_32);
	}
	if (p_32 >= 1.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_32 value: %f should be < 1.0", p_32);
	}

	/*
	 * Compute probability of rank NUMBER_OF_ROWS_RANK - 1
	 */
	r = NUMBER_OF_ROWS_RANK - 1;
	product = 1.0;
	for (i = 0; i <= r - 1; i++) {
		product *= ((1.0 - pow(2.0, i - NUMBER_OF_ROWS_RANK))
			    * (1.0 - pow(2.0, i - NUMBER_OF_COLS_RANK))) / (1.0 - pow(2.0, i - r));
	}
	p_31 = pow(2.0, r * (NUMBER_OF_ROWS_RANK + NUMBER_OF_COLS_RANK - r)
			- NUMBER_OF_ROWS_RANK * NUMBER_OF_COLS_RANK) * product;
	if (p_31 <= 0.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_31 value: %f should be > 0.0", p_31);
	}
	if (p_31 >= 1.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_31 value: %f should be < 1.0", p_31);
	}

	/*
	 * Compute probability of rank < NUMBER_OF_ROWS_RANK - 1
	 */
	p_30 = 1.0 - (p_32 + p_31);
	if (p_30 <= 0.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_30 value: %f == (1.0 - p32: %f - p_31: %f) should be > 0.0",
		    p_30, p_31, p_32);
	}
	if (p_30 >= 1.0) {	// paranoia
		err(50, __FUNCTION__, "bogus p_30 value: %f == (1.0 - p32: %f - p_31: %f) should be < 1.0",
		    p_30, p_31, p_32);
	}

	/*
	 * Allocate the special Rank test matrix
	 */
	state->rank_matrix = create_matrix(NUMBER_OF_ROWS_RANK, NUMBER_OF_COLS_RANK);

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
	state->stats[test_num] = create_dyn_array(sizeof(struct Rank_private_stats),
						  DEFAULT_CHUNK, state->tp.numOfBitStreams, false);	// stats.txt
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
 * Rank_iterate - iterate one bit stream for Rank test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Rank_iterate(struct state *state)
{
	struct Rank_private_stats stat;	// Stats for this iteration
	BitSequence **matrix;		// The matrix state->rank_matrix
	BitSequence *row;		// A row of the matrix state->rank_matrix
	int R;				// Rank of a given NUMBER_OF_ROWS_RANK by NUMBER_OF_COLS_RANK matrix
	double p_value;			// p_value iteration test result(s)
	long int k;
	long int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(171, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "iterate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	if (state->epsilon == NULL) {
		err(171, __FUNCTION__, "state->epsilon is NULL");
	}
	if (state->rank_matrix == NULL) {
		err(171, __FUNCTION__, "state->rank_matrix is NULL");
	}
	if (state->cSetup != true) {
		err(171, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_INIT && state->driver_state[test_num] != DRIVER_ITERATE) {
		err(171, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_INIT: %d and != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_INIT, DRIVER_ITERATE);
	}

	/*
	 * Zeroize the Rank test matrix
	 */
	for (i = 0; i < NUMBER_OF_ROWS_RANK; ++i) {

		/*
		 * Find the row
		 */
		row = state->rank_matrix[i];
		if (row == NULL) {	// paranoia
			err(171, __FUNCTION__, "row pointer %ld of rank_matrix is NULL", i);
		}

		/*
		 * Zeroize the full row
		 */
		memset(row, 0, NUMBER_OF_COLS_RANK * sizeof(row[0]));
	}

	/*
	 * Setup test parameters
	 */
	matrix = state->rank_matrix;
	stat.F_M = 0;
	stat.F_M_minus_one = 0;

	/*
	 * Step 1a: divide the sequence into disjoint blocks of NUMBER_OF_ROWS_RANK * NUMBER_OF_COLS_RANK bits
	 */
	for (k = 0; k < matrix_count; k++) {

		/*
	 	 * Step 1b: copy bits of each block into a NUMBER_OF_ROWS_RANK * NUMBER_OF_COLS_RANK matrix
	 	 */
		def_matrix(state, NUMBER_OF_ROWS_RANK, NUMBER_OF_COLS_RANK, matrix, k);

		/*
	 	 * Step 2: determine the binary rank of each matrix
	 	 */
		R = computeRank(NUMBER_OF_ROWS_RANK, NUMBER_OF_COLS_RANK, matrix);

		/*
		 * Step 3a: count the number of matrices with rank = (full rank) and rank = (full rank - 1)
		 */
		if (R == NUMBER_OF_ROWS_RANK) {
			stat.F_M++;	// rank NUMBER_OF_ROWS_RANK found
		} else if (R == (NUMBER_OF_ROWS_RANK - 1)) {
			stat.F_M_minus_one++;	// rank NUMBER_OF_ROWS_RANK-1 found
		}
	}

	/*
	 * Step 3b: count the number of matrices with rank less than (full rank - 1)
	 */
	stat.F_remaining = matrix_count - (stat.F_M + stat.F_M_minus_one);

	/*
	 * Step 4: compute the test statistic
	 */
	stat.chi_squared = (((stat.F_M - matrix_count * p_32) *
			     (stat.F_M - matrix_count * p_32) /
			     (matrix_count * p_32)) +
			    ((stat.F_M_minus_one - matrix_count * p_31) *
			     (stat.F_M_minus_one - matrix_count * p_31) /
			     (matrix_count * p_31)) +
			    ((stat.F_remaining - matrix_count * p_30) *
			     (stat.F_remaining - matrix_count * p_30) /
			     (matrix_count * p_30)));

	/*
	 * Step 5: compute the test P-value
	 */
	p_value = exp(-stat.chi_squared / 2.0);

	/*
	 * Record success or failure for this iteration
	 */
	state->count[test_num]++;	// Count this iteration
	state->valid[test_num]++;	// Count this valid iteration
	if (isNegative(p_value)) {
		state->failure[test_num]++;	// Bogus p_value < 0.0 treated as a failure
		stat.success = false;		// FAILURE
		warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value: %f < 0.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value);
	} else if (isGreaterThanOne(p_value)) {
		state->failure[test_num]++;	// Bogus p_value > 1.0 treated as a failure
		stat.success = false;		// FAILURE
		warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value: %f > 1.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value);
	} else if (p_value < state->tp.alpha) {
		state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
		state->failure[test_num]++;	// Valid p_value but too low is a failure
		stat.success = false;		// FAILURE
	} else {
		state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
		state->success[test_num]++;	// Valid p_value not too low is a success
		stat.success = true;		// SUCCESS
	}

	/*
	 * Record values computed during this iteration
	 */
	append_value(state->stats[test_num], &stat);
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
 * Rank_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct Rank_private_stats for format and print
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
Rank_print_stat(FILE * stream, struct state *state, struct Rank_private_stats *stat, double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(172, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(172, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(172, __FUNCTION__, "stat arg is NULL");
	}
	if (state->cSetup != true) {
		err(172, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (p_value == NON_P_VALUE && stat->success == true) {
		err(172, __FUNCTION__, "p_value was set to NON_P_VALUE but stat->success == true");
	}

	/*
	 * Print stat to a file
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t\t\tRANK TEST\n");
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\t\t\tRank test\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t---------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\tCOMPUTATIONAL INFORMATION:\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t---------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t(a) Probability P_%d = %f\n", NUMBER_OF_ROWS_RANK, p_32);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(b)             P_%d = %f\n", NUMBER_OF_ROWS_RANK - 1, p_31);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(c)             P_%d = %f\n", NUMBER_OF_ROWS_RANK - 2, p_30);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(d) Frequency   F_%d = %ld\n", NUMBER_OF_ROWS_RANK, stat->F_M);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(e)             F_%d = %ld\n", NUMBER_OF_ROWS_RANK - 1, stat->F_M_minus_one);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(f)             F_%d = %ld\n", NUMBER_OF_ROWS_RANK - 2, stat->F_remaining);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(g) # of matrices    = %ld\n", matrix_count);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(h) Chi^2            = %f\n", stat->chi_squared);
	if (io_ret <= 0) {
		return false;
	}
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t(i) NOTE: %ld BITS WERE DISCARDED.\n",
				 state->tp.n % (NUMBER_OF_ROWS_RANK * NUMBER_OF_COLS_RANK));
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\t(i) %ld bits were discarded\n",
				 state->tp.n % (NUMBER_OF_ROWS_RANK * NUMBER_OF_COLS_RANK));
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
 * Rank_print_p_value - print p_value information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      stat            // struct Rank_private_stats for format and print
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
Rank_print_p_value(FILE * stream, double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(173, __FUNCTION__, "stream arg is NULL");
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
 * Rank_print - print to results.txt, data*.txt, stats.txt for all iterations
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
Rank_print(struct state *state)
{
	struct Rank_private_stats *stat;	// Pointer to statistics of an iteration
	double p_value;				// p_value iteration test result(s)
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
		err(174, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "print driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->resultstxtFlag == false) {
		dbg(DBG_LOW, "print driver interface for %s[%d] disabled due to -n", state->testNames[test_num], test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(174, __FUNCTION__,
		    "print driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(174, __FUNCTION__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->datatxt_fmt[test_num] == NULL) {
		err(174, __FUNCTION__, "format for data0*.txt filename is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_ITERATE) {
		err(174, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_ITERATE: %d",
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
		stat = addr_value(state->stats[test_num], struct Rank_private_stats, i);

		/*
		 * Get p_value for this iteration
		 */
		p_value = get_value(state->p_val[test_num], double, i);

		/*
		 * Print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = Rank_print_stat(stats, state, stat, p_value);
		if (ok == false) {
			errp(174, __FUNCTION__, "error in writing to %s", stats_txt);
		}

		/*
		 * Print p_value to results.txt
		 */
		errno = 0;	// paranoia
		ok = Rank_print_p_value(results, p_value);
		if (ok == false) {
			errp(174, __FUNCTION__, "error in writing to %s", results_txt);
		}
	}

	/*
	 * Flush and close stats.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(stats);
	if (io_ret != 0) {
		errp(174, __FUNCTION__, "error flushing to: %s", stats_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(stats);
	if (io_ret != 0) {
		errp(174, __FUNCTION__, "error closing: %s", stats_txt);
	}
	free(stats_txt);
	stats_txt = NULL;

	/*
	 * Flush and close results.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(results);
	if (io_ret != 0) {
		errp(174, __FUNCTION__, "error flushing to: %s", results_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(results);
	if (io_ret != 0) {
		errp(174, __FUNCTION__, "error closing: %s", results_txt);
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
				errp(174, __FUNCTION__,
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
					ok = Rank_print_p_value(data, p_value);
					if (ok == false) {
						errp(174, __FUNCTION__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * Flush and close data*.txt, free pathname
			 */
			errno = 0;	// paranoia
			io_ret = fflush(data);
			if (io_ret != 0) {
				errp(174, __FUNCTION__, "error flushing to: %s", data_txt);
			}
			errno = 0;	// paranoia
			io_ret = fclose(data);
			if (io_ret != 0) {
				errp(174, __FUNCTION__, "error closing: %s", data_txt);
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
 * Rank_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state           	// run state to test under
 *      sampleCount             // number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // uniformity frequency bins
 */
static void
Rank_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
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
		err(175, __FUNCTION__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(175, __FUNCTION__, "freqPerBin arg is NULL");
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
	 * Check uniformity failure
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
	 * Output uniformity results in traditional format to finalAnalysisReport.txt
	 */
	for (i = 0; i < state->tp.uniformity_bins; ++i) {
		fprintf(state->finalRept, "%3ld ", freqPerBin[i]);
	}
	if (expCount <= 0.0) {
		// Not enough samples for uniformity check
		fprintf(state->finalRept, "    ----    ");
		state->uniformity_failure[test_num] = false;
		dbg(DBG_HIGH, "too few iterations for uniformity check on %s", state->testNames[test_num]);
	} else if (uniformity < state->tp.uniformity_level) { // check if it's smaller than the uniformity_level (default 0.0001)
		// Uniformity failure
		fprintf(state->finalRept, " %8.6f * ", uniformity);
		state->uniformity_failure[test_num] = true;
		dbg(DBG_HIGH, "metrics detected uniformity failure for %s", state->testNames[test_num]);
	} else {
		// Uniformity success
		fprintf(state->finalRept, " %8.6f   ", uniformity);
		state->uniformity_failure[test_num] = false;
		dbg(DBG_HIGH, "metrics detected uniformity success for %s", state->testNames[test_num]);
	}

	/*
	 * Output proportional results in traditional format to finalAnalysisReport.txt
	 */
	if (sampleCount == 0) {
		// Not enough samples for proportional check
		fprintf(state->finalRept, " ------     %s\n", state->testNames[test_num]);
		state->proportional_failure[test_num] = false;
		dbg(DBG_HIGH, "too few samples for proportional check on %s", state->testNames[test_num]);
	} else if ((passCount < proportion_threshold_min) || (passCount > proportion_threshold_max)) {
		// Proportional failure
		state->proportional_failure[test_num] = true;
		fprintf(state->finalRept, "%4ld/%-4ld *	 %s\n", passCount, sampleCount, state->testNames[test_num]);
		dbg(DBG_HIGH, "metrics detected proportional failure for %s", state->testNames[test_num]);
	} else {
		// Proportional success
		state->proportional_failure[test_num] = false;
		fprintf(state->finalRept, "%4ld/%-4ld	 %s\n", passCount, sampleCount, state->testNames[test_num]);
		dbg(DBG_HIGH, "metrics detected proportional success for %s", state->testNames[test_num]);
	}
	errno = 0;		// paranoia
	io_ret = fflush(state->finalRept);
	if (io_ret != 0) {
		errp(175, __FUNCTION__, "error flushing to: %s", state->finalReptPath);
	}

	return;
}


/*
 * Rank_metrics - uniformity and proportional analysis of a test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to complete the test analysis for all iterations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
Rank_metrics(struct state *state)
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
		err(176, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(176, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(176, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with p_val length: %ld != bit streams: %ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->driver_state[test_num] != DRIVER_PRINT) {
		err(176, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_PRINT: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_PRINT);
	}

	/*
	 * Allocate uniformity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(176, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
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
		Rank_metric_print(state, sampleCount, toolow, freqPerBin);

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
 * Rank_destroy - post process results for this text
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to cleanup any storage or state
 * associated with this test.
 */
void
Rank_destroy(struct state *state)
{
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(177, __FUNCTION__, "state arg is NULL");
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
	// Free the Rank test matrix
	if (state->rank_matrix != NULL) {

		// Free all rows of the Rank test matrix
		for (i = 0; i < NUMBER_OF_ROWS_RANK; i++) {
			if (state->rank_matrix[i] != NULL) {
				free(state->rank_matrix[i]);
				state->rank_matrix[i] = NULL;
			}
		}

		// Free the rows array of the Rank test matrix
		free(state->rank_matrix);
		state->rank_matrix = NULL;
	}

	/*
	 * Set driver state to DRIVER_DESTROY
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_DESTROY);
	state->driver_state[test_num] = DRIVER_DESTROY;

	return;
}
