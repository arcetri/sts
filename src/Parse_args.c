// Parse_args.c
// Parse command line arguments for batch automation of assess.c

#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "externs.h"
#include "utilities.h"

#define PARAM_COUNT (7)
#define GENERATOR_COUNT NUMOFGENERATORS
#define ITERATE_ONLY ('i')
#define ASSESS_ONLY ('a')
#define ITERATE_AND_ASSESS ('b')

// forward declarations
static void change_params(struct state *state, long parameter, long value);
static void print_option_summary(struct state *state);

/*
 * default run state
 *
 * set the default state for options,
 * default state allows assess to run without any modifications
 */
static struct state const defaultstate = {

	// batchmode
	false,			// classic (interactive) mode

	// testVectorFlag & testVector
	false,			// no -t test1[,test2].. was given
	{false,
	 false, false, false, false, false,
	 false, false, false, false, false,
	 false, false, false, false, false,
	 },			// run no tests

	// generatorFlag & generator
	false,			// no -g generator was given
	0,			// default read data from a file (-g 0)

	// iterationFlag
	false,			// no -i iterations was given

	// reportCycleFlag & reportCycle
	false,			// no -I reportCycle was given
	0,			// do not report on interation progress

	// statePathFlag & statePath
	false,			// no -s statePath was given
	"__none__",		// no state file to write into

	// processStateDirFlag & stateDir
	false,			// no -p stateDir was given
	".",			// default stateDir

	// recursive
	false,			// no -r was given

	// assessmodeFlag & assessmode
	false,			// no -m mode was given
	ITERATE_AND_ASSESS,	// no -m mode, iterate and assess

	// workDirFlag & workDir
	false,			// no -w workDir was given
	"experiments",		// default is to write results under experiments
				// depending on -b and -g gen, this may become ./experiments/generator

	// subDirsFlag & subDirs
	false,			// no -c was given
	true,			// default is to create directories

	// resultstxtFlag
	true,			// no -n, create results.txt, data*.txt and stats.txt files

	// randomDataFlag & randomDataPath
	false,			// no -f randdata was given
	"/dev/null",		// default input file is /dev/null

	// dataFormatFlag & dataFormat
	false,			// if -F format was given
	FORMAT_RAW_BINARY,	// 'r': raw binary, 'a': ASCII '0'/'1' chars (def: 'r')

	// jobnumFlag & jobnum
	false,			// no -j jobnum was given
	0,			// -j 0: begin at start of randdata

	// dataDirFlag & dataDir
	false,			// no -d dataDir was given
	".",			// data and templates found under .

	// tp & promptFlag
	{DEFAULT_BITCOUNT,		// -P 0=bitcount, Length of a single bit stream
	 DEFAULT_BLOCK_FREQUENCY,	// -P 1=M, Block Frequency Test - block length
	 DEFAULT_NONPERIODIC,		// -P 2=m, NonOverlapping Template Test - block length
	 DEFAULT_OVERLAPPING,		// -P 3=m, Overlapping Template Test - block length
	 DEFAULT_APEN,			// -P 4=m, Approximate Entropy Test - block length
	 DEFAULT_SERIAL,		// -P 5=m, Serial Test - block length
	 DEFAULT_LINEARCOMPLEXITY,	// -P 6=M, Linear Complexity Test - block length
	 DEFAULT_ITERATIONS,		// -P 7=iterations (-i iterations)
	 },
	false,				// no -p, prompt for change of parameters if no -b

	// streamFile
	NULL,				// initially the randomDataPath is not open

	// generator names
	{"AlgorithmTesting", 		// -g 0, Read from file
	 "LCG", 			// -g 1, Linear Congruential
	 "QCG1", 			// -g 2, Quadratic Congruential I
	 "QCG2",			// -g 3, Quadratic Congruential II
	 "CCG", 			// -g 4, Cubic Congruential
	 "XOR",				// -g 5, XOR
	 "MODEXP", 			// -g 6, Modular Exponentiation
	 "BBS", 			// -g 7, Blum-Blum-Shub
	 "MS", 				// -g 8, Micali-Schnorr
	 "G-SHA1",			// -g 9, G Using SHA-1
	},

	// testNames & workDir
	{"((all_tests))",	// TEST_ALL = 0, converence for indicating run all tests
	 "Frequency",		// TEST_FREQUENCY = 1, Frequency test (frequency.c)
	 "BlockFrequency",	// TEST_BLOCK_FREQUENCY = 2, Block Frequency test (blockFrequency.c)
	 "CumulativeSums",	// TEST_CUSUM = 3, Cumluative Sums test (cusum.c)
	 "Runs",		// TEST_RUNS = 4, Runs test (runs.c)
	 "LongestRun",		// TEST_LONGEST_RUN = 5, Longest Runs test (longestRunOfOnes.c)
	 "Rank",		// TEST_RANK = 6, Rank test (rank.c)
	 "FFT",			// TEST_FFT = 7, Discrete Fourier Transform test (discreteFourierTransform.c)
	 "NonOverlappingTemplate",	// TEST_NONPERIODIC = 8, Nonoverlapping Template test (nonOverlappingTemplateMatchings.c)
	 "OverlappingTemplate",	// TEST_OVERLAPPING = 9, Overlapping Template test (overlappingTemplateMatchings.c)
	 "Universal",		// TEST_UNIVERSAL = 10, Universal test (universal.c)
	 "ApproximateEntropy",	// TEST_APEN = 11, Aproximate Entrooy test (approximateEntropy.c)
	 "RandomExcursions",	// TEST_RND_EXCURSION = 12, Random Excursions test (randomExcursions.c)
	 "RandomExcursionsVariant",	// TEST_RND_EXCURSION_VAR = 13, Random Excursions Variant test (randomExcursionsVariant.c)
	 "Serial",		// TEST_SERIAL = 14, Serial test (serial.c)
	 "LinearComplexity",	// TEST_LINEARCOMPLEXITY = 15, Linear Complexity test (linearComplexity.c)
	 },
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	},

	// numOfFiles - how much to partition the p_value / results.txt file
	{0,			// TEST_ALL = 0 - not a real test
	 1,			// TEST_FREQUENCY = 1
	 1,			// TEST_BLOCK_FREQUENCY = 2
	 2,			// TEST_CUSUM = 3
	 1,			// TEST_RUNS = 4
	 1,			// TEST_LONGEST_RUN = 5
	 1,			// TEST_RANK = 6
	 1,			// TEST_FFT = 7
	 MAXNUMOFTEMPLATES,	// TEST_NONPERIODIC = 8
	 1,			// TEST_OVERLAPPING = 9
	 1,			// TEST_UNIVERSAL = 10
	 1,			// TEST_APEN = 11
	 8,			// TEST_RND_EXCURSION = 12
	 18,			// TEST_RND_EXCURSION_VAR = 13
	 2,			// TEST_SERIAL = 14
	 1,			// TEST_LINEARCOMPLEXITY = 15
	 },

	// freq, stats & p_val - per test dynamic arrays
	NULL,
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	 },
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	 },

	// curIteration
	0,			// no interaction completed so far
};


/*
 * command line usage information
 */
/* *INDENT-OFF* */
static char const *const usage =
    "[-v level] [-b] [-t test1[,test2]..] [-g generator]\n"
"               [-P num=value[,num=value]..] [-p] [-i iterations] [-I reportCycle]\n"
"               [-w workDir] [-c] [-n] [-f randdata] [-F format] [-j jobnum] [-d dataDir]\n"
"               [-s statePath] [-r stateDir] [-R] [-m mode]\n"
"               [-h]\n"
"               bitcount\n"
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
"       0: Length of a single bit stream (bitcount):      1000000\n"
"       1: Block Frequency Test - block length(M):            128\n"
"       2: NonOverlapping Template Test - block length(m):      9\n"
"       3: Overlapping Template Test - block length(m):         9\n"
"       4: Approximate Entropy Test - block length(m):         10\n"
"       5: Serial Test - block length(m):                      16\n"
"       6: Linear Complexity Test - block length(M):          500\n"
"       7: Number of bitcount runs (same as -i iterations):     1\n"
"\n"
"    -p    In interactive mode (no -b), do not prompt for parameters (def: prompt)\n"
"\n"
"    -i iterations    number of bitstream runs (if -b, def: 1)\n"
"\n"
"       NOTE: -i iterations is the same as -P 7=iterations\n"
"\n"
"    -I reportCycle   Report after completion of reportCycle iterations (def: 0: do not report)\n"
"\n"
"    -w workDir       write experiment results under workDir (def: .)\n"
"    -c               don't create any directories needed for creating files (def: do create)\n"
"    -n               don't create result.txt, data*.txt, nor stats.txt (def: do create)\n"
"    -f randdata      -g 0 inputfile is randdata (required if -b and -g 0)\n"
"    -F format        randdata format: 'r': raw binary, 'a': ASCII '0'/'1' chars (def: 'r')\n"
"    -j jobnum        seek into randdata, jobnum*bitcount*iterations bits (def: 0)\n"
"    -d dataDir       data and template found under dataDir (def: .)\n"
"\n"
"    -s statePath     write state into statePath.state (def: don't)\n"
"    -r stateDir      process *.state files under stateDir (def: don't)\n"
"    -R               recursive search stateDir (def: no recurse)\n"
"    -m mode          i=> iterate only, a=>assess only (def: do both)\n"
"\n"
"    -h               print this message and exit\n"
"\n"
"    bitcount         Length of a single bit stream (same as -P 0=bitcount)\n";
/* *INDENT-ON* */


/*
 * Parse_args - parse command line arguments and setup run state
 *
 * given:
 *      state           // run state it initialize and set according to command line
 *      argc            // command line arg count
 *      argv            // array of argument strings
 *
 * This function does not return on error.
 */
void
Parse_args(struct state *state, int argc, char *argv[])
{
	int option;		// getopt() parsed option
	extern char *optarg;	// parsed option argument
	extern int optind;	// index to the next argv element to parse
	extern int opterr;	// 0 ==> disable internal getopt() error messages
	extern int optopt;	// last known option character returned by getopt()
	int scan_cnt;		// number of items scanned by sscanf()
	char *brkt;		// last state of strtok_r()
	char *phrase;		// string without separator as parsed by strtok_r()
	long int testnum;	// parsed test number
	long num;		// parsed parameter number
	long value;		// parsed parameter value
	bool success = false;	// if str2longtint was sucessful
	int test_cnt = 0;
	int j;

	// record the program name
	program = argv[0];

	// firewall
	if (argc <= 0 || argv == NULL || state == NULL) {
		err(1, __FUNCTION__, "called with bogus args");
	}
	// initialize state to defaults
	*state = defaultstate;

	/*
	 * parse the command line
	 */
	opterr = 0;
	while ((option = getopt(argc, argv, "v:bt:g:" "P:pi:I:" "w:cnf:F:j:d:" "s:r:Rm:" "h")) != -1) {
		switch (option) {

		case 'v':	// -v debuglevel
			debuglevel = str2longint(&success, optarg);
			if (success == false) {
				usage_errp(usage, 1, __FUNCTION__, "error in parsing -v debuglevel: %s", optarg);
			} else if (debuglevel < 0) {
				usage_err(usage, 1, __FUNCTION__, "error debuglevel: %d must >= 0", debuglevel);
			}
			break;

		case 'b':	// -b (batch, non-interactive mode)
			state->batchmode = true;
			state->promptFlag = true;	// -b implies -p
			break;

		case 't':	// -t test1[,test2]..
			// parse each comma separated arg
			state->testVectorFlag = true;
			for (phrase = strtok_r(optarg, ",", &brkt); phrase != NULL; phrase = strtok_r(NULL, ",", &brkt)) {

				// determine test number
				testnum = str2longint(&success, phrase);
				if (success == false) {
					usage_errp(usage, 2, __FUNCTION__,
						   "-t test1[,test2].. must only have " "comma separated integers: %s", phrase);
				}
				// 0 is special case, enable all tests
				if (testnum == 0) {
					for (j = 0; j <= NUMOFTESTS; j++) {
						state->testVector[j] = true;
					}
				} else if (testnum < 0 || testnum > NUMOFTESTS) {
					usage_err(usage, 2, __FUNCTION__, "-t test: %d must be [0-%d]: %d", testnum, NUMOFTESTS);
				} else {
					state->testVector[testnum] = true;
				}

			}
			break;


		case 'g':	// -g generator
			state->generatorFlag = true;
			state->generator = str2longint(&success, optarg);
			if (success == false) {
				usage_errp(usage, 3, __FUNCTION__, "-g generator must be numeric: %s", optarg);
			}
			if (state->generator < 0 || state->generator > GENERATOR_COUNT) {
				usage_err(usage, 3, __FUNCTION__, "-g generator: %d must be [0-%d]",
					  state->generator, GENERATOR_COUNT);
			}
			break;

		case 'P':	// -P num=value[,num=value]..
			// parse each comma separated arg
			for (phrase = strtok_r(optarg, ",", &brkt); phrase != NULL; phrase = strtok_r(NULL, ",", &brkt)) {

				// parse number and value
				scan_cnt = sscanf(phrase, "%ld=%ld", &num, &value);
				if (scan_cnt == EOF) {
					usage_errp(usage, 3, __FUNCTION__,
						   "-P num=value[,num=value].. " "error parsing a num=value: %s", phrase);
				} else if (scan_cnt != 2) {
					usage_err(usage, 3, __FUNCTION__,
						  "-P num=value[,num=value].. "
						  "failed to parse a num=value, " "expecting integer=integer: %s", phrase);
				}
				if (num < 0 || num > PARAM_COUNT) {
					usage_err(usage, 3, __FUNCTION__,
						  "-P num=value[,num=value].. " "num: %d must be [1-%d]", num, PARAM_COUNT);
				}
				change_params(state, num, value);
			}
			break;

		case 'p':	// -p (interactive mode (no -b), do not prompt for change of parameters)
			state->promptFlag = true;
			break;

		case 'i':	// -i iterations
			state->iterationFlag = true;
			state->tp.numOfBitStreams = str2longint(&success, optarg);
			if (success == false) {
				usage_errp(usage, 9, __FUNCTION__, "error in parsing -i iterations: %s", optarg);
			}
			if (state->tp.numOfBitStreams < 1) {
				usage_err(usage, 4, __FUNCTION__, "iterations (number of bit streams): %d can't be less than 1",
					  state->tp.numOfBitStreams);
			}
			break;

		case 'I':	// -I reportCycle
			state->reportCycleFlag = true;
			state->reportCycle = str2longint(&success, optarg);
			if (success == false) {
				usage_errp(usage, 9, __FUNCTION__, "error in parsing -I reportCycle: %s", optarg);
			}
			if (state->reportCycle < 0) {
				usage_err(usage, 9, __FUNCTION__, "-I reportCycle: %d must be >= 0", state->reportCycle);
			}
			break;

		case 'w':	// -w workDir
			// write experiment results under workDir
			state->workDirFlag = true;
			state->workDir = strdup(optarg);
			if (state->workDir == NULL) {
				errp(5, __FUNCTION__, "strdup of %d octets for -w workDir failed", strlen(optarg));
			}
			break;

		case 'c':	// -c (do not create directories)
			state->subDirsFlag = true;
			state->subDirs = false;	// do not create directorties
			break;

		case 'n':	// -n (don't create result.txt or stats.txt)
			state->resultstxtFlag = false;
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
				err(6, __FUNCTION__, "-F format: %s must be r or a", optarg);
			}
			if (optarg[1] != '\0') {
				err(6, __FUNCTION__, "-F format: %s must be a single character: r or a", optarg);
			}
			break;

		case 'f':	// -f randdata (path to input data)
			state->randomDataFlag = true;
			state->randomDataPath = strdup(optarg);
			if (state->randomDataPath == NULL) {
				errp(7, __FUNCTION__, "strdup of %d octets for -f randdata failed", strlen(optarg));
			}
			break;

		case 'j':	// -j jobnum (seek into randomData)
			state->jobnumFlag = true;
			state->jobnum = str2longint(&success, optarg);
			if (success == false) {
				usage_errp(usage, 9, __FUNCTION__, "error in parsing -j jobnum: %s", optarg);
			}
			if (state->jobnum < 0) {
				usage_err(usage, 9, __FUNCTION__, "-j jobnum: %d must be greater than 0", state->jobnum);
			}
			break;

		case 'd':	// -d dataDir (data and template location)
			state->dataDirFlag = true;
			state->dataDir = strdup(optarg);
			if (state->dataDir == NULL) {
				errp(6, __FUNCTION__, "strdup of %d octets for -d dataDir failed", strlen(optarg));
			}
			break;

		case 's':	// -s statePath (write state into statePath.state)
			state->statePathFlag = true;
			usage_err(usage, 10, __FUNCTION__, "option: -%c not yet implemented, sorry!", (char) optopt);
			break;

		case 'r':	// -r stateDir (process *.state under stateDir)
			state->processStateDirFlag = true;
			usage_err(usage, 10, __FUNCTION__, "option: -%c not yet implemented, sorry!", (char) optopt);
			break;

		case 'R':	// -R (recursive search stateDir)
			state->recursive = true;
			usage_err(usage, 10, __FUNCTION__, "option: -%c not yet implemented, sorry!", (char) optopt);
			break;

		case 'm':	// -m mode (i=> iterate only, a=>assess only)
			state->assessmodeFlag = true;
			usage_err(usage, 10, __FUNCTION__, "option: -%c not yet implemented, sorry!", (char) optopt);
			break;

		case 'h':	// -h (print out help)
			usage_err(usage, 0, __FUNCTION__, "this arg ignored");
			break;

			// ----------------------------------------------------------------

		case '?':
		default:
			if (option == '?') {
				usage_errp(usage, 14, __FUNCTION__, "getopt returned an error");
			} else {
				usage_err(usage, 14, __FUNCTION__, "unknown option: -%c", (char) optopt);
			}
			break;
		}
	}
	if (optind >= argc) {
		usage_err(usage, 15, __FUNCTION__, "missing required bitcount argumnt");
	}
	if (optind != argc - 1) {
		usage_err(usage, 15, __FUNCTION__, "unexpected arguments");
	}
	state->tp.n = (int) str2longint(&success, argv[optind]);
	if (success == false) {
		usage_errp(usage, 15, __FUNCTION__, "bitcount must be a number: %s", argv[optind]);
	}

	/*
	 * post processing
	 *
	 * make sure complimentary/conflicting options are set/not set
	 */
	if (state->batchmode == true && state->generatorFlag == true && state->generator == 0 && state->randomDataFlag == false) {
		usage_err(usage, 16, __FUNCTION__, "-f randdata required when using -b and -g 0");
	}
	if (state->batchmode == true && state->generatorFlag == false && state->randomDataFlag == false) {
		usage_err(usage, 16, __FUNCTION__, "-f randdata required when using -b and no -g generator given");
	}
	if (state->randomDataFlag == true && state->generator != 0) {
		usage_err(usage, 16, __FUNCTION__, "-f randdata may only be used with -g 0 (read from file)");
	}
	if (state->jobnumFlag == true && state->generator != 0) {
		usage_err(usage, 16, __FUNCTION__, "-j jobnum may only be used with -g 0 (read from file)");
	}
	if (state->generator == 0 && (state->tp.n % 8) != 0) {
		usage_err(usage, 16, __FUNCTION__,
			  "when -g 0 (read from file) is used, bitcount: %d " "must be a multiple of 8", state->tp.n);
	}
	if (state->jobnumFlag == true && state->generator != 0) {
		usage_err(usage, 16, __FUNCTION__, "-j jobnum may only used with -g 0 (read from file)");
	}
	if (state->randomDataFlag == true && state->generator != 0) {
		usage_err(usage, 16, __FUNCTION__, "-F format may only used with -g 0 (read from file)");
	}

	/*
	 * if -b and no -t test given, enable all tests
	 *
	 * Also test 0 is a historical alias for all tests enabled.
	 */
	if ((state->batchmode == true && state->testVectorFlag == false) || state->testVector[0] == true) {
		// under -b without and -t test, enable all tests
		for (j = 0; j <= NUMOFTESTS; j++) {
			state->testVector[j] = true;
		}
	}

	/*
	 * check on the number of tests enabled
	 *
	 * We do not count test 0 (historical alias for all tests enabled).
	 */
	if (state->batchmode == true) {
		for (j = 1; j <= NUMOFTESTS; j++) {
			if (state->testVector[j] == true) {
				++test_cnt;
			}
		}
	}
	if (test_cnt == 0 && state->batchmode == true) {
		err(17, __FUNCTION__, "no tests enabled");
	}

	/*
	 * set the workDir if no -w workdir was given
	 */
	if (state->workDirFlag != true) {

		/*
		 * -b batch mode: default workdir is the name of the generator
		 */
		if (state->batchmode == true) {
			state->workDir = strdup(state->generatorDir[state->generator]);
			if (state->workDir == NULL) {
				errp(18, __FUNCTION__, "strdup of %s failed", state->generatorDir[state->generator]);
			}

		/*
		 * classic mode (no -b): default workdir is experiments/__generator_name__
		 */
		} else {
			j = strlen("experiments/") + strlen(state->generatorDir[state->generator]) + 1; // string + NUL
			state->workDir = malloc(j + 1);	// +1 for later paranoia
			if (state->workDir == NULL) {
				errp(18, __FUNCTION__, "malloc of %d octets for experiments/%s failed",
						       j+1, state->generatorDir[state->generator]);
			}
			snprintf(state->workDir, j, "experiments/%s", state->generatorDir[state->generator]);
			state->workDir[j] = '\0';	// paranoia
		}
	}

	/*
	 * report on how we will run, if debugging
	 */
	if (debuglevel > 0) {
		print_option_summary(state);
	}
	return;
}


static void
change_params(struct state *state, long parameter, long value)
{
	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}

	/*
	 * NOTE: It would have been better if sts code had used a enum
	 *       Sorry for the magic numbers!
	 */
	switch (parameter) {
	case 0:		// see also final argument: argv[argc-1]
		state->tp.n = value;
		break;
	case 1:
		state->tp.blockFrequencyBlockLength = value;
		break;
	case 2:
		state->tp.nonOverlappingTemplateBlockLength = value;
		break;
	case 3:
		state->tp.overlappingTemplateBlockLength = value;
		break;
	case 4:
		state->tp.approximateEntropyBlockLength = value;
		break;
	case 5:
		state->tp.serialBlockLength = value;
		break;
	case 6:
		state->tp.linearComplexitySequenceLength = value;
		break;
	case 7:
		state->tp.numOfBitStreams = value;
		break;
	default:
		err(18, __FUNCTION__, "invalid parameter option: %d", parameter);
		break;
	}
	return;
}

static void
print_option_summary(struct state *state)
{
	int j;
	int test_cnt = 0;

	/*
	 * report on high level state
	 */
	dbg(DBG_LOW, "High level state:");
	dbg(DBG_LOW, "\tdebuglevel = %d", debuglevel);
	if (state->batchmode == true) {
		dbg(DBG_LOW, "\tRunning in (non-interactive) batch mode");
		if (state->assessmodeFlag == true) {
			switch (state->assessmode) {

			case ITERATE_ONLY:
				dbg(DBG_LOW, "\tWill iterate only (no assessment)");
				dbg(DBG_LOW, "\t    Interation state saved in: %s", state->statePath);
				break;

			case ASSESS_ONLY:
				dbg(DBG_LOW, "\tWill assess only (skip interation)");
				dbg(DBG_LOW,
				    "\t    State files read %s from under: %s",
				    (state->recursive ? "recursively" : "directly"), state->stateDir);
				break;

			case ITERATE_AND_ASSESS:
				dbg(DBG_LOW, "\tWill interate and assess");
				break;

			default:
				dbg(DBG_LOW, "\tUnknown assessment mode: %c", state->assessmode);
				break;
			}
		} else {
			dbg(DBG_LOW, "\tWill interate and assess");
		}
		dbg(DBG_MED, "\tTesting %lld bits of data", (long long) state->tp.numOfBitStreams * (long long) state->tp.n);
		dbg(DBG_MED, "\t    Testing %lld octets of data",
		    ((((long long) state->tp.numOfBitStreams * (long long) state->tp.n) + 7) / 8));
		dbg(DBG_LOW, "\tTesting %d interations of %d bits\n", state->tp.numOfBitStreams, state->tp.n);
	} else {
		dbg(DBG_LOW, "\tclassic (interactive mode)\n");
	}

	/*
	 * report on tests enabled
	 */
	dbg(DBG_LOW, "Tests enabled:");
	for (j = 1; j <= NUMOFTESTS; j++) {
		if (state->testVector[j]) {
			dbg(DBG_MED, "\ttest[%d] %s: enabled", j, state->testNames[j]);
			++test_cnt;
		}
	}
	if (state->batchmode == true) {
		if (test_cnt == NUMOFTESTS) {
			if (debuglevel <= DBG_LOW) {
				dbg(DBG_LOW, "\tAll tests enabled\n");
			} else {
				dbg(DBG_MED, "\tAll tests enabled");
			}
		}
		dbg(DBG_MED, "\t    %d tests enabled\n", test_cnt);
	} else {
		if (state->testVectorFlag == true) {
			dbg(DBG_LOW, "\t-t used, will NOT prompt for tests to enable\n");
		} else {
			dbg(DBG_LOW, "\tno -t used, will prompt for tests to enable\n");
		}
	}

	/*
	 * report on generator (or file) to be used
	 */
	if (state->generatorFlag == true) {

		// sts should use enum for generator numbers, but it doesn't: sorry!
		switch (state->generator) {

		case 0:
			if (state->batchmode == true) {
				dbg(DBG_LOW, "Testing data from file: %s", state->randomDataPath);
			} else {
				dbg(DBG_LOW, "Will use data from a file");
				dbg(DBG_LOW, "\tWill prompt for filename\n");
			}
			break;

		case 1:
			dbg(DBG_LOW, "Using builtin generator [%d]: " "Linear Congruential\n", state->generator);
			break;

		case 2:
			dbg(DBG_LOW, "Using builtin generator [%d]: " "Quadratic Congruential I\n", state->generator);
			break;

		case 3:
			dbg(DBG_LOW, "Using builtin generator [%d]: " "Quadratic Congruential II\n", state->generator);
			break;

		case 4:
			dbg(DBG_LOW, "Using builtin generator [%d]: " "Cubic Congruential\n", state->generator);
			break;

		case 5:
			dbg(DBG_LOW, "Using builtin generator [%d]: " "XOR\n", state->generator);
			break;

		case 6:
			dbg(DBG_LOW, "Using builtin generator [%d]: " "Modular Exponentiation\n", state->generator);
			break;

		case 7:
			dbg(DBG_LOW, "Using builtin generator [%d]: " "Blum-Blum-Shub\n", state->generator);
			break;

		case 8:
			dbg(DBG_LOW, "Using builtin generator [%d]: " "Micali-Schnorr\n", state->generator);
			break;

		case 9:
			dbg(DBG_LOW, "Using builtin generator [%d]: " "G Using SHA-1\n", state->generator);
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
	if (state->jobnumFlag == true) {
		dbg(DBG_LOW, "\tJob number: %d", state->jobnum);
		dbg(DBG_LOW,
		    "\t    Will skip %lld octets of data before processing\n",
		    ((long long) state->jobnum * (((long long) state->tp.numOfBitStreams * (long long) state->tp.n) + 7 / 8)));
	} else {
		dbg(DBG_LOW, "\tno -j numnum was given");
		dbg(DBG_LOW, "\t    Will start processing at the beginning of data\n");
	}

	/*
	 * detailed state summary
	 */
	dbg(DBG_MED, "Details of run state:");
	dbg(DBG_MED, "\tgenerator: -g %d", state->generator);
	if (state->iterationFlag == true) {
		dbg(DBG_MED, "\t-i iterations was given");
	} else {
		dbg(DBG_MED, "\tno -i iterations was given");
	}
	dbg(DBG_MED, "\t    iterations (bitstreams): -i %d", state->tp.numOfBitStreams);
	if (state->reportCycleFlag == true) {
		dbg(DBG_MED, "\t-I reportCycle was given");
	} else {
		dbg(DBG_MED, "\tno -I reportCycle was given");
	}
	if (state->reportCycle == 0) {
		dbg(DBG_MED, "\t    will not report on progress of interations");
	} else {
		dbg(DBG_MED, "\t    will progress after every %ld interations", state->reportCycle);
	}
	if (state->statePathFlag == true) {
		dbg(DBG_MED, "\t-s statePath was given");
		dbg(DBG_MED, "\t    statePath: -s %s", state->statePath);
	} else {
		dbg(DBG_MED, "\tno -s statePath was given");
		dbg(DBG_MED, "\t    no statePath will be used");
	}
	if (state->processStateDirFlag == true) {
		dbg(DBG_MED, "\t-r stateDir was given");
		dbg(DBG_MED, "\t    stateDir: -r %s", state->stateDir);
	} else {
		dbg(DBG_MED, "\tno -r stateDir was given");
		dbg(DBG_MED, "\t    no stateDir will be searched for *.state files");
	}
	if (state->recursive == true) {
		dbg(DBG_MED, "\t-R was given");
		dbg(DBG_MED, "\t    recursive state file search");
	} else {
		dbg(DBG_MED, "\tno -R was given");
		dbg(DBG_MED, "\t    only 1st level state files will be read");
	}
	if (state->assessmodeFlag == true) {
		dbg(DBG_MED, "\t-m node was given");
	} else {
		dbg(DBG_MED, "\tno -m mode was given");
	}
	dbg(DBG_MED, "\t    assessmode: -m %c", state->assessmode);
	if (state->workDirFlag == true) {
		dbg(DBG_MED, "\t-w dataDir was given");
	} else {
		dbg(DBG_MED, "\tno -w dataDir was given");
	}
	dbg(DBG_MED, "\t    workDir: -w %s", state->workDir);
	if (state->subDirsFlag == true) {
		dbg(DBG_MED, "\t-c was given");
	} else {
		dbg(DBG_MED, "\tno -c was given");
	}
	if (state->subDirs == true) {
		dbg(DBG_MED, "\t    Create directories needed for writing to any file");
	} else {
		dbg(DBG_MED, "\t    Do not create directories, assume they exist");
	}
	if (state->resultstxtFlag == true) {
		dbg(DBG_MED, "\tno -n was given");
		dbg(DBG_MED, "\t    Create result.txt, data*.txt and stats.txt");
	} else {
		dbg(DBG_MED, "\t-n was given");
		dbg(DBG_MED, "\t    Do not create result.txt, data*.t, not stats.txt");
	}
	if (state->randomDataFlag == true) {
		dbg(DBG_MED, "\t-f was given");
	} else {
		dbg(DBG_MED, "\tno -f was given");
	}
	dbg(DBG_MED, "\t    randomDataPath: -f %s", state->randomDataPath);
	if (state->dataFormatFlag == true) {
		dbg(DBG_MED, "\t-F format was given");
	} else {
		dbg(DBG_MED, "\tno -F format was given");
	}
	switch (state->dataFormat) {
	case FORMAT_ASCII_01:
	case FORMAT_0:
		dbg(DBG_MED, "\t    ASCII '0' and '1' character bits");
		break;
	case FORMAT_RAW_BINARY:
	case FORMAT_1:
		dbg(DBG_MED, "\t    raw 8 binary bits per byte");
		break;
	default:
		dbg(DBG_MED, "\t    unknown format: %c", (char)state->dataFormat);
		break;
	}
	dbg(DBG_MED, "\tjobnum: -j %d", state->jobnum);
	if (state->dataDirFlag == true) {
		dbg(DBG_MED, "\t-d dataDir was given");
	} else {
		dbg(DBG_MED, "\tno -d dataDir was given");
	}
	dbg(DBG_MED, "\t    dataDir: -d %s", state->dataDir);
	dbg(DBG_MED, "\tprogram name: %s\n", program);

	/*
	 * report on test parameters
	 */
	dbg(DBG_MED, "Test parameters:");
	if (state->batchmode == false) {
		dbg(DBG_MED, "\t    Showing default parameters");
	}
	dbg(DBG_MED, "\tSingleBitStreamLength = %d", state->tp.n);
	dbg(DBG_MED, "\tblockFrequencyBlockLength = %d", state->tp.blockFrequencyBlockLength);
	dbg(DBG_MED, "\tnonOverlappingTemplateBlockLength = %d", state->tp.nonOverlappingTemplateBlockLength);
	dbg(DBG_MED, "\toverlappingTemplateBlockLength = %d", state->tp.overlappingTemplateBlockLength);
	dbg(DBG_MED, "\tserialBlockLength = %d", state->tp.serialBlockLength);
	dbg(DBG_MED, "\tlinearComplexitySequenceLength = %d", state->tp.linearComplexitySequenceLength);
	dbg(DBG_MED, "\tapproximateEntropyBlockLength = %d", state->tp.approximateEntropyBlockLength);
	if (state->batchmode == true) {
		dbg(DBG_MED, "\tnumOfBitStreams = %d\n", state->tp.numOfBitStreams);
	} else {
		dbg(DBG_MED, "\tnumOfBitStreams = %d", state->tp.numOfBitStreams);
		if (state->promptFlag == true) {
			dbg(DBG_LOW, "\t-p was given: will NOT prompt for any changes to default parameters\n");
		} else {
			dbg(DBG_LOW, "\tno -p was given: will prompt for any changes to default parameters\n");
		}
	}
	return;
}
