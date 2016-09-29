/*****************************************************************************
		    C U M U L A T I V E   S U M S   T E S T
 *****************************************************************************/

/*
 * This code has been heavily modified by Landon Curt Noll (chongo at cisco dot com) and Tom Gilgan (thgilgan at cisco dot com).
 * See the initial comment in assess.c and the file README.txt for more information.
 *
 * TOM GILGAN AND LANDON CURT NOLL DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL TOM GILGAN NOR LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * chongo (Landon Curt Noll, http://www.isthe.com/chongo/index.html) /\oo/\
 *
 * Share and enjoy! :-)
 */

// Exit codes: 30 thru 39

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "externs.h"
#include "cephes.h"
#include "utilities.h"
#include "debug.h"


/*
 * private_stats - stats.txt information for this test
 */
struct CumulativeSums_private_stats {
	bool success;		// success or failure forward of interation test
	bool rev_success;	// success or failure reverse of interation test
	long int z;		// maximum forward partial sum
	long int zrev;		// maximum reverse partial sum
};


/*
 * static constant variable declarations
 */
static const enum test test_num = TEST_CUSUM;	// this test number


/*
 * forward static function declarations
 */
static bool CumulativeSums_print_stat(FILE * stream, struct state *state, struct CumulativeSums_private_stats *stat,
				      double p_value, double rev_p_value);
static bool CumulativeSums_print_p_value(FILE * stream, double p_value);
static void CumulativeSums_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin);


/*
 * CumulativeSums_init - initalize the Cumluative Sums test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
CumulativeSums_init(struct state *state)
{
	/*
	 * firewall
	 */
	if (state == NULL) {
		err(30, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false",
		    state->testNames[test_num], test_num);
		return;
	}
	if (state->cSetup != true) {
		err(30, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_NULL && state->driver_state[test_num] != DRIVER_DESTROY) {
		err(30, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_NULL: %d and != DRIVER_DESTROY: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_NULL, DRIVER_DESTROY);
	}

	/*
	 * create working sub-directory if forming files such as results.txt and stats.txt
	 */
	if (state->resultstxtFlag == true) {
		state->subDir[test_num] = precheckSubdir(state, state->testNames[test_num]);
		dbg(DBG_HIGH, "test %s[%d] will use subdir: %s", state->testNames[test_num], test_num, state->subDir[test_num]);
	}

	/*
	 * allocate dynamic arrays
	 */
	// stats.txt data
	state->stats[test_num] =
	    create_dyn_array(sizeof(struct CumulativeSums_private_stats), DEFAULT_CHUNK, state->tp.numOfBitStreams, false);

	// results.txt data
	state->p_val[test_num] = create_dyn_array(sizeof(double), DEFAULT_CHUNK, 2 * (state->tp.numOfBitStreams), false);

	/*
	 * determine format of data*.txt filenames based on state->partitionCount[test_num]
	 *
	 * NOTE: If we are not partitioning the p_values, no data*.txt filenames are needed
	 */
	if (state->partitionCount[test_num] > 1) {
		state->datatxt_fmt[test_num] = data_filename_format(state->partitionCount[test_num]);
		dbg(DBG_HIGH, "%s[%d] will form data*.txt filenames with the following format: %s",
		    state->testNames[test_num], test_num, state->datatxt_fmt[test_num]);
	} else {
		state->datatxt_fmt[test_num] = NULL;
		dbg(DBG_HIGH, "%s[%d] not paritioning, no data*.txt filename format", state->testNames[test_num], test_num);
	}

	/*
	 * driver initialized - set driver state to DRIVER_INIT
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_INIT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_INIT);
	state->driver_state[test_num] = DRIVER_INIT;
	return;
}


/*
 * CumulativeSums_iterate - interate one bit stream for Cumluative Sums test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
CumulativeSums_iterate(struct state *state)
{
	struct CumulativeSums_private_stats stat;	// stats for this interation
	long int n;		// Length of a single bit stream
	long int S;
	long int sup;
	long int inf;
	long int k;
	double sum1;
	double sum2;
	double p_value;		// p_value interation test result(s)
	double rev_p_value;	// reverse test p_value

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(31, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "interate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	if (state->epsilon == NULL) {
		err(31, __FUNCTION__, "state->epsilon is NULL");
	}
	if (state->cSetup != true) {
		err(31, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_INIT && state->driver_state[test_num] != DRIVER_ITERATE) {
		err(31, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_INIT: %d and != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_INIT, DRIVER_ITERATE);
	}

	/*
	 * collect parameters from state
	 */
	n = state->tp.n;

	/*
	 * setup to perform the test
	 */
	S = 0;
	sup = 0;
	inf = 0;
	memset(&stat, 0, sizeof(stat));
	for (k = 0; k < n; k++) {
		state->epsilon[k] ? S++ : S--;
		if (S > sup) {
			sup++;
		}
		if (S < inf) {
			inf--;
		}
		stat.z = (sup > -inf) ? sup : -inf;
		stat.zrev = (sup - S > S - inf) ? sup - S : S - inf;
	}

	/*
	 * perform the forward test
	 */
	sum1 = 0.0;
	for (k = (-n / stat.z + 1) / 4; k <= (n / stat.z - 1) / 4; k++) {
		sum1 += cephes_normal(((4 * k + 1) * stat.z) / state->c.sqrtn);
		sum1 -= cephes_normal(((4 * k - 1) * stat.z) / state->c.sqrtn);
	}
	sum2 = 0.0;
	for (k = (-n / stat.z - 3) / 4; k <= (n / stat.z - 1) / 4; k++) {
		sum2 += cephes_normal(((4 * k + 3) * stat.z) / state->c.sqrtn);
		sum2 -= cephes_normal(((4 * k + 1) * stat.z) / state->c.sqrtn);
	}
	p_value = 1.0 - sum1 + sum2;

	/*
	 * record testable test success or failure
	 */
	state->count[test_num]++;	// count a testable test
	if (isNegative(p_value)) {
		state->failure[test_num]++;	// bogus p_value < 0.0 treated as a failure
		stat.success = false;	// FAILURE
		warn(__FUNCTION__, "interation %ld of test %s[%d] produced bogus p_value: %f < 0.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value);
	} else if (isGreaterThanOne(p_value)) {
		state->failure[test_num]++;	// bogus p_value > 1.0 treated as a failure
		stat.success = false;	// FAILURE
		warn(__FUNCTION__, "interation %ld of test %s[%d] produced bogus p_value: %f > 1.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value);
	} else if (p_value < state->tp.alpha) {
		state->valid_p_val[test_num]++;	// valid p_value in [0.0, 1.0] range
		state->failure[test_num]++;	// valid p_value but too low is a failure
		stat.success = false;	// FAILURE
	} else {
		state->valid_p_val[test_num]++;	// valid p_value in [0.0, 1.0] range
		state->success[test_num]++;	// valid p_value not too low is a success
		stat.success = true;	// SUCCESS
	}

	/*
	 * perform the reverse test
	 */
	sum1 = 0.0;
	for (k = (-n / stat.zrev + 1) / 4; k <= (n / stat.zrev - 1) / 4; k++) {
		sum1 += cephes_normal(((4 * k + 1) * stat.zrev) / state->c.sqrtn);
		sum1 -= cephes_normal(((4 * k - 1) * stat.zrev) / state->c.sqrtn);
	}
	sum2 = 0.0;
	for (k = (-n / stat.zrev - 3) / 4; k <= (n / stat.zrev - 1) / 4; k++) {
		sum2 += cephes_normal(((4 * k + 3) * stat.zrev) / state->c.sqrtn);
		sum2 -= cephes_normal(((4 * k + 1) * stat.zrev) / state->c.sqrtn);
	}
	rev_p_value = 1.0 - sum1 + sum2;
	if (isNegative(rev_p_value)) {
		warn(__FUNCTION__, "\t\tWARNING:  REVERSE P_VALUE: %f < 0.0\n", rev_p_value);
	}
	if (isGreaterThanOne(rev_p_value)) {
		warn(__FUNCTION__, "\t\tWARNING:  REVERSE P_VALUE: %f > 1.0\n", rev_p_value);
	}

	/*
	 * record success or failure of reverse test
	 */
	state->count[test_num]++;	// count this test
	state->valid[test_num]++;	// count this valid test
	if (isNegative(rev_p_value)) {
		state->failure[test_num]++;
		stat.rev_success = false;
		warn(__FUNCTION__, "interation %ld of reverse test %s[%d] produced bogus p_value: %f < 0.0\n",
		     state->curIteration, state->testNames[test_num], test_num, rev_p_value);
	} else if (isGreaterThanOne(rev_p_value)) {
		state->failure[test_num]++;
		stat.rev_success = false;
		warn(__FUNCTION__, "interation %ld of reverse test %s[%d] produced bogus p_value: %f > 1.0\n",
		     state->curIteration, state->testNames[test_num], test_num, rev_p_value);
	} else if (rev_p_value < state->tp.alpha) {
		state->failure[test_num]++;
		stat.rev_success = false;
	} else {
		state->success[test_num]++;
		stat.rev_success = true;
	}

	/*
	 * results.txt and stats.txt accounting
	 */
	append_value(state->stats[test_num], &stat);
	append_value(state->p_val[test_num], &p_value);
	append_value(state->p_val[test_num], &rev_p_value);

	/*
	 * driver iterating - set driver state to DRIVER_ITERATE
	 */
	if (state->driver_state[test_num] != DRIVER_ITERATE) {
		dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_ITERATE: %d",
		    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_ITERATE);
		state->driver_state[test_num] = DRIVER_ITERATE;
	}
	return;
}


/*
 * CumulativeSums_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct CumulativeSums_private_stats for format and print
 *      p_value         // p_value interation test result(s) - forward direction
 *      rev_p_value     // p_value interation test result(s) - reverse direction
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
CumulativeSums_print_stat(FILE * stream, struct state *state, struct CumulativeSums_private_stats *stat, double p_value,
			  double rev_p_value)
{
	int io_ret;		// I/O return status

	/*
	 * firewall
	 */
	if (stream == NULL) {
		err(32, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(32, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(32, __FUNCTION__, "stat arg is NULL");
	}
	if (p_value == NON_P_VALUE && stat->success == true) {
		err(32, __FUNCTION__, "p_value was set to NON_P_VALUE but stat->success == true");
	}
	if (rev_p_value == NON_P_VALUE && stat->rev_success == true) {
		err(32, __FUNCTION__, "rev_p_value was set to NON_P_VALUE but stat->rev_success == true");
	}

	/*
	 * print stat to a file for the forward test
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t      CUMULATIVE SUMS (FORWARD) TEST\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\tCOMPUTATIONAL INFORMATION:\n");
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\t      Cumulative sums forward test\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(a) The maximum partial sum = %ld\n", stat->z);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
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
	 * print stat to a file for the reverse test
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t      CUMULATIVE SUMS (REVERSE) TEST\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\tCOMPUTATIONAL INFORMATION:\n");
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\t      Cumulative sums reverse test\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(a) The maximum partial sum = %ld\n", stat->zrev);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	if (stat->rev_success == true) {
		io_ret = fprintf(stream, "SUCCESS\t\tp_value = %f\n\n", rev_p_value);
		if (io_ret <= 0) {
			return false;
		}
	} else if (rev_p_value == NON_P_VALUE) {
		io_ret = fprintf(stream, "FAILURE\t\tp_value = __INVALID__\n\n");
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "FAILURE\t\tp_value = %f\n\n", rev_p_value);
		if (io_ret <= 0) {
			return false;
		}
	}

	/*
	 * all printing successful
	 */
	return true;
}


/*
 * CumulativeSums_print_p_value - print p_value information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      stat            // struct CumulativeSums_private_stats for format and print
 *      p_value         // p_value interation test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
CumulativeSums_print_p_value(FILE * stream, double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * firewall
	 */
	if (stream == NULL) {
		err(33, __FUNCTION__, "stream arg is NULL");
	}

	/*
	 * print p_value to a file
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
	 * all printing successful
	 */
	return true;
}


/*
 * CumulativeSums_print - print to results.txt, data*.txt, stats.txt for all interations
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
CumulativeSums_print(struct state *state)
{
	struct CumulativeSums_private_stats *stat;	// pointer to statistics of an interation
	double p_value;		// p_value interation test result(s) - forward direction
	double rev_p_value;	// p_value interation test result(s) - reverse direction
	FILE *stats = NULL;	// open stats.txt file
	FILE *results = NULL;	// open stats.txt file
	FILE *data = NULL;	// open data*.txt file
	char *stats_txt = NULL;	// pathname for stats.txt
	char *results_txt = NULL;	// pathname for results.txt
	char *data_txt = NULL;	// pathname for data*.txt
	char data_filename[BUFSIZ + 1];	// basebame for a given data*.txt pathname
	bool ok;		// true -> I/O was OK
	int snprintf_ret;	// snprintf return value
	int io_ret;		// I/O return status
	long int i;
	long int j;

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(34, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "print driver interface for %s[%d] called when test vector was false",
		    state->testNames[test_num], test_num);
		return;
	}
	if (state->resultstxtFlag == false) {
		dbg(DBG_LOW, "print driver interface for %s[%d] disabled due to -n", state->testNames[test_num], test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(34, __FUNCTION__,
		    "print driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(34, __FUNCTION__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->datatxt_fmt[test_num] == NULL) {
		err(34, __FUNCTION__, "format for data0*.txt filename is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_ITERATE) {
		err(34, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_ITERATE);
	}

	/*
	 * open stats.txt file
	 */
	stats_txt = filePathName(state->subDir[test_num], "stats.txt");
	dbg(DBG_MED, "about to open/truncate: %s", stats_txt);
	stats = openTruncate(stats_txt);

	/*
	 * open results.txt file
	 */
	results_txt = filePathName(state->subDir[test_num], "results.txt");
	dbg(DBG_MED, "about to open/truncate: %s", results_txt);
	results = openTruncate(results_txt);

	/*
	 * write results.txt and stats.txt files
	 */
	for (i = 0; i < state->stats[test_num]->count; ++i) {

		/*
		 * locate stat for this interation
		 */
		stat = addr_value(state->stats[test_num], struct CumulativeSums_private_stats, i);

		/*
		 * get p_value pair (forwaed and reverse) for this interation
		 */
		p_value = get_value(state->p_val[test_num], double, i * 2);
		rev_p_value = get_value(state->p_val[test_num], double, (i * 2) + 1);

		/*
		 * print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = CumulativeSums_print_stat(stats, state, stat, p_value, rev_p_value);
		if (ok == false) {
			errp(34, __FUNCTION__, "error in writing to %s", stats_txt);
		}

		/*
		 * print p_value and rev_p_value to results.txt
		 */
		errno = 0;	// paranoia
		ok = CumulativeSums_print_p_value(results, p_value);
		if (ok == false) {
			errp(34, __FUNCTION__, "error in writing to %s", results_txt);
		}
		errno = 0;	// paranoia
		ok = CumulativeSums_print_p_value(results, rev_p_value);
		if (ok == false) {
			errp(34, __FUNCTION__, "error in writing to %s", results_txt);
		}
	}

	/*
	 * flush and close stats.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(stats);
	if (io_ret != 0) {
		errp(34, __FUNCTION__, "error flushing to: %s", stats_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(stats);
	if (io_ret != 0) {
		errp(34, __FUNCTION__, "error closing: %s", stats_txt);
	}
	free(stats_txt);
	stats_txt = NULL;

	/*
	 * flush and close results.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(results);
	if (io_ret != 0) {
		errp(34, __FUNCTION__, "error flushing to: %s", results_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(results);
	if (io_ret != 0) {
		errp(34, __FUNCTION__, "error closing: %s", results_txt);
	}
	free(results_txt);
	results_txt = NULL;

	/*
	 * write data*.txt if we need to partition results
	 */
	if (state->partitionCount[test_num] > 1) {

		/*
		 * for each data file
		 */
		for (j = 0; j < state->partitionCount[test_num]; ++j) {

			/*
			 * form the data*.txt basename
			 */
			errno = 0;	// paranoia
			snprintf_ret = snprintf(data_filename, BUFSIZ, state->datatxt_fmt[test_num], j + 1);
			data_filename[BUFSIZ] = '\0';	// paranoia
			if (snprintf_ret <= 0 || snprintf_ret >= BUFSIZ || errno != 0) {
				errp(34, __FUNCTION__,
				     "snprintf failed for %d bytes for data%03ld.txt, returned: %d", BUFSIZ, j + 1, snprintf_ret);
			}

			/*
			 * form the data*.txt filename
			 */
			data_txt = filePathName(state->subDir[test_num], data_filename);
			dbg(DBG_MED, "about to open/truncate: %s", data_txt);
			data = openTruncate(data_txt);

			/*
			 * write this particular data*.txt filename
			 */
			if (j < state->p_val[test_num]->count) {
				for (i = j; i < state->p_val[test_num]->count; i += state->partitionCount[test_num]) {

					/*
					 * get p_value for an interation belonging to this data*.txt filename
					 */
					p_value = get_value(state->p_val[test_num], double, i);

					/*
					 * print p_value to results.txt
					 */
					errno = 0;	// paranoia
					ok = CumulativeSums_print_p_value(data, p_value);
					if (ok == false) {
						errp(34, __FUNCTION__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * flush and close data*.txt, free pathname
			 */
			errno = 0;	// paranoia
			io_ret = fflush(data);
			if (io_ret != 0) {
				errp(34, __FUNCTION__, "error flushing to: %s", data_txt);
			}
			errno = 0;	// paranoia
			io_ret = fclose(data);
			if (io_ret != 0) {
				errp(34, __FUNCTION__, "error closing: %s", data_txt);
			}
			free(data_txt);
			data_txt = NULL;

		}
	}

	/*
	 * driver print - set driver state to DRIVER_PRINT
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_PRINT);
	state->driver_state[test_num] = DRIVER_PRINT;
	return;
}


/*
 * CumulativeSums_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state           // run state to test under
 *      sampleCount             // number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // uniformanity frequency bins
 */
static void
CumulativeSums_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
{
	long int passCount;	// p_values that pass
	double p_hat;		// 1 - alpha
	double proportion_threshold_max;	// when passCount is too high
	double proportion_threshold_min;	// when passCount is too low
	double chi2;		// sum of chi^2 for each tenth
	double uniformity;	// uniformitu of frequency bins
	double expCount;	// sample size divided by frequency bin count
	int io_ret;		// I/O return status
	long int i;

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(35, __FUNCTION__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(35, __FUNCTION__, "freqPerBin arg is NULL");
	}

	/*
	 * determine the number tests that passed
	 */
	if ((sampleCount <= 0) || (sampleCount < toolow)) {
		passCount = 0;
	} else {
		passCount = sampleCount - toolow;
	}

	/*
	 * determine proportion threadholds
	 */
	p_hat = 1.0 - state->tp.alpha;
	proportion_threshold_max = (p_hat + 3.0 * sqrt((p_hat * state->tp.alpha) / sampleCount)) * sampleCount;
	proportion_threshold_min = (p_hat - 3.0 * sqrt((p_hat * state->tp.alpha) / sampleCount)) * sampleCount;

	/*
	 * uniformity failure check
	 */
	chi2 = 0.0;
	expCount = sampleCount / state->tp.uniformity_bins;
	if (expCount <= 0.0) {
		// not enough samples for uniformity check
		uniformity = 0.0;
	} else {
		// sum chi squared of the frequency bins
		for (i = 0; i < state->tp.uniformity_bins; ++i) {
			chi2 += (freqPerBin[i] - expCount) * (freqPerBin[i] - expCount) / expCount;
		}
		// uniformity threashold level
		uniformity = cephes_igamc((state->tp.uniformity_bins - 1.0) / 2.0, chi2 / 2.0);
	}

	/*
	 * output uniformity results in trandtional format to finalAnalysisReport.txt
	 */
	for (i = 0; i < state->tp.uniformity_bins; ++i) {
		fprintf(state->finalRept, "%3ld ", freqPerBin[i]);
	}
	if (expCount <= 0.0) {
		// not enough samples for uniformity check
		fprintf(state->finalRept, "    ----    ");
		state->uniformity_failure[test_num] = false;
		dbg(DBG_HIGH, "too few iterations for uniformity check on %s", state->testNames[test_num]);
	} else if (uniformity < state->tp.uniformity_level) {
		// uniformity failure
		fprintf(state->finalRept, " %8.6f * ", uniformity);
		state->uniformity_failure[test_num] = true;
		dbg(DBG_HIGH, "metrics detected uniformity failure for %s", state->testNames[test_num]);
	} else {
		// uniformity success
		fprintf(state->finalRept, " %8.6f   ", uniformity);
		state->uniformity_failure[test_num] = false;
		dbg(DBG_HIGH, "metrics detected uniformity success for %s", state->testNames[test_num]);
	}

	/*
	 * output proportional results in trandtional format to finalAnalysisReport.txt
	 */
	if (sampleCount == 0) {
		// not enough samples for proportional check
		fprintf(state->finalRept, " ------     %s\n", state->testNames[test_num]);
		state->proportional_failure[test_num] = false;
		dbg(DBG_HIGH, "too few samples for proportional check on %s", state->testNames[test_num]);
	} else if ((passCount < proportion_threshold_min) || (passCount > proportion_threshold_max)) {
		// proportional failure
		state->proportional_failure[test_num] = true;
		fprintf(state->finalRept, "%4ld/%-4ld *  %s\n", passCount, sampleCount, state->testNames[test_num]);
		dbg(DBG_HIGH, "metrics detected proportional failure for %s", state->testNames[test_num]);
	} else {
		// proportional success
		state->proportional_failure[test_num] = false;
		fprintf(state->finalRept, "%4ld/%-4ld    %s\n", passCount, sampleCount, state->testNames[test_num]);
		dbg(DBG_HIGH, "metrics detected proportional success for %s", state->testNames[test_num]);
	}
	errno = 0;		// paranoia
	io_ret = fflush(state->finalRept);
	if (io_ret != 0) {
		errp(35, __FUNCTION__, "error flushing to: %s", state->finalReptPath);
	}
	return;
}


/*
 * CumulativeSums_metrics - uniformity and proportional analysis of a test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to complete the test analysis for all interations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
CumulativeSums_metrics(struct state *state)
{
	long int sampleCount;	// number of bitstreams in which we will count p_values
	long int toolow;	// p_values that were below alpha
	double p_value;		// p_value interation test result(s)
	long int *freqPerBin;	// uniformanity frequency bins
	long int i;
	long int j;

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(36, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false",
		    state->testNames[test_num], test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(36, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(36, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with p_val length: %ld != bit streams: %ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->driver_state[test_num] != DRIVER_PRINT) {
		err(36, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_PRINT: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_PRINT);
	}

	/*
	 * allocate uniformanity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(36, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
		     state->tp.uniformity_bins, sizeof(long int));
	}

	/*
	 * print for each partition (or the whole set of p_values if partitionCount is 1)
	 */
	for (j = 0; j < state->partitionCount[test_num]; ++j) {

		/*
		 * zero counters
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
		 * p_value tally
		 */
		for (i = j; i < state->p_val[test_num]->count; i += state->partitionCount[test_num]) {

			// get the intertion p_value
			p_value = get_value(state->p_val[test_num], double, i);
			if (p_value == NON_P_VALUE) {
				continue;	// the test was not possible for this interation
			}
			// case: random excursion test
			if (state->is_excursion[test_num] == true) {
				// random excursion tests only sample > 0 p_values
				if (p_value > 0.0) {
					++sampleCount;
				} else {
					// ignore p_value of 0 for random excursion tests
					continue;
				}

				// case: general (non-random excursion) test
			} else {
				// all other tests count all p_values
				++sampleCount;
			}

			// count the number of p_values below alpha
			if (p_value < state->tp.alpha) {
				++toolow;
			}
			// tally the p_value in a uniformity bin
			if (p_value >= 1.0) {
				++freqPerBin[state->tp.uniformity_bins - 1];
			} else if (p_value >= 0.0) {
				++freqPerBin[(int) floor(p_value * (double) state->tp.uniformity_bins)];
			} else {
				++freqPerBin[0];
			}
		}

		/*
		 * print uniformity and proportional information for a tallied count
		 */
		CumulativeSums_metric_print(state, sampleCount, toolow, freqPerBin);

		/*
		 * track maximum samples
		 */
		// case: random excursion test
		if (state->is_excursion[test_num] == true) {
			if (sampleCount > state->maxRandomExcursionSampleSize) {
				state->maxRandomExcursionSampleSize = sampleCount;
			}
			// case: general (non-random excursion) test
		} else {
			if (sampleCount > state->maxGeneralSampleSize) {
				state->maxGeneralSampleSize = sampleCount;
			}
		}

	}

	/*
	 * free allocated storage
	 */
	free(freqPerBin);
	freqPerBin = NULL;

	/*
	 * driver uniformity and proportional analysis - set driver state to DRIVER_METRICS
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_METRICS);
	state->driver_state[test_num] = DRIVER_METRICS;
	return;
}


/*
 * CumulativeSums_destroy - post process results for this text
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to cleanup any storage or state
 * associated with this test.
 */
void
CumulativeSums_destroy(struct state *state)
{
	/*
	 * firewall
	 */
	if (state == NULL) {
		err(37, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "destroy function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}

	/*
	 * free dynamic arrays
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
	 * free other test storage
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
	 * driver state destroyed - set driver state to DRIVER_DESTROY
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_DESTROY);
	state->driver_state[test_num] = DRIVER_DESTROY;
	return;
}
