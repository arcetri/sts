/*****************************************************************************
	 D I S C R E T E   F O U R I E R   T R A N S F O R M   T E S T
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


// Exit codes: 40 thru 49

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <complex.h>
#include "../utils/externs.h"
#include "../utils/utilities.h"
#include "../utils/cephes.h"
#include "../utils/debug.h"

#if defined(LEGACY_FFT)
#include "../utils/dfft.h"
#else /* LEGACY_FFT */
#include <fftw3.h>
#endif /* LEGACY_FFT */


/*
 * Private stats - stats.txt information for this test
 */
struct DiscreteFourierTransform_private_stats {
	bool success;		// Success or failure of iteration test
	long int N_1;		// Actual observed number of peaks in M that are less than T
	double N_0;		// Expected theoretical (95 %) number of peaks that are less than T
	double d;		// Normalized difference between observed and expected
				// number of frequency components that are beyond the 95% threshold
};


/*
 * Static const variables declarations
 */
static const enum test test_num = TEST_DFT;	// This test number


/*
 * Static variables declarations
 */
static double sqrtn4_095_005;			// Square root of (n / 4.0 * 0.95 * 0.05)
static double sqrt_log20_n;			// Square root of ln(20) * n


/*
 * Forward static function declarations
 */
static bool DiscreteFourierTransform_print_stat(FILE * stream, struct state *state,
						struct DiscreteFourierTransform_private_stats *stat, double p_value);
static bool DiscreteFourierTransform_print_p_value(FILE * stream, double p_value);
static void DiscreteFourierTransform_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin);


/*
 * DiscreteFourierTransform_init - initialize the Discrete Fourier Transform test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
DiscreteFourierTransform_init(struct state *state)
{
	long int n;		// Length of a single bit stream
	long int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(40, __func__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->cSetup != true) {
		err(40, __func__, "test constants not setup prior to calling %s for %s[%d]",
		    __func__, state->testNames[test_num], test_num);
	}

	/*
	 * Collect parameters from state
	 */
	n = state->tp.n;

	/*
	 * Disable test if conditions do not permit this test from being run
	 */
	if (n < MIN_LENGTH_FFT) {
		warn(__func__, "disabling test %s[%d]: requires bitcount(n): %ld >= %d",
		     state->testNames[test_num], test_num, n, MIN_LENGTH_FFT);
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
	 * Compute constants needed for the test
	 */
	sqrtn4_095_005 = sqrt((double) state->tp.n / 4.0 * 0.95 * 0.05);
	sqrt_log20_n = sqrt(log(20.0) * (double) state->tp.n);	// 2.995732274 * n

	/*
	 * Allocate arrays that will be used by the DFT libraries, for each thread
	 */
	state->fft_X = malloc((size_t) state->numberOfThreads * sizeof(*state->fft_X));
	if (state->fft_X == NULL) {
		errp(40, __func__, "cannot malloc for fft_X: %ld elements of %ld bytes each", state->numberOfThreads,
		     sizeof(*state->fft_X));
	}
#if defined(LEGACY_FFT)
	state->fft_wsave = malloc((size_t) state->numberOfThreads * sizeof(*state->fft_wsave));
	if (state->fft_wsave == NULL) {
		errp(40, __func__, "cannot malloc for fft_wsave: %ld elements of %ld bytes each", state->numberOfThreads,
		     sizeof(*state->fft_wsave));
	}
#else /* LEGACY_FFT */
	state->fftw_out = malloc((size_t) state->numberOfThreads * sizeof(*state->fftw_out));
	if (state->fftw_out == NULL) {
		errp(40, __func__, "cannot malloc for fftw_out: %ld elements of %ld bytes each", state->numberOfThreads,
		     sizeof(*state->fftw_out));
	}
	state->fftw_p = malloc((size_t) state->numberOfThreads * sizeof(*state->fftw_p));
	if (state->fftw_p == NULL) {
		errp(40, __func__, "cannot malloc for fftw_p: %ld elements of %ld bytes each", state->numberOfThreads,
		     sizeof(*state->fftw_p));
	}
#endif /* LEGACY_FFT */
	state->fft_m = malloc((size_t) state->numberOfThreads * sizeof(*state->fft_m));
	if (state->fft_m == NULL) {
		errp(40, __func__, "cannot malloc for fft_m: %ld elements of %ld bytes each", state->numberOfThreads,
		     sizeof(*state->fft_m));
	}

	for (i = 0; i < state->numberOfThreads; i++) {
		state->fft_X[i] = calloc((size_t) state->tp.n, sizeof(state->fft_X[i][0]));
		if (state->fft_X[i] == NULL) {
			errp(40, __func__, "cannot calloc of %ld elements of %ld bytes each for state->fft_X[%ld]",
			     n, sizeof(state->fft_X[i][0]), i);
		}
#if defined(LEGACY_FFT)
		state->fft_wsave[i] = calloc((size_t) 2 * state->tp.n, sizeof(state->fft_wsave[i][0]));
		if (state->fft_wsave[i] == NULL) {
			errp(40, __func__, "cannot calloc of %ld elements of %ld bytes each for state->fft_wsave[%ld]",
			     2 * n, sizeof(state->fft_wsave[i][0]), i);
		}
#else /* LEGACY_FFT */
		state->fftw_out[i] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (n / 2 + 1));
		if (state->fftw_out[i] == NULL) {
			errp(40, __func__, "cannot fftw_malloc of %ld elements of %ld bytes each for state->fftw_out[%ld]",
			     n / 2 + 1, sizeof(fftw_complex), i);
		}
		state->fftw_p[i] = fftw_plan_dft_r2c_1d((int) n, state->fft_X[i], state->fftw_out[i], FFTW_ESTIMATE);
#endif /* LEGACY_FFT */
		state->fft_m[i] = calloc((size_t) (n / 2 + 1), sizeof(state->fft_m[i][0]));
		if (state->fft_m[i] == NULL) {
			errp(40, __func__, "cannot calloc of %ld elements of %ld bytes each for state->fft_m[%ld]",
			     n / 2 + 1, sizeof(state->fft_m[i][0]), i);
		}
	}

	/*
	 * Allocate dynamic arrays
	 */
	if (state->resultstxtFlag == true) {
		state->stats[test_num] = create_dyn_array(sizeof(struct DiscreteFourierTransform_private_stats),
							  DEFAULT_CHUNK, state->tp.numOfBitStreams, false);        // stats.txt
	}
	state->p_val[test_num] = create_dyn_array(sizeof(double),
						  DEFAULT_CHUNK, state->tp.numOfBitStreams, false);	// results.txt

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
 * DiscreteFourierTransform_iterate - iterate one bit stream for Discrete Fourier Transform test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
DiscreteFourierTransform_iterate(struct thread_state *thread_state)
{
	struct DiscreteFourierTransform_private_stats stat;	// Stats for this iteration
	long int n;			// Length of a single bit stream
	double p_value;			// p_value iteration test result(s)
	double *X = NULL;		// Adjusted sequence with +1 and -1 bits
	double *m = NULL;		// Magnitude of the DFT
	long int i;
#if defined(LEGACY_FFT)
	double *wsave = NULL;		// Work array used by __ogg_fdrffti() and __ogg_fdrfftf()
	long ifac[WORK_ARRAY_LEN + 1];	// work array used by __ogg_fdrffti() and __ogg_fdrfftf()
#else /* LEGACY_FFT */
	fftw_complex *out;		// Output of the DFT
	fftw_plan p;			// Information on the fastest way to compute the DFT on this machine
#endif /* LEGACY_FFT */

	/*
	 * Check preconditions (firewall)
	 */
	if (thread_state == NULL) {
		err(41, __func__, "thread_state arg is NULL");
	}
	struct state *state = thread_state->global_state;
	if (state == NULL) {
		err(41, __func__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "iterate function[%d] %s called when test vector was false", test_num, __func__);
		return;
	}
	if (state->epsilon == NULL) {
		err(41, __func__, "state->epsilon is NULL");
	}
	if (state->epsilon[thread_state->thread_id] == NULL) {
		err(41, __func__, "state->epsilon[%ld] is NULL", thread_state->thread_id);
	}
	if (state->fft_X == NULL) {
		err(41, __func__, "state->fft_X is NULL");
	}
	if (state->fft_X[thread_state->thread_id] == NULL) {
		err(41, __func__, "state->fft_X[%ld] is NULL", thread_state->thread_id);
	}
	if (state->fft_m == NULL) {
		err(41, __func__, "state->fft_m is NULL");
	}
	if (state->fft_m[thread_state->thread_id] == NULL) {
		err(41, __func__, "state->fft_m[%ld] is NULL", thread_state->thread_id);
	}
	if (state->cSetup != true) {
		err(41, __func__, "test constants not setup prior to calling %s for %s[%d]",
		    __func__, state->testNames[test_num], test_num);
	}
#if defined(LEGACY_FFT)
	if (state->fft_wsave == NULL) {
		err(41, __func__, "state->fft_wsave is NULL");
	}
	if (state->fft_wsave[thread_state->thread_id] == NULL) {
		err(41, __func__, "state->fft_wsave[%ld] is NULL", thread_state->thread_id);
	}
#else
	if (state->fftw_out == NULL) {
		err(41, __func__, "state->fftw_out is NULL");
	}
	if (state->fftw_out[thread_state->thread_id] == NULL) {
		err(41, __func__, "state->fftw_out[%ld] is NULL", thread_state->thread_id);
	}
	if (state->fftw_p == NULL) {
		err(41, __func__, "state->fftw_p is NULL");
	}
	if (state->fftw_p[thread_state->thread_id] == NULL) {
		err(41, __func__, "state->fftw_p[%ld] is NULL", thread_state->thread_id);
	}
#endif /* LEGACY_FFT */

	/*
	 * Collect parameters from state
	 */
	n = state->tp.n;
	X = state->fft_X[thread_state->thread_id];
#if defined(LEGACY_FFT)
	wsave = state->fft_wsave[thread_state->thread_id];
#else /* LEGACY_FFT */
	out = state->fftw_out[thread_state->thread_id];
	p = state->fftw_p[thread_state->thread_id];
#endif /* LEGACY_FFT */
	m = state->fft_m[thread_state->thread_id];

	/*
	 * Step 1: initialize X for this iteration
	 */
	for (i = 0; i < n; i++) {
		if ((int) state->epsilon[thread_state->thread_id][i] == 1) {
			X[i] = 1;
		} else if ((int) state->epsilon[thread_state->thread_id][i] == 0) {
			X[i] = -1;
		} else {
			err(41, __func__, "found a bit different than 1 or 0 in the sequence");
		}
	}

	/*
	 * Step 2: apply discrete Fourier transform on X.
	 *
	 * Because the input array are purely real numbers (X is an array of +1 and -1),
	 * the DFT output satisfies the "Hermitian" redundancy.
	 * As a consequence, both the options (legacy dfft and fttw) save only the first
	 * (n / 2 + 1) non-redundant values of the DFT output.
	 */
#if defined(LEGACY_FFT)
	/*
	 * The dfft (legacy option) does the transform in-place.
	 * As a consequence, the values of X will be replaces with frequencies.
	 *
	 * After the function returns, X will look like (saying that values followed by I are the imaginary parts):
	 * 	[a, b, bI, c, cI, ..., l, lI] when n is odd
	 *	[a, b, bI, c, cI, ..., l, lI, m] when n is even
	 */
	__ogg_fdrffti(n, wsave, ifac);
	__ogg_fdrfftf(n, X, wsave, ifac);
#else /* LEGACY_FFT */
	/*
	 * The fftw library does the transform out-of-place.
	 * As a consequence, the computed complex frequencies will be saved in the out array
	 * of size n / 2 + 1.
	 */
	fftw_execute(p);
#endif /* LEGACY_FFT */

#if defined(LEGACY_FFT)
	/*
	 * Step 3a: compute modulus (absolute value) of the first element of the DFT output.
	 * This first element is always real, and has no imaginary part.
	 */
	m[0] = fabs(X[0]);

	/*
	 * Step 3b: compute the modulus of the following n/2 elements of the DFT output.
	 * These elements are always complex, so we have to consider both real and imaginary value.
	 */
	long int j;
	for (i = 0, j = 1; i < n - 1; i += 2, j++) {
		m[j] = sqrt((X[i] * X[i]) + (X[i + 1] * X[i + 1]));
	}

	/*
	 * Step 3c: if n is even, consider the remaining additional element at the end of the DFT output.
	 * This last element is always real, and has no imaginary part.
	 */
	if ((n % 2) == 0) {
		m[i+1] = fabs(X[n-1]);
	}
#else /* LEGACY_FFT */
	/*
	 * Step 3: compute modulus (absolute value) of the first (n / 2 + 1) elements (in our case
	 * all the ones that we have) of the DFT output.
	 */
	for (i = 0; i < n / 2 + 1; i++) {
		m[i] = cabs(out[i]);
	}
#endif /* LEGACY_FFT */

	/*
	 * Step 5: compute N0
	 * NOTE: Step 4 is skipped because T has already been computed
	 */
	stat.N_0 = (double) 0.95 * n / 2.0;

	/*
	 * Step 6: compute N1
	 */
	stat.N_1 = 0;
	for (i = 0; i < n / 2; i++) {
		if (m[i] < sqrt_log20_n) {
			stat.N_1++;
		}
	}

	/*
	 * Step 7: compute the test statistic
	 */
	stat.d = (stat.N_1 - stat.N_0) / sqrtn4_095_005;

	/*
	 * Step 8: compute the test P-value
	 */
	p_value = erfc(fabs(stat.d) / state->c.sqrt2);

	/*
	 * Lock mutex before making changes to the shared state
	 */
	if (thread_state->mutex != NULL) {
		pthread_mutex_lock(thread_state->mutex);
	}

	/*
	 * Record success or failure for this iteration
	 */
	state->count[test_num]++;	// Count this iteration
	state->valid[test_num]++;	// Count this valid iteration
	if (isNegative(p_value)) {
		state->failure[test_num]++;	// Bogus p_value < 0.0 treated as a failure
		stat.success = false;		// FAILURE
		warn(__func__, "iteration %ld of test %s[%d] produced bogus p_value: %f < 0.0\n",
		     thread_state->iteration_being_done + 1, state->testNames[test_num], test_num, p_value);
	} else if (isGreaterThanOne(p_value)) {
		state->failure[test_num]++;	// Bogus p_value > 1.0 treated as a failure
		stat.success = false;		// FAILURE
		warn(__func__, "iteration %ld of test %s[%d] produced bogus p_value: %f > 1.0\n",
		     thread_state->iteration_being_done + 1, state->testNames[test_num], test_num, p_value);
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
	if (state->resultstxtFlag == true) {
		append_value(state->stats[test_num], &stat);
	}
	append_value(state->p_val[test_num], &p_value);

	/*
	 * Unlock mutex after making changes to the shared state
	 */
	if (thread_state->mutex != NULL) {
		pthread_mutex_unlock(thread_state->mutex);
	}

	return;
}


/*
 * DiscreteFourierTransform_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct DiscreteFourierTransform_private_stats for format and print
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
DiscreteFourierTransform_print_stat(FILE * stream, struct state *state, struct DiscreteFourierTransform_private_stats *stat,
				    double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(42, __func__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(42, __func__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(42, __func__, "stat arg is NULL");
	}
	if (p_value == NON_P_VALUE && stat->success == true) {
		err(42, __func__, "p_value was set to NON_P_VALUE but stat->success == true");
	}

	/*
	 * Print stat to a file
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t\t\tFFT TEST\n");
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
		io_ret = fprintf(stream, "\t\t\t\tFFT test\n");
		if (io_ret <= 0) {
			return false;
		}
	}
	io_ret = fprintf(stream, "\t\t-------------------------------------------\n");
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(a) Percentile = %f\n", (double) stat->N_1 / (state->tp.n / 2) * 100.0);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(b) N_1        = %ld\n", stat->N_1);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(c) N_0        = %f\n", stat->N_0);
	if (io_ret <= 0) {
		return false;
	}
	io_ret = fprintf(stream, "\t\t(d) d          = %f\n", stat->d);
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
	 * All printing successful
	 */
	return true;
}


/*
 * DiscreteFourierTransform_print_p_value - print p_value information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      stat            // struct DiscreteFourierTransform_private_stats for format and print
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
DiscreteFourierTransform_print_p_value(FILE * stream, double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(43, __func__, "stream arg is NULL");
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
 * DiscreteFourierTransform_print - print to results.txt, data*.txt, stats.txt for all iterations
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
DiscreteFourierTransform_print(struct state *state)
{
	struct DiscreteFourierTransform_private_stats *stat;	// Pointer to statistics of an iteration
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

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(44, __func__, "state arg is NULL");
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
		err(44, __func__,
		    "print driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(44, __func__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->datatxt_fmt[test_num] == NULL) {
		err(44, __func__, "format for data0*.txt filename is NULL");
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
		stat = addr_value(state->stats[test_num], struct DiscreteFourierTransform_private_stats, i);

		/*
		 * Get p_value for this iteration
		 */
		p_value = get_value(state->p_val[test_num], double, i);

		/*
		 * Print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = DiscreteFourierTransform_print_stat(stats, state, stat, p_value);
		if (ok == false) {
			errp(44, __func__, "error in writing to %s", stats_txt);
		}

		/*
		 * Print p_value to results.txt
		 */
		errno = 0;	// paranoia
		ok = DiscreteFourierTransform_print_p_value(results, p_value);
		if (ok == false) {
			errp(44, __func__, "error in writing to %s", results_txt);
		}
	}

	/*
	 * Flush and close stats.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(stats);
	if (io_ret != 0) {
		errp(44, __func__, "error flushing to: %s", stats_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(stats);
	if (io_ret != 0) {
		errp(44, __func__, "error closing: %s", stats_txt);
	}
	free(stats_txt);
	stats_txt = NULL;

	/*
	 * Flush and close results.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(results);
	if (io_ret != 0) {
		errp(44, __func__, "error flushing to: %s", results_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(results);
	if (io_ret != 0) {
		errp(44, __func__, "error closing: %s", results_txt);
	}
	free(results_txt);
	results_txt = NULL;

	/*
	 * Write data*.txt for each data file if we need to partition results
	 */
	if (state->partitionCount[test_num] > 1) {

		/*
		 * For each data file
		 */
		for (j = 0; j < state->partitionCount[test_num]; ++j) {

			/*
			 * Form the data*.txt basename
			 */
			errno = 0;	// paranoia
			snprintf_ret = snprintf(data_filename, BUFSIZ, state->datatxt_fmt[test_num], j + 1);
			data_filename[BUFSIZ] = '\0';	// paranoia
			if (snprintf_ret <= 0 || snprintf_ret >= BUFSIZ || errno != 0) {
				errp(44, __func__, "snprintf failed for %d bytes for data%03ld.txt, returned: %d", BUFSIZ,
				     j + 1, snprintf_ret);
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
					ok = DiscreteFourierTransform_print_p_value(data, p_value);
					if (ok == false) {
						errp(44, __func__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * Flush and close data*.txt, free pathname
			 */
			errno = 0;	// paranoia
			io_ret = fflush(data);
			if (io_ret != 0) {
				errp(44, __func__, "error flushing to: %s", data_txt);
			}
			errno = 0;	// paranoia
			io_ret = fclose(data);
			if (io_ret != 0) {
				errp(44, __func__, "error closing: %s", data_txt);
			}
			free(data_txt);
			data_txt = NULL;
		}
	}

	return;
}


/*
 * DiscreteFourierTransform_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state           // run state to test under
 *      sampleCount             // Number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // uniformity frequency bins
 */
static void
DiscreteFourierTransform_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
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
		err(45, __func__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(45, __func__, "freqPerBin arg is NULL");
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
	expCount = (double)sampleCount / state->tp.uniformity_bins;
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
			errp(45, __func__, "error flushing to: %s", state->finalReptPath);
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
			state->metric_results.dft = FAILED_BOTH;
		} else if (proportion_passed == false) {
			state->metric_results.dft = FAILED_PROPORTION;
		} else if (uniformity_passed == false) {
			state->metric_results.dft = FAILED_UNIFORMITY;
		} else {
			state->metric_results.dft = PASSED_BOTH;
			state->successful_tests++;
		}
	}

	return;
}


/*
 * DiscreteFourierTransform_metrics - uniformity and proportional analysis of a test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to complete the test analysis for all iterations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
DiscreteFourierTransform_metrics(struct state *state)
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
		err(46, __func__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(46, __func__,
		    "metrics driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		warn(__func__,
		     "metrics driver interface for %s[%d] called with p_val length: %ld != bit streams: %ld",
		     state->testNames[test_num], test_num, state->p_val[test_num]->count,
		     state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}

	/*
	 * Allocate uniformity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(46, __func__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
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
		DiscreteFourierTransform_metric_print(state, sampleCount, toolow, freqPerBin);

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
 * DiscreteFourierTransform_destroy - post process results for this text
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to cleanup any storage or state
 * associated with this test.
 */
void
DiscreteFourierTransform_destroy(struct state *state)
{
	long int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(47, __func__, "state arg is NULL");
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
		if (state->fft_X[i] != NULL) {
			free(state->fft_X[i]);
			state->fft_X[i] = NULL;
		}
#if defined(LEGACY_FFT)
		if (state->fft_wsave[i] != NULL) {
			free(state->fft_wsave[i]);
			state->fft_wsave[i] = NULL;
		}
#else /* LEGACY_FFT */
		if (state->fftw_out[i] != NULL) {
			fftw_free(state->fftw_out[i]);
			state->fftw_out[i] = NULL;
		}
		if (state->fftw_p[i] != NULL) {
			fftw_destroy_plan(state->fftw_p[i]);
			state->fftw_p[i] = NULL;
		}
#endif /* LEGACY_FFT */
		if (state->fft_m[i] != NULL) {
			free(state->fft_m[i]);
			state->fft_m[i] = NULL;
		}
	}

	if (state->fft_X != NULL) {
		free(state->fft_X);
		state->fft_X = NULL;
	}
#if defined(LEGACY_FFT)
	if (state->fft_wsave != NULL) {
		free(state->fft_wsave);
		state->fft_wsave = NULL;
	}
#else /* LEGACY_FFT */
	if (state->fftw_out != NULL) {
		free(state->fftw_out);
		state->fftw_out = NULL;
	}
	if (state->fftw_p != NULL) {
		free(state->fftw_p);
		state->fftw_p = NULL;
	}
#endif /* LEGACY_FFT */
	if (state->fft_m != NULL) {
		free(state->fft_m);
		state->fft_m = NULL;
	}

	return;
}
