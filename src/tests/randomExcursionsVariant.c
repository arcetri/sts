/*****************************************************************************
	    R A N D O M	  E X C U R S I O N S	V A R I A N T	T E S T
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


// Exit codes: 160 thru 169

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
struct RandomExcursionsVariant_private_stats {
	bool success[NUMBER_OF_STATES_RND_EXCURSION_VAR]; 	// Success or failure of iteration test for each excursion state
	long int counter[NUMBER_OF_STATES_RND_EXCURSION_VAR];	// Counters of visits to each excursion state
	bool test_possible;					// true --> test is possible for this iteration
	long int number_of_cycles;				// Number of cycles (and zero crossings) for this iteration
};


/*
 * Static const variables declarations
 */
static const enum test test_num = TEST_RND_EXCURSION_VAR;	// This test number


/*
 * Forward static function declarations
 */
static bool RandomExcursionsVariant_print_stat(FILE * stream, struct state *state,
					       struct RandomExcursionsVariant_private_stats *stat, long int iteration);
static bool RandomExcursionsVariant_print_stat2(FILE * stream, struct state *state,
						struct RandomExcursionsVariant_private_stats *stat, long int p, double p_value);
static bool RandomExcursionsVariant_print_p_value(FILE * stream, double p_value);
static void RandomExcursionsVariant_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin);


/*
 * RandomExcursionsVariant_init - initialize the Random Excursions Variant test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
RandomExcursionsVariant_init(struct state *state)
{
	long int n;		// Length of a single bit stream
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(160, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->cSetup != true) {
		err(160, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_NULL && state->driver_state[test_num] != DRIVER_DESTROY) {
		err(160, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_NULL: %d and != DRIVER_DESTROY: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_NULL, DRIVER_DESTROY);
	}

	/*
	 * Collect parameters from state
	 */
	n = state->tp.n;

	/*
	 * Disable test if conditions do not permit this test from being run
	 */
	if (n < MIN_LENGTH_RND_EXCURSION_VAR) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires bitcount(n): %ld >= %d",
		     state->testNames[test_num], test_num, n, MIN_LENGTH_RND_EXCURSION_VAR);
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
	 * Allocate and initialize excursion states mapping array
	 */
	state->rnd_excursion_var_stateX = malloc(NUMBER_OF_STATES_RND_EXCURSION_VAR * sizeof(state->rnd_excursion_var_stateX[0]));
	if (state->rnd_excursion_var_stateX == NULL) {
		errp(160, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->rnd_excursion_var_stateX",
		     (long int) NUMBER_OF_STATES_RND_EXCURSION_VAR, sizeof(long int));
	}

	/*
	 * Store the values of each excursion state in order (to later retrieve the value of a state from its index)
	 */
	for (i = 1; i <= MAX_EXCURSION_RND_EXCURSION_VAR; ++i) {
		state->rnd_excursion_var_stateX[i - 1] = -MAX_EXCURSION_RND_EXCURSION_VAR + i - 1;
		state->rnd_excursion_var_stateX[NUMBER_OF_STATES_RND_EXCURSION_VAR - MAX_EXCURSION_RND_EXCURSION_VAR + i - 1] = i;
	}

	/*
	 * Allocate partial sums working array
	 */
	state->ex_var_partial_sums = malloc(state->tp.n * sizeof(state->ex_var_partial_sums[0]));
	if (state->ex_var_partial_sums == NULL) {
		errp(160, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->ex_var_partial_sums",
		     state->tp.n, sizeof(long int));
	}

	/*
	 * Allocate dynamic arrays
	 */
	state->stats[test_num] = create_dyn_array(sizeof(struct RandomExcursionsVariant_private_stats),
						  DEFAULT_CHUNK, state->tp.numOfBitStreams, false);	// stats.txt
	state->p_val[test_num] = create_dyn_array(sizeof(double), DEFAULT_CHUNK, NUMBER_OF_STATES_RND_EXCURSION_VAR *
			state->tp.numOfBitStreams, false);						// results.txt

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
 * RandomExcursionsVariant_iterate - iterate one bit stream for Random Excursions Variant test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
RandomExcursionsVariant_iterate(struct state *state)
{
	struct RandomExcursionsVariant_private_stats stat;	// Stats for this iteration
	long int n;		// Length of a single bit stream
	long int *S;		// Array of the partial sums of the -1/+1 states
	double p_value;		// p_value iteration test result(s)
	long int i;
	long int j;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(161, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "iterate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	if (state->epsilon == NULL) {
		err(161, __FUNCTION__, "state->epsilon is NULL");
	}
	if (state->rnd_excursion_var_stateX == NULL) {
		err(161, __FUNCTION__, "state->rnd_excursion_var_stateX is NULL");
	}
	if (state->ex_var_partial_sums == NULL) {
		err(161, __FUNCTION__, "state->ex_var_partial_sums is NULL");
	}
	if (state->cSetup != true) {
		err(161, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_INIT && state->driver_state[test_num] != DRIVER_ITERATE) {
		err(161, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_INIT: %d and != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_INIT, DRIVER_ITERATE);
	}

	/*
	 * Collect parameters from state
	 */
	S = state->ex_var_partial_sums;
	n = state->tp.n;

	/*
	 * Set the initial value of the counter of the number of cycles
	 */
	stat.number_of_cycles = 0;

	/*
	 * Step 2: compute the partial sums of successively larger sub-sequences
	 */
	if ((int) state->epsilon[0] == 1) {
		S[0] = 1;
	} else if ((int) state->epsilon[0] == 0) {
		S[0] = - 1;
	} else {
		err(41, __FUNCTION__, "found a bit different than 1 or 0 in the sequence");
	}
	for (j = 1; j < n; j++) {
		if ((int) state->epsilon[j] == 1) {
			S[j] = S[j - 1] + 1;
		} else if ((int) state->epsilon[j] == 0) {
			S[j] = S[j - 1] - 1;
		} else {
			err(41, __FUNCTION__, "found a bit different than 1 or 0 in the sequence");
		}

		/*
		 * Step 3a: whenever a 0 in the partial sums is found, which means that a cycle has
		 * ended, count a new cycle in the counter of cycles
		 */
		if (S[j] == 0) {
			stat.number_of_cycles++;
		}
	}

	/*
	 * Step 3b: count the last cycle if it was not counted already
	 */
	if (S[n - 1] != 0) {
		stat.number_of_cycles++;
	}

	/*
	 * Step 3c: determine if there are enough cycles
	 */
	stat.test_possible = (stat.number_of_cycles < state->c.min_zero_crossings) ? false : true;

	/*
	 * Perform and record the test if it is possible to test
	 */
	if (stat.test_possible == true) {

		/*
		 * For each of the state values, compute the test statistic and the p-value
		 */
		for (i = 0; i < NUMBER_OF_STATES_RND_EXCURSION_VAR; i++) {

			/*
			 * Step 4: count times when the partial sum matches this excursion state value
			 */
			stat.counter[i] = 0;
			for (j = 0; j < n; j++) {
				if (S[j] == state->rnd_excursion_var_stateX[i]) {
					stat.counter[i]++;
				}
			}

			/*
			 * Step 5: compute the test p-value for this excursion state value
			 */
			p_value = erfc(labs(stat.counter[i] - stat.number_of_cycles)
				       / (sqrt(2.0 * stat.number_of_cycles
					       * (4.0 * labs(state->rnd_excursion_var_stateX[i]) - 2.0))));

			/*
			 * Record success or failure for this iteration
			 */
			state->count[test_num]++;	// Count this iteration
			state->valid[test_num]++;	// Count this valid iteration
			if (isNegative(p_value)) {
				state->failure[test_num]++;	// Bogus p_value < 0.0 treated as a failure
				stat.success[i] = false;	// FAILURE
				warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value: %f < 0.0\n",
				     state->curIteration, state->testNames[test_num], test_num, p_value);
			} else if (isGreaterThanOne(p_value)) {
				state->failure[test_num]++;	// Bogus p_value > 1.0 treated as a failure
				stat.success[i] = false;	// FAILURE
				warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value: %f > 1.0\n",
				     state->curIteration, state->testNames[test_num], test_num, p_value);
			} else if (p_value < state->tp.alpha) {
				state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
				state->failure[test_num]++;	// Valid p_value but too low is a failure
				stat.success[i] = false;	// FAILURE
			} else {
				state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
				state->success[test_num]++;	// Valid p_value not too low is a success
				stat.success[i] = true;		// SUCCESS
			}

			/*
			 * Record values computed during this iteration
			 */
			append_value(state->p_val[test_num], &p_value);
		}

		/*
		 * Record stats of this iteration
		 */
		append_value(state->stats[test_num], &stat);
	}

	/*
	 * Record values when the test could not be performed
	 */
	else {

		/*
		 * Count this iteration, which happens to be invalid
		 */
		state->count[test_num]++;

		/*
		 * Record statistics of this invalid iteration
		 */
		for (i = 0; i < NUMBER_OF_STATES_RND_EXCURSION_VAR; i++) {
			stat.success[i] = false;	// FAILURE
		}
		memset(stat.counter, 0, sizeof(stat.counter));
		append_value(state->stats[test_num], &stat);

		/*
		 * Record non p-value of this invalid iteration
		 */
		p_value = NON_P_VALUE;
		for (i = 0; i < NUMBER_OF_STATES_RND_EXCURSION_VAR; i++) {
			append_value(state->p_val[test_num], &p_value);
		}
	}

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
 * RandomExcursionsVariant_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct RandomExcursionsVariant_private_stats for format and print
 *      iteration       // current iteration number being printed
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 *
 * Unlike most *_print_stat() functions, the NUMBER_OF_STATES_RND_EXCURSION_VAR number of p_values
 * will be printed by repeated calls to RandomExcursionsVariant_print_stat2()
 * if the test was possible.
 */
static bool
RandomExcursionsVariant_print_stat(FILE * stream, struct state *state, struct RandomExcursionsVariant_private_stats *stat,
				   long int iteration)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(162, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(162, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(162, __FUNCTION__, "stat arg is NULL");
	}
	if (state->cSetup != true) {
		err(162, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}

	/*
	 * Print stat to a file
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
	io_ret = fprintf(stream, "\t\t(a) Number Of Cycles (J) = %ld\n", stat->number_of_cycles);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(b) Sequence Length (n)  = %ld\n", state->tp.n);
	if (io_ret <= 0) {
		return false;
	}
	if (state->legacy_output == false) {
		io_ret = fprintf(stream, "\t\t(c) Rejection Constraint = %ld\n", state->c.min_zero_crossings);
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t--------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}

	/*
	 * Case: test was not possible for this iteration
	 * Note that in this cycle, this test is not applicable
	 */
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
			io_ret = fprintf(stream, "\t\titeration %ld test not applicable\n", iteration);
			if (io_ret <= 0) {
				return false;
			}
			io_ret = fprintf(stream, "\t\texcessive cycles, J: %ld >= max expected: %lu\n\n",
					 stat->number_of_cycles, state->c.min_zero_crossings);
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
 * RandomExcursionsVariant_print_stat2 - print a excursion info for a possible test
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct RandomExcursionsVariant_private_stats for format and print
 *      p               // excursion number to print [0,NUMBER_OF_STATES_RND_EXCURSION_VAR)
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 *
 * Unlike most *_print_stat() functions, the NUMBER_OF_STATES_RND_EXCURSION_VAR number of p_values
 * will be printed by repeated calls to RandomExcursionsVariant_print_stat2()
 * with different excursion numbers.
 *
 * If the test was possible, then the excursion state, visit count and p_value are printed.
 * If the test was not possible due to an insufficient number of crossings, this function
 * does not print, just returns.
 */
static bool
RandomExcursionsVariant_print_stat2(FILE * stream, struct state *state, struct RandomExcursionsVariant_private_stats *stat,
				    long int p, double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(163, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(163, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(163, __FUNCTION__, "stat arg is NULL");
	}
	if (state->rnd_excursion_var_stateX == NULL) {
		err(163, __FUNCTION__, "state->rnd_excursion_var_stateX is NULL");
	}
	if (p < 0) {
		err(163, __FUNCTION__, "p arg: %ld must be > 0", p);
	}
	if (p >= NUMBER_OF_STATES_RND_EXCURSION_VAR) {
		err(163, __FUNCTION__, "p arg: %ld must be < %d", p, NUMBER_OF_STATES_RND_EXCURSION_VAR);
	}

	/*
	 * Nothing to print if the test was not possible
	 */
	if (stat->test_possible != true) {
		return true;
	}
	if (p_value == NON_P_VALUE && stat->success[p] == true) {
		err(163, __FUNCTION__, "p_value was set to NON_P_VALUE but stat->success[%ld] == true", p);
	}

	/*
	 * Print stat to a file
	 */
	if (stat->success[p] == true) {
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
					 state->rnd_excursion_var_stateX[p], stat->counter[p]);
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret = fprintf(stream, "(x = %2ld) Total visits = %4ld; p-value = %f\n",
					 state->rnd_excursion_var_stateX[p], stat->counter[p], p_value);
			if (io_ret <= 0) {
				return false;
			}
		}
	} else {
		if (p_value == NON_P_VALUE) {
			io_ret = fprintf(stream, "x = %2ld  visits = %4ld  p_value = __INVALID__\n",
					 state->rnd_excursion_var_stateX[p], stat->counter[p]);
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret = fprintf(stream, "x = %2ld  visits = %4ld  p_value = %f\n",
					 state->rnd_excursion_var_stateX[p], stat->counter[p], p_value);
			if (io_ret <= 0) {
				return false;
			}
		}
	}
	// extra newline on the last excursion state
	if (p >= NUMBER_OF_STATES_RND_EXCURSION_VAR - 1) {
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
 *      p_value         // p_value iteration test result(s)
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
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(164, __FUNCTION__, "stream arg is NULL");
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
 * RandomExcursionsVariant_print - print to results.txt, data*.txt, stats.txt for all iterations
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
	struct RandomExcursionsVariant_private_stats *stat;	// Pointer to statistics of an iteration
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
	long int p;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(165, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "print driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->resultstxtFlag == false) {
		dbg(DBG_LOW, "print driver interface for %s[%d] was not enabled with -s", state->testNames[test_num], test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(165, __FUNCTION__,
		    "print driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(165, __FUNCTION__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->datatxt_fmt[test_num] == NULL) {
		err(165, __FUNCTION__, "format for data0*.txt filename is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_ITERATE) {
		err(165, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_ITERATE: %d",
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
	for (i = 0, j = 0; i < state->stats[test_num]->count; ++i, j += NUMBER_OF_STATES_RND_EXCURSION_VAR) {

		/*
		 * Locate stat for this iteration
		 */
		stat = addr_value(state->stats[test_num], struct RandomExcursionsVariant_private_stats, i);

		/*
		 * Print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = RandomExcursionsVariant_print_stat(stats, state, stat, i);
		if (ok == false) {
			errp(165, __FUNCTION__, "error in writing to %s", stats_txt);
		}

		/*
		 * Print all of the excursion states for this iteration
		 */
		for (p = 0; p < NUMBER_OF_STATES_RND_EXCURSION_VAR; p++) {

			/*
			 * Get p_value for this excursion state of this iteration
			 */
			p_value = get_value(state->p_val[test_num], double, j + p);

			/*
			 * Print, if possible, the excursion success or failure, visit and p_value
			 */
			errno = 0;	// paranoia
			ok = RandomExcursionsVariant_print_stat2(stats, state, stat, p, p_value);
			if (ok == false) {
				errp(165, __FUNCTION__, "error in writing to %s", stats_txt);
			}

			/*
			 * Print p_value to results.txt
			 */
			errno = 0;	// paranoia
			ok = RandomExcursionsVariant_print_p_value(results, p_value);
			if (ok == false) {
				errp(165, __FUNCTION__, "error in writing to %s", results_txt);
			}
		}
	}

	/*
	 * Flush and close stats.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(stats);
	if (io_ret != 0) {
		errp(165, __FUNCTION__, "error flushing to: %s", stats_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(stats);
	if (io_ret != 0) {
		errp(165, __FUNCTION__, "error closing: %s", stats_txt);
	}
	free(stats_txt);
	stats_txt = NULL;

	/*
	 * Flush and close results.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(results);
	if (io_ret != 0) {
		errp(165, __FUNCTION__, "error flushing to: %s", results_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(results);
	if (io_ret != 0) {
		errp(165, __FUNCTION__, "error closing: %s", results_txt);
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
				errp(165, __FUNCTION__,
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
					ok = RandomExcursionsVariant_print_p_value(data, p_value);
					if (ok == false) {
						errp(165, __FUNCTION__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * Flush and close data*.txt, free pathname
			 */
			errno = 0;	// paranoia
			io_ret = fflush(data);
			if (io_ret != 0) {
				errp(165, __FUNCTION__, "error flushing to: %s", data_txt);
			}
			errno = 0;	// paranoia
			io_ret = fclose(data);
			if (io_ret != 0) {
				errp(165, __FUNCTION__, "error closing: %s", data_txt);
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
 * RandomExcursionsVariant_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state          		 // run state to test under
 *      sampleCount             // Number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // uniformity frequency bins
 */
static void
RandomExcursionsVariant_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
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
		err(166, __FUNCTION__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(166, __FUNCTION__, "freqPerBin arg is NULL");
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
		errp(166, __FUNCTION__, "error flushing to: %s", state->finalReptPath);
	}

	return;
}


/*
 * RandomExcursionsVariant_metrics - uniformity and proportional analysis of a test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to complete the test analysis for all iterations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
RandomExcursionsVariant_metrics(struct state *state)
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
		err(167, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(167, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(167, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with p_val length: %ld != bit streams: %ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->driver_state[test_num] != DRIVER_PRINT && state->resultstxtFlag == true) {
		err(167, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_PRINT: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_PRINT);
	} else if (state->driver_state[test_num] != DRIVER_ITERATE && state->resultstxtFlag == false) {
		err(167, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_ITERATE);
	}

	/*
	 * Allocate uniformity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(167, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
		     (long int) state->tp.uniformity_bins, sizeof(long int));
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
		RandomExcursionsVariant_metric_print(state, sampleCount, toolow, freqPerBin);

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
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(168, __FUNCTION__, "state arg is NULL");
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
	if (state->rnd_excursion_var_stateX != NULL) {
		free(state->rnd_excursion_var_stateX);
		state->rnd_excursion_var_stateX = NULL;
	}
	if (state->ex_var_partial_sums != NULL) {
		free(state->ex_var_partial_sums);
		state->ex_var_partial_sums = NULL;
	}

	/*
	 * Set driver state to DRIVER_DESTROY
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_DESTROY);
	state->driver_state[test_num] = DRIVER_DESTROY;

	return;
}
