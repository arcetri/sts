/*****************************************************************************
		   L I N E A R   C O M P L E X I T Y   T E S T
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
 * private_stats - stats.txt information for this test
 */
struct LinearComplexity_private_stats {
	bool success;		// success or failure of interation test
	long int discarded;	// bit discarded from the test
	long int nu[LINEARCOMPLEXITY_K_DEGREES + 1];	// T range count
	double chi2;		// chi^2 of test
};


/*
 * static constant variable declarations
 */
static const enum test test_num = TEST_LINEARCOMPLEXITY;	// this test number

/*
 * pi_term - theoretical probabilities of classes
 *
 * See SP800-22Rev1a section 3.10.
 */
static const double pi_term[LINEARCOMPLEXITY_K_DEGREES + 1] = { 0.01047, 0.03125, 0.12500, 0.50000, 0.25000, 0.06250, 0.020833 };


/*
 * forward static function declarations
 */
static bool LinearComplexity_print_stat(FILE * stream, struct state *state, struct LinearComplexity_private_stats *stat,
					double p_value);
static bool LinearComplexity_print_p_value(FILE * stream, double p_value);
static void LinearComplexity_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin);


/*
 * LinearComplexity_init - initalize the Linear Complexity test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
LinearComplexity_init(struct state *state)
{
	long int M;		// Linear Complexity Test - block length

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(100, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false",
		    state->testNames[test_num], test_num);
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

	/*
	 * collect parameters from state
	 */
	M = state->tp.linearComplexitySequenceLength;

	/*
	 * disable test if conditions do not permit this test from being run
	 */
	if (M < MIN_LINEARCOMPLEXITY) {
		warn(__FUNCTION__, "-P 6: linear complexith sequence length(M): %ld must be >= %d", M, MIN_LINEARCOMPLEXITY);
		state->testVector[test_num] = false;
		return;
	} else if (M > MAX_LINEARCOMPLEXITY + 1) {
		warn(__FUNCTION__, "-P 6: linear complexith sequence length(M): %ld must be <= %d", M, MAX_LINEARCOMPLEXITY + 1);
		state->testVector[test_num] = false;
		return;
	}

	/*
	 * create working sub-directory if forming files such as results.txt and stats.txt
	 */
	if (state->resultstxtFlag == true) {
		state->subDir[test_num] = precheckSubdir(state, state->testNames[test_num]);
		dbg(DBG_HIGH, "test %s[%d] will use subdir: %s", state->testNames[test_num], test_num, state->subDir[test_num]);
	}

	/*
	 * allocate special Linear Feedback Shift Register arrays
	 */
	state->linear_B_ = malloc(state->tp.linearComplexitySequenceLength * sizeof(state->linear_B_[0]));
	if (state->linear_B_ == NULL) {
		errp(100, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->linear_B_",
		     state->tp.linearComplexitySequenceLength, sizeof(BitSequence));
	}
	state->linear_C = malloc(state->tp.linearComplexitySequenceLength * sizeof(state->linear_C[0]));
	if (state->linear_C == NULL) {
		errp(100, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->linear_C",
		     state->tp.linearComplexitySequenceLength, sizeof(BitSequence));
	}
	state->linear_P = malloc(state->tp.linearComplexitySequenceLength * sizeof(state->linear_P[0]));
	if (state->linear_P == NULL) {
		errp(100, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->linear_P",
		     state->tp.linearComplexitySequenceLength, sizeof(BitSequence));
	}
	state->linear_T = malloc(state->tp.linearComplexitySequenceLength * sizeof(state->linear_T[0]));
	if (state->linear_T == NULL) {
		errp(100, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->linear_T",
		     state->tp.linearComplexitySequenceLength, sizeof(BitSequence));
	}

	/*
	 * allocate dynamic arrays
	 */
	// stats.txt data
	state->stats[test_num] =
	    create_dyn_array(sizeof(struct LinearComplexity_private_stats), DEFAULT_CHUNK, state->tp.numOfBitStreams, false);

	// results.txt data
	state->p_val[test_num] = create_dyn_array(sizeof(double), DEFAULT_CHUNK, state->tp.numOfBitStreams, false);

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
 * LinearComplexity_iterate - interate one bit stream for Linear Complexity test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
LinearComplexity_iterate(struct state *state)
{
	struct LinearComplexity_private_stats stat;	// stats for this interation
	long int M;		// Linear Complexity Test - block length
	long int n;		// Length of a single bit stream
	long int N;		// number of independent blocks the bit stream is partitioned into
	long int j;
	long int d;
	long int L;
	long int m;
	long int N_;
	long int parity;
	double p_value;		// p_value interation test result(s)
	double T_;
	double mean;
	long int i;
	long int ii;

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(101, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "interate function[%d] %s called when test vector was false", test_num, __FUNCTION__);
		return;
	}
	if (state->epsilon == NULL) {
		err(101, __FUNCTION__, "state->epsilon is NULL");
	}
	if (state->linear_B_ == NULL) {
		err(101, __FUNCTION__, "state->linear_B_ is NULL");
	}
	if (state->linear_C == NULL) {
		err(101, __FUNCTION__, "state->linear_C is NULL");
	}
	if (state->linear_P == NULL) {
		err(101, __FUNCTION__, "state->linear_P is NULL");
	}
	if (state->linear_T == NULL) {
		err(101, __FUNCTION__, "state->linear_T is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_INIT && state->driver_state[test_num] != DRIVER_ITERATE) {
		err(101, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_INIT: %d and != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_INIT, DRIVER_ITERATE);
	}

	/*
	 * collect parameters from state
	 */
	M = state->tp.linearComplexitySequenceLength;
	n = state->tp.n;
	stat.discarded = n % M;
	/*
	 * NOTE: Mathematical expression code rewrite, old code commented out below:
	 *
	 * N = (int) floor(n / M);
	 */
	N = state->tp.n / state->tp.linearComplexitySequenceLength;

	/*
	 * zeroize the nu counters
	 */
	/*
	 * NOTE: Logical code rewrite, old code commented out below:
	 *
	 * for (i = 0; i < LINEARCOMPLEXITY_K_DEGREES + 1; i++) {
	 *      stat.nu[i] = 0;
	 * }
	 */
	memset(stat.nu, 0, (LINEARCOMPLEXITY_K_DEGREES + 1) * sizeof(stat.nu[0]));

	/*
	 * perform the test for each independent block
	 */
	for (ii = 0; ii < N; ii++) {

		/*
		 * zeroize working LSRF storage
		 */
		/*
		 * NOTE: Logical code rewrite, old code commented out below:
		 *
		 * for (i = 0; i < M; ++i) {
		 *      state->linear_B_[i] = 0;
		 *      state->linear_C[i] = 0;
		 *      state->linear_P[i] = 0;
		 *      state->linear_T[i] = 0;
		 * }
		 */
		memset(state->linear_B_, 0, M * sizeof(state->linear_B_[0]));
		memset(state->linear_C, 0, M * sizeof(state->linear_C[0]));
		memset(state->linear_P, 0, M * sizeof(state->linear_P[0]));
		memset(state->linear_T, 0, M * sizeof(state->linear_T[0]));

		/*
		 * initialize values
		 */
		L = 0;
		m = -1;
		d = 0;
		state->linear_C[0] = 1;
		state->linear_B_[0] = 1;

		/*
		 * Determine linear complexity
		 */
		N_ = 0;
		while (N_ < M) {
			d = (int) state->epsilon[ii * M + N_];
			for (i = 1; i <= L; i++) {
				d += state->linear_C[i] * state->epsilon[ii * M + N_ - i];
			}
			d = d % 2;
			if (d == 1) {
				/*
				 * NOTE: Logical code rewrite, old code commented out below:
				 *
				 * for (i = 0; i < M; i++) {
				 *      state->linear_T[i] = state->linear_C[i];
				 *      state->linear_P[i] = 0;
				 * }
				 */
				memcpy(state->linear_T, state->linear_C, M * sizeof(state->linear_T[0]));
				memset(state->linear_P, 0, M * sizeof(state->linear_P[0]));
				for (j = 0; j < M; j++) {
					if (state->linear_B_[j] == 1) {
						state->linear_P[j + N_ - m] = 1;
					}
				}
				for (i = 0; i < M; i++) {
					state->linear_C[i] = (state->linear_C[i] + state->linear_P[i]) % 2;
				}
				if (L <= N_ / 2) {
					L = N_ + 1 - L;
					m = N_;
					for (i = 0; i < M; i++) {
						state->linear_B_[i] = state->linear_T[i];
					}
				}
			}
			N_++;
		}

		/*
		 * determine mean
		 */
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
		if ((parity = (M + 1) % 2) == 0) {
			mean = (M / 2.0) + (8.0) / 36.0;
		} else {
			mean = (M / 2.0) + (10.0) / 36.0;
		}

		/*
		 * determine T value
		 */
		/*
		 * NOTE: Mathematical expression code rewrite, old code commented out below:
		 *
		 * if ((parity = M % 2) == 0)
		 * sign = 1;
		 * else
		 * sign = -1;
		 * T_ = sign * (L - mean) + 2.0 / 9.0;
		 */
		if ((parity = M % 2) == 0) {
			T_ = (L - mean) + 2.0 / 9.0;
		} else {
			T_ = (mean - L) + 2.0 / 9.0;
		}

		/*
		 * count T ranges
		 */
		if (T_ <= -2.5) {
			stat.nu[0]++;
		} else if (T_ > -2.5 && T_ <= -1.5) {
			stat.nu[1]++;
		} else if (T_ > -1.5 && T_ <= -0.5) {
			stat.nu[2]++;
		} else if (T_ > -0.5 && T_ <= 0.5) {
			stat.nu[3]++;
		} else if (T_ > 0.5 && T_ <= 1.5) {
			stat.nu[4]++;
		} else if (T_ > 1.5 && T_ <= 2.5) {
			stat.nu[5]++;
		} else {
			stat.nu[6]++;
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
		stat.chi2 += (stat.nu[i] - N * pi_term[i]) * (stat.nu[i] - N * pi_term[i]) / (N * pi_term[i]);
	}
	p_value = cephes_igamc(LINEARCOMPLEXITY_K_DEGREES / 2.0, stat.chi2 / 2.0);

	/*
	 * record testable test success or failure
	 */
	state->count[test_num]++;	// count this test
	state->valid[test_num]++;	// count this valid test
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
	 * results.txt and stats.txt accounting
	 */
	append_value(state->stats[test_num], &stat);
	append_value(state->p_val[test_num], &p_value);

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
 * LinearComplexity_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct LinearComplexity_private_stats for format and print
 *      p_value         // p_value interation test result(s)// p_value interation test result(s)
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
	 * firewall
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
	 * print stat to a file
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "-----------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\tL I N E A R  C O M P L E X I T Y\n");
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
		io_ret = fprintf(stream, "\tN (number of substrings) = %ld\n",
				 state->tp.n / state->tp.linearComplexitySequenceLength);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "-----------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "        F R E Q U E N C Y                            \n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "-----------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "  C0   C1   C2   C3   C4   C5   C6    CHI2    P-value\n");
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
		io_ret = fprintf(stream, "\t\tN (number of substrings) = %ld\n",
				 state->tp.n / state->tp.linearComplexitySequenceLength);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\tbits discarded           = %ld\n", stat->discarded);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t        T range count\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t  C0   C1   C2   C3   C4   C5   C6    CHI2\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t--------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	for (i = 0; i < LINEARCOMPLEXITY_K_DEGREES + 1; i++) {
		io_ret = fprintf(stream, "%4ld ", stat->nu[i]);
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
	 * all printing successful
	 */
	return true;
}


/*
 * LinearComplexity_print_p_value - print p_value information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      stat            // struct LinearComplexity_private_stats for format and print
 *      p_value         // p_value interation test result(s)
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
	 * firewall
	 */
	if (stream == NULL) {
		err(103, __FUNCTION__, "stream arg is NULL");
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
 * LinearComplexity_print - print to results.txt, data*.txt, stats.txt for all interations
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
	struct LinearComplexity_private_stats *stat;	// pointer to statistics of an interation
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

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(104, __FUNCTION__, "state arg is NULL");
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
		stat = addr_value(state->stats[test_num], struct LinearComplexity_private_stats, i);

		/*
		 * get p_value for this interation
		 */
		p_value = get_value(state->p_val[test_num], double, i);

		/*
		 * print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = LinearComplexity_print_stat(stats, state, stat, p_value);
		if (ok == false) {
			errp(104, __FUNCTION__, "error in writing to %s", stats_txt);
		}

		/*
		 * print p_value to results.txt
		 */
		errno = 0;	// paranoia
		ok = LinearComplexity_print_p_value(results, p_value);
		if (ok == false) {
			errp(104, __FUNCTION__, "error in writing to %s", results_txt);
		}
	}

	/*
	 * flush and close stats.txt, free pathname
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
	 * flush and close results.txt, free pathname
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
				errp(104, __FUNCTION__,
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
					ok = LinearComplexity_print_p_value(data, p_value);
					if (ok == false) {
						errp(104, __FUNCTION__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * flush and close data*.txt, free pathname
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
	 * driver print - set driver state to DRIVER_PRINT
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
 *      sampleCount             // number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // uniformanity frequency bins
 */
static void
LinearComplexity_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
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
		err(105, __FUNCTION__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(105, __FUNCTION__, "freqPerBin arg is NULL");
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
 * This function is called once to complete the test analysis for all interations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
LinearComplexity_metrics(struct state *state)
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
		err(106, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false",
		    state->testNames[test_num], test_num);
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
	 * allocate uniformanity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(106, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
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
		LinearComplexity_metric_print(state, sampleCount, toolow, freqPerBin);

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
	 * firewall
	 */
	if (state == NULL) {
		err(107, __FUNCTION__, "state arg is NULL");
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
	if (state->linear_B_ != NULL) {
		free(state->linear_B_);
		state->linear_B_ = NULL;
	}
	if (state->linear_C != NULL) {
		free(state->linear_C);
		state->linear_C = NULL;
	}
	if (state->linear_P != NULL) {
		free(state->linear_P);
		state->linear_P = NULL;
	}
	if (state->linear_T != NULL) {
		free(state->linear_T);
		state->linear_T = NULL;
	}

	/*
	 * driver state destroyed - set driver state to DRIVER_DESTROY
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_DESTROY);
	state->driver_state[test_num] = DRIVER_DESTROY;
	return;
}
