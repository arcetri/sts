/*****************************************************************************
		     R A N D O M   E X C U R S I O N S	 T E S T
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


// Exit codes: 150 thru 159

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
struct RandomExcursions_private_stats {
	bool excursion_success[EXCURSION_TEST_CNT];	// Success or failure of iteration test for an excursion state
	long int counter[EXCURSION_TEST_CNT];	// state visit count
	double chi2[EXCURSION_TEST_CNT];	// Chi^2 sum for a given excursion state
	long int J;		// Number of cycles
	bool test_possible;	// true --> test is possible for this iteration
};


/*
 * Static const variables declarations
 */
static const enum test test_num = TEST_RND_EXCURSION;	// This test number

/*
 * pi_term - probabilities
 *
 * These constants are from SP800-22Rev1a section 3.14 (page 3-24).
 *
 * pi_term[x][y] refers to row x, column pi_y(x).
 */
static const double pi_term[EXCURSION_CLASSES][EXCURSION_FREEDOM] = {	// TODO make it compute dynamically
	{0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
	{0.5000000000, 0.25000000000, 0.12500000000, 0.06250000000, 0.03125000000, 0.0312500000},	// x = 1
	{0.7500000000, 0.06250000000, 0.04687500000, 0.03515625000, 0.02636718750, 0.0791015625},	// x = 2
	{0.8333333333, 0.02777777778, 0.02314814815, 0.01929012346, 0.01607510288, 0.0803755143},	// x = 3
	{0.8750000000, 0.01562500000, 0.01367187500, 0.01196289063, 0.01046752930, 0.0732727051},	// x = 4
};

/*
 * Forward static function declarations
 */
static bool RandomExcursions_print_stat(FILE * stream, struct state *state, struct RandomExcursions_private_stats *stat,
					long int iteration);
static bool RandomExcursions_print_stat2(FILE * stream, struct state *state, struct RandomExcursions_private_stats *stat,
					 long int p, double p_value);
static bool RandomExcursions_print_p_value(FILE * stream, double p_value);
static void RandomExcursions_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin);


/*
 * RandomExcursions_init - initialize the Random Excursions test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
RandomExcursions_init(struct state *state)
{
	long int n;		// Length of a single bit stream
	long int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(150, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->cSetup != true) {
		err(150, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_NULL && state->driver_state[test_num] != DRIVER_DESTROY) {
		err(150, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_NULL: %d and != DRIVER_DESTROY: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_NULL, DRIVER_DESTROY);
	}

	/*
	 * Collect parameters from state
	 */
	n = state->tp.n;

	// TODO n >= 10^6

	/*
	 * Allocate sum state and cycle count arrays
	 */
	state->rnd_excursion_S_k = malloc(n * sizeof(state->rnd_excursion_S_k[0]));
	if (state->rnd_excursion_S_k == NULL) {
		errp(150, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->rnd_excursion_S_k",
		     n, sizeof(long int));
	}
	state->rnd_excursion_cycle_len = MAX(1000, n / 100);	// TODO why? use dynamic array starting size sqrt(n)
	state->rnd_excursion_cycle = malloc(state->rnd_excursion_cycle_len * sizeof(state->rnd_excursion_cycle[0]));
	if (state->rnd_excursion_cycle == NULL) {
		errp(150, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->rnd_excursion_cycle",
		     state->rnd_excursion_cycle_len, sizeof(long int));
	}

	/*
	 * Create working sub-directory if forming files such as results.txt and stats.txt
	 */
	if (state->resultstxtFlag == true) {
		state->subDir[test_num] = precheckSubdir(state, state->testNames[test_num]);
		dbg(DBG_HIGH, "test %s[%d] will use subdir: %s", state->testNames[test_num], test_num, state->subDir[test_num]);
	}

	/*
	 * Allocate and initialize excursion states
	 */
	state->excursion_stateX = malloc(EXCURSION_TEST_CNT * sizeof(state->excursion_stateX[0]));
	if (state->excursion_stateX == NULL) {
		errp(150, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->excursion_stateX",
		     (long int) EXCURSION_TEST_CNT, sizeof(long int));
	}
	for (i = 1; i <= MAX_EXCURSION; ++i) {
		state->excursion_stateX[i - 1] = -MAX_EXCURSION + i - 1;;
		state->excursion_stateX[EXCURSION_TEST_CNT - MAX_EXCURSION + i - 1] = i;
	}

	/*
	 * Allocate dynamic arrays
	 */
	state->stats[test_num] = create_dyn_array(sizeof(struct RandomExcursions_private_stats),
	                                          DEFAULT_CHUNK, state->tp.numOfBitStreams, false);	// stats.txt
	state->p_val[test_num] = create_dyn_array(sizeof(double), DEFAULT_CHUNK,
	                                          EXCURSION_TEST_CNT * state->tp.numOfBitStreams, false);	// results.txt

	/*
	 * Determine format of data*.txt filenames based on state->partitionCount[test_num]
	 *
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
 * RandomExcursions_iterate - iterate one bit stream for Random Excursions test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
RandomExcursions_iterate(struct state *state)
{
	struct RandomExcursions_private_stats stat;	// Stats for this iteration
	long int n;		// Length of a single bit stream
	long int v[EXCURSION_FREEDOM][EXCURSION_TEST_CNT];	// Count of cycles where state x occurs exactly y times
	long int *cycle;	// cycle counts for an iteration
	long int *S_k;		// Sum of -1/+1 states for an iteration
	long int count_index;	// The index of the count array to be incremented
	long int b;		// sum offset
	long int x;		// a particular state to test
	long int labs_x;	// absolute value of x
	long int cycleStart;	// When a cycle starts
	long int cycleStop;	// When a cycle ends or we give up
	double p_value;		// p_value iteration test result(s)
	double sum;		// Sum of degrees of freedom
	double sum_term;	// a value whose square is used to compute sum
	long int i;
	long int j;
	long int k;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(151, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "iterate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	if (state->epsilon == NULL) {
		err(151, __FUNCTION__, "state->epsilon is NULL");
	}
	if (state->excursion_stateX == NULL) {
		err(151, __FUNCTION__, "state->excursion_stateX is NULL");
	}
	if (state->rnd_excursion_S_k == NULL) {
		err(151, __FUNCTION__, "state->rnd_excursion_S_k is NULL");
	}
	if (state->rnd_excursion_cycle_len <= 0) {
		err(151, __FUNCTION__, "state->rnd_excursion_cycle_len: %ld mst be > 0", state->rnd_excursion_cycle_len);
	}
	if (state->rnd_excursion_cycle == NULL) {
		err(151, __FUNCTION__, "state->rnd_excursion_cycle is NULL");
	}
	if (state->cSetup != true) {
		err(151, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_INIT && state->driver_state[test_num] != DRIVER_ITERATE) {
		err(151, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_INIT: %d and != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_INIT, DRIVER_ITERATE);
	}

	/*
	 * Collect parameters from state
	 */
	n = state->tp.n;
	S_k = state->rnd_excursion_S_k;
	cycle = state->rnd_excursion_cycle;

	/*
	 * Zeroize cycles
	 */
	memset(cycle, 0, state->rnd_excursion_cycle_len * sizeof(cycle[0]));

	/*
	 * Determine the number of cycles and store in the cycle array
	 * the starting position of each cycle
	 */
	stat.test_possible = true;	// assume the test is possible
	stat.J = 0;
	S_k[0] = 2 * (long int) state->epsilon[0] - 1;
	for (i = 1; i < n; i++) {
		S_k[i] = S_k[i - 1] + 2 * state->epsilon[i] - 1;
		if (S_k[i] == 0) {
			stat.J++;
			if (stat.J >= state->rnd_excursion_cycle_len) {	// TODO why? If we make a dynamic array, remove this
				/*
				 * too many cycles, test is not possible
				 */
				stat.test_possible = false;
				break;
			}

			cycle[stat.J] = i;
		}
	}

	if (stat.test_possible == true) {	// TODO and also this
		if (S_k[n - 1] != 0) {
			stat.J++;
		}

		/*
		 * Check preconditions (firewall)
		 *
		 * NOTE: The cycle[j + 1] far below requires us to check against a -1 of the rnd_excursion_cycle_len
		 */
		if (stat.J >= (state->rnd_excursion_cycle_len - 1)) {
			err(151, __FUNCTION__, "J: %ld >= (state->rnd_excursion_cycle_len: %ld - 1)",
			    stat.J, state->rnd_excursion_cycle_len - 1);
		}

		cycle[stat.J] = n;	// TODO shouldn't this be inside the if of stat.J++ ? Okay

		/*
		 * Determine if we can still test
		 */
		stat.test_possible = (stat.J < state->c.excursion_constraint) ? false : true;
	}

	/*
	 * Perform and record the test if it is possible to test
	 */
	if (stat.test_possible == true) {	// TODO and also this

		cycleStart = 0;
		cycleStop = cycle[1];
		/*
		 * NOTE: Logical code rewrite, old code commented out below:
		 *
		 * for (k = 0; k < EXCURSION_FREEDOM; k++)
		 *      for (i = 0; i < EXCURSION_TEST_CNT; i++)
		 *              v[k][i] = 0.0;
		 *
		 * The v[][] counters were converted to long int.
		 */

		memset(v, 0, sizeof(v));

		/*
		 * For each cycle
		 */
		for (j = 1; j <= stat.J; j++) {
			/*
			 * NOTE: Logical code rewrite, old code commented out below:
			 *
			 * for (i = 0; i < EXCURSION_TEST_CNT; i++)
			 *      stat.counter[i] = 0;
			 */
			memset(stat.counter, 0, sizeof(stat.counter));

			/*
			 * Check preconditions (firewall)
			 */
			if (cycleStart < 0) {
				err(151, __FUNCTION__, "cycleStart: %ld < 0", cycleStart);
			}
			if (cycleStart > n) {
				err(151, __FUNCTION__, "cycleStart: %ld > n: %ld", cycleStart, n);
			}
			if (cycleStop < 0) {
				err(151, __FUNCTION__, "cycleStop: %ld < 0", cycleStop);
			}
			if (cycleStop > n) {
				err(151, __FUNCTION__, "cycleStop: %ld > n: %ld", cycleStop, n);
			}

			/*
			 * For each partial sum value of the cycle
			 */
			for (i = cycleStart; i < cycleStop; i++) {
				/*
				 * NOTE: Logical code rewrite, old code commented out below:
				 *
				 * if ( (S_k[i] >= 1 && S_k[i] <= 4) || (S_k[i] >= -4 && S_k[i] <= -1) ) {
				 * ...
				 * }
				 */

				/*
				 * if the value of the partial sum is of interest
				 */
				if ((labs(S_k[i]) >= 1) && (labs(S_k[i]) <= MAX_EXCURSION)) {
					/*
					 * NOTE: Logical code rewrite, old code commented out below:
					 *
					 * if ( S_k[i] < 0 )
					 *      b = 4;
					 * else
					 *      b = 3;
					 */

					/*
					 * Get the corresponding index for the counter array. This is done because the array cannot
					 * have negative indexes. Thus, the most negative x will correspond to the item in index 0
					 */
					if (S_k[i] < 0) {
						b = MAX_EXCURSION;
					} else {
						b = MAX_EXCURSION - 1;
					}

					count_index = S_k[i] + b;

					/*
					 * Check preconditions (firewall)
					 */
					if (count_index < 0) {
						err(151, __FUNCTION__, "count_index: %ld < 0", count_index);
					}
					if (count_index >= state->rnd_excursion_cycle_len) {
						err(151, __FUNCTION__, "count_index: %ld >= state->count_index: %ld",
						    count_index, state->rnd_excursion_cycle_len);
					}

					/*
					 * increase the counter for the considered state value
					 */
					stat.counter[count_index]++;
				}
			}

			/*
			 * go to the next cycle
			 */
			cycleStart = cycle[j] + 1;
			if (j < stat.J) {
				cycleStop = cycle[j + 1];
			}

			/*
			 * NOTE: Logical code rewrite, old code commented out below:
			 *
			 * for ( i=0; i<8; i++ ) {
			 *   if ( (stat.counter[i] >= 0) && (stat.counter[i] <= 4) )
			 *        v[stat.counter[i]][i]++;
			 *   else if ( stat.counter[i] >= 5 )
			 *        v[5][i]++;
			 * }
			 */

			/*
			 * v[k][i] contains the exact number of cycles in which state i occurs exactly k times
			 */
			for (i = 0; i < EXCURSION_TEST_CNT; i++) {
				if ((stat.counter[i] >= 0) && (stat.counter[i] <= (EXCURSION_FREEDOM - 2))) {
					v[stat.counter[i]][i]++;
				} else if (stat.counter[i] >= EXCURSION_FREEDOM - 1) {
					v[EXCURSION_FREEDOM - 1][i]++;
				}
			}
		}

		/*
		 * For each of the 8 states, compute the test statistic 8 p-values will be produced by this test
		 */
		for (i = 0; i < EXCURSION_TEST_CNT; i++) {

			/*
			 * x is the real value of the state (can be negative), where i was only the index
			 */
			x = state->excursion_stateX[i];
			labs_x = labs(x);

			/*
			 * Check preconditions (firewall)
			 */
			if (labs_x > MAX_EXCURSION) {
				err(151, __FUNCTION__, "labs(%ld): %ld > MAX_EXCURSION: %d", x, labs_x, MAX_EXCURSION);
			}

			sum = 0.0;
			for (k = 0; k < EXCURSION_FREEDOM; k++) {
				/*
				 * NOTE: Mathematical expression code rewrite, old code commented out below:
				 *
				 * sum += pow(v[k][i] - stat.J * pi_term[labs(x)][k], 2) / (stat.J * pi_term[labs(x)][k]);
				 */
				sum_term = (double) v[k][i] - ((double) stat.J * pi_term[labs(x)][k]);
				sum += sum_term * sum_term / ((double) stat.J * pi_term[labs(x)][k]);
			}

			stat.chi2[i] = sum;

			/*
			 * NOTE: Mathematical expression code rewrite, old code commented out below:
			 *
			 * p_value = cephes_igamc(2.5, sum/2.0);
			 */
			p_value = cephes_igamc((double) (EXCURSION_FREEDOM - 1) / 2.0, sum / 2.0);

			/*
			 * Record testable test success or failure
			 */
			state->count[test_num]++;	// Count this test
			state->valid[test_num]++;	// Count this valid test
			if (isNegative(p_value)) {
				state->failure[test_num]++;	// Bogus p_value < 0.0 treated as a failure
				stat.excursion_success[i] = false;	// FAILURE
				warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value: %f < 0.0\n",
				     state->curIteration, state->testNames[test_num], test_num, p_value);
			} else if (isGreaterThanOne(p_value)) {
				state->failure[test_num]++;	// Bogus p_value > 1.0 treated as a failure
				stat.excursion_success[i] = false;	// FAILURE
				warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value: %f > 1.0\n",
				     state->curIteration, state->testNames[test_num], test_num, p_value);
			} else if (p_value < state->tp.alpha) {
				state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
				state->failure[test_num]++;	// Valid p_value but too low is a failure
				stat.excursion_success[i] = false;	// FAILURE
			} else {
				state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
				state->success[test_num]++;	// Valid p_value not too low is a success
				stat.excursion_success[i] = true;	// SUCCESS
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

		/*
		 * Count this test, which happens to be invalid
		 */
		state->count[test_num]++;

		/*
		 * results.txt accounting
		 */
		p_value = NON_P_VALUE;
		for (i = 0; i < EXCURSION_TEST_CNT; i++) {
			append_value(state->p_val[test_num], &p_value);
		}

		/*
		 * stats.txt accounting
		 */
		for (i = 0; i < EXCURSION_TEST_CNT; i++) {
			stat.excursion_success[i] = false;	// FAILURE
		}
		/*
		 * NOTE: Logical code rewrite, old code commented out below:
		 *
		 * for (i = 0; i < EXCURSION_TEST_CNT; i++) {
		 *      stat.counter[i] = 0;
		 * }
		 */
		memset(stat.counter, 0, sizeof(stat.counter));
		append_value(state->stats[test_num], &stat);
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
 * RandomExcursions_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct RandomExcursions_private_stats for format and print
 *      iteration       // current iteration number being printed
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 *
 * Unlike most *_print_stat() functions, the EXCURSION_TEST_CNT number of p_values
 * will be printed by repeated calls to RandomExcursions_print_stat2()
 * if the test was possible.
 */
static bool
RandomExcursions_print_stat(FILE * stream, struct state *state, struct RandomExcursions_private_stats *stat, long int iteration)
{
	int io_ret;		// I/O return status
	long int n;		// Length of a single bit stream

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(152, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(152, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(152, __FUNCTION__, "stat arg is NULL");
	}
	if (state->cSetup != true) {
		err(152, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}

	/*
	 * Collect parameters from state
	 */
	n = state->tp.n;

	/*
	 * Print stat to a file
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t\t  RANDOM EXCURSIONS TEST\n");
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
		io_ret = fprintf(stream, "\t\t\t  Random excursions test\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t--------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t(a) Number Of Cycles (J) = %04ld\n", stat->J);
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\t(a) Number Of Cycles (J) = %ld\n", stat->J);
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t(b) Sequence Length (n)  = %ld\n", n);
	if (io_ret <= 0) {
		return false;
	}

	/*
	 * Case: test was not possible for this iteration
	 */
	if (stat->test_possible == false) {
		if (state->legacy_output == true) {

			/*
			 * Note that in this cycle, this test is not applicable
			 */
			io_ret = fprintf(stream, "\t\t---------------------------------------------\n");
			if (io_ret <= 0) {
				return false;
			}
			io_ret = fprintf(stream, "\t\tWARNING:  TEST NOT APPLICABLE.  THERE ARE AN\n");
			if (io_ret <= 0) {
				return false;
			}
			io_ret = fprintf(stream, "\t\t\t  INSUFFICIENT NUMBER OF CYCLES.\n");
			if (io_ret <= 0) {
				return false;
			}
			io_ret = fprintf(stream, "\t\t---------------------------------------------\n\n");
			if (io_ret <= 0) {
				return false;
			}

		} else {
			/*
			 * note which cycle is not applicable
			 */
			io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
			if (io_ret <= 0) {
				return false;
			}
			io_ret = fprintf(stream, "\t\titeration %ld test not applicable\n", iteration);
			if (io_ret <= 0) {
				return false;
			}

			/*
			 * case: too many cycles
			 */
			if (stat->J >= state->rnd_excursion_cycle_len) {
				io_ret = fprintf(stream, "\t\texcessive cycles, J: %ld >= max expected: %lu\n\n",
						 stat->J, state->rnd_excursion_cycle_len);
				if (io_ret <= 0) {
					return false;
				}

				/*
				 * case: too few cycles
				 */
			} else {
				io_ret =
				    fprintf(stream, "\t\tinsufficient cycles, %ld < %ld\n\n", stat->J,
					    state->c.excursion_constraint);
				if (io_ret <= 0) {
					return false;
				}
			}
		}

		/*
		 * Case: test was possible for this iteration
		 */
	} else {
		if (state->legacy_output == true) {
			io_ret = fprintf(stream, "\t\t(c) Rejection Constraint = %f\n", (double) state->c.excursion_constraint);
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret = fprintf(stream, "\t\t(c) Rejection Constraint = %ld\n", state->c.excursion_constraint);
			if (io_ret <= 0) {
				return false;
			}
			io_ret = fprintf(stream, "\t\tmax expected cycles	   = %ld\n", state->rnd_excursion_cycle_len);
			if (io_ret <= 0) {
				return false;
			}
		}
		io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
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
 * RandomExcursions_print_stat2 - print a excursion info for a possible test
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct RandomExcursions_private_stats for format and print
 *      p               // excursion number to print [0,EXCURSION_TEST_CNT)
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 *
 * Unlike most *_print_stat() functions, the EXCURSION_TEST_CNT number of p_values
 * will be printed by repeated calls to RandomExcursions_print_stat2()
 * with differet excursion numbers.
 *
 * If the test was possible, then the excursion state, visit cound and p_value are printed.
 * If the test was not possible due to an insufficent number of crossings, this function
 * does not print, just returns.
 */
static bool
RandomExcursions_print_stat2(FILE * stream, struct state *state, struct RandomExcursions_private_stats *stat, long int p,
			     double p_value)
{
	int io_ret;		// I/O return status

	// need to cal this function with stat.test_possible == true

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(153, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(153, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(153, __FUNCTION__, "stat arg is NULL");
	}
	if (state->excursion_stateX == NULL) {
		err(153, __FUNCTION__, "state->excursion_stateX is NULL");
	}
	if (p < 0) {
		err(153, __FUNCTION__, "p arg: %ld must be > 0", p);
	}
	if (p >= EXCURSION_TEST_CNT) {
		err(153, __FUNCTION__, "p arg: %ld must be < %d", p, EXCURSION_TEST_CNT);
	}

	/*
	 * Nothint to print if the test was not possible
	 */
	if (stat->test_possible != true) {
		return true;
	}
	if (p_value == NON_P_VALUE && stat->excursion_success[p] == true) {
		err(153, __FUNCTION__, "p_value was set to NON_P_VALUE but stat->excursion_success[%ld] == true", p);
	}

	/*
	 * Print stat to a file
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
			io_ret =
			    fprintf(stream, "x = %2ld chi^2 = %9.6f p_value = __INVALID__\n", state->excursion_stateX[p],
				    stat->chi2[p]);
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret =
			    fprintf(stream, "x = %2ld chi^2 = %9.6f p_value = %f\n", state->excursion_stateX[p], stat->chi2[p],
				    p_value);
			if (io_ret <= 0) {
				return false;
			}
		}
	} else {
		if (p_value == NON_P_VALUE) {
			io_ret = fprintf(stream, "x = %2ld	visits = %4ld  p_value = __INVALID__\n",
					 state->excursion_stateX[p], stat->counter[p]);
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret = fprintf(stream, "x = %2ld	visits = %4ld  p_value = %f\n",
					 state->excursion_stateX[p], stat->counter[p], p_value);
			if (io_ret <= 0) {
				return false;
			}
		}
	}
	// extra newline on the last excursion state
	if (p >= EXCURSION_TEST_CNT - 1) {
		io_ret = fputc('\n', stream);
		if (io_ret == EOF) {
			return false;
		}
	}
	return true;
}


/*
 * RandomExcursions_print_p_value - print p_value information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      stat            // struct RandomExcursions_private_stats for format and print
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
RandomExcursions_print_p_value(FILE * stream, double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(154, __FUNCTION__, "stream arg is NULL");
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
 * RandomExcursions_print - print to results.txt, data*.txt, stats.txt for all iterations
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
RandomExcursions_print(struct state *state)
{
	struct RandomExcursions_private_stats *stat;	// pointer to statistics of an iteration
	double p_value;		// p_value iteration test result(s)
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
	long int p;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(155, __FUNCTION__, "state arg is NULL");
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
		err(155, __FUNCTION__,
		    "print driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(155, __FUNCTION__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->datatxt_fmt[test_num] == NULL) {
		err(155, __FUNCTION__, "format for data0*.txt filename is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_ITERATE) {
		err(155, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_ITERATE);
	}

	/*
	 * Open stats.txt file
	 */
	stats_txt = filePathName(state->subDir[test_num], "stats.txt");
	dbg(DBG_MED, "about to open/truncate: %s", stats_txt);
	stats = openTruncate(stats_txt);

	/*
	 * Open results.txt file
	 */
	results_txt = filePathName(state->subDir[test_num], "results.txt");
	dbg(DBG_MED, "about to open/truncate: %s", results_txt);
	results = openTruncate(results_txt);

	/*
	 * Write results.txt and stats.txt files
	 */
	for (i = 0, j = 0; i < state->stats[test_num]->count; ++i, j += EXCURSION_TEST_CNT) {

		/*
		 * Locate stat for this iteration
		 */
		stat = addr_value(state->stats[test_num], struct RandomExcursions_private_stats, i);

		/*
		 * Print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = RandomExcursions_print_stat(stats, state, stat, i);
		if (ok == false) {
			errp(155, __FUNCTION__, "error in writing to %s", stats_txt);
		}

		/*
		 * Print all of the excursion states for this iteration
		 */
		for (p = 0; p < EXCURSION_TEST_CNT; p++) {

			/*
			 * Get p_value for this excursion state of this iteration
			 */
			p_value = get_value(state->p_val[test_num], double, j + p);

			/*
			 * Print, if possible, the excursion success or failure, visit and p_value
			 */
			errno = 0;	// paranoia
			ok = RandomExcursions_print_stat2(stats, state, stat, p, p_value);
			if (ok == false) {
				errp(155, __FUNCTION__, "error in writing to %s", stats_txt);
			}

			/*
			 * Print p_value to results.txt
			 */
			errno = 0;	// paranoia
			ok = RandomExcursions_print_p_value(results, p_value);
			if (ok == false) {
				errp(155, __FUNCTION__, "error in writing to %s", results_txt);
			}
		}
	}

	/*
	 * Flush and close stats.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(stats);
	if (io_ret != 0) {
		errp(155, __FUNCTION__, "error flushing to: %s", stats_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(stats);
	if (io_ret != 0) {
		errp(155, __FUNCTION__, "error closing: %s", stats_txt);
	}
	free(stats_txt);
	stats_txt = NULL;

	/*
	 * Flush and close results.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(results);
	if (io_ret != 0) {
		errp(155, __FUNCTION__, "error flushing to: %s", results_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(results);
	if (io_ret != 0) {
		errp(155, __FUNCTION__, "error closing: %s", results_txt);
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
				errp(155, __FUNCTION__,
				     "snprintf failed for %d bytes for data%03ld.txt, returned: %d", BUFSIZ, j + 1, snprintf_ret);
			}

			/*
			 * Form the data*.txt filename
			 */
			data_txt = filePathName(state->subDir[test_num], data_filename);
			dbg(DBG_MED, "about to open/truncate: %s", data_txt);
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
					ok = RandomExcursions_print_p_value(data, p_value);
					if (ok == false) {
						errp(155, __FUNCTION__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * Flush and close data*.txt, free pathname
			 */
			errno = 0;	// paranoia
			io_ret = fflush(data);
			if (io_ret != 0) {
				errp(155, __FUNCTION__, "error flushing to: %s", data_txt);
			}
			errno = 0;	// paranoia
			io_ret = fclose(data);
			if (io_ret != 0) {
				errp(155, __FUNCTION__, "error closing: %s", data_txt);
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
 * RandomExcursions_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state           // run state to test under
 *      sampleCount             // Number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // Uniformity frequency bins
 */
static void
RandomExcursions_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
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
		err(156, __FUNCTION__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(156, __FUNCTION__, "freqPerBin arg is NULL");
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
		errp(156, __FUNCTION__, "error flushing to: %s", state->finalReptPath);
	}

	return;
}


/*
 * RandomExcursions_metrics - uniformity and proportional analysis of a test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to complete the test analysis for all iterations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
RandomExcursions_metrics(struct state *state)
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
		err(157, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(157, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(157, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with p_val length: %ld != bit streams: %ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->driver_state[test_num] != DRIVER_PRINT) {
		err(157, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_PRINT: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_PRINT);
	}

	/*
	 * Allocate uniformity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(157, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
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
		RandomExcursions_metric_print(state, sampleCount, toolow, freqPerBin);

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
 * RandomExcursions_destroy - post process results for this text
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to cleanup any storage or state
 * associated with this test.
 */
void
RandomExcursions_destroy(struct state *state)
{
	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(158, __FUNCTION__, "state arg is NULL");
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
	if (state->excursion_stateX != NULL) {
		free(state->excursion_stateX);
		state->excursion_stateX = NULL;
	}
	if (state->rnd_excursion_S_k != NULL) {
		free(state->rnd_excursion_S_k);
		state->rnd_excursion_S_k = NULL;
	}
	if (state->rnd_excursion_cycle != NULL) {
		free(state->rnd_excursion_cycle);
		state->rnd_excursion_cycle = NULL;
		state->rnd_excursion_cycle_len = 0;
	}

	/*
	 * Set driver state to DRIVER_DESTROY
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_DESTROY);
	state->driver_state[test_num] = DRIVER_DESTROY;
	return;
}
