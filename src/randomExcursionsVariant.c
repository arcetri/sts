/*****************************************************************************
	    R A N D O M   E X C U R S I O N S   V A R I A N T   T E S T
 *****************************************************************************/

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
 * private_stats - stats.txt information for this test
 */
struct RandomExcursionsVariant_private_stats {
	bool excursion_success[EXCURSTION_VAR_STATES];	// success or failure of interation test for an excursion state
	long int J;		// number of zero crossings (cycles) for this interation
	long int counter[EXCURSTION_VAR_STATES];	// times when partial sun matches the excursion state
	bool test_possible;	// true --> test is possible for this interation
};


/*
 * static constant variable declarations
 */
static const enum test test_num = TEST_RND_EXCURSION_VAR;	// this test number


/*
 * forward static function declarations
 */
static bool RandomExcursionsVariant_print_stat(FILE * stream, struct state *state,
					       struct RandomExcursionsVariant_private_stats *stat, long int iteration);
static bool RandomExcursionsVariant_print_stat2(FILE * stream, struct state *state,
						struct RandomExcursionsVariant_private_stats *stat, long int p, double p_value);
static bool RandomExcursionsVariant_print_p_value(FILE * stream, double p_value);
static void RandomExcursionsVariant_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin);


/*
 * RandomExcursionsVariant_init - initalize the Random Excursions Variant test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
RandomExcursionsVariant_init(struct state *state)
{
	int i;

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(10, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false",
		    state->testNames[test_num], test_num);
		return;
	}
	if (state->cSetup != true) {
		err(10, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_NULL && state->driver_state[test_num] != DRIVER_DESTROY) {
		err(10, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_NULL: %d and != DRIVER_DESTROY: %d",
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
	 * allocate and initialize excursion states
	 */
	state->excursion_var_stateX = malloc(EXCURSTION_VAR_STATES * sizeof(state->excursion_var_stateX[0]));
	if (state->excursion_var_stateX == NULL) {
		errp(10, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->excursion_var_stateX",
		     (long int) EXCURSTION_VAR_STATES, sizeof(long int));
	}
	for (i = 1; i <= MAX_EXCURSION_VAR; ++i) {
		state->excursion_var_stateX[i - 1] = -MAX_EXCURSION_VAR + i - 1;
		state->excursion_var_stateX[EXCURSTION_VAR_STATES - MAX_EXCURSION_VAR + i - 1] = i;
	}

	/*
	 * allocate partial sums working array
	 */
	state->ex_var_partial_sums = malloc(state->tp.n * sizeof(state->ex_var_partial_sums[0]));
	if (state->ex_var_partial_sums == NULL) {
		errp(10, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->ex_var_partial_sums",
		     state->tp.n, sizeof(long int));
	}

	/*
	 * allocate dynamic arrays
	 */
	// stats.txt data
	state->stats[test_num] =
	    create_dyn_array(sizeof(struct RandomExcursionsVariant_private_stats), DEFAULT_CHUNK, state->tp.numOfBitStreams, false);

	// results.txt data
	state->p_val[test_num] = create_dyn_array(sizeof(double), DEFAULT_CHUNK,
						  EXCURSTION_VAR_STATES * state->tp.numOfBitStreams, false);

	/*
	 * determine format of data*.txt filenames based on state->partitionCount[test_num]
	 *
	 * NOTE: If we are not partitioning the p_values, no data*.txt filenames are needed
	 */
	state->datatxt_fmt[test_num] = data_filename_format(state->partitionCount[test_num]);
	dbg(DBG_HIGH, "%s[%d] will form data*.txt filenames with the following format: %s",
	    state->testNames[test_num], test_num, state->datatxt_fmt[test_num]);

	/*
	 * driver initialized - set driver state to DRIVER_INIT
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_INIT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_INIT);
	state->driver_state[test_num] = DRIVER_INIT;
	return;
}


/*
 * RandomExcursionsVariant_iterate - interate one bit stream for Random Excursions Variant test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
RandomExcursionsVariant_iterate(struct state *state)
{
	struct RandomExcursionsVariant_private_stats stat;	// stats for this interation
	long int n;		// Length of a single bit stream
	long int *S_k;		// partial sums working array
	double p_value;		// p_value interation test result(s)
	long int p;
	long int i;

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(10, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "interate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	if (state->epsilon == NULL) {
		err(10, __FUNCTION__, "state->epsilon is NULL");
	}
	if (state->excursion_var_stateX == NULL) {
		err(10, __FUNCTION__, "state->excursion_var_stateX is NULL");
	}
	if (state->ex_var_partial_sums == NULL) {
		err(10, __FUNCTION__, "state->ex_var_partial_sums is NULL");
	}
	if (state->cSetup != true) {
		err(10, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_INIT && state->driver_state[test_num] != DRIVER_ITERATE) {
		err(10, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_INIT: %d and != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_INIT, DRIVER_ITERATE);
	}

	/*
	 * collect parameters from state
	 */
	S_k = state->ex_var_partial_sums;
	n = state->tp.n;

	/*
	 * setup excursion state machine for this interation
	 *
	 * Count the number of zero crossings for this interation
	 */
	stat.J = 0;
	S_k[0] = 2 * (long int) state->epsilon[0] - 1;
	for (i = 1; i < n; i++) {
		S_k[i] = S_k[i - 1] + 2 * state->epsilon[i] - 1;
		if (S_k[i] == 0) {
			stat.J++;
		}
	}
	if (S_k[n - 1] != 0) {
		stat.J++;
	}

	/*
	 * determine if we still can test
	 */
	stat.test_possible = (stat.J < state->c.excursion_constraint) ? false : true;

	/*
	 * perform and record the test if it is possible to test
	 */
	if (stat.test_possible == true) {

		/*
		 * NOTE: Mathematical expression code rewrite, old code commented out below:
		 *
		 * for (p = 0; p <= EXCURSTION_VAR_STATES-1; p++) {
		 * x = stateX[p];
		 * ...
		 * }
		 */
		for (p = 0; p < EXCURSTION_VAR_STATES; p++) {

			/*
			 * count times when the partial sum (S_k[i] matches the excursion state state->excursion_var_stateX[p]
			 */
			stat.counter[p] = 0;
			for (i = 0; i < n; i++) {
				if (S_k[i] == state->excursion_var_stateX[p]) {
					stat.counter[p]++;
				}
			}
			/*
			 * NOTE: Mathematical expression code rewrite, old code commented out below:
			 *
			 * p_value = erfc(fabs(count-J)/(sqrt(2.0*J*(4.0*fabs(x)-2))));
			 */
			p_value = erfc(labs(stat.counter[p] - stat.J) /
				       (sqrt(2.0 * stat.J * (4.0 * labs(state->excursion_var_stateX[p]) - 2.0))));

			/*
			 * record testable test success or failure
			 */
			state->count[test_num]++;	// count this test
			state->valid[test_num]++;	// count this valid test
			if (isNegative(p_value)) {
				state->failure[test_num]++;	// bogus p_value < 0.0 treated as a failure
				stat.excursion_success[p] = false;	// FAILURE
				warn(__FUNCTION__, "interation %ld of test %s[%d] produced bogus p_value: %f < 0.0\n",
				     state->curIteration, state->testNames[test_num], test_num, p_value);
			} else if (isGreaterThanOne(p_value)) {
				state->failure[test_num]++;	// bogus p_value > 1.0 treated as a failure
				stat.excursion_success[p] = false;	// FAILURE
				warn(__FUNCTION__, "interation %ld of test %s[%d] produced bogus p_value: %f > 1.0\n",
				     state->curIteration, state->testNames[test_num], test_num, p_value);
			} else if (p_value < state->tp.alpha) {
				state->valid_p_val[test_num]++;	// valid p_value in [0.0, 1.0] range
				state->failure[test_num]++;	// valid p_value but too low is a failure
				stat.excursion_success[p] = false;	// FAILURE
			} else {
				state->valid_p_val[test_num]++;	// valid p_value in [0.0, 1.0] range
				state->success[test_num]++;	// valid p_value not too low is a success
				stat.excursion_success[p] = true;	// SUCCESS
			}

			/*
			 * results.txt accounting
			 */
			append_value(state->p_val[test_num], &p_value);
		}

		/*
		 * stats.txt accounting
		 */
		append_value(state->stats[test_num], &stat);

		/*
		 * accounting for tests that cannot be performed
		 */
	} else {

		// count this test, which happens to be invalid
		state->count[test_num]++;

		/*
		 * results.txt accounting
		 */
		p_value = NON_P_VALUE;
		for (p = 0; p < EXCURSTION_VAR_STATES; p++) {
			append_value(state->p_val[test_num], &p_value);
		}

		/*
		 * stats.txt accounting
		 */
		for (p = 0; p < EXCURSTION_VAR_STATES; p++) {
			stat.excursion_success[p] = false;	// FAILURE
		}
		/*
		 * NOTE: Logical code rewrite, old code commented out below:
		 *
		 * for (i = 0; i < EXCURSTION_VAR_STATES; i++) {
		 *      stat.counter[i] = 0;
		 * }
		 */
		memset(stat.counter, 0, sizeof(stat.counter));
		append_value(state->stats[test_num], &stat);
	}

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
 * RandomExcursionsVariant_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct RandomExcursionsVariant_private_stats for format and print
 *      iteration       // current intertion number being printed
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 *
 * Unlike most *_print_stat() functions, the EXCURSTION_VAR_STATES number of p_values
 * will be printed by repeated calls to RandomExcursionsVariant_print_stat2()
 * if the test was possible.
 */
static bool
RandomExcursionsVariant_print_stat(FILE * stream, struct state *state, struct RandomExcursionsVariant_private_stats *stat,
				   long int iteration)
{
	int io_ret;		// I/O return status

	/*
	 * firewall
	 */
	if (stream == NULL) {
		err(10, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(10, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(10, __FUNCTION__, "stat arg is NULL");
	}
	if (state->cSetup != true) {
		err(10, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}

	/*
	 * print stat to a file
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t\tRANDOM EXCURSIONS VARIANT TEST\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t--------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\tCOMPUTATIONAL INFORMATION:\n");
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\t\tRandom excursions variant test\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t--------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(a) Number Of Cycles (J) = %ld\n", stat->J);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(b) Sequence Length (n)  = %ld\n", state->tp.n);
	if (io_ret <= 0) {
		return false;
	}
	if (state->legacy_output == false) {
		io_ret = fprintf(stream, "\t\t(c) Rejection Constraint = %ld\n", state->c.excursion_constraint);
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t--------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	if (stat->test_possible == false) {
		if (state->legacy_output == true) {
			io_ret = fprintf(stream, "\n\t\tWARNING:  TEST NOT APPLICABLE.  THERE ARE AN\n");
			if (io_ret <= 0) {
				return false;
			}
			io_ret = fprintf(stream, "\t\t\t  INSUFFICIENT NUMBER OF CYCLES.\n");
			if (io_ret <= 0) {
				return false;
			}
			io_ret = fprintf(stream, "\t\t---------------------------------------------\n");
			if (io_ret <= 0) {
				return false;
			}
			io_ret = fputc('\n', stream);
			if (io_ret == EOF) {
				return false;
			}
		} else {
			/*
			 * note which cycle is not applicable
			 */
			io_ret = fprintf(stream, "\t\tInteration %ld test not applicable\n", iteration);
			if (io_ret <= 0) {
				return false;
			}
			io_ret = fprintf(stream, "\t\texcessive cycles, J: %ld >= max expected: %lu\n\n",
					 stat->J, state->c.excursion_constraint);
			if (io_ret <= 0) {
				return false;
			}
		}
	}

	/*
	 * all printing successful
	 */
	return true;
}


/*
 * RandomExcursionsVariant_print_stat2 - print a excursion info for a possibe test
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct RandomExcursionsVariant_private_stats for format and print
 *      p               // excursion number to print [0,EXCURSTION_VAR_STATES)
 *      p_value         // p_value interation test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 *
 * Unlike most *_print_stat() functions, the EXCURSTION_VAR_STATES number of p_values
 * will be printed by repeated calls to RandomExcursionsVariant_print_stat2()
 * with differet excursion numbers.
 *
 * If the test was possible, then the excursion state, visit cound and p_value are printed.
 * If the test was not possible due to an insufficent number of crossings, this function
 * does not print, just returns.
 */
static bool
RandomExcursionsVariant_print_stat2(FILE * stream, struct state *state, struct RandomExcursionsVariant_private_stats *stat,
				    long int p, double p_value)
{
	int io_ret;		// I/O return status

	// need to cal this function with stat.test_possible == true

	/*
	 * firewall
	 */
	if (stream == NULL) {
		err(10, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(10, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(10, __FUNCTION__, "stat arg is NULL");
	}
	if (state->excursion_var_stateX == NULL) {
		err(10, __FUNCTION__, "state->excursion_var_stateX is NULL");
	}
	if (p < 0) {
		err(10, __FUNCTION__, "p arg: %ld must be > 0", p);
	}
	if (p >= EXCURSTION_VAR_STATES) {
		err(10, __FUNCTION__, "p arg: %ld must be < %d", p, EXCURSTION_VAR_STATES);
	}

	/*
	 * Nothint to print if the test was not possible
	 */
	if (stat->test_possible != true) {
		return true;
	}
	if (p_value == NON_P_VALUE && stat->excursion_success[p] == true) {
		err(10, __FUNCTION__, "p_value was set to NON_P_VALUE but stat->excursion_success[%ld] == true", p);
	}

	/*
	 * print stat to a file
	 */
	if (stat->excursion_success[p] == true) {
		io_ret = fprintf(stream, "SUCCESS\t\t");
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "FAILURE\t\t");
		if (io_ret <= 0) {
			return false;
		}
	}
	if (state->legacy_output == true) {
		if (p_value == NON_P_VALUE) {
			io_ret = fprintf(stream, "(x = %2ld) Total visits = %4ld; p-value = __INVALID__\n",
					 state->excursion_var_stateX[p], stat->counter[p]);
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret = fprintf(stream, "(x = %2ld) Total visits = %4ld; p-value = %f\n",
					 state->excursion_var_stateX[p], stat->counter[p], p_value);
			if (io_ret <= 0) {
				return false;
			}
		}
	} else {
		if (p_value == NON_P_VALUE) {
			io_ret = fprintf(stream, "x = %2ld  visits = %4ld  p_value = __INVALID__\n",
					 state->excursion_var_stateX[p], stat->counter[p]);
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret = fprintf(stream, "x = %2ld  visits = %4ld  p_value = %f\n",
					 state->excursion_var_stateX[p], stat->counter[p], p_value);
			if (io_ret <= 0) {
				return false;
			}
		}
	}
	// extra newline on the last excursion state
	if (p >= EXCURSTION_VAR_STATES - 1) {
		io_ret = fputc('\n', stream);
		if (io_ret == EOF) {
			return false;
		}
	}
	return true;
}


/*
 * RandomExcursionsVariant_print_p_value - print p_value information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      stat            // struct RandomExcursionsVariant_private_stats for format and print
 *      p_value         // p_value interation test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
RandomExcursionsVariant_print_p_value(FILE * stream, double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * firewall
	 */
	if (stream == NULL) {
		err(10, __FUNCTION__, "stream arg is NULL");
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
 * RandomExcursionsVariant_print - print to results.txt, data*.txt, stats.txt for all interations
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
RandomExcursionsVariant_print(struct state *state)
{
	struct RandomExcursionsVariant_private_stats *stat;	// pointer to statistics of an interation
	double p_value;		// p_value interation test result(s)
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
	long int p;

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(10, __FUNCTION__, "state arg is NULL");
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
		err(10, __FUNCTION__,
		    "print driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(10, __FUNCTION__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->datatxt_fmt[test_num] == NULL) {
		err(10, __FUNCTION__, "format for data0*.txt filename is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_ITERATE) {
		err(10, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_ITERATE: %d",
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
	for (i = 0, j = 0; i < state->stats[test_num]->count; ++i, j += EXCURSTION_VAR_STATES) {

		/*
		 * locate stat for this interation
		 */
		stat = addr_value(state->stats[test_num], struct RandomExcursionsVariant_private_stats, i);

		/*
		 * print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = RandomExcursionsVariant_print_stat(stats, state, stat, i);
		if (ok == false) {
			errp(10, __FUNCTION__, "error in writing to %s", stats_txt);
		}

		/*
		 * print all of the excursion states for this interation
		 */
		for (p = 0; p < EXCURSTION_VAR_STATES; p++) {

			/*
			 * get p_value for this excursion state of this interation
			 */
			p_value = get_value(state->p_val[test_num], double, j + p);

			/*
			 * print, if possibe, the excursion success or failure, visit and p_value
			 */
			errno = 0;	// paranoia
			ok = RandomExcursionsVariant_print_stat2(stats, state, stat, p, p_value);
			if (ok == false) {
				errp(10, __FUNCTION__, "error in writing to %s", stats_txt);
			}

			/*
			 * print p_value to results.txt
			 */
			errno = 0;	// paranoia
			ok = RandomExcursionsVariant_print_p_value(results, p_value);
			if (ok == false) {
				errp(10, __FUNCTION__, "error in writing to %s", results_txt);
			}
		}
	}

	/*
	 * flush and close stats.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(stats);
	if (io_ret != 0) {
		errp(10, __FUNCTION__, "error flushing to: %s", stats_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(stats);
	if (io_ret != 0) {
		errp(10, __FUNCTION__, "error closing: %s", stats_txt);
	}
	free(stats_txt);
	stats_txt = NULL;

	/*
	 * flush and close results.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(results);
	if (io_ret != 0) {
		errp(10, __FUNCTION__, "error flushing to: %s", results_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(results);
	if (io_ret != 0) {
		errp(10, __FUNCTION__, "error closing: %s", results_txt);
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
				errp(10, __FUNCTION__,
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
					ok = RandomExcursionsVariant_print_p_value(data, p_value);
					if (ok == false) {
						errp(10, __FUNCTION__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * flush and close data*.txt, free pathname
			 */
			errno = 0;	// paranoia
			io_ret = fflush(data);
			if (io_ret != 0) {
				errp(10, __FUNCTION__, "error flushing to: %s", data_txt);
			}
			errno = 0;	// paranoia
			io_ret = fclose(data);
			if (io_ret != 0) {
				errp(10, __FUNCTION__, "error closing: %s", data_txt);
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
 * RandomExcursionsVariant_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state           // run state to test under
 *      sampleCount             // number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // uniformanity frequency bins
 */
static void
RandomExcursionsVariant_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
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
		err(10, __FUNCTION__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(10, __FUNCTION__, "freqPerBin arg is NULL");
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
			/*
			 * NOTE: Mathematical expression code rewrite, old code commented out below:
			 *
			 * chi2 += pow(freqPerBin[j]-expCount, 2)/expCount;
			 */
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
		errp(10, __FUNCTION__, "error flushing to: %s", state->finalReptPath);
	}
	return;
}


/*
 * RandomExcursionsVariant_metrics - uniformity and proportional analysis of a test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to complete the test analysis for all interations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
RandomExcursionsVariant_metrics(struct state *state)
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
		err(10, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false",
		    state->testNames[test_num], test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(10, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(10, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with p_val length: %ld != bit streams: %ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->driver_state[test_num] != DRIVER_PRINT) {
		err(10, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_PRINT: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_PRINT);
	}

	/*
	 * allocate uniformanity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(10, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
		     (long int) state->tp.uniformity_bins, sizeof(long int));
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
		RandomExcursionsVariant_metric_print(state, sampleCount, toolow, freqPerBin);

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
 * RandomExcursionsVariant_destroy - post process results for this text
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to cleanup any storage or state
 * associated with this test.
 */
void
RandomExcursionsVariant_destroy(struct state *state)
{
	/*
	 * firewall
	 */
	if (state == NULL) {
		err(10, __FUNCTION__, "state arg is NULL");
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
	if (state->excursion_var_stateX != NULL) {
		free(state->excursion_var_stateX);
		state->excursion_var_stateX = NULL;
	}
	if (state->ex_var_partial_sums != NULL) {
		free(state->ex_var_partial_sums);
		state->ex_var_partial_sums = NULL;
	}

	/*
	 * driver state destroyed - set driver state to DRIVER_DESTROY
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_DESTROY);
	state->driver_state[test_num] = DRIVER_DESTROY;
	return;
}
