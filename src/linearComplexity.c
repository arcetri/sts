/*****************************************************************************
		   L I N E A R	 C O M P L E X I T Y   T E S T
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


// Exit codes: 100 thru 109

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "externs.h"
#include "cephes.h"
#include "utilities.h"
#include "debug.h"


/*
 * Private stats - stats.txt information for this test
 */
struct LinearComplexity_private_stats {
	bool success;		// Success or failure of iteration test
	long int discarded;	// bit discarded from the test
	long int v[LINEARCOMPLEXITY_K_DEGREES + 1];	// T range count
	double chi2;		// chi^2 of test
};


/*
 * Static const variables declarations
 */
static const enum test test_num = TEST_LINEARCOMPLEXITY;	// This test number

/*
 * pi_term - theoretical probabilities of classes
 *
 * See SP800-22Rev1a section 3.10.
 */
static const double pi_term[LINEARCOMPLEXITY_K_DEGREES + 1] = { 0.01047, 0.03125, 0.12500, 0.50000, 0.25000, 0.06250, 0.020833 };


/*
 * Forward static function declarations
 */
static bool LinearComplexity_print_stat(FILE * stream, struct state *state, struct LinearComplexity_private_stats *stat,
					double p_value);
static bool LinearComplexity_print_p_value(FILE * stream, double p_value);
static void LinearComplexity_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin);


/*
 * LinearComplexity_init - initialize the Linear Complexity test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
LinearComplexity_init(struct state *state)
{
	long int M;		// Linear Complexity Test - block length

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(100, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->cSetup != true) {
		err(100, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_NULL && state->driver_state[test_num] != DRIVER_DESTROY) {
		err(100, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_NULL: %d and != DRIVER_DESTROY: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_NULL, DRIVER_DESTROY);
	}
	// TODO n >= 10^6. The value of M must be in the range 500 <= M <= 5000, and N >= 200 for the Chi^2 result to be valid

	/*
	 * Collect parameters from state
	 */
	M = state->tp.linearComplexitySequenceLength;

	/*
	 * Disable test if conditions do not permit this test from being run
	 */
	if (M < MIN_LINEARCOMPLEXITY) {
		warn(__FUNCTION__, "-P 6: linear complexity sequence length(M): %ld must be >= %d", M, MIN_LINEARCOMPLEXITY);
		state->testVector[test_num] = false;
		return;
	} else if (M > MAX_LINEARCOMPLEXITY + 1) {
		warn(__FUNCTION__, "-P 6: linear complexity sequence length(M): %ld must be <= %d", M, MAX_LINEARCOMPLEXITY + 1);
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
	 * Allocate special Linear Feedback Shift Register arrays
	 */
	state->linear_b = malloc(state->tp.linearComplexitySequenceLength * sizeof(state->linear_b[0]));
	if (state->linear_b == NULL) {
		errp(100, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->linear_b",
		     state->tp.linearComplexitySequenceLength, sizeof(BitSequence));
	}
	state->linear_c = malloc(state->tp.linearComplexitySequenceLength * sizeof(state->linear_c[0]));
	if (state->linear_c == NULL) {
		errp(100, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->linear_c",
		     state->tp.linearComplexitySequenceLength, sizeof(BitSequence));
	}
	state->linear_p = malloc(state->tp.linearComplexitySequenceLength * sizeof(state->linear_p[0]));
	if (state->linear_p == NULL) {
		errp(100, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->linear_p",
		     state->tp.linearComplexitySequenceLength, sizeof(BitSequence));
	}
	state->linear_t = malloc(state->tp.linearComplexitySequenceLength * sizeof(state->linear_t[0]));
	if (state->linear_t == NULL) {
		errp(100, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->linear_t",
		     state->tp.linearComplexitySequenceLength, sizeof(BitSequence));
	}

	/*
	 * Allocate dynamic arrays
	 */
	state->stats[test_num] = create_dyn_array(sizeof(struct LinearComplexity_private_stats),
	                                          DEFAULT_CHUNK, state->tp.numOfBitStreams, false);	// stats.txt
	state->p_val[test_num] = create_dyn_array(sizeof(double),
	                                          DEFAULT_CHUNK, state->tp.numOfBitStreams, false);	// results.txt

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
 * LinearComplexity_iterate - iterate one bit stream for Linear Complexity test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
LinearComplexity_iterate(struct state *state)
{
	struct LinearComplexity_private_stats stat;	// Stats for this iteration
	long int M;		// Linear Complexity Test - block length
	long int n;		// Length of a single bit stream
	long int N;		// Number of independent blocks the bit stream is partitioned into
	long int j;
	long int d;		// discrepancy for LFSR algorithm
	long int L;
	long int m;
	long int N_;
	double p_value;		// p_value iteration test result(s)
	double T_;
	double mean;
	long int i;
	long int ii;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(101, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "iterate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	if (state->epsilon == NULL) {
		err(101, __FUNCTION__, "state->epsilon is NULL");
	}
	if (state->linear_b == NULL) {
		err(101, __FUNCTION__, "state->linear_b is NULL");
	}
	if (state->linear_c == NULL) {
		err(101, __FUNCTION__, "state->linear_c is NULL");
	}
	if (state->linear_p == NULL) {
		err(101, __FUNCTION__, "state->linear_p is NULL");
	}
	if (state->linear_t == NULL) {
		err(101, __FUNCTION__, "state->linear_t is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_INIT && state->driver_state[test_num] != DRIVER_ITERATE) {
		err(101, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_INIT: %d and != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_INIT, DRIVER_ITERATE);
	}

	/*
	 * Collect parameters from state
	 */
	M = state->tp.linearComplexitySequenceLength;
	n = state->tp.n;
	stat.discarded = n % M;
	/*
	 * NOTE: Mathematical expression code rewrite, old code commented out below:
	 *
	 * N = (int) floor(n / M);
	 */
	N = n / M;

	/*
	 * Zeroize the nu counters
	 */
	/*
	 * NOTE: Logical code rewrite, old code commented out below:
	 *
	 * for (i = 0; i < LINEARCOMPLEXITY_K_DEGREES + 1; i++) {
	 *      stat.nu[i] = 0;
	 * }
	 */
	memset(stat.v, 0, (LINEARCOMPLEXITY_K_DEGREES + 1) * sizeof(stat.v[0]));

	/*
	 * Perform the test for each independent block
	 */
	for (ii = 0; ii < N; ii++) {

		/*
		 * Zeroize working LSRF storage
		 */
		/*
		 * NOTE: Logical code rewrite, old code commented out below:
		 *
		 * for (i = 0; i < M; ++i) {
		 *      state->linear_b[i] = 0;
		 *      state->linear_c[i] = 0;
		 *      state->linear_p[i] = 0;
		 *      state->linear_t[i] = 0;
		 * }
		 */
		memset(state->linear_b, 0, M * sizeof(state->linear_b[0]));
		memset(state->linear_c, 0, M * sizeof(state->linear_c[0]));
		memset(state->linear_p, 0, M * sizeof(state->linear_p[0]));
		memset(state->linear_t, 0, M * sizeof(state->linear_t[0]));

		/*
		 * Determine linear complexity with the Berlekamp-Massey algorithm.
		 * NOTE: here we use the version specialized for the binary finite field F2
		 * REFERENCE: https://goo.gl/Um0YUr
		 */

		/*
		 * Initialize values as specified in step 2 and 3 of the test NOTE: M is always equal to (N_ - 1)
		 */
		L = 0;
		m = -1;
		state->linear_c[0] = 1;
		state->linear_b[0] = 1;

		/*
		 * loop as specified in step 4 NB: N_ is N; M is n
		 */
		for (N_ = 0; N_ < M; N_++) {

			/*
			 * set the discrepancy
			 */
			d = (int) state->epsilon[ii * M + N_];
			for (i = 1; i <= L; i++) {
				d += state->linear_c[i] * state->epsilon[ii * M + N_ - i];
			}

			d = d % 2;
			if (d == 1) {
				/*
				 * NOTE: Logical code rewrite, old code commented out below:
				 *
				 * for (i = 0; i < M; i++) {
				 *      state->linear_t[i] = state->linear_c[i];
				 *      state->linear_p[i] = 0;
				 * }
				 */

				/*
				 * let t be a copy of c
				 */
				memcpy(state->linear_t, state->linear_c, M * sizeof(state->linear_t[0]));

				/*
				 * this piece of code is letting P be like B, with values shifted by 1 position to the right:
				 * P[j+1] = B[j]
				 */
				memset(state->linear_p, 0, M * sizeof(state->linear_p[0]));
				for (j = 0; j < M; j++) {
					if (state->linear_b[j] == 1) {
						state->linear_p[j + N_ - m] = 1;
					}
				}

				/*
				 * update c array
				 */
				for (i = 0; i < M; i++) {
					state->linear_c[i] = (BitSequence) ((state->linear_c[i] + state->linear_p[i]) % 2);
				}

				/*
				 * update L, M and b
				 */
				if (L <= N_ / 2) {
					L = N_ + 1 - L;
					m = N_;
					for (i = 0; i < M; i++) {
						state->linear_b[i] = state->linear_t[i];
					}
				}
			}
		}

		/*
		 * NOTE: Mathematical expression code rewrite, old code commented out below:
		 *
		 * The value M is forced to be: 500 <= M <= 5000, so 1.0 / pow(2, M) is very close to 0.
		 * It is so close to zero that the final term is dropped to avoid overflow / underflow.
		 *
		 * if ((parity = (M + 1) % 2) == 0)
		 * sign = -1;
		 * else
		 * sign = 1;
		 * mean = M / 2.0 + (9.0 + sign) / 36.0 - 1.0 / pow(2, M) * (M / 3.0 + 2.0 / 9.0);
		 */

		/*
		 * Determine mean
		 * NOTE: the if is checking if (M + 1) is even or odd
		 */
		if ((M + 1) % 2 == 0) {
			mean = (M / 2.0) + (10.0) / 36.0;
		} else {	// if (M + 1) is odd
			mean = (M / 2.0) + (8.0) / 36.0;
		}

		/*
		 * NOTE: Mathematical expression code rewrite, old code commented out below:
		 *
		 * if ((parity = M % 2) == 0)
		 * sign = 1;
		 * else
		 * sign = -1;
		 * T_ = sign * (L - mean) + 2.0 / 9.0;
		 */

		/*
		 * Determine T value
		 * NOTE: the if is checking if M is even or odd
		 */
		if (M % 2 == 0) {
			T_ = (L - mean) + 2.0 / 9.0;
		} else {
			T_ = (mean - L) + 2.0 / 9.0;
		}

		/*
		 * Count T ranges // TODO shall we make this K dependent?
		 */
		if (T_ <= -2.5) {
			stat.v[0]++;
		} else if (T_ > -2.5 && T_ <= -1.5) {
			stat.v[1]++;
		} else if (T_ > -1.5 && T_ <= -0.5) {
			stat.v[2]++;
		} else if (T_ > -0.5 && T_ <= 0.5) {
			stat.v[3]++;
		} else if (T_ > 0.5 && T_ <= 1.5) {
			stat.v[4]++;
		} else if (T_ > 1.5 && T_ <= 2.5) {
			stat.v[5]++;
		} else {
			stat.v[6]++;
		}
	}

	/*
	 * evaluate the test
	 */
	stat.chi2 = 0.0;
	for (i = 0; i < LINEARCOMPLEXITY_K_DEGREES + 1; i++) {
		/*
		 * NOTE: Mathematical expression code rewrite, old code commented out below:
		 *
		 * stat.chi2 += pow(nu[i] - N * pi_term[i], 2) / (N * pi_term[i]);
		 */
		stat.chi2 += (stat.v[i] - N * pi_term[i]) * (stat.v[i] - N * pi_term[i]) / (N * pi_term[i]);
	}
	p_value = cephes_igamc(LINEARCOMPLEXITY_K_DEGREES / 2.0, stat.chi2 / 2.0);

	/*
	 * Record testable test success or failure
	 */
	state->count[test_num]++;	// Count this test
	state->valid[test_num]++;	// Count this valid test
	if (isNegative(p_value)) {
		state->failure[test_num]++;	// Bogus p_value < 0.0 treated as a failure
		stat.success = false;	// FAILURE
		warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value: %f < 0.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value);
	} else if (isGreaterThanOne(p_value)) {
		state->failure[test_num]++;	// Bogus p_value > 1.0 treated as a failure
		stat.success = false;	// FAILURE
		warn(__FUNCTION__, "iteration %ld of test %s[%d] produced bogus p_value: %f > 1.0\n",
		     state->curIteration, state->testNames[test_num], test_num, p_value);
	} else if (p_value < state->tp.alpha) {
		state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
		state->failure[test_num]++;	// Valid p_value but too low is a failure
		stat.success = false;	// FAILURE
	} else {
		state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
		state->success[test_num]++;	// Valid p_value not too low is a success
		stat.success = true;	// SUCCESS
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
 * LinearComplexity_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct LinearComplexity_private_stats for format and print
 *      p_value         // p_value iteration test result(s)// p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
LinearComplexity_print_stat(FILE * stream, struct state *state, struct LinearComplexity_private_stats *stat, double p_value)
{
	int io_ret;		// I/O return status
	long int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(102, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(102, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(102, __FUNCTION__, "stat arg is NULL");
	}
	if (p_value == NON_P_VALUE && stat->success == true) {
		err(102, __FUNCTION__, "p_value was set to NON_P_VALUE but stat->success == true");
	}

	/*
	 * Print stat to a file
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "-----------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\tL I N E A R	 C O M P L E X I T Y\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "-----------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\tM (substring length)     = %ld\n", state->tp.linearComplexitySequenceLength);
		if (io_ret <= 0) {
			return false;
		}
		io_ret =
		    fprintf(stream, "\tN (number of substrings) = %ld\n", state->tp.n / state->tp.linearComplexitySequenceLength);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "-----------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "	  F R E Q U E N C Y			       \n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "-----------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "  C0	 C1   C2   C3	C4   C5	  C6	CHI2	P-value\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "-----------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\tNote: %ld bits were discarded!\n", stat->discarded);
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\tLinear complexity\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\tM (substring length)     = %ld\n", state->tp.linearComplexitySequenceLength);
		if (io_ret <= 0) {
			return false;
		}
		io_ret =
		    fprintf(stream, "\t\tN (number of substrings) = %ld\n", state->tp.n / state->tp.linearComplexitySequenceLength);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\tbits discarded	       = %ld\n", stat->discarded);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t	      T range count\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t	C0   C1	  C2   C3   C4	 C5   C6    CHI2\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t--------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	for (i = 0; i < LINEARCOMPLEXITY_K_DEGREES + 1; i++) {
		io_ret = fprintf(stream, "%4ld ", stat->v[i]);
		if (io_ret <= 0) {
			return false;
		}
	}
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "%9.6f%9.6f\n", stat->chi2, p_value);
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "%9.6f\n", stat->chi2);
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
	}

	/*
	 * All printing successful
	 */
	return true;
}


/*
 * LinearComplexity_print_p_value - print p_value information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      stat            // struct LinearComplexity_private_stats for format and print
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
LinearComplexity_print_p_value(FILE * stream, double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(103, __FUNCTION__, "stream arg is NULL");
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
 * LinearComplexity_print - print to results.txt, data*.txt, stats.txt for all iterations
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
LinearComplexity_print(struct state *state)
{
	struct LinearComplexity_private_stats *stat;	// pointer to statistics of an iteration
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

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(104, __FUNCTION__, "state arg is NULL");
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
		err(104, __FUNCTION__,
		    "print driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(104, __FUNCTION__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->datatxt_fmt[test_num] == NULL) {
		err(104, __FUNCTION__, "format for data0*.txt filename is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_ITERATE) {
		err(104, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_ITERATE: %d",
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
		stat = addr_value(state->stats[test_num], struct LinearComplexity_private_stats, i);

		/*
		 * Get p_value for this iteration
		 */
		p_value = get_value(state->p_val[test_num], double, i);

		/*
		 * Print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = LinearComplexity_print_stat(stats, state, stat, p_value);
		if (ok == false) {
			errp(104, __FUNCTION__, "error in writing to %s", stats_txt);
		}

		/*
		 * Print p_value to results.txt
		 */
		errno = 0;	// paranoia
		ok = LinearComplexity_print_p_value(results, p_value);
		if (ok == false) {
			errp(104, __FUNCTION__, "error in writing to %s", results_txt);
		}
	}

	/*
	 * Flush and close stats.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(stats);
	if (io_ret != 0) {
		errp(104, __FUNCTION__, "error flushing to: %s", stats_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(stats);
	if (io_ret != 0) {
		errp(104, __FUNCTION__, "error closing: %s", stats_txt);
	}
	free(stats_txt);
	stats_txt = NULL;

	/*
	 * Flush and close results.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(results);
	if (io_ret != 0) {
		errp(104, __FUNCTION__, "error flushing to: %s", results_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(results);
	if (io_ret != 0) {
		errp(104, __FUNCTION__, "error closing: %s", results_txt);
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
				errp(104, __FUNCTION__,
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
					ok = LinearComplexity_print_p_value(data, p_value);
					if (ok == false) {
						errp(104, __FUNCTION__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * Flush and close data*.txt, free pathname
			 */
			errno = 0;	// paranoia
			io_ret = fflush(data);
			if (io_ret != 0) {
				errp(104, __FUNCTION__, "error flushing to: %s", data_txt);
			}
			errno = 0;	// paranoia
			io_ret = fclose(data);
			if (io_ret != 0) {
				errp(104, __FUNCTION__, "error closing: %s", data_txt);
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
 * LinearComplexity_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state           // run state to test under
 *      sampleCount             // Number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // Uniformity frequency bins
 */
static void
LinearComplexity_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
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
		err(105, __FUNCTION__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(105, __FUNCTION__, "freqPerBin arg is NULL");
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
		errp(105, __FUNCTION__, "error flushing to: %s", state->finalReptPath);
	}

	return;
}


/*
 * LinearComplexity_metrics - uniformity and proportional analysis of a test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to complete the test analysis for all iterations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
LinearComplexity_metrics(struct state *state)
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
		err(106, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(106, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(106, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with p_val length: %ld != bit streams: %ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->driver_state[test_num] != DRIVER_PRINT) {
		err(106, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_PRINT: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_PRINT);
	}

	/*
	 * Allocate uniformity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(106, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
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
		LinearComplexity_metric_print(state, sampleCount, toolow, freqPerBin);

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
 * LinearComplexity_destroy - post process results for this text
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to cleanup any storage or state
 * associated with this test.
 */
void
LinearComplexity_destroy(struct state *state)
{
	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(107, __FUNCTION__, "state arg is NULL");
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
	if (state->linear_b != NULL) {
		free(state->linear_b);
		state->linear_b = NULL;
	}
	if (state->linear_c != NULL) {
		free(state->linear_c);
		state->linear_c = NULL;
	}
	if (state->linear_p != NULL) {
		free(state->linear_p);
		state->linear_p = NULL;
	}
	if (state->linear_t != NULL) {
		free(state->linear_t);
		state->linear_t = NULL;
	}

	/*
	 * Set driver state to DRIVER_DESTROY
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_DESTROY);
	state->driver_state[test_num] = DRIVER_DESTROY;

	return;
}
