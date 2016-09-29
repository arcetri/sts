/*****************************************************************************
			 U N I V E R S A L   T E S T
 *****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "externs.h"
#include "utilities.h"
#include "cephes.h"
#include "debug.h"


/*
 * private_stats - stats.txt information for this test
 */
struct Universal_private_stats {
	bool success;		// success or failure of interation test
	long int Q;		// number of blocks in the initialization sequence
	long int K;		// number of blocks in the test segment
	double sum;		// sum of the log2 distances betwween matching L-bit templates
	double sigma;		// sqrt(variance[L] / K)
	double phi;		// theoritical standard devistion
};


/*
 * static constant variable declarations
 */
static const enum test test_num = TEST_UNIVERSAL;	// this test number

/*
 * The expected_value[] and variance[] constants are from the following table:
 *
 *      A Handbook of Applied Cryptography, by Alfred J. Menezes, Paul C. van Oorschot, Scott A. Vanstone, 1997
 *      Section 5.4.5 "Maurer's universal stastical test", page 184.
 *
 * The index for these arrays is L.  The expected_value[] array is from the 2nd column (mu),
 * and the variance[] array is from the 3rd column (sigma^2(1)) in the above reference.
 *
 * Because MIM_L_UNIVERSAL is 6, the values for 0 <= L < MIM_L_UNIVERSAL forced to zero.
 */
static const double expected_value[MAX_L_UNIVERSAL + 1] = {
	0, 0, 0, 0, 0, 0, 5.2177052, 6.1962507, 7.1836656,
	8.1764248, 9.1723243, 10.170032, 11.168765,
	12.168070, 13.167693, 14.167488, 15.167379
};
static const double variance[MAX_L_UNIVERSAL + 1] = {
	0, 0, 0, 0, 0, 0, 2.954, 3.125, 3.238, 3.311, 3.356, 3.384,
	3.401, 3.410, 3.416, 3.419, 3.421
};


/*
 * forward static function declarations
 */
static bool Universal_print_stat(FILE * stream, struct state *state, struct Universal_private_stats *stat, double p_value);
static bool Universal_print_p_value(FILE * stream, double p_value);
static void Universal_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin);


/*
 * Universal_init - initalize the Universal test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Universal_init(struct state *state)
{
	long int n;		// Length of a single bit stream
	long int L;		// Length of each block
	long int p;		// elements in the working Universal template

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
	 * collect parameters from state
	 */
	n = state->tp.n;

	/*
	 * disable test if conditions do not permit this test from being run
	 */
	if (n < MIN_UNIVERSAL) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires bitcount(n): %ld >= %d for L >= 6",
		     state->testNames[test_num], test_num, n, MIN_UNIVERSAL);
		state->testVector[test_num] = false;
		return;
	}
	if (n >= MAX_UNIVERSAL) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires bitcount(n): %ld < %ld for L <= 16",
		     state->testNames[test_num], test_num, n, MAX_UNIVERSAL);
		state->testVector[test_num] = false;
		return;
	}
	/*
	 * Detrmine L, the length of each block
	 *
	 * given n, determine L, the smallest value such that:
	 *
	 *      n >= 1010 * 2^L *L
	 */
	for (L = MIN_L_UNIVERSAL - 1; L <= MAX_L_UNIVERSAL; ++L) {
		if (L > (BITS_N_LONGINT - 1)) {	// firewall
			warn(__FUNCTION__, "disabling test %s[%d]: L: %ld is too large, "
			     "1 << L:%ld > %ld bits long", state->testNames[test_num], test_num, L, L, BITS_N_LONGINT - 1);
			state->testVector[test_num] = false;
			return;
		}
		if (((long int) 1 << L) > ((long int) LONG_MAX / 1010 / L)) {	// paranoia
			warn(__FUNCTION__, "disabling test %s[%d]: L: %ld is too large, "
			     "1010 * 1 << L * L will overflow long int", state->testNames[test_num], test_num, L);
			state->testVector[test_num] = false;
			return;
		}
		// is L still big enough?
		if (n < (1010 * (1 << L) * L)) {
			--L;	// move back to the L that was not too big
			break;
		}
	}
	// firewall
	if (L < MIN_L_UNIVERSAL) {
		warn(__FUNCTION__, "disabling test %s[%d]: L is out of range: %ld < %d",
		     state->testNames[test_num], test_num, L, MIN_L_UNIVERSAL);
		state->testVector[test_num] = false;
		return;
	} else if (L > MAX_L_UNIVERSAL) {
		warn(__FUNCTION__, "disabling test %s[%d]: L is out of range: %ld > %d",
		     state->testNames[test_num], test_num, L, MAX_L_UNIVERSAL);
		state->testVector[test_num] = false;
		return;
	}
	state->universal_L = L;
	p = (long int) 1 << L;

	/*
	 * allocate the working template array
	 */
	state->universal_T = malloc(p * sizeof(state->universal_T[0]));
	if (state->universal_T == NULL) {
		errp(10, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->universal_T", p, sizeof(long));
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
	    create_dyn_array(sizeof(struct Universal_private_stats), DEFAULT_CHUNK, state->tp.numOfBitStreams, false);

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
 * Universal_iterate - interate one bit stream for Universal test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every interation noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
Universal_iterate(struct state *state)
{
	struct Universal_private_stats stat;	// stats for this interation
	long int n;		// Length of a single bit stream
	long int L;		// Length of each block
	long int *T;		// working Universal template
	long int p;		// elements in the working Universal template
	double arg;
	double p_value;		// p_value interation test result(s)
	double c;
	long decRep;
	long int i;
	long int j;

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
	if (state->universal_T == NULL) {
		err(10, __FUNCTION__, "state->universal_T is NULL");
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
	n = state->tp.n;
	L = state->universal_L;
	T = state->universal_T;
	// firewall
	if (L < MIN_L_UNIVERSAL) {
		err(10, __FUNCTION__, "L is out of range: %ld < %d", L, MIN_L_UNIVERSAL);
	} else if (L > MAX_L_UNIVERSAL) {
		err(10, __FUNCTION__, "L is out of range: %ld > %d", L, MAX_L_UNIVERSAL);
	}

	/*
	 * NOTE: Mathematical expression code rewrite, old code commented out below:
	 *
	 * p = (long int) pow(2, L);
	 */
	p = (long int) 1 << L;
	if (p > ((long int) LONG_MAX / 10)) {	// paranoia
		err(10, __FUNCTION__, "L: %ld is too large, 10 * 1 << L will overflow long int", L);
	}
	/*
	 * NOTE: Mathematical expression code rewrite, old code commented out below:
	 *
	 * stat.Q = 10 * (long int) pow(2, L);
	 */
	stat.Q = 10 * p;
	/*
	 * NOTE: Mathematical expression code rewrite, old code commented out below:
	 *
	 * stat.K = (int) (floor(n/L) - (double)stat.Q); // BLOCKS TO TEST
	 */
	stat.K = ((n / L) - stat.Q);	// BLOCKS TO TEST
	/*
	 * This test is not needed because stat.Q is set to 10 * (long int) pow(2, L) above
	 *
	 * if ((double) stat.Q < 10 * pow(2, L)) {
	 * fprintf(stats[test_num], "\t\tUNIVERSAL STATISTICAL TEST\n");
	 * fprintf(stats[test_num], "\t\t---------------------------------------------\n");
	 * fprintf(stats[test_num], "\t\tQ %ld < %f\n", stat.Q, 10 * pow(2, L));
	 * warn(__FUNCTION__, "Q %ld < %f", stat.Q, 10 * pow(2, L));
	 * return;
	 * }
	 */

	/*
	 * setup and initialize for the test
	 */
	/*
	 * COMPUTE THE EXPECTED: Formula 16, in Marsaglia's Paper
	 */
	c = 0.7 - 0.8 / (double) L + (4 + 32 / (double) L) * pow(stat.K, -3.0 / (double) L) / 15;
	stat.sigma = c * sqrt(variance[L] / (double) stat.K);
	stat.sum = 0.0;
	/*
	 * NOTE: Logical code rewrite, old code commented out below:
	 *
	 * for (i = 0; i < p; i++)
	 *      T[i] = 0;
	 */
	memset(T, 0, p * sizeof(T[0]));	// zeroize T
	for (i = 1; i <= stat.Q; i++) {	/* INITIALIZE TABLE */
		decRep = 0;
		for (j = 0; j < L; j++) {
			/*
			 * NOTE: Mathematical expression code rewrite, old code commented out below:
			 *
			 * decRep += state->epsilon[(i - 1) * L + j] * (long) pow(2.0, L - 1 - j);
			 */
			decRep += state->epsilon[(i - 1) * L + j] * ((long int) 1 << (L - 1 - j));
		}
		T[decRep] = i;
	}

	/*
	 * perform the test
	 */
	for (i = stat.Q + 1; i <= stat.Q + stat.K; i++) {	/* PROCESS BLOCKS */
		decRep = 0;
		for (j = 0; j < L; j++) {
			/*
			 * NOTE: Mathematical expression code rewrite, old code commented out below:
			 *
			 * decRep += state->epsilon[(i - 1) * L + j] * (long) pow(2.0, L - 1 - j);
			 */
			decRep += state->epsilon[(i - 1) * L + j] * ((long int) 1 << (L - 1 - j));
		}
		/*
		 * NOTE: Mathematical expression code rewrite, old code commented out below:
		 *
		 * stat.sum += log(i - T[decRep]) / log(2);
		 */
		stat.sum += log(i - T[decRep]) / state->c.log2;
		T[decRep] = i;
	}
	stat.phi = (double) (stat.sum / (double) stat.K);
	arg = fabs(stat.phi - expected_value[L]) / (state->c.sqrt2 * stat.sigma);
	p_value = erfc(arg);

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
 * Universal_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct Universal_private_stats for format and print
 *      p_value         // p_value interation test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
Universal_print_stat(FILE * stream, struct state *state, struct Universal_private_stats *stat, double p_value)
{
	long int n;		// Length of a single bit stream
	long int L;		// Length of each block
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
	if (p_value == NON_P_VALUE && stat->success == true) {
		err(10, __FUNCTION__, "p_value was set to NON_P_VALUE but stat->success == true");
	}

	/*
	 * collect parameters from state
	 */
	n = state->tp.n;
	L = state->universal_L;

	/*
	 * print stat to a file
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\tUNIVERSAL STATISTICAL TEST\n");
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
		io_ret = fprintf(stream, "\t\tUniversal statistical test\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t--------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(a) L         = %ld\n", L);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(b) Q         = %ld\n", stat->Q);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(c) K         = %ld\n", stat->K);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(d) sum       = %f\n", stat->sum);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(e) sigma     = %f\n", stat->sigma);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(f) variance  = %f\n", variance[L]);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(g) exp_value = %f\n", expected_value[L]);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(h) phi       = %f\n", stat->phi);
	if (io_ret <= 0) {
		return false;
	}
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t(i) WARNING:  %ld bits were discarded.\n", n - (stat->Q + stat->K) * L);
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\t(i) discarded = %ld\n", n - (stat->Q + stat->K) * L);
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t-----------------------------------------\n");
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
	 * all printing successful
	 */
	return true;
}


/*
 * Universal_print_p_value - print p_value information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      stat            // struct Universal_private_stats for format and print
 *      p_value         // p_value interation test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
Universal_print_p_value(FILE * stream, double p_value)
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
 * Universal_print - print to results.txt, data*.txt, stats.txt for all interations
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
Universal_print(struct state *state)
{
	struct Universal_private_stats *stat;	// pointer to statistics of an interation
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
	for (i = 0; i < state->stats[test_num]->count; ++i) {

		/*
		 * locate stat for this interation
		 */
		stat = addr_value(state->stats[test_num], struct Universal_private_stats, i);

		/*
		 * get p_value for this interation
		 */
		p_value = get_value(state->p_val[test_num], double, i);

		/*
		 * print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = Universal_print_stat(stats, state, stat, p_value);
		if (ok == false) {
			errp(10, __FUNCTION__, "error in writing to %s", stats_txt);
		}

		/*
		 * print p_value to results.txt
		 */
		errno = 0;	// paranoia
		ok = Universal_print_p_value(results, p_value);
		if (ok == false) {
			errp(10, __FUNCTION__, "error in writing to %s", results_txt);
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
					ok = Universal_print_p_value(data, p_value);
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
 * Universal_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state           // run state to test under
 *      sampleCount             // number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // uniformanity frequency bins
 */
static void
Universal_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
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
 * Universal_metrics - uniformity and proportional analysis of a test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to complete the test analysis for all interations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
Universal_metrics(struct state *state)
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
		Universal_metric_print(state, sampleCount, toolow, freqPerBin);

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
 * Universal_destroy - post process results for this text
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to cleanup any storage or state
 * associated with this test.
 */
void
Universal_destroy(struct state *state)
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
	if (state->universal_T != NULL) {
		free(state->universal_T);
		state->universal_T = NULL;
	}

	/*
	 * driver state destroyed - set driver state to DRIVER_DESTROY
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_DESTROY);
	state->driver_state[test_num] = DRIVER_DESTROY;
	return;
}
