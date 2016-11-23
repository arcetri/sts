/*****************************************************************************
	  N O N O V E R L A P P I N G	T E M P L A T E	  T E S T
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


// Exit codes: 130 thru 139

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "externs.h"
#include "utilities.h"
#include "cephes.h"
#include "debug.h"


/*
 * Private stats - stats.txt information for this test
 */
struct NonOverlappingTemplateMatchings_private_stats {
	double mu;		// Theoretical mean under an assumption of randomness
	double sigma_squared;	// Theoretical variance under an assumption of randomness
	long int M;		// Length of the blocks to be tested
};


/*
 * Special data for each template of each iteration in p_val (instead of just p_value doubles)
 */
struct nonover_stats {
	double p_value;			// Test p_value for a given template
	bool success;			// Success or failure for a given template
	double chi2;			// Test statistic for a given template
	long int template_index;	// Template number of a given template
	unsigned int Wj[BLOCKS_NON_OVERLAPPING]; // Number of times that m-bit template occurs within each block
};


/*
 * Static const variables declarations
 */
static const enum test test_num = TEST_NON_OVERLAPPING;	// This test number

/*
 * Expected number of non-overlapping templates for each possible template length
 *
 * These values were derived from the mkapertemplate utility.  We keep only
 * enough words to hold the size of the largest template allowed by MAXTEMPLEN.
 *
 * Read NOTE in defs.h for an additional explanation.
 */
static const long int numOfTemplates[MAXTEMPLEN + 1] = {
#if MAXTEMPLEN >= 2
	0,			// invalid template length 0
	0,			// invalid template length 1
	2,			// for template length 2
#endif
#if MAXTEMPLEN >= 3
	4,			// for template length 3
#endif
#if MAXTEMPLEN >= 4
	6,			// for template length 4
#endif
#if MAXTEMPLEN >= 5
	12,			// for template length 5
#endif
#if MAXTEMPLEN >= 6
	20,			// for template length 6
#endif
#if MAXTEMPLEN >= 7
	40,			// for template length 7
#endif
#if MAXTEMPLEN >= 8
	74,			// for template length 8
#endif
#if MAXTEMPLEN >= 9
	148,			// for template length 9
#endif
#if MAXTEMPLEN >= 10
	284,			// for template length 10
#endif
#if MAXTEMPLEN >= 11
	568,			// for template length 11
#endif
#if MAXTEMPLEN >= 12
	1116,			// for template length 12
#endif
#if MAXTEMPLEN >= 13
	2232,			// for template length 13
#endif
#if MAXTEMPLEN >= 14
	4424,			// for template length 14
#endif
#if MAXTEMPLEN >= 15
	8848,			// for template length 15
#endif
#if MAXTEMPLEN >= 16
	17622,			// for template length 16
#endif
#if MAXTEMPLEN >= 17
	35244,			// for template length 17
#endif
#if MAXTEMPLEN >= 18
	70340,			// for template length 18
#endif
#if MAXTEMPLEN >= 19
	140680,			// for template length 19
#endif
#if MAXTEMPLEN >= 20
	281076,			// for template length 20
#endif
#if MAXTEMPLEN >= 21
	562152,			// for template length 21
#endif
#if MAXTEMPLEN >= 22
	1123736,		// for template length 22
#endif
#if MAXTEMPLEN >= 23
	2247472,		// for template length 23
#endif
#if MAXTEMPLEN >= 24
	4493828,		// for template length 24
#endif
#if MAXTEMPLEN >= 25
	8987656,		// for template length 25
#endif
#if MAXTEMPLEN >= 26
	17973080,		// for template length 26
#endif
#if MAXTEMPLEN >= 27
	35946160,		// for template length 27
#endif
#if MAXTEMPLEN >= 28
	71887896,		// for template length 28
#endif
#if MAXTEMPLEN >= 29
	143775792,		// for template length 29
#endif
#if MAXTEMPLEN >= 30
	287542736,		// for template length 30
#endif
#if MAXTEMPLEN >= 31
	575085472,		// for template length 31
#endif
};


/*
 * Forward static function declarations
 */
static void appendTemplate(struct state *state, ULONG value, long int m);
static bool NonOverlappingTemplateMatchings_print_stat(FILE * stream, struct state *state,
						       struct NonOverlappingTemplateMatchings_private_stats *stat,
						       struct dyn_array *nonover_stats, long int nonstat_index);
static bool NonOverlappingTemplateMatchings_print_p_value(FILE * stream, double p_value);
static void NonOverlappingTemplateMatchings_metric_print(struct state *state, long int sampleCount, long int toolow,
							 long int *freqPerBin);

/*
 * NonOverlappingTemplateMatchings_init - initialize the NonOverlapping Template test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
NonOverlappingTemplateMatchings_init(struct state *state)
{
	long int n;		// Length of a single bit stream
	long int M;		// Length of each block to be tested
	long int m;		// Length of a template
	ULONG max_num;		// Max decimal value of a template
	ULONG i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(130, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "init driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->cSetup != true) {
		err(130, __FUNCTION__, "test constants not setup prior to calling %s for %s[%d]",
		    __FUNCTION__, state->testNames[test_num], test_num);
	}
	if (state->driver_state[test_num] != DRIVER_NULL && state->driver_state[test_num] != DRIVER_DESTROY) {
		err(130, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_NULL: %d and != DRIVER_DESTROY: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_NULL, DRIVER_DESTROY);
	}

	/*
	 * Collect parameters from state
	 */
	n = state->tp.n;
	M = state->tp.n / BLOCKS_NON_OVERLAPPING;
	m = state->tp.nonOverlappingTemplateLength; // Default value = 9

	/*
	 * Disable test if conditions do not permit this test from being run
	 */
	if (M <= MIN_RATIO_M_OVER_n_NON_OVERLAPPING * n) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires block length(M): %ld > %f * n, and here n = %ld",
		     state->testNames[test_num], test_num, M, MIN_RATIO_M_OVER_n_NON_OVERLAPPING, n);
		state->testVector[test_num] = false;
		return;
	} else if (m < MINTEMPLEN) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires template length(m): %ld >= MINTEMPLEN: %d",
		     state->testNames[test_num], test_num, m, MINTEMPLEN);
		state->testVector[test_num] = false;
		return;
	} else if (m > MAXTEMPLEN) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires template length(m): %ld <= MAXTEMPLEN: %d",
		     state->testNames[test_num], test_num, m, MAXTEMPLEN);
		state->testVector[test_num] = false;
		return;
	} else if (m > M) {
		warn(__FUNCTION__, "disabling test %s[%d]: requires template length(m): %ld <= M: %d",
		     state->testNames[test_num], test_num, m, M);
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
	 * Allocate special BitSequence
	 */
	state->nonper_seq = malloc(m * sizeof(state->nonper_seq[0]));
	if (state->nonper_seq == NULL) {
		errp(130, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for state->nonper_seq",
		     m, sizeof(BitSequence));
	}

	/*
	 * Set the proper partitionCount value for this test [there will be more data*.txt for each iteration]
	 */
	state->partitionCount[test_num] = (int) numOfTemplates[m];
	dbg(DBG_HIGH, "partitionCount for %s[%d] is %ld", state->testNames[test_num], test_num, numOfTemplates[m]);

	/*
	 * Allocate dynamic arrays
	 *
	 * NonOverlapping Template Test uses array of struct nonover_stats instead of p_value doubles
	 */
	state->stats[test_num] = create_dyn_array(sizeof(struct NonOverlappingTemplateMatchings_private_stats),
	                                          DEFAULT_CHUNK, state->tp.numOfBitStreams, false);	// stats.txt
	state->p_val[test_num] = create_dyn_array(sizeof(struct nonover_stats), DEFAULT_CHUNK,
						  numOfTemplates[m] * state->tp.numOfBitStreams, false);	// results.txt

	/*
	 * Generate nonovTemplates - array of non-overlapping templates
	 *
	 * We will generate, in the style of mkapertemplate.c, an array
	 * of 32-bit (ULONG) non-overlapping templates for use in
	 * in this test.  Generating them once here is usually much faster
	 * than reading them from a pre-computed file as the old code did.
	 */
	dbg(DBG_LOW, "forming an array of %ld non-overlapping templates of %ld bytes each", numOfTemplates[m], m);
	state->nonovTemplates = create_dyn_array(sizeof(BitSequence), DEFAULT_CHUNK, numOfTemplates[m], false);

	/*
	 * Append to nonovTemplates each non-overlapping template found among all the 32bits natural numbers.
	 * NOTE: The fact that templates are created from 32-bit values is one reason why MAXTEMPLEN cannot be greater than 31.
	 */
	max_num = (ULONG) 1 << m;
	for (i = 1; i < max_num; i++) {
		appendTemplate(state, i, m);
	}

	/*
	 * Verify that the size of nonovTemplates is as expected
	 */
	if (state->nonovTemplates->count / 9 != numOfTemplates[m]) {
		err(130, __FUNCTION__, "nonovTemplates->count / 9: %ld != numOfTemplates[%ld]: %ld",
		    state->nonovTemplates->count / 9, m, numOfTemplates[m]);
	}
	dbg(DBG_LOW, "formed an array of %ld non-overlapping templates of %ld bytes each", numOfTemplates[m], m);

	/*
	 * Determine format of data*.txt filenames based on state->partitionCount[test_num]
	 */
	state->datatxt_fmt[test_num] = data_filename_format((int) numOfTemplates[m]);
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
 * appendTemplate - append value to the template array if non-periodic
 *
 * given:
 *      state           // run state to test under
 *      value           // value being test if it is non-periodic
 *      m           	// significant bit count of value
 *
 * NOTE: This function was based on, in part, code from NIST Special Publication 800-22 Revision 1a:
 *
 *      http://csrc.nist.gov/groups/ST/toolkit/rng/documents/SP800-22rev1a.pdf
 *
 * In particular see section F.2 (page F-4) of the document revised April 2010.  See mkapertemplate.c
 * in this source directory.  This function uses ULONG (32-bit) instead of uint64_t (64-bit) as
 * found in the mkapertemplate.c source file.  Moreover it appends each periodic template to
 * a dynamic array instead of writing ASCII bits to a file.
 */
static void
appendTemplate(struct state *state, ULONG value, long int m)
{
	BitSequence A[m];	// Array of the bit values of a template
	ULONG displayMask;	// Bit mask used to convert value to its binary representation
	bool overlap = true;	// Indicator if potential template overlaps
	int c;
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(131, __FUNCTION__, "state arg is NULL");
	}
	if (state->nonovTemplates == NULL) {
		err(131, __FUNCTION__, "state->nonovTemplates is NULL");
	}

	/*
	 * Setup display mask
	 */
	displayMask = (ULONG) 1 << (m - 1);

	/*
	 * Convert value into an array of binary values.
	 * Bits are stored to A in a big-endian way, with the least significant bit last.
	 */
	for (c = 0; c < m; c++) {
		A[c] = (BitSequence) ((value & displayMask) ? 1 : 0);
		value <<= 1;
	}

	/*
	 * Determine if value is non-periodic.
	 */
	for (i = 1; i < m; i++) {
		overlap = true;

		/*
		 * Check if the sequence is non-periodic considering the current stride i
		 */
		if ((A[0] != A[m - 1]) && ((A[0] != A[m - 2]) || (A[1] != A[m - 1]))) {
			for (c = 0; c < m - i; c++) {
				if (A[c] != A[c + i]) {
					overlap = false;
					break;
				}
			}
		}

		/*
		 * If the sequence is periodic, exit the loop
		 */
		if (overlap == true) {
			break;
		}
	}

	/*
	 * If the value is non-periodic, append it to nonovTemplates
	 */
	if (overlap == false) {
		append_array(state->nonovTemplates, &A, m);
	}

	return;
}


/*
 * NonOverlappingTemplateMatchings_iterate - iterate one bit stream for Nonoverlapping Template test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called for each and every iteration noted in state->tp.numOfBitStreams.
 *
 * NOTE: The initialize function must be called first.
 */
void
NonOverlappingTemplateMatchings_iterate(struct state *state)
{
	struct NonOverlappingTemplateMatchings_private_stats stat;	// Stats for this iteration
	struct nonover_stats nonover_stat;	// Stats for a template of this iteration
	long int n;				// Length of a single bit stream
	long int m;				// NonOverlapping Template Test - block length
	unsigned int W_obs;			// Counter of the number of occurrences of a template in a block
	double chi2_term;			// Term used to compute chi squared
	bool match;				// Indicator of a match of a template in a block
	long int i;
	long int j;
	long int jj;
	long int k;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(132, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] == false) {
		dbg(DBG_LOW, "iterate function[%d] %s called when testVector was false", test_num, __FUNCTION__);
		return;
	}
	if (state->testNames[test_num] == NULL) {
		dbg(DBG_LOW, "iterate function[%d] %s called when testNames was NULL", test_num, __FUNCTION__);
		return;
	}
	if (state->epsilon == NULL) {
		err(132, __FUNCTION__, "state->epsilon is NULL");
	}
	if (state->nonper_seq == NULL) {
		err(132, __FUNCTION__, "state->nonper_seq is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_INIT && state->driver_state[test_num] != DRIVER_ITERATE) {
		err(132, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_INIT: %d and != DRIVER_ITERATE: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_INIT, DRIVER_ITERATE);
	}

	/*
	 * Collect parameters
	 */
	m = state->tp.nonOverlappingTemplateLength;
	if ((m * 2) > (BITS_N_LONGINT - 1)) {	// firewall
		err(132, __FUNCTION__, "(m*2): %ld is too large, 1 << (m:%ld * 2) > %ld bits long", m * 2, m, BITS_N_LONGINT - 1);
	}
	n = state->tp.n;
	stat.M = n / BLOCKS_NON_OVERLAPPING;

	/*
	 * Step 3: compute the theoretical mean mu and variance sigma_squared
	 * NOTE: The presence of the term [ 2^(2m) == 1 << m * 2 ] is the reason why MAXTEMPLEN
	 * 	 cannot be greater than 15 in architectures where long int is 32 bits.
	 */
	stat.mu = (stat.M - m + 1) / ((double) ((long int) 1 << m));
	stat.sigma_squared = stat.M * (1.0 / ((double) ((long int) 1 << m)) - (2.0 * m - 1.0) / ((double) ((long int) 1 << m * 2)));

	/*
	 * Check preconditions (firewall)
	 */
	if (stat.sigma_squared < 0.0) {
		err(132, __FUNCTION__, "sigma_squared: %f < 0.0", stat.sigma_squared);
	}
	if (isNegative(stat.mu)) {
		err(132, __FUNCTION__, "aborting %s, mean(mu) < 0.0: %f", state->testNames[test_num], stat.mu);
	}
	if (isZero(stat.mu)) {
		err(132, __FUNCTION__, "aborting %s, mean(mu) == 0.0: %f", state->testNames[test_num], stat.mu);
	}

	/*
	 * Process all template values
	 */
	for (jj = 0; jj < numOfTemplates[m]; jj++) {

		/*
		 * Get the next template from the pool of precomputed ones
		 */
		memcpy(state->nonper_seq, addr_value(state->nonovTemplates, BitSequence, m * jj), m * sizeof(BitSequence));

		/*
		 * Zeroize the occurrences counters for this template
		 */
		memset(nonover_stat.Wj, 0, sizeof(nonover_stat.Wj));

		/*
	 	 * Step 2: count the number of times that this template occurs within each block
	 	 */
		for (i = 0; i < BLOCKS_NON_OVERLAPPING; i++) {
			W_obs = 0;

			/*
			 * Count occurrences of the current template in block i
			 */
			for (j = 0; j < stat.M - m + 1; j++) {
				match = true;

				/*
				 * Check if all the m bits of the template match the bits being
				 * considered in the block.
				 */
				for (k = 0; k < m; k++) {
					if (state->nonper_seq[k] != state->epsilon[i * stat.M + j + k]) {
						match = false;
						break;
					}
				}

				/*
				 * If all the bits match, count one occurrence of this template and
				 * slide the window over m bits.
				 */
				if (match == true) {
					W_obs++;
					j += m - 1;
				}
			}

			/*
			 * Store the count of occurrences found in this block
			 */
			nonover_stat.Wj[i] = W_obs;
		}

		/*
		 * Step 4: compute the test statistic
		 */
		nonover_stat.chi2 = 0.0;
		for (i = 0; i < BLOCKS_NON_OVERLAPPING; i++) {
			chi2_term = ((double) nonover_stat.Wj[i] - stat.mu) / sqrt(stat.sigma_squared);
			nonover_stat.chi2 += (chi2_term * chi2_term);
		}

		/*
		 * Step 5: compute the test p-value
		 */
		nonover_stat.p_value = cephes_igamc(BLOCKS_NON_OVERLAPPING / 2.0, nonover_stat.chi2 / 2.0);

		/*
		 * Store the index of the template just tested in the stats
		 */
		nonover_stat.template_index = jj;

		/*
		 * Record success or failure for this iteration
		 */
		state->count[test_num]++;	// Count this iteration
		state->valid[test_num]++;	// Count this valid iteration
		if (isNegative(nonover_stat.p_value)) {
			state->failure[test_num]++;	// Bogus p_value < 0.0 treated as a failure
			nonover_stat.success = false;	// FAILURE
			warn(__FUNCTION__, "iteration %ld template[%ld] of test %s[%d] produced bogus p_value: %f < 0.0\n",
			     state->curIteration, jj, state->testNames[test_num], test_num,
			     nonover_stat.p_value);
		} else if (isGreaterThanOne(nonover_stat.p_value)) {
			state->failure[test_num]++;	// Bogus p_value > 1.0 treated as a failure
			nonover_stat.success = false;	// FAILURE
			warn(__FUNCTION__, "iteration %ld template[%ld] of test %s[%d] produced bogus p_value: %f > 1.0\n",
			     state->curIteration, jj, state->testNames[test_num], test_num,
			     nonover_stat.p_value);
		} else if (nonover_stat.p_value < state->tp.alpha) {
			state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
			state->failure[test_num]++;	// Valid p_value but too low is a failure
			nonover_stat.success = false;	// FAILURE
		} else {
			state->valid_p_val[test_num]++;	// Valid p_value in [0.0, 1.0] range
			state->success[test_num]++;	// Valid p_value not too low is a success
			nonover_stat.success = true;	// SUCCESS
		}

		/*
		 * Record values computed during this iteration
		 */
		append_value(state->p_val[test_num], &nonover_stat);
	}

	/*
	 * Record special values computed during this iteration
	 *
	 * Unlike other tests, we have one stat but multiple nonover_stat
	 * (one for each template) per iteration.
	 *
	 * NOTE: The number of nonover_stat values in state->p_val is numOfTemplates[m].
	 */
	append_value(state->stats[test_num], &stat);

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
 * NonOverlappingTemplateMatchings_print_stat - print private_stats information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      state           // run state to test under
 *      stat            // struct NonOverlappingTemplateMatchings_private_stats for format and print
 *      nonover_stats   // dynamic array of nonover_stats values
 *      nonstat_index   // starting index in nonover_stats array for 1st template of the iteration
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 *
 * NOTE: This function prints the initial header for an iteration to the stats.txt file.
 *       Finally the nonover_stats dynamic array is used to print the results from each
 *       template to stats.txt for this iteration.
 */
static bool
NonOverlappingTemplateMatchings_print_stat(FILE * stream, struct state *state,
					   struct NonOverlappingTemplateMatchings_private_stats *stat,
					   struct dyn_array *nonover_stats, long int nonstat_index)
{
	struct nonover_stats *nonover_stat;	// Current nonover_stats for a given iteration
	int io_ret;				// I/O return status
	long int i;
	long int j;

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(133, __FUNCTION__, "stream arg is NULL");
	}
	if (state == NULL) {
		err(133, __FUNCTION__, "state arg is NULL");
	}
	if (stat == NULL) {
		err(133, __FUNCTION__, "stat arg is NULL");
	}
	if (nonover_stats == NULL) {
		err(133, __FUNCTION__, "stat nonover_stats is NULL");
	}
	if (nonstat_index < 0) {
		err(133, __FUNCTION__, "stat nonstat_index: %ld < 0", nonstat_index);
	}
	if (nonstat_index >= nonover_stats->count) {
		err(133, __FUNCTION__, "stat nonstat_index: %ld >= dyn_array count: %ld", nonstat_index, nonover_stats->count);
	}

	/*
	 * Print head of stat to a file
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(stream, "\t\t  NONPERIODIC TEMPLATES TEST\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "-------------------------------------------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\t  COMPUTATIONAL INFORMATION\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "-------------------------------------------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret =
		    fprintf(stream, "\tMean = %f\tVariance = %f\tM = %ld\tm = %ld\tn = %ld\n", stat->mu, stat->sigma_squared,
			    stat->M, state->tp.nonOverlappingTemplateLength, state->tp.n);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "-------------------------------------------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t\tF R E Q U E N C Y\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "Template   W_1  W_2  W_3  W_4  W_5  W_6  W_7  W_8    Chi^2   P_value Assignment Index\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "-------------------------------------------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
	} else {
		io_ret = fprintf(stream, "\t\t  Non-periodic templates test\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "--------------------------------------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "Mean = %f\n", stat->mu);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "Variance = %f\n", stat->sigma_squared);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "M = %ld\n", stat->M);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "m = %ld\n", state->tp.nonOverlappingTemplateLength);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "n = %ld\n", state->tp.n);
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "--------------------------------------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "\t   m-bit template within a block count\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "Template   W[1] W[2] W[3] W[4] W[5] W[6] W[7] W[8]   Chi^2   P_value       Index\n");
		if (io_ret <= 0) {
			return false;
		}
		io_ret = fprintf(stream, "--------------------------------------------------------------------------------\n");
		if (io_ret <= 0) {
			return false;
		}
	}

	/*
	 * Print values for each template of this iteration
	 */
	for (i = 0; i < numOfTemplates[state->tp.nonOverlappingTemplateLength]; ++i, ++nonstat_index) {

		/*
		 * Find address of the current nonover_stats element
		 */
		if (nonstat_index >= nonover_stats->count) {
			warn(__FUNCTION__, "nonstat_index: %ld went beyond nonover_stats dyn_array count: %ld",
			     nonstat_index, nonover_stats->count);
			return false;
		}
		if (nonstat_index < 0) {
			warn(__FUNCTION__, "nonstat_index: %ld underflowed < 0", nonstat_index);
			return false;
		}
		nonover_stat = addr_value(nonover_stats, struct nonover_stats, nonstat_index);

		/*
		 * Print template bits
		 */
		for (j = 0; j < state->tp.nonOverlappingTemplateLength; j++) {
			io_ret = fprintf(stream, "%1d", (get_value(state->nonovTemplates, BitSequence,
								   state->tp.nonOverlappingTemplateLength * i + j)));
			if (io_ret <= 0) {
				return false;
			}
		}
		io_ret = fputc(' ', stream);
		if (io_ret == EOF) {
			return false;
		}

		/*
		 * Print observation count per bit in a byte
		 */
		for (j = 0; j < BITS_N_BYTE; j++) {
			io_ret = fprintf(stream, "%4d ", nonover_stat->Wj[j]);
			if (io_ret <= 0) {
				return false;
			}
		}

		/*
		 * Print remainder of template stats line
		 */
		if (nonover_stat->p_value == NON_P_VALUE && nonover_stat->success == true) {
			err(133, __FUNCTION__, "nonover_stat->p_value was set to NON_P_VALUE but "
			    "nonover_stat->success == true for jj: %ld", nonover_stat->template_index);
		}
		if (nonover_stat->success == true) {
			io_ret = fprintf(stream, "%9.6f %f SUCCESS %3ld\n", nonover_stat->chi2, nonover_stat->p_value,
					 nonover_stat->template_index);
			if (io_ret <= 0) {
				return false;
			}
		} else if (nonover_stat->p_value == NON_P_VALUE) {
			io_ret = fprintf(stream, "%9.6f	 __INVALID__ %3ld\n", nonover_stat->chi2, nonover_stat->template_index);
			if (io_ret <= 0) {
				return false;
			}
		} else {
			io_ret = fprintf(stream, "%9.6f %f FAILURE %3ld\n", nonover_stat->chi2, nonover_stat->p_value,
					 nonover_stat->template_index);
			if (io_ret <= 0) {
				return false;
			}
		}
	}

	/*
	 * Final newline for this iteration
	 */
	io_ret = fputc('\n', stream);
	if (io_ret == EOF) {
		return false;
	}

	/*
	 * All printing successful
	 */
	return true;
}


/*
 * NonOverlappingTemplateMatchings_print_p_value - print p_value information to the end of an open file
 *
 * given:
 *      stream          // open writable FILE stream
 *      p_value         // p_value iteration test result(s)
 *
 * returns:
 *      true --> no errors
 *      false --> an I/O error occurred
 */
static bool
NonOverlappingTemplateMatchings_print_p_value(FILE * stream, double p_value)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(134, __FUNCTION__, "stream arg is NULL");
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
 * NonOverlappingTemplateMatchings_print - print to results.txt, data*.txt, stats.txt for all iterations
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
NonOverlappingTemplateMatchings_print(struct state *state)
{
	struct NonOverlappingTemplateMatchings_private_stats *stat;	// Pointer to statistics of an iteration
	struct nonover_stats *nonover_stat;	// current nonover_stats for a given iteration
	FILE *stats = NULL;			// Open stats.txt file
	FILE *results = NULL;			// Open results.txt file
	FILE *data = NULL;			// Open data*.txt file
	char *stats_txt = NULL;			// Pathname for stats.txt
	char *results_txt = NULL;		// Pathname for results.txt
	char *data_txt = NULL;			// Pathname for data*.txt
	char data_filename[BUFSIZ + 1];		// Basename for a given data*.txt pathname
	bool ok;				// true -> I/O was OK
	long int nonstat_index;			// Starting index into state->nonstat from which to print
	int snprintf_ret;			// snprintf return value
	int io_ret;				// I/O return status
	long int i;
	long int j;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(135, __FUNCTION__, "state arg is NULL");
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
		err(135, __FUNCTION__,
		    "print driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(135, __FUNCTION__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->datatxt_fmt[test_num] == NULL) {
		err(135, __FUNCTION__, "format for data0*.txt filename is NULL");
	}
	if (state->driver_state[test_num] != DRIVER_ITERATE) {
		err(135, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_ITERATE: %d",
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
	nonstat_index = 0;
	for (i = 0; i < state->stats[test_num]->count; ++i) {

		/*
		 * Locate stat for this iteration
		 */
		stat = addr_value(state->stats[test_num], struct NonOverlappingTemplateMatchings_private_stats, i);

		/*
		 * Print stat to stats.txt
		 */
		errno = 0;	// paranoia
		ok = NonOverlappingTemplateMatchings_print_stat(stats, state, stat, state->p_val[test_num], nonstat_index);
		if (ok == false) {
			errp(135, __FUNCTION__, "error in writing to %s", stats_txt);
		}

		/*
		 * Print p_value to results.txt
		 */
		for (j = 0; j < numOfTemplates[state->tp.nonOverlappingTemplateLength]; ++j, ++nonstat_index) {

			/*
			 * Find address of the current nonover_stats element
			 */
			if (nonstat_index >= state->p_val[test_num]->count) {
				err(135, __FUNCTION__, "nonstat_index: %ld went beyond p_val count: %ld",
				    nonstat_index, state->p_val[test_num]->count);
			}
			if (nonstat_index < 0) {
				err(135, __FUNCTION__, "nonstat_index: %ld underflowed < 0", nonstat_index);
			}
			nonover_stat = addr_value(state->p_val[test_num], struct nonover_stats, nonstat_index);

			/*
			 * Print p_value
			 */
			errno = 0;	// paranoia
			ok = NonOverlappingTemplateMatchings_print_p_value(results, nonover_stat->p_value);
			if (ok == false) {
				errp(135, __FUNCTION__, "error in writing to %s", results_txt);
			}
		}
	}

	/*
	 * Flush and close stats.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(stats);
	if (io_ret != 0) {
		errp(135, __FUNCTION__, "error flushing to: %s", stats_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(stats);
	if (io_ret != 0) {
		errp(135, __FUNCTION__, "error closing: %s", stats_txt);
	}
	free(stats_txt);
	stats_txt = NULL;

	/*
	 * Flush and close results.txt, free pathname
	 */
	errno = 0;		// paranoia
	io_ret = fflush(results);
	if (io_ret != 0) {
		errp(135, __FUNCTION__, "error flushing to: %s", results_txt);
	}
	errno = 0;		// paranoia
	io_ret = fclose(results);
	if (io_ret != 0) {
		errp(135, __FUNCTION__, "error closing: %s", results_txt);
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
				errp(135, __FUNCTION__,
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
				for (nonstat_index = j; nonstat_index < state->p_val[test_num]->count;
				     nonstat_index += state->partitionCount[test_num]) {

					/*
					 * Get p_value for an iteration belonging to this data*.txt filename
					 */
					nonover_stat = addr_value(state->p_val[test_num], struct nonover_stats, nonstat_index);

					/*
					 * Print p_value to results.txt
					 */
					errno = 0;	// paranoia
					ok = NonOverlappingTemplateMatchings_print_p_value(data, nonover_stat->p_value);
					if (ok == false) {
						errp(135, __FUNCTION__, "error in writing to %s", data_txt);
					}

				}
			}

			/*
			 * Flush and close data*.txt, free pathname
			 */
			errno = 0;	// paranoia
			io_ret = fflush(data);
			if (io_ret != 0) {
				errp(135, __FUNCTION__, "error flushing to: %s", data_txt);
			}
			errno = 0;	// paranoia
			io_ret = fclose(data);
			if (io_ret != 0) {
				errp(135, __FUNCTION__, "error closing: %s", data_txt);
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
 * NonOverlappingTemplateMatchings_metric_print - print uniformity and proportional information for a tallied count
 *
 * given:
 *      state           // run state to test under
 *      sampleCount             // Number of bitstreams in which we counted p_values
 *      toolow                  // p_values that were below alpha
 *      freqPerBin              // uniformity frequency bins
 */
static void
NonOverlappingTemplateMatchings_metric_print(struct state *state, long int sampleCount, long int toolow, long int *freqPerBin)
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
		err(136, __FUNCTION__, "state arg is NULL");
	}
	if (freqPerBin == NULL) {
		err(136, __FUNCTION__, "freqPerBin arg is NULL");
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
		errp(136, __FUNCTION__, "error flushing to: %s", state->finalReptPath);
	}

	return;
}


/*
 * NonOverlappingTemplateMatchings_metrics - uniformity and proportional analysis of a test
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to complete the test analysis for all iterations.
 *
 * NOTE: The initialize and iterate functions must be called before this function is called.
 */
void
NonOverlappingTemplateMatchings_metrics(struct state *state)
{
	struct nonover_stats *nonover_stat;	// current nonover_stats for a given iteration
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
		err(137, __FUNCTION__, "state arg is NULL");
	}
	if (state->testVector[test_num] != true) {
		dbg(DBG_LOW, "metrics driver interface for %s[%d] called when test vector was false", state->testNames[test_num],
		    test_num);
		return;
	}
	if (state->partitionCount[test_num] < 1) {
		err(137, __FUNCTION__,
		    "metrics driver interface for %s[%d] called with state.partitionCount: %d < 0",
		    state->testNames[test_num], test_num, state->partitionCount[test_num]);
	}
	if (state->p_val[test_num]->count != (state->tp.numOfBitStreams * state->partitionCount[test_num])) {
		err(137, __FUNCTION__,
		    "print driver interface for %s[%d] called with p_val count: %ld != %ld*%d=%ld",
		    state->testNames[test_num], test_num, state->p_val[test_num]->count,
		    state->tp.numOfBitStreams, state->partitionCount[test_num],
		    state->tp.numOfBitStreams * state->partitionCount[test_num]);
	}
	if (state->driver_state[test_num] != DRIVER_PRINT) {
		err(137, __FUNCTION__, "driver state %d for %s[%d] != DRIVER_PRINT: %d",
		    state->driver_state[test_num], state->testNames[test_num], test_num, DRIVER_PRINT);
	}

	/*
	 * Allocate uniformity frequency bins
	 */
	freqPerBin = malloc(state->tp.uniformity_bins * sizeof(freqPerBin[0]));
	if (freqPerBin == NULL) {
		errp(137, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for freqPerBin",
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
			nonover_stat = addr_value(state->p_val[test_num], struct nonover_stats, i);
			p_value = nonover_stat->p_value;
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
		NonOverlappingTemplateMatchings_metric_print(state, sampleCount, toolow, freqPerBin);

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
 * NonOverlappingTemplateMatchings_destroy - post process results for this text
 *
 * given:
 *      state           // run state to test under
 *
 * This function is called once to cleanup any storage or state
 * associated with this test.
 */
void
NonOverlappingTemplateMatchings_destroy(struct state *state)
{
	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(138, __FUNCTION__, "state arg is NULL");
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
	if (state->nonovTemplates != NULL) {
		free_dyn_array(state->nonovTemplates);
		free(state->nonovTemplates);
		state->nonovTemplates = NULL;
	}
	if (state->nonper_seq != NULL) {
		free(state->nonper_seq);
		state->nonper_seq = NULL;
	}

	/*
	 * Set driver state to DRIVER_DESTROY
	 */
	dbg(DBG_HIGH, "state for driver for %s[%d] changing from %d to DRIVER_PRINT: %d",
	    state->testNames[test_num], test_num, state->driver_state[test_num], DRIVER_DESTROY);
	state->driver_state[test_num] = DRIVER_DESTROY;

	return;
}
