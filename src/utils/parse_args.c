// parse_args.c
// Parse command line arguments for batch automation of sts.c

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


// Exit codes: 0 thru 4
// NOTE: 5-9 is used in sts.c

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <math.h>
#include "../constants/externs.h"
#include "utilities.h"
#include "debug.h"

#define GENERATOR_COUNT NUMOFGENERATORS

/*
 * Forward static function declarations
 */
static void change_params(struct state *state, long int parameter, long int value, double d_value);

/*
 * Default run state
 *
 * Default state allows sts to run without any human intervention
 */
static struct state const defaultstate = {
/* *INDENT-OFF* */

	// batchmode
	false,				// Classic (interactive) mode

	// testVectorFlag & testVector
	false,				// No -t test1[,test2].. was given
	{false, false, false, false,
	 false, false, false, false,
	 false, false, false, false,
	 false, false, false, false,
	},				// Do not invoke any test

	// generatorFlag & generator
	false,				// No -g generator was given
	GENERATOR_FROM_FILE,		// Read data from a file (-g 0)

	// iterationFlag
	false,				// No -i iterations was given

	// reportCycleFlag & reportCycle
	false,				// No -I reportCycle was given
	0,				// Do not report on iteration progress

	// runModeFlag & runMode
	false,				// No -m mode was given
	MODE_ITERATE_AND_ASSESS,	// Iterate and assess only

	// workDirFlag & workDir
	false,				// No -w workDir was given
	"experiments",			// Write results under experiments
					// Depending on -b and -g gen, this may become ./experiments/generator

	// subDirsFlag & subDirs
	false,				// No -c was given
	true,				// Create directories

	// resultstxtFlag
	false,				// No -s, don't create results.txt, data*.txt and stats.txt files

	// randomDataFlag & randomDataPath
	false,				// No -f randdata was given
	"/dev/null",			// Input file is /dev/null

	// dataFormatFlag & dataFormat
	false,				// -F format was not given
	FORMAT_RAW_BINARY,		// Read data as raw binary

	// jobnumFlag & jobnum
	false,				// No -j jobnum was given
	0,				// Begin at start of randdata (-j 0)

	// tp, promptFlag, uniformityBinsFlag
	{DEFAULT_BLOCK_FREQUENCY,	// -P 1=M, Block Frequency Test - block length
	 DEFAULT_NON_OVERLAPPING,	// -P 2=m, NonOverlapping Template Test - block length
	 DEFAULT_OVERLAPPING,		// -P 3=m, Overlapping Template Test - block length
	 DEFAULT_APEN,			// -P 4=m, Approximate Entropy Test - block length
	 DEFAULT_SERIAL,		// -P 5=m, Serial Test - block length
	 DEFAULT_LINEARCOMPLEXITY,	// -P 6=M, Linear Complexity Test - block length
	 DEFAULT_ITERATIONS,		// -P 7=iterations (-i iterations)
	 DEFAULT_UNIFORMITY_BINS,	// -P 8=bins, uniformity test is thru this many bins
	 DEFAULT_BITCOUNT,		// -P 9=bitcount, Length of a single bit stream
	 DEFAULT_UNIFORMITY_LEVEL,	// -P 10=uni_level, uniformity errors have values below this
	 DEFAULT_ALPHA,			// -P 11=alpha, p_value significance level
	},
	false,				// No -p, prompt for change of parameters if no -b
	false,				// No -P 8 was given with custom uniformity bins

	// c, cSetup
	{UNSET_DOUBLE,			// Square root of 2
	 UNSET_DOUBLE,			// log(2)
	 UNSET_DOUBLE,			// Square root of n
	 UNSET_DOUBLE,			// log(n)
	 0,				// Number of crossings required to complete the test
	},
	false,				// init() has not yet initialized c

	// streamFile, finalReptPath, finalRept, freqFilePath, finalRept
	NULL,				// Initially the randomDataPath is not open
	NULL,				// Path of the final results file
	NULL,				// Initially the final results file is not open
	NULL,				// Path of freq.txt
	NULL,				// Initially freq.txt is not open

	// generatorDir
	{"AlgorithmTesting",		// -g 0, Read from file
	 "LCG",				// -g 1, Linear Congruential
	 "QCG1",			// -g 2, Quadratic Congruential I
	 "QCG2",			// -g 3, Quadratic Congruential II
	 "CCG",				// -g 4, Cubic Congruential
	 "XOR",				// -g 5, XOR
	 "MODEXP",			// -g 6, Modular Exponentiation
	 "BBS",				// -g 7, Blum-Blum-Shub
	 "MS",				// -g 8, Micali-Schnorr
	 "G-SHA1",			// -g 9, G Using SHA-1
	},

	// testNames, subDir, driver_state
	{"((all_tests))",		// TEST_ALL = 0, convention for indicating run all tests
	 "Frequency",			// TEST_FREQUENCY = 1, Frequency test (frequency.c)
	 "BlockFrequency",		// TEST_BLOCK_FREQUENCY = 2, Block Frequency test (blockFrequency.c)
	 "CumulativeSums",		// TEST_CUSUM = 3, Cumulative Sums test (cusum.c)
	 "Runs",			// TEST_RUNS = 4, Runs test (runs.c)
	 "LongestRun",			// TEST_LONGEST_RUN = 5, Longest Runs test (longestRunOfOnes.c)
	 "Rank",			// TEST_RANK = 6, Rank test (rank.c)
	 "DFT",				// TEST_DFT = 7, Discrete Fourier Transform test (discreteFourierTransform.c)
	 "NonOverlappingTemplate",	// TEST_NON_OVERLAPPING = 8, Non-overlapping Template Matchings test
					// (nonOverlappingTemplateMatchings.c)
	 "OverlappingTemplate",		// TEST_OVERLAPPING = 9, Overlapping Template test (overlappingTemplateMatchings.c)
	 "Universal",			// TEST_UNIVERSAL = 10, Universal test (universal.c)
	 "ApproximateEntropy",		// TEST_APEN = 11, Approximate Entropy test (approximateEntropy.c)
	 "RandomExcursions",		// TEST_RND_EXCURSION = 12, Random Excursions test (randomExcursions.c)
	 "RandomExcursionsVariant",	// TEST_RND_EXCURSION_VAR = 13, Random Excursions Variant test (randomExcursionsVariant.c)
	 "Serial",			// TEST_SERIAL = 14, Serial test (serial.c)
	 "LinearComplexity",		// TEST_LINEARCOMPLEXITY = 15, Linear Complexity test (linearComplexity.c)
	},
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	},
	{DRIVER_NULL, DRIVER_NULL, DRIVER_NULL, DRIVER_NULL, DRIVER_NULL, DRIVER_NULL, DRIVER_NULL, DRIVER_NULL,
	 DRIVER_NULL, DRIVER_NULL, DRIVER_NULL, DRIVER_NULL, DRIVER_NULL, DRIVER_NULL, DRIVER_NULL, DRIVER_NULL,
	},

	// partitionCount, datatxt_fmt
	{0,					// TEST_ALL = 0 - not a real test
	 1,					// TEST_FREQUENCY = 1
	 1,					// TEST_BLOCK_FREQUENCY = 2
	 2,					// TEST_CUSUM = 3
	 1,					// TEST_RUNS = 4
	 1,					// TEST_LONGEST_RUN = 5
	 1,					// TEST_RANK = 6
	 1,					// TEST_DFT = 7
	 MAX_NUMOFTEMPLATES,			// TEST_NON_OVERLAPPING = 8
						// NOTE: Value may be changed by OverlappingTemplateMatchings_init()
	 1,					// TEST_OVERLAPPING = 9
	 1,					// TEST_UNIVERSAL = 10
	 1,					// TEST_APEN = 11
	 NUMBER_OF_STATES_RND_EXCURSION,	// TEST_RND_EXCURSION = 12
	 NUMBER_OF_STATES_RND_EXCURSION_VAR,	// TEST_RND_EXCURSION_VAR = 13
	 2,					// TEST_SERIAL = 14
	 1,					// TEST_LINEARCOMPLEXITY = 15
	},
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	},

	// stats, p_val - per test dynamic arrays
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	},
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	},

	// is_excursion
	{false, false, false, false, false, false, false, false,
	 false, false, false, false, true, true, false, false,
	},

	// epsilon, tmpepsilon
	NULL,
	NULL,

	// count, valid, success, failure, valid_p_val
	{0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	},
	{0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	},
	{0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	},
	{0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	},
	{0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	},

	// metric_results & successful_tests
	{FAILED_BOTH, FAILED_BOTH, FAILED_BOTH, FAILED_BOTH,
	 FAILED_BOTH, FAILED_BOTH, FAILED_BOTH, FAILED_BOTH,
	 FAILED_BOTH, FAILED_BOTH, {FAILED_BOTH, FAILED_BOTH},
	 {FAILED_BOTH, FAILED_BOTH}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
	},
	0,

	// maxGeneralSampleSize, maxRandomExcursionSampleSize
	0,
	0,

	// nonovTemplates
	NULL,

	// fft_m, fft_X
	NULL,
	NULL,

# if defined(LEGACY_FFT)
	// fft_wsave
	NULL,
#else /* LEGACY_FFT */
	// fftw_p and fftw_out
	NULL,
	NULL,
#endif /* LEGACY_FFT */

	// rank_matrix
	NULL,

	// rnd_excursion_var_stateX, ex_var_partial_sums
	NULL,
	NULL,

	// linear_b, linear_c, linear_t
	NULL,
	NULL,
	NULL,

	// apen_C, apen_C_len
	NULL,
	0,

	// serial_v, serial_v_len
	NULL,
	0,

	// nonper_seq
	NULL,

	// universal_L, universal_T
	0,
	0,

	// rnd_excursion_S, rnd_excursion_cycle, rnd_excursion_stateX, rnd_excursion_pi_terms
	NULL,
	NULL,
	NULL,
	NULL,

	// legacy_output
	false,

	// curIteration
	0,			// No interaction completed so far
/* *INDENT-ON* */
};


/*
 * Command line usage information
 */
/* *INDENT-OFF* */
static const char * const usage =
"[-v level] [-b] [-t test1[,test2]..] [-g generator]\n"
"               [-P num=value[,num=value]..] [-p] [-i iterations] [-I reportCycle] [-O]\n"
"               [-w workDir] [-c] [-s] [-f randdata] [-F format] [-j jobnum]\n"
"               [-m mode] [-h] bitcount\n"
"\n"
"    -v  debuglevel     debug level (def: 0 -> no debug messages)\n"
"    -b                 batch mode - no stdin (def: prompt when needed)\n"
"    -t test1[,test2].. tests to invoke, 0-15 (def: 0 -> run all tests)\n"
"\n"
"        0: Run all tests (1-15)\n"
"        1: Frequency                        2: Block Frequency\n"
"        3: Cumulative Sums                  4: Runs\n"
"        5: Longest Run of Ones              6: Rank\n"
"        7: Discrete Fourier Transform       8: Nonperiodic Template Matchings\n"
"        9: Overlapping Template Matchings  10: Universal Statistical\n"
"       11: Approximate Entropy             12: Random Excursions\n"
"       13: Random Excursions Variant       14: Serial\n"
"       15: Linear Complexity\n"
"\n"
"    -g generator       generator to use, 0-9 (if -b, def: 0)\n"
"\n"
"       0: Read from file               1: Linear Congruential\n"
"       2: Quadratic Congruential I     3: Quadratic Congruential II\n"
"       4: Cubic Congruential           5: XOR\n"
"       6: Modular Exponentiation       7: Blum-Blum-Shub\n"
"       8: Micali-Schnorr               9: G Using SHA-1\n"
"\n"
"    -P num=value[,num=value]..     change parameter num to value (def: keep defaults)\n"
"\n"
"       1: Block Frequency Test - block length(M):		128\n"
"       2: NonOverlapping Template Test - block length(m):	9\n"
"       3: Overlapping Template Test - block length(m):		9\n"
"       4: Approximate Entropy Test - block length(m):		10\n"
"       5: Serial Test - block length(m):			16\n"
"       6: Linear Complexity Test - block length(M):		500\n"
"       7: Number of bitcount runs (same as -i iterations):	1\n"
"       8: Uniformity bins:                         		sqrt(iterations) or 10 (if -O)\n"
"       9: Length of a single bit stream (bitcount):		1000000\n"
"      10: Uniformity Cutoff Level:				0.0001\n"
"      11: Alpha Confidence Level:				0.01\n"
"\n"
"    -p    In interactive mode (no -b), do not prompt for parameters (def: prompt)\n"
"\n"
"    -i iterations    number of bitstream runs (if -b, def: 1)\n"
"                     this flag is the same as -P 7=iterations\n"
"\n"
"    -I reportCycle   report after completion of reportCycle iterations (def: 0: do not report)\n"
"    -O               try to mimic output format of legacy code (def: don't be output compatible)\n"
"\n"
"    -w workDir       write experiment results under workDir (def: .)\n"
"    -c               don't create any directories needed for creating files (def: do create)\n"
"    -s               create result.txt, data*.txt, and stats.txt (def: don't create)\n"
"    -f randdata      -g 0 inputfile is randdata (required if -b and -g 0)\n"
"    -F format        randdata format: 'r': raw binary, 'a': ASCII '0'/'1' chars (def: 'r')\n"
"    -j jobnum        seek into randdata, jobnum*bitcount*iterations bits (def: 0)\n"
"\n"
"    -m mode          w --> only write generated data to -f randdata in -F format (-g must not be 0)\n"
"                     i --> iterate only (not currently implemented)\n"
"                     a --> assess only (not currently implemented)\n"
"                     b --> iterate and then assess (default: b)\n"
"\n"
"    -h               print this message and exit\n"
"\n"
"    bitcount         length of a single bit stream, must be a multiple of 8 (same as -P 9=bitcount)\n";
/* *INDENT-ON* */


/*
 * parse_args - parse command line arguments and setup run state
 *
 * given:
 *      state           // run state it initialize and set according to command line
 *      argc            // command line arg count
 *      argv            // array of argument strings
 *
 * This function does not return on error.
 */
void
parse_args(struct state *state, int argc, char **argv)
{
	int option;		// getopt() parsed option
	extern char *optarg;	// Parsed option argument
	extern int optind;	// Index to the next argv element to parse
	extern int opterr;	// 0 ==> disable internal getopt() error messages
	extern int optopt;	// Last known option character returned by getopt()
	int scan_cnt;		// Number of items scanned by sscanf()
	char *brkt;		// Last state of strtok_r()
	char *phrase;		// String without separator as parsed by strtok_r()
	long int testnum;	// Parsed test number
	long int num;		// Parsed parameter number
	long int value;		// Parsed parameter integer value
	double d_value;		// Parsed parameter floating point
	bool success = false;	// true if str2longtint was successful
	int snprintf_ret;	// snprintf return value
	int test_cnt = 0;
	long int i;
	size_t j;

	/*
	 * Record the program name
	 */
	program = argv[0];
	if (program == NULL) {
		program = "((NULL))";	// paranoia
	} else if (program[0] == '\0') {
		program = "((empty))";	// paranoia
	}

	/*
	 * Check preconditions (firewall)
	 */
	if (argc <= 0 || state == NULL) {
		err(1, __func__, "called with bogus args");
	}

	/*
	 * Initialize state to default state
	 */
	*state = defaultstate;

	/*
	 * Parse the command line arguments
	 */
	opterr = 0;
	brkt = NULL;
	while ((option = getopt(argc, argv, "v:bt:g:P:pi:I:Ow:csf:F:j:m:h")) != -1) {
		switch (option) {

		case 'v':	// -v debuglevel
			debuglevel = str2longint(&success, optarg);
			if (success == false) {
				usage_errp(usage, 1, __func__, "error in parsing -v debuglevel: %s", optarg);
			} else if (debuglevel < 0) {
				usage_err(usage, 1, __func__, "error debuglevel: %lu must >= 0", debuglevel);
			}
			break;

		case 'b':	// -b (batch, non-interactive mode)
			state->batchmode = true;
			state->promptFlag = true;	// -b implies -p
			break;

		case 't':	// -t test1[,test2]..
			state->testVectorFlag = true;

			/*
			 * Parse each comma separated arg
			 */
			for (phrase = strtok_r(optarg, ",", &brkt); phrase != NULL; phrase = strtok_r(NULL, ",", &brkt)) {

				/*
			 	 * Determine test number
			 	 */
				testnum = str2longint(&success, phrase);
				if (success == false) {
					usage_errp(usage, 1, __func__,
						   "-t test1[,test2].. must only have comma separated integers: %s", phrase);
				}

				/*
				 * Enable all tests if testnum is 0 (special case)
				 */
				if (testnum == 0) {
					for (i = 1; i <= NUMOFTESTS; i++) {
						state->testVector[i] = true;
					}
				} else if (testnum < 0 || testnum > NUMOFTESTS) {
					usage_err(usage, 1, __func__, "-t test: %lu must be in the range [0-%d]", testnum,
						  NUMOFTESTS);
				} else {
					state->testVector[testnum] = true;
				}
			}
			break;


		case 'g':	// -g generator
			state->generatorFlag = true;
			i = str2longint(&success, optarg);
			if (success == false) {
				usage_errp(usage, 1, __func__, "-g generator must be numeric: %s", optarg);
			}
			if (i < 0 || i > GENERATOR_COUNT) {
				usage_err(usage, 1, __func__, "-g generator: %ld must be [0-%d]", i, GENERATOR_COUNT);
			}
			state->generator = (enum gen) i;
			break;

		case 'P':	// -P num=value[,num=value]..

			/*
			 * Parse each comma separated arg
			 */
			for (phrase = strtok_r(optarg, ",", &brkt); phrase != NULL; phrase = strtok_r(NULL, ",", &brkt)) {

				/*
				 * Parse parameter number
				 */
				scan_cnt = sscanf(phrase, "%ld=", &num);
				if (scan_cnt == EOF) {
					usage_errp(usage, 1, __func__,
						   "-P num=value[,num=value].. error parsing a num=value: %s", phrase);
				} else if (scan_cnt != 2) {
					usage_err(usage, 1, __func__,
						  "-P num=value[,num=value].. "
						  "failed to parse a num=value, expecting integer=integer: %s", phrase);
				}
				if (num < MIN_PARAM || num > MAX_PARAM) {
					usage_err(usage, 1, __func__,
						  "-P num=value[,num=value].. num: %lu must be in the range [1-%d]", num,
						  MAX_PARAM);
				}

				/*
				 * Parse parameter value
				 */
				if (num <= MAX_INT_PARAM) {

					// Parse parameter number as an integer
					scan_cnt = sscanf(phrase, "%ld=%ld", &num, &value);
					if (scan_cnt == EOF) {
						usage_errp(usage, 1, __func__,
							   "-P num=value[,num=value].. error parsing a num=value: %s", phrase);
					} else if (scan_cnt != 2) {
						usage_err(usage, 1, __func__,
							  "-P num=value[,num=value].. "
							  "failed to parse a num=value, expecting integer=integer: %s", phrase);
					}
					if (num < 0 || num > MAX_PARAM) {
						usage_err(usage, 1, __func__,
							  "-P num=value[,num=value].. num: %lu must be in the range [1-%d]", num,
							  MAX_PARAM);
					}
					change_params(state, num, value, 0.0);
				} else {

					// Parse parameter number as a floating point number
					scan_cnt = sscanf(phrase, "%ld=%lf", &num, &d_value);
					if (scan_cnt == EOF) {
						usage_errp(usage, 1, __func__,
							   "-P num=value[,num=value].. error parsing a num=value: %s", phrase);
					} else if (scan_cnt != 2) {
						usage_err(usage, 1, __func__,
							  "-P num=value[,num=value].. "
							  "failed to parse a num=value, expecting integer=integer: %s", phrase);
					}
					if (num < 0 || num > MAX_PARAM) {
						usage_err(usage, 1, __func__,
							  "-P num=value[,num=value].. num: %lu must be in the range [1-%d]", num,
							  MAX_PARAM);
					}
					change_params(state, num, 0, d_value);
				}
			}
			break;

		case 'p':	// -p (interactive mode (no -b), do not prompt for change of parameters)
			state->promptFlag = true;
			break;

		case 'i':	// -i iterations
			state->iterationFlag = true;
			state->tp.numOfBitStreams = str2longint(&success, optarg);
			if (success == false) {
				usage_errp(usage, 1, __func__, "error in parsing -i iterations: %s", optarg);
			}
			if (state->tp.numOfBitStreams < 1) {
				usage_err(usage, 1, __func__, "iterations (number of bit streams): %lu can't be less than 1",
					  state->tp.numOfBitStreams);
			}
			break;

		case 'I':	// -I reportCycle
			state->reportCycleFlag = true;
			state->reportCycle = str2longint(&success, optarg);
			if (success == false) {
				usage_errp(usage, 1, __func__, "error in parsing -I reportCycle: %s", optarg);
			}
			if (state->reportCycle < 0) {
				usage_err(usage, 1, __func__, "-I reportCycle: %lu must be >= 0", state->reportCycle);
			}
			break;

		case 'O':	// -O (try to mimic output format of legacy code)
			state->legacy_output = true;
			break;

		case 'w':	// -w workDir

			/*
			 * Write experiment results under workDir
			 */
			state->workDirFlag = true;
			state->workDir = strdup(optarg);
			if (state->workDir == NULL) {
				errp(1, __func__, "strdup of %lu bytes for -w workDir failed", strlen(optarg));
			}
			break;

		case 'c':	// -c (do not create directories)
			state->subDirsFlag = true;
			state->subDirs = false;	// do not create directories
			break;

		case 's':	// -s (create result.txt and stats.txt)
			state->resultstxtFlag = true;
			break;

		case 'F':	// -F format: 'r' or '1': raw binary, 'a' or '0': ASCII '0'/'1' chars
			state->dataFormatFlag = true;
			state->dataFormat = (enum format) (optarg[0]);
			switch (state->dataFormat) {
			case FORMAT_0:
				state->dataFormat = FORMAT_ASCII_01;
				break;
			case FORMAT_1:
				state->dataFormat = FORMAT_RAW_BINARY;
				break;
			case FORMAT_ASCII_01:
			case FORMAT_RAW_BINARY:
				break;
			default:
				err(1, __func__, "-F format: %s must be r or a", optarg);
			}
			if (optarg[1] != '\0') {
				err(1, __func__, "-F format: %s must be a single character: r or a", optarg);
			}
			break;

		case 'f':	// -f randdata (path to input data)
			state->randomDataFlag = true;
			state->randomDataPath = strdup(optarg);
			if (state->randomDataPath == NULL) {
				errp(1, __func__, "strdup of %lu bytes for -f randdata failed", strlen(optarg));
			}
			break;

		case 'j':	// -j jobnum (seek into randomData)
			state->jobnumFlag = true;
			state->jobnum = str2longint(&success, optarg);
			if (success == false) {
				usage_errp(usage, 1, __func__, "error in parsing -j jobnum: %s", optarg);
			}
			if (state->jobnum < 0) {
				usage_err(usage, 1, __func__, "-j jobnum: %lu must be greater than 0", state->jobnum);
			}
			break;

		case 'm':	// -m mode (w-->write only. i-->iterate only, a-->assess only, b-->iterate & assess)
			state->runModeFlag = true;
			if (optarg[0] == '\0' || optarg[1] != '\0') {
				usage_err(usage, 1, __func__, "-m mode must be a single character: %s", optarg);
			}
			switch (optarg[0]) {
			case MODE_WRITE_ONLY:
				state->runMode = MODE_WRITE_ONLY;
				state->resultstxtFlag = false;	// -m w implies that result.txt and stats.txt are not written
				break;
			case MODE_ASSESS_ONLY:
			case MODE_ITERATE_ONLY:
				usage_err(usage, 1, __func__, "-m %c not yet implemented, sorry!", (char) optarg[0]);
				break;
			case MODE_ITERATE_AND_ASSESS:
				state->runMode = MODE_ITERATE_AND_ASSESS;
				break;
			default:
				usage_err(usage, 1, __func__, "-m mode must be one of w, i, a, or b: %c", (char) optarg[0]);
				break;
			}
			break;

		case 'h':	// -h (print out help)
			usage_err(usage, 0, __func__, "this arg ignored");
			break;

			// ----------------------------------------------------------------

		case '?':
		default:
			if (option == '?') {
				usage_errp(usage, 1, __func__, "getopt returned an error");
			} else {
				usage_err(usage, 1, __func__, "unknown option: -%c", (char) optopt);
			}
			break;
		}
	}
	if (optind >= argc) {
		usage_err(usage, 1, __func__, "missing required bitcount argument");
	}
	if (optind != argc - 1) {
		usage_err(usage, 1, __func__, "unexpected arguments");
	}
	state->tp.n = str2longint(&success, argv[optind]);
	if (success == false) {
		usage_errp(usage, 1, __func__, "bitcount(n) must be a number: %s", argv[optind]);
	}
	if ((state->tp.n % 8) != 0) {
		usage_err(usage, 1, __func__, "bitcount(n): %ld must be a multiple of 8. The added complexity of supporting "
				"a sequence that starts or ends on a non-byte boundary outweighs the convenience of "
				"permitting arbitrary bit lengths", state->tp.n);
	}
	if (state->tp.n < GLOBAL_MIN_BITCOUNT) {
		usage_err(usage, 1, __func__, "bitcount(n): %ld must >= %d", state->tp.n, GLOBAL_MIN_BITCOUNT);
	}

	/*
	 * Ask how many iterations have to be performed unless batch mode (-b) is enabled or -i bitstreams was given
	 */
	if (state->batchmode == false && state->iterationFlag == false) {
		// Ask question
		printf("   How many bitstreams? ");
		fflush(stdout);

		// Read numberic answer
		state->tp.numOfBitStreams = getNumber(stdin, stdout);
		putchar('\n');
		fflush(stdout);
	}

	/*
	 * Post processing: make sure complimentary/conflicting options are set/not set
	 */
	if (state->batchmode == true) {
		if (state->generatorFlag == true) {
			if (state->generator == 0 && state->randomDataFlag == false) {
				usage_err(usage, 1, __func__, "-f randdata required when using -b and -g 0");
			}
		} else {
			if (state->randomDataFlag == false) {
				usage_err(usage, 1, __func__, "-f randdata required when using -b and no -g generator given");
			}
		}
	}
	if (state->generator == 0) {
		if (state->runMode == MODE_WRITE_ONLY) {
			usage_err(usage, 1, __func__, "when -m w the -g generator must be non-zero");
		}
	} else {
		if (state->randomDataFlag == true && state->runMode != MODE_WRITE_ONLY) {
			usage_err(usage, 1, __func__, "-f randdata may only be used with -g 0 (read from file) or -m w");
		}
		if (state->jobnumFlag == true) {
			usage_err(usage, 1, __func__, "-j jobnum may only be used with -g 0 (read from file)");
		}
	}

	/*
	 * If -b and no -t test given, enable all tests
	 *
	 * Test 0 is an historical alias for all tests enabled.
	 */
	if ((state->batchmode == true && state->testVectorFlag == false) || state->testVector[0] == true) {
		// Under -b without and -t test, enable all tests
		for (i = 1; i <= NUMOFTESTS; i++) {
			state->testVector[i] = true;
		}
	}

	/*
	 * Count the number of tests enabled
	 *
	 * We do not count test 0 (historical alias for all tests enabled).
	 */
	if (state->batchmode == true) {
		for (i = 1; i <= NUMOFTESTS; i++) {
			if (state->testVector[i] == true) {
				++test_cnt;
			}
		}
	}
	if (test_cnt == 0 && state->batchmode == true) {
		err(1, __func__, "no tests enabled");
	}

	/*
	 * Set the workDir if no -w workdir was given
	 */
	if (state->workDirFlag != true) {

		/*
		 * -b batch mode: default workdir is the name of the generator
		 */
		if (state->batchmode == true) {
			state->workDir = strdup(state->generatorDir[state->generator]);
			if (state->workDir == NULL) {
				errp(1, __func__, "strdup of %s failed", state->generatorDir[state->generator]);
			}
		}

		/*
		 * Classic mode (no -b): default workdir is experiments/__generator_name__
		 */
		else {
			j = strlen("experiments/") + strlen(state->generatorDir[state->generator]) + 1;	// string + NUL
			state->workDir = malloc(j + 1);	// +1 for later paranoia
			if (state->workDir == NULL) {
				errp(1, __func__, "cannot malloc of %ld elements of %ld bytes each for experiments/%s",
				     j + 1, sizeof(state->workDir[0]), state->generatorDir[state->generator]);
			}
			errno = 0;	// paranoia
			snprintf_ret = snprintf(state->workDir, j, "experiments/%s", state->generatorDir[state->generator]);
			state->workDir[j] = '\0';	// paranoia
			if (snprintf_ret <= 0 || snprintf_ret > j || errno != 0) {
				errp(1, __func__,
				     "snprintf failed for %ld bytes for experiments/%s, returned: %d",
				     j, state->generatorDir[state->generator], snprintf_ret);
			}
		}
	}

	/*
	 * Set the number of uniformity bins to sqrt(iterations) if running in non-legacy mode and
	 * no custom number was provided
	 */
	if (state->uniformityBinsFlag == false && state->legacy_output == false) {
		// state->tp.uniformity_bins = (long int) sqrt(state->tp.numOfBitStreams); TODO uncomment when the new metric print is done
	}

	/*
	 * Set the number of uniformity bins back to their default value if a custom number was
	 * provided but the legacy mode is on
	 */
	if (state->uniformityBinsFlag == true && state->legacy_output == true) {
		warn(__func__, "The number of uniformity bins was set back to %d due to '-O' legacy mode flag",
		    DEFAULT_UNIFORMITY_BINS);
		state->tp.uniformity_bins = DEFAULT_UNIFORMITY_BINS;
	}

	/*
	 * Report on how we will run, if debugging
	 */
	if (debuglevel > 0) {
		print_option_summary(state, "parsed command line");
	}

	return;
}


static void
change_params(struct state *state, long int parameter, long int value, double d_value)
{
	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(2, __func__, "state arg is NULL");
	}

	/*
	 * Load integer or floating point value into a parameter
	 */
	switch (parameter) {
	case PARAM_blockFrequencyBlockLength:
		state->tp.blockFrequencyBlockLength = value;
		break;
	case PARAM_nonOverlappingTemplateBlockLength:
		state->tp.nonOverlappingTemplateLength = value;
		break;
	case PARAM_overlappingTemplateBlockLength:
		state->tp.overlappingTemplateLength = value;
		break;
	case PARAM_approximateEntropyBlockLength:
		state->tp.approximateEntropyBlockLength = value;
		break;
	case PARAM_serialBlockLength:
		state->tp.serialBlockLength = value;
		break;
	case PARAM_linearComplexitySequenceLength:
		state->tp.linearComplexitySequenceLength = value;
		break;
	case PARAM_numOfBitStreams:
		state->uniformityBinsFlag = true;
		state->tp.numOfBitStreams = value;
		break;
	case PARAM_uniformity_bins:
		state->tp.uniformity_bins = value;
		break;
	case PARAM_n:		// see also final argument: argv[argc-1]
		state->tp.n = value;
		break;
	case PARAM_uniformity_level:
		state->tp.uniformity_level = d_value;
		break;
	case PARAM_alpha:
		state->tp.alpha = d_value;
		break;
	default:
		err(2, __func__, "invalid parameter option: %ld", parameter);
		break;
	}

	return;
}

void
print_option_summary(struct state *state, char *where)
{
	int j;
	int test_cnt = 0;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(3, __func__, "state arg is NULL");
	}
	if (where == NULL) {
		err(3, __func__, "where arg is NULL");
	}

	/*
	 * Report on high level state
	 */
	dbg(DBG_LOW, "High level state for: %s", where);
	dbg(DBG_LOW, "\tsts version: %s", version);
	dbg(DBG_LOW, "\tdebuglevel = %ld", debuglevel);
	if (state->batchmode == true) {
		dbg(DBG_LOW, "\tRunning in (non-interactive) batch mode");
		if (state->runModeFlag == true) {
			switch (state->runMode) {

			case MODE_WRITE_ONLY:
				switch (state->dataFormat) {
				case FORMAT_ASCII_01:
				case FORMAT_0:
					dbg(DBG_MED, "\t    -m w: only write generated data to -f file: %s in -F format: %s",
					    state->randomDataPath, "ASCII '0' and '1' character bits");
					break;
				case FORMAT_RAW_BINARY:
				case FORMAT_1:
					dbg(DBG_MED, "\t    -m w: only write generated data to -f file: %s in -F format: %s",
					    state->randomDataPath, "raw 8 binary bits per byte");
					break;
				default:
					dbg(DBG_MED, "\t    -m w: only write generated data to -f file: %s"
					    "in -F format: unknown: %c", state->randomDataPath, (char) state->dataFormat);
					break;
				}
				break;

			case MODE_ITERATE_ONLY:
				dbg(DBG_LOW, "\tWill iterate only (no assessment)");
				break;

			case MODE_ASSESS_ONLY:
				dbg(DBG_LOW, "\tWill assess only (skip iteration)");
				break;

			case MODE_ITERATE_AND_ASSESS:
				dbg(DBG_LOW, "\tWill iterate and assess");
				break;

			default:
				dbg(DBG_LOW, "\tUnknown assessment mode: %c", state->runMode);
				break;
			}
		} else {
			dbg(DBG_LOW, "\tWill iterate and assess");
		}
		dbg(DBG_MED, "\tTesting %lld bits of data (%lld bytes)", (long long) state->tp.numOfBitStreams *
				(long long) state->tp.n, (((long long) state->tp.numOfBitStreams *
				(long long) state->tp.n) + 7) / 8);
		dbg(DBG_LOW, "\tPerforming %ld iterations each of %ld bits\n", state->tp.numOfBitStreams, state->tp.n);
	} else {
		dbg(DBG_LOW, "\tclassic (interactive mode)\n");
	}

	/*
	 * Report on tests enabled
	 */
	dbg(DBG_MED, "Tests enabled:");
	for (j = 1; j <= NUMOFTESTS; j++) {
		if (state->testVector[j]) {
			dbg(DBG_MED, "\ttest[%d] %s: enabled", j, state->testNames[j]);
			++test_cnt;
		}
	}
	if (state->batchmode == true) {
		dbg(DBG_MED, "\t%d tests enabled\n", test_cnt);
	} else {
		if (state->testVectorFlag == true) {
			dbg(DBG_LOW, "\t-t used, will NOT prompt for tests to enable\n");
		} else {
			dbg(DBG_LOW, "\tno -t used, will prompt for tests to enable\n");
		}
	}

	/*
	 * Report on generator (or file) to be used
	 */
	if (state->generatorFlag == true) {
		switch (state->generator) {

		case GENERATOR_FROM_FILE:
			if (state->batchmode == true) {
				dbg(DBG_LOW, "Testing data from file:\n\t%s\n", state->randomDataPath);
			} else {
				dbg(DBG_LOW, "Will use data from a file");
				dbg(DBG_LOW, "\tWill prompt for filename\n");
			}
			break;

		case GENERATOR_LCG:
			dbg(DBG_LOW, "Using builtin generator [%d]: Linear Congruential\n", state->generator);
			break;

		case GENERATOR_QCG1:
			dbg(DBG_LOW, "Using builtin generator [%d]: Quadratic Congruential I\n", state->generator);
			break;

		case GENERATOR_QCG2:
			dbg(DBG_LOW, "Using builtin generator [%d]: Quadratic Congruential II\n", state->generator);
			break;

		case GENERATOR_CCG:
			dbg(DBG_LOW, "Using builtin generator [%d]: Cubic Congruential\n", state->generator);
			break;

		case GENERATOR_XOR:
			dbg(DBG_LOW, "Using builtin generator [%d]: XOR\n", state->generator);
			break;

		case GENERATOR_MODEXP:
			dbg(DBG_LOW, "Using builtin generator [%d]: Modular Exponentiation\n", state->generator);
			break;

		case GENERATOR_BBS:
			dbg(DBG_LOW, "Using builtin generator [%d]: Blum-Blum-Shub\n", state->generator);
			break;

		case GENERATOR_MS:
			dbg(DBG_LOW, "Using builtin generator [%d]: Micali-Schnorr\n", state->generator);
			break;

		case GENERATOR_SHA1:
			dbg(DBG_LOW, "Using builtin generator [%d]: G Using SHA-1\n", state->generator);
			break;

		default:
			dbg(DBG_LOW, "Unknown generator [%d]\n", state->generator);
			break;
		}
	} else if (state->batchmode == true) {
		dbg(DBG_LOW, "Testing data from file: %s", state->randomDataPath);
	} else {
		dbg(DBG_LOW, "Will prompt user for generator to use");
	}

	/*
	 * Report detailed state summary
	 */
	dbg(DBG_MED, "Details of run state:");
	dbg(DBG_MED, "\tgenerator: -g %d", state->generator);
	if (state->iterationFlag == true) {
		dbg(DBG_MED, "\t-i iterations was given");
	} else {
		dbg(DBG_MED, "\tno -i iterations was given");
	}
	dbg(DBG_MED, "\t  iterations (bitstreams): -i %ld", state->tp.numOfBitStreams);
	if (state->reportCycleFlag == true) {
		dbg(DBG_MED, "\t-I reportCycle was given");
	} else {
		dbg(DBG_MED, "\tno -I reportCycle was given");
	}
	if (state->reportCycle == 0) {
		dbg(DBG_MED, "\t  will not report on progress of iterations");
	} else {
		dbg(DBG_MED, "\t  will report on progress every %ld iterations", state->reportCycle);
	}
	if (state->legacy_output == true) {
		dbg(DBG_MED, "\t-O was given, legacy output mode where reasonable");
	} else {
		dbg(DBG_MED, "\tno -O was given, legacy output is not important");
	}
	if (state->runModeFlag == true) {
		dbg(DBG_MED, "\t-m node was given");
	} else {
		dbg(DBG_MED, "\tno -m mode was given");
	}
	switch (state->runMode) {
	case MODE_WRITE_ONLY:
		switch (state->dataFormat) {
		case FORMAT_ASCII_01:
		case FORMAT_0:
			dbg(DBG_MED, "\t  -m w: only write generated data to -f file: %s in -F format: %s",
			    state->randomDataPath, "ASCII '0' and '1' character bits");
			break;
		case FORMAT_RAW_BINARY:
		case FORMAT_1:
			dbg(DBG_MED, "\t  -m w: only write generated data to -f file: %s in -F format: %s",
			    state->randomDataPath, "raw 8 binary bits per byte");
			break;
		default:
			dbg(DBG_MED, "\t  -m w: only write generated data to -f file: %s in -F format: unknown: %c",
			    state->randomDataPath, (char) state->dataFormat);
			break;
		}
		break;
	case MODE_ITERATE_ONLY:
		dbg(DBG_MED, "\t  -m i: iterate only and exit");
		break;
	case MODE_ASSESS_ONLY:
		dbg(DBG_MED, "\t  -m a: assess only and exit");
		break;
	case MODE_ITERATE_AND_ASSESS:
		dbg(DBG_MED, "\t  -m b: iterate and then assess, no *.state files written");
		break;
	default:
		dbg(DBG_MED, "\t  -m %c: unknown runMode", state->runMode);
		break;
	}
	dbg(DBG_MED, "\tworkDir: -w %s", state->workDir);
	if (state->subDirsFlag == true) {
		dbg(DBG_MED, "\t-c was given");
	} else {
		dbg(DBG_MED, "\tno -c was given");
	}
	if (state->subDirs == true) {
		dbg(DBG_MED, "\t  create directories needed for writing to any file");
	} else {
		dbg(DBG_MED, "\t  do not create directories, assume they exist");
	}
	if (state->resultstxtFlag == true) {
		dbg(DBG_MED, "\t-s was given");
		dbg(DBG_MED, "\t  create result.txt, data*.txt and stats.txt");
	} else {
		dbg(DBG_MED, "\tno -s was given");
		dbg(DBG_MED, "\t  do not create result.txt, data*.txt and stats.txt");
	}
	if (state->randomDataFlag == true) {
		dbg(DBG_MED, "\t-f was given");
	} else {
		dbg(DBG_MED, "\tno -f was given");
	}
	dbg(DBG_MED, "\t  randomDataPath: -f %s", state->randomDataPath);
	if (state->dataFormatFlag == true) {
		dbg(DBG_MED, "\t-F format was given");
	} else {
		dbg(DBG_MED, "\tno -F format was given");
	}
	switch (state->dataFormat) {
	case FORMAT_ASCII_01:
	case FORMAT_0:
		dbg(DBG_MED, "\t  read as ASCII '0' and '1' character bits");
		break;
	case FORMAT_RAW_BINARY:
	case FORMAT_1:
		dbg(DBG_MED, "\t  read as raw binary 8 bits per byte");
		break;
	default:
		dbg(DBG_MED, "\t  unknown format: %c", (char) state->dataFormat);
		break;
	}
	dbg(DBG_MED, "\tjobnum: -j %ld", state->jobnum);
	if (state->jobnumFlag == true) {
		dbg(DBG_MED, "\t-j jobnum was set to %ld", state->jobnum);
		dbg(DBG_MED, "\t  will skip %lld bytes of data before processing\n",
		    ((long long) state->jobnum * (((long long) state->tp.numOfBitStreams * (long long) state->tp.n) + 7 / 8)));
	} else {
		dbg(DBG_MED, "\tno -j jobnum was given");
		dbg(DBG_MED, "\t  will start processing at the beginning of data\n");
	}

	/*
	 * Report on test parameters
	 */
	dbg(DBG_MED, "Test parameters:");
	if (state->batchmode == false) {
		dbg(DBG_MED, "\t(showing default parameters)");
	}
	dbg(DBG_MED, "\tsingleBitStreamLength = %ld", state->tp.n);
	dbg(DBG_MED, "\tblockFrequencyBlockLength = %ld", state->tp.blockFrequencyBlockLength);
	dbg(DBG_MED, "\tnonOverlappingTemplateBlockLength = %ld", state->tp.nonOverlappingTemplateLength);
	dbg(DBG_MED, "\toverlappingTemplateBlockLength = %ld", state->tp.overlappingTemplateLength);
	dbg(DBG_MED, "\tserialBlockLength = %ld", state->tp.serialBlockLength);
	dbg(DBG_MED, "\tlinearComplexitySequenceLength = %ld", state->tp.linearComplexitySequenceLength);
	dbg(DBG_MED, "\tapproximateEntropyBlockLength = %ld", state->tp.approximateEntropyBlockLength);
	dbg(DBG_MED, "\tnumOfBitStreams = %ld", state->tp.numOfBitStreams);
	dbg(DBG_MED, "\tbins = %ld", state->tp.uniformity_bins);
	dbg(DBG_MED, "\tuniformityLevel = %f", state->tp.uniformity_level);
	if (state->batchmode == true) {
		dbg(DBG_MED, "\talpha = %f\n", state->tp.alpha);
	} else {
		dbg(DBG_MED, "\talpha = %f", state->tp.alpha);
		if (state->promptFlag == true) {
			dbg(DBG_LOW, "\t    -p was given: will NOT prompt for any changes to default parameters\n");
		} else {
			dbg(DBG_LOW, "\t    no -p was given: will prompt for any changes to default parameters\n");
		}
	}

	return;
}
