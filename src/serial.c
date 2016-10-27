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


// Exit codes: 190 thru 199

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "externs.h"
#include "cephes.h"
#include "utilities.h"
#include "debug.h"


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
static double psi2(struct state *state, long int m, long int n);
static bool Serial_print_stat(FILE * stream, struct state *state, struct Serial_private_stats *stat, double p_value1,
			      double p_value2);
static bool Serial_print_p_value(FILE * stream, double p_value);
static void Serial_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin);


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

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(190, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->cSetup != true) {
		err(190, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_NULL && state->driver_state[test_num] != DRIVER_DESTROY) {
		err(190, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_NULL: %d and != DRIVER_DESTROY: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_NULL, DRIVER_DESTROY);
	}
	// TODO Choose m and n such that m < floor(log2 n) -2.

	/*
	 * Allocate frequency count P array
	 *
	 * The P array, an array of long ints, is allocated with length:
	 *
	 *      powLen = ((long int) 1 << (m + 1)) - 1;
	 *
	 * Where m is state->tp.serialBlockLength
	 *
	 * We ignore the final "- 1" for safety.   // TODO why? what does "for safety" mean?
	 */
	m = state->tp.serialBlockLength;
	if ((m + 1) > (BITS_N_LONGINT - 1 - 3)) {	// firewall, -3 is for 8 byte long int
		err(190, __FUNCTION__, "(m+1): %ld is too large, 1 << (m:%ld + 2) > %ld bits long", m + 1, m,
		    BITS_N_LONGINT - 1 - 3);
	}
	state->serial_p_len = (long int) 1 << (m + 1);
	state->serial_P = malloc(state->serial_p_len * sizeof(state->serial_P[0]));
	if (state->serial_P == NULL) {
		errp(190, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->serial_P",
		     state->apen_p_len, sizeof(long int));
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
	state->stats[test_num] = create_dyn_array(sizeof(struct Serial_private_stats),
	                                          DEFAULT_CHUNK, state->tp.numOfBitStreams, false); // stats.txt data
	state->p_val[test_num] = create_dyn_array(sizeof(double),
	                                          DEFAULT_CHUNK, 2 * state->tp.numOfBitStreams, false); // results.txt data // TODO check why size is doubled

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
Serial_iterate(struct state *state)
{
	struct Serial_private_stats stat;	// Stats for this iteration
	long int n;		// Length of a single bit stream
	long int m;		// Serial block length (state->tp.serialBlockLength)
	double p_value1;	// p_value iteration test result(s) - #1
	double p_value2;	// p_value iteration test result(s) - #2

	// firewall
	if (state == NULL) {
		err(191, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "iterate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	if (state->driver_state[test_num] != DRIVER_INIT && state->driver_state[test_num] != DRIVER_ITERATE) {
		err(191, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_INIT: %d and != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_INIT, DRIVER_ITERATE);
	}

	/*
	 * Collect parameters from state
	 */
	n = state->tp.n;
	m = state->tp.serialBlockLength;
	if ((state->tp.serialBlockLength - 1) > (BITS_N_LONGINT - 1)) {	// firewall
		err(191, __FUNCTION__, "(state->tp.serialBlockLength - 1): %ld is too large, "
		    "1 << (state->tp.serialBlockLength:%ld - 1) > %ld bits long",
		    (state->tp.serialBlockLength - 1), state->tp.serialBlockLength, BITS_N_LONGINT - 1);
	}

	/*
	 * Perform the test
	 */
	stat.psim0 = psi2(state, m, n);
	stat.psim1 = psi2(state, m - 1, n);
	stat.psim2 = psi2(state, m - 2, n);
	stat.del1 = stat.psim0 - stat.psim1;
	stat.del2 = stat.psim0 - 2.0 * stat.psim1 + stat.psim2;
	/*
	 * NOTE: Mathematical expression code rewrite, old code commented out below:
	 *
	 * p_value1 = cephes_igamc(pow(2.0, m - 1) / 2.0, del1 / 2.0);
	 * p_value2 = cephes_igamc(pow(2.0, m - 2) / 2.0, del2 / 2.0);
	 */
	p_value1 = cephes_igamc((double) ((long int) 1 << (m - 1)) / 2.0, stat.del1 / 2.0);
	p_value2 = cephes_igamc((double) ((long int) 1 << (m - 2)) / 2.0, stat.del2 / 2.0);

	/*
	 * Record testable test success or failure for 1st p_value
	 */
	state->valid[test_num]++;	// Count this valid test
	state->count[test_num]++;	// Count a testable test
	if (isNegative(p_value1)) {
		state->failure[test_num]++;	// Bogus p_value1 < 0.0 treated as a failure
		stat.success1 = false;	// FAILURE
		warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value1: %f < 0.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value1);
	} else if (isGreaterThanOne(p_value1)) {
		state->failure[test_num]++;	// Bogus p_value1 > 1.0 treated as a failure
		stat.success1 = false;	// FAILURE
		warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value1: %f > 1.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value1);
	} else if (p_value1 < state->tp.alpha) {
		state->valid_p_val[test_num]++;	// Valid p_value1 in [0.0, 1.0] range
		state->failure[test_num]++;	// Valid p_value1 but too low is a failure
		stat.success1 = false;	// FAILURE
	} else {
		state->valid_p_val[test_num]++;	// Valid p_value1 in [0.0, 1.0] range
		state->success[test_num]++;	// Valid p_value1 not too low is a success
		stat.success1 = true;	// SUCCESS
	}

	/*
	 * Record testable test success or failure for 2nd p_value
	 */
	state->count[test_num]++;	// Count this test
	state->valid[test_num]++;	// Count this valid test
	if (isNegative(p_value2)) {
		state->failure[test_num]++;	// Bogus p_value2 < 0.0 treated as a failure
		stat.success2 = false;	// FAILURE
		warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value2: %f < 0.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value2);
	} else if (isGreaterThanOne(p_value2)) {
		state->failure[test_num]++;	// Bogus p_value2 > 1.0 treated as a failure
		stat.success2 = false;	// FAILURE
		warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value2: %f > 1.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value2);
	} else if (p_value2 < state->tp.alpha) {
		state->valid_p_val[test_num]++;	// Valid p_value2 in [0.0, 1.0] range
		state->failure[test_num]++;	// Valid p_value2 but too low is a failure
		stat.success2 = false;	// FAILURE
	} else {
		state->valid_p_val[test_num]++;	// Valid p_value2 in [0.0, 1.0] range
		state->success[test_num]++;	// Valid p_value2 not too low is a success
		stat.success2 = true;	// SUCCESS
	}

	/*
	 * Record values computed during this iteration
	 */
	append_value(state->stats[test_num], &stat);
	append_value(state->p_val[test_num], &p_value1);
	append_value(state->p_val[test_num], &p_value2);

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


static double
psi2(struct state *state, long int m, long int n)
{
	long int powLen;
	long int i;
	long int j;
	long int k;
	double sum;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(192, __FUNCTION__, "state arg is NULL");
	}
	if (state->epsilon == NULL) {
		err(192, __FUNCTION__, "state->epsilon is NULL");
	}
	if ((m == 0) || (m == -1)) {
		return 0.0;
	}
	if ((m + 1) > (BITS_N_LONGINT - 1)) {	// firewall
		err(192, __FUNCTION__, "m+1: %ld is too large, 1 << (m:%ld + 1) > %ld bits long", m + 1, m, BITS_N_LONGINT - 1);
	}
	if (state->serial_P == NULL) {
		err(192, __FUNCTION__, "state->serial_P is NULL");
	}
	/*
	 * NOTE: Mathematical expression code rewrite, old code commented out below:
	 *
	 * powLen = (int) pow(2, m + 1) - 1;
	 */
	powLen = ((long int) 1 << (m + 1)) - 1;	// TODO linked to the previous why
	if (powLen > state->serial_p_len) {
		err(192, __FUNCTION__, "powLen: %ld is too large, "
		    "1 << (m:%ld + 1) - 1 > state->serial_p_len: %ld ", powLen, m, state->serial_p_len);
	}

	/*
	 * Zeroize nodes
	 */
	memset(state->serial_P, 0, powLen * sizeof(state->serial_P[0]));

	/*
	 * Compute frequency
	 *
	 * % n is used to avoid appending m-1 bits in the end
	 */
	// TODO Change the algorithm to improve both time and space complexity (see whiteboard)
	for (i = 0; i < n; i++) {	/* COMPUTE FREQUENCY */
		k = 1;
		for (j = 0; j < m; j++) {
			if (state->epsilon[(i + j) % n] == 0) {
				k = 2 * k;
			} else if (state->epsilon[(i + j) % n] == 1) {
				k = 2 * k + 1;
			}
		}

		/*
		 * Check preconditions (firewall)
		 */
		if (k <= 0) {
			err(192, __FUNCTION__, "k: %ld <= 0", k);
		} else if (k > powLen) {
			err(192, __FUNCTION__, "k: %ld > powLen: %ld", k, powLen);
		}
		state->serial_P[k - 1]++;
	}
	sum = 0.0;

	/*
	 * NOTE: Mathematical expression code rewrite, old code commented out below:
	 *
	 * for (i = (int) pow(2, m) - 1; i < (int) pow(2, m + 1) - 1; i++) {
	 * sum += pow(state->serial_P[i], 2);
	 * }
	 * sum = (sum * pow(2, m) / (double) n) - (double) n;
	 */
	for (i = ((long int) 1 << m) - 1; i < ((long int) 1 << (m + 1)) - 1; i++) {
		sum += (double) state->serial_P[i] * (double) state->serial_P[i];
	}

	/*
	 * return frequency
	 */
	return (sum * (double) ((long int) 1 << m) / (double) n) - (double) n;
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
		err(193, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(193, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(193, __FUNCTION__, "stat arg is NULL");
	}
	if (p_value1 == NON_P_VALUE && stat->success1 == true) {
		err(193, __FUNCTION__, "p_value1 was set to NON_P_VALUE but stat->success1 == true");
	}
	if (p_value2 == NON_P_VALUE && stat->success2 == true) {
		err(193, __FUNCTION__, "p_value2 was set to NON_P_VALUE but stat->success2 == true");
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
	io_ret = fprintf(stream, "\t\t(c) Psi_m		  = %f\n", stat->psim0);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(d) Psi_m-1		  = %f\n", stat->psim1);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(e) Psi_m-2		  = %f\n", stat->psim2);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(f) Del_1		  = %f\n", stat->del1);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(g) Del_2		  = %f\n", stat->del2);
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
		err(194, __FUNCTION__, "stream arg is NULL");
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
	double p_value;		// generic p_value iteration
	double p_value1;	// p_value iteration test result(s) - #1
	double p_value2;	// p_value iteration test result(s) - #2
	FILE *stats = NULL;	// Open stats.txt file
	FILE *results = NULL;	// Open results.txt file
	FILE *data = NULL;	// Open data*.txt file
	char *stats_txt = NULL;	// Pathname for stats.txt
	char *results_txt = NULL;	// Pathname for results.txt
	char *data_txt = NULL;	// Pathname for data*.txt
	char data_filename[BUFSIZ + 1];	// Basename for a given data*.txt pathname
	bool ok;		// true -> I/O was OK
	int snprintf_ret;	// snprintf return value
	int io_ret;		// I/O return status
	long int i;
	long int j;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(195, __FUNCTION__, "state arg is NULL");
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
		err(195, __FUNCTION__,
		    "print driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(195, __FUNCTION__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->datatxt_fmt[test_num] == NULL) {
		err(195, __FUNCTION__, "format for data0*.txt filename is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_ITERATE) {
		err(195, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_ITERATE: %d",
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
			errp(195, __FUNCTION__, "error in writing to %s", stats_txt);
		}

		/*
		 * Print 1st p_value to results.txt
		 */
		errno = 0;	// paranoia
		ok = Serial_print_p_value(results, p_value1);
		if (ok == false) {
			errp(195, __FUNCTION__, "error in writing to %s", results_txt);
		}

		/*
		 * Print 2nd p_value to results.txt
		 */
		errno = 0;	// paranoia
		ok = Serial_print_p_value(results, p_value2);
		if (ok == false) {
			errp(195, __FUNCTION__, "error in writing to %s", results_txt);
		}
	}

	/*
	 * Flush and close stats.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(stats);
	if (io_ret != 0) {
		errp(195, __FUNCTION__, "error flushing to: %s", stats_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(stats);
	if (io_ret != 0) {
		errp(195, __FUNCTION__, "error closing: %s", stats_txt);
	}
	free(stats_txt);
	stats_txt = NULL;

	/*
	 * Flush and close results.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(results);
	if (io_ret != 0) {
		errp(195, __FUNCTION__, "error flushing to: %s", results_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(results);
	if (io_ret != 0) {
		errp(195, __FUNCTION__, "error closing: %s", results_txt);
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
				errp(195, __FUNCTION__,
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
						errp(195, __FUNCTION__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * Flush and close data*.txt, free pathname
			 */
			errno = 0;	// paranoia
			io_ret = fflush(data);
			if (io_ret != 0) {
				errp(195, __FUNCTION__, "error flushing to: %s", data_txt);
			}
			errno = 0;	// paranoia
			io_ret = fclose(data);
			if (io_ret != 0) {
				errp(195, __FUNCTION__, "error closing: %s", data_txt);
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
 * Serial_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state           // run state to test under
 *      sampleCount             // Number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // Uniformity frequency bins
 */
static void
Serial_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
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
		err(196, __FUNCTION__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(196, __FUNCTION__, "freqPerBin arg is NULL");
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
			/*
			 * NOTE: Mathematical expression code rewrite, old code commented out below:
			 *
			 * chi2 += pow(freqPerBin[j]-expCount, 2)/expCount;
			 */
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
	} else if (uniformity < state->tp.uniformity_level) {
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
		errp(196, __FUNCTION__, "error flushing to: %s", state->finalReptPath);
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
		err(197, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(197, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(197, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with p_val length: %ld != bit streams: %ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->driver_state[test_num] != DRIVER_PRINT) {
		err(197, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_PRINT: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_PRINT);
	}

	/*
	 * Allocate uniformity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(197, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
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
		/*
		 * NOTE: Logical code rewrite, old code commented out below:
		 *
		 * for (i = 0; i < state->tp.uniformity_bins; ++i) {
		 *      freqPerBin[i] = 0;
		 * }
		 */
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
		Serial_metric_print(state, sampleCount, toolow, freqPerBin);

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
	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(198, __FUNCTION__, "state arg is NULL");
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
	if (state->serial_P != NULL) {
		free(state->serial_P);
		state->serial_P = NULL;
	}

	/*
	 * Set driver state to DRIVER_DESTROY
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_DESTROY);
	state->driver_state[test_num] = DRIVER_DESTROY;

	return;
}
