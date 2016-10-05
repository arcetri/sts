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

#ifndef DEFS_H
#   define DEFS_H

#   include "config.h"
#   include "dyn_alloc.h"

/*****************************************************************************
 M A C R O S
 *****************************************************************************/

#   define MAX(x,y)		((x) <	(y)  ? (y)  : (x))
#   define MIN(x,y)		((x) >	(y)  ? (y)  : (x))
#   define isNonPositive(x)	((x) <= 0.e0 ?	 1  : 0)
#   define isPositive(x)	((x) >	0.e0 ?	 1 : 0)
#   define isNegative(x)	((x) <	0.e0 ?	 1 : 0)
#   define isGreaterThanOne(x)	((x) >	1.e0 ?	 1 : 0)
#   define isZero(x)		((x) == 0.e0 ?	 1 : 0)
#   define isOne(x)		((x) == 1.e0 ?	 1 : 0)


/*****************************************************************************
 G L O B A L  C O N S T A N T S
 *****************************************************************************/

/*
 * NOTE: This code was designed to support a MAXTEMPLEN of up to 31.  Going beyond
 *       31 is likely excessive and will cause you to run into signed 32 bit issues
 *       as well as 64 bit issues.
 *
 *       A MAXTEMPLEN of 15 may be more sane value.  Normal DEFAULT_NON_OVERLAPPING and
 *       DEFAULT_OVERLAPPING are both 9 and the PDF documentation suggests 8 or 9
 *       is common.
 *
 *       The absolute minimum for MINTEMPLEN is 2.  However for practical purposes
 *       such a small value is likely to be next to useless.  Since the PDF documentation
 *       suggests a value of 8 or 9, setting MINTEMPLEN to 8 may be a more sane minimum.
 */

/* *INDENT-OFF* */

#   define MINTEMPLEN			(8)		// minimum template length supported
#   define MAXTEMPLEN			(15)		// maximum template length supported // TODO why not 32?

#   if MINTEMPLEN > MAXTEMPLEN
// force syntax error if MINTEMPLEN vs. MAXTEMPLEN is bogus
-=*#@#*=- ERROR: MAXTEMPLEN must be >= MINTEMPLEN -=*#@#*=-
#endif

#   if MAXTEMPLEN < 2
// force syntax error if MAXTEMPLEN is too small
-=*#@#*=- ERROR: MAXTEMPLEN must be at least 2, we recommend at least 9 such as perhaps 15 -=*#@#*=-
#   elif MAXTEMPLEN == 2
#      define MAXNUMOFTEMPLATES		(2)		// APERIODIC TEMPLATES: for template length 2
#   elif MAXTEMPLEN == 3
#      define MAXNUMOFTEMPLATES		(4)		// APERIODIC TEMPLATES: for template length 3
#   elif MAXTEMPLEN == 4
#      define MAXNUMOFTEMPLATES		(6)		// APERIODIC TEMPLATES: for template length 4
#   elif MAXTEMPLEN == 5
#      define MAXNUMOFTEMPLATES		(12)		// APERIODIC TEMPLATES: for template length 5
#   elif MAXTEMPLEN == 6
#      define MAXNUMOFTEMPLATES		(20)		// APERIODIC TEMPLATES: for template length 6
#   elif MAXTEMPLEN == 7
#      define MAXNUMOFTEMPLATES		(40)		// APERIODIC TEMPLATES: for template length 7
#   elif MAXTEMPLEN == 8
#      define MAXNUMOFTEMPLATES		(74)		// APERIODIC TEMPLATES: for template length 8
#   elif MAXTEMPLEN == 9
#      define MAXNUMOFTEMPLATES		(148)		// APERIODIC TEMPLATES: for template length 9
#   elif MAXTEMPLEN == 10
#      define MAXNUMOFTEMPLATES		(284)		// APERIODIC TEMPLATES: for template length 10
#   elif MAXTEMPLEN == 11
#      define MAXNUMOFTEMPLATES		(568)		// APERIODIC TEMPLATES: for template length 11
#   elif MAXTEMPLEN == 12
#      define MAXNUMOFTEMPLATES		(1116)		// APERIODIC TEMPLATES: for template length 12
#   elif MAXTEMPLEN == 13
#      define MAXNUMOFTEMPLATES		(2232)		// APERIODIC TEMPLATES: for template length 13
#   elif MAXTEMPLEN == 14
#      define MAXNUMOFTEMPLATES		(4424)		// APERIODIC TEMPLATES: for template length 14
#   elif MAXTEMPLEN == 15
#      define MAXNUMOFTEMPLATES		(8848)		// APERIODIC TEMPLATES: for template length 15 // TODO change
#   elif MAXTEMPLEN == 16
#      define MAXNUMOFTEMPLATES		(17622)		// APERIODIC TEMPLATES: for template length 16
#   elif MAXTEMPLEN == 17
#      define MAXNUMOFTEMPLATES		(35244)		// APERIODIC TEMPLATES: for template length 17
#   elif MAXTEMPLEN == 18
#      define MAXNUMOFTEMPLATES		(70340)		// APERIODIC TEMPLATES: for template length 18
#   elif MAXTEMPLEN == 19
#      define MAXNUMOFTEMPLATES		(140680)	// APERIODIC TEMPLATES: for template length 19
#   elif MAXTEMPLEN == 20
#      define MAXNUMOFTEMPLATES		(281076)	// APERIODIC TEMPLATES: for template length 20
#   elif MAXTEMPLEN == 21
#      define MAXNUMOFTEMPLATES		(562152)	// APERIODIC TEMPLATES: for template length 21
#   elif MAXTEMPLEN == 22
#      define MAXNUMOFTEMPLATES		(1123736)	// APERIODIC TEMPLATES: for template length 22
#   elif MAXTEMPLEN == 23
#      define MAXNUMOFTEMPLATES		(2247472)	// APERIODIC TEMPLATES: for template length 23
#   elif MAXTEMPLEN == 24
#      define MAXNUMOFTEMPLATES		(4493828)	// APERIODIC TEMPLATES: for template length 24
#   elif MAXTEMPLEN == 25
#      define MAXNUMOFTEMPLATES		(8987656)	// APERIODIC TEMPLATES: for template length 25
#   elif MAXTEMPLEN == 26
#      define MAXNUMOFTEMPLATES		(17973080)	// APERIODIC TEMPLATES: for template length 26
#   elif MAXTEMPLEN == 27
#      define MAXNUMOFTEMPLATES		(35946160)	// APERIODIC TEMPLATES: for template length 27
#   elif MAXTEMPLEN == 28
#      define MAXNUMOFTEMPLATES		(71887896)	// APERIODIC TEMPLATES: for template length 28
#   elif MAXTEMPLEN == 29
#      define MAXNUMOFTEMPLATES		(143775792)	// APERIODIC TEMPLATES: for template length 29
#   elif MAXTEMPLEN == 30
#      define MAXNUMOFTEMPLATES		(287542736)	// APERIODIC TEMPLATES: for template length 30
#   elif MAXTEMPLEN == 31
#      define MAXNUMOFTEMPLATES		(575085472)	// APERIODIC TEMPLATES: for template length 31
#   else
      // force syntax error if MAXTEMPLEN is too large
      -=*#@#*=-
      ERROR: MAXNUMOFTEMPLATES is not known for this bit_count!

      To get the MAXNUMOFTEMPLATES for a given template_length, try:

	cd src				# i.e., cd to source code directory
	make mkapertemplate

	rm -f dataInfo; ./mkapertemplate template_length /dev/null dataInfo; cat dataInfo

	      where template_length is an integer > 0

      From the dataInfo file, use the "# of nonperiodic templates =" line to determine MAXNUMOFTEMPLATES.

      WARNING: If you extend MAXTEMPLEN beyond 31, you will have to deal
	 with signed 32-bit issues and then 64-bit issue in the computig of
	 templates of the nonOverlappingTemplateMatchings.c code.
	 For example, you will have to at least change, in the file
	 nonOverlappingTemplateMatchings.c, the use of ULONG with uint64_t.

	 On the other hand, the memory requirements and CPU cycles
	 needed for even MAXTEMPLEN of 31 borders on the asburd.

      NOTE: Running ./mkapertemplate with a non-trivial template_length can take a long time to run!
	    For example, a UCS C240 M4 tool almost 27 CPU minutes to calculate the value for 31.

      FYI: The compile line below should also work in place of the above make rule:

	cd src				# i.e., cd to source code directory
	cc ../tools/mkapertemplate.c debug.c -o mkapertemplate -O3 -I . --std=c99
      -=*#@#*=-
#   endif

#   define BITS_N_BYTE			(8)		// Number of bits in a byte
#   define BITS_N_INT			(BITS_N_BYTE * sizeof(int))		// bits in an int
#   define BITS_N_LONGINT		(BITS_N_BYTE * sizeof(long int))	// bits in a long int
#   define MAX_DATA_DIGITS		(21)		// decimal digits in (2^64)-1

#   define NUMOFTESTS			(15)		// MAX TESTS DEFINED - must match max enum test value below
#   define NUMOFGENERATORS		(10)		// MAX PRNGs
#   define MAXFILESPERMITTEDFORPARTITION (MAXNUMOFTEMPLATES)	// maximum value in default struct state.partitionCount[i]

#   define DEFAULT_BLOCK_FREQUENCY	(128)		// -P 1=M, Block Frequency Test - block length
#   define DEFAULT_NON_OVERLAPPING	(9)		// -P 2=m, NonOverlapping Template Test - block length
#   define DEFAULT_OVERLAPPING		(9)		// -P 3=m, Overlapping Template Test - block length
#   define DEFAULT_APEN			(10)		// -P 4=m, Approximate Entropy Test - block length
#   define DEFAULT_SERIAL		(16)		// -P 5=m, Serial Test - block length
#   define DEFAULT_LINEARCOMPLEXITY	(500)		// -P 6=M, Linear Complexity Test - block length
#   define DEFAULT_ITERATIONS		(1)		// -P 7=iterations (-i iterations)
#   define DEFAULT_UNIFORMITY_BINS	(10)		// -P 8=bins, uniformity test is thru this many bins
#   define DEFAULT_BITCOUNT		(1000000)	// -P 9=bitcount, Length of a single bit stream
#   define DEFAULT_UNIFORMITY_LEVEL	(0.0001)	// -P 10=uni_level, uniformity errors have values below this
#   define DEFAULT_ALPHA		(0.01)		// -P 11=alpha, p_value significance level

// TODO let MIN_BITCOUNT be really the smallest
#   define MIN_BITCOUNT			(1000)		// Section 2.0 min recommended length of a single bit stream, must be > 0
// TODO this is test-dependent. For example 1,2,3 tests require only 100 bits, 4 requires 128, 5 requires 38,912
#   define MAX_BITCOUNT			(10000000)	// Section 2.0 max recommended length of a single bit stream

#   define MIN_LINEARCOMPLEXITY		(500)		// Section 2.10.5 input size recommendation
#   define MAX_LINEARCOMPLEXITY		(5000)		// Section 2.10.5 input size recommendation

#   define RANK_ROWS			(32)		// Number of rows in the rank_matrix used by TEST_RANK
#   define RANK_COLS			(32)		// Number of columns in the rank_matrix used by TEST_RANK

#   define MAX_EXCURSION_VAR		(9)		// excursion states: -MAX_EXCURSION_VAR to -1,
//     and 1 to MAX_EXCURSION_VAR - used by TEST_RND_EXCURSION_VAR
#   define EXCURSION_VAR_STATES	(2*MAX_EXCURSION_VAR)	// Number of excursion states possible for TEST_RND_EXCURSION_VAR

#   define OVERLAP_M_SUBSTRING		(1032)		// bit length of tested substring (set in SP800-22Rev1a section 2.8.2)
#   define OVERLAP_K_DEGREES		(5)		// degrees of freedom (set in SP800-22Rev1a section 2.8.2)

#   define LINEARCOMPLEXITY_K_DEGREES	(6)		// degrees of freedom (set in SP800-22Rev1a section 2.10.2)

#   define MIN_LONGESTRUN		(128)		// minimum n for a Longest Runs test for TEST_LONGEST_RUN
#   define LONGEST_RUN_CLASS_COUNT	(6)		// Number of classes == max_len - min_len + 1 for TEST_LONGEST_RUN

#   define MIN_UNIVERSAL		(387840)	// minimum n to allow L >= 6 for TEST_UNIVERSAL
#   define MIN_L_UNIVERSAL		(6)		// minimum of L
#   define MAX_UNIVERSAL		(2250506239L)	// maximum n to allow L <= 16 for TEST_UNIVERSAL
#   define MAX_L_UNIVERSAL		(16)		// maximum of L

#   define MAX_EXCURSION		(4)		// excursion states: -MAX_EXCURSION to -1,
//    and 1 to MAX_EXCURSION
#   define EXCURSION_CLASSES		(MAX_EXCURSION+1)		// pool sigma values into 0 <= classes < EXCURSION_CLASSES,
// and for class >= EXCURSION_CLASSES for TEST_RND_EXCURSION
#   define EXCURSION_TEST_CNT		(2*MAX_EXCURSION)	// Number of tests & conclusions for TEST_RND_EXCURSION
#   define EXCURSION_FREEDOM		(6)			// degrees of freedom for TEST_RND_EXCURSION

#   if DEFAULT_NON_OVERLAPPING < MINTEMPLEN
// force syntax error if DEFAULT_NON_OVERLAPPING is too small
      -=*#@#*=- DEFAULT_NON_OVERLAPPING must be >= MINTEMPLEN -=*#@#*=-
#   elif DEFAULT_NON_OVERLAPPING > MAXTEMPLEN
// force syntax error if DEFAULT_NON_OVERLAPPING is too large
      -=*#@#*=- DEFAULT_NON_OVERLAPPING must be <= MAXTEMPLEN -=*#@#*=-
#   endif

#   if DEFAULT_OVERLAPPING < MINTEMPLEN
// force syntax error if DEFAULT_OVERLAPPING is too small
      -=*#@#*=- DEFAULT_OVERLAPPING must be >= MINTEMPLEN -=*#@#*=-
#   elif DEFAULT_OVERLAPPING > MAXTEMPLEN
// force syntax error if DEFAULT_OVERLAPPING is too large
      -=*#@#*=- DEFAULT_OVERLAPPING must be <= MAXTEMPLEN -=*#@#*=-
#   endif

/* *INDENT-ON* */

/*****************************************************************************
 G L O B A L   D A T A	S T R U C T U R E S
 *****************************************************************************/

typedef unsigned char BitSequence;

/* *INDENT-OFF* */

// random data generators
enum gen {
	GENERATOR_FROM_FILE = 0,	// -g 0, read data from a file
	GENERATOR_LCG = 1,		// -g 1, Linear Congruential
	GENERATOR_QCG1 = 2,		// -g 2, Quadratic Congruential I
	GENERATOR_QCG2 = 3,		// -g 3, Quadratic Congruential II
	GENERATOR_CCG = 4,		// -g 4, Cubic Congruential
	GENERATOR_XOR = 5,		// -g 5, XOR
	GENERATOR_MODEXP = 6,		// -g 6, Modular Exponentiation
	GENERATOR_BBS = 7,		// -g 7, Blum-Blum-Shub
	GENERATOR_MS = 8,		// -g 8, Micali-Schnorr
	GENERATOR_SHA1 = 9,		// -g 9, G Using SHA-1
};

// test(s) to perform
enum test {
	TEST_ALL = 0,			// convention for indicating run all tests
	TEST_FREQUENCY = 1,		// Frequency test (frequency.c)
	TEST_BLOCK_FREQUENCY = 2,	// Block Frequency test (blockFrequency.c)
	TEST_CUSUM = 3,			// Cumulative Sums test (cusum.c)
	TEST_RUNS = 4,			// Runs test (runs.c)
	TEST_LONGEST_RUN = 5,		// Longest Runs test (longestRunOfOnes.c)
	TEST_RANK = 6,			// Rank test (rank.c)
	TEST_FFT = 7,			// Discrete Fourier Transform test (discreteFourierTransform.c)
	TEST_NONPERIODIC = 8,		// Non-overlapping Template test (nonOverlappingTemplateMatchings.c)
	TEST_OVERLAPPING = 9,		// Overlapping Template test (overlappingTemplateMatchings.c)
	TEST_UNIVERSAL = 10,		// Universal test (universal.c)
	TEST_APEN = 11,			// Approximate Entropy test (approximateEntropy.c)
	TEST_RND_EXCURSION = 12,	// Random Excursions test (randomExcursions.c)
	TEST_RND_EXCURSION_VAR = 13,	// Random Excursions Variant test (randomExcursionsVariant.c)
	TEST_SERIAL = 14,		// Serial test (serial.c)
	TEST_LINEARCOMPLEXITY = 15,	// Linear Complexity test (linearComplexity.c)
	// IMPORTANT: The last enum test value must match the NUMOFTESTS defined above!!!
};

// Format of data when read from a file
enum format {
	FORMAT_ASCII_01 = 'a',		// Use ascii '0' and '1' chars, 1 bit per octet
	FORMAT_0 = '0',			// alias for FORMAT_ASCII_01
	FORMAT_RAW_BINARY = 'r',	// data in raw binary, 8 bits per octet
	FORMAT_1 = '1',			// alias for FORMAT_RAW_BINARY
};

// Run modes
enum run_mode {
	MODE_WRITE_ONLY = 'w',		// only write generated data to -f randata in -F format
	MODE_ITERATE_ONLY = 'i',	// iterate only, write state to -s statePath and exit
	MODE_ASSESS_ONLY = 'a',		// assess only, read state from *.state files under -r stateDir and assess results
	MODE_ITERATE_AND_ASSESS = 'b',	// iterate and then assess, no *.state files written
};

/*
 * driver state
 */
enum driver_state {
	DRIVER_NULL = 0,			// no driver state assigned
	DRIVER_INIT,				// initialized test for driver
	DRIVER_ITERATE,				// iterating for driver
	DRIVER_PRINT,				// log iteration info for driver
	DRIVER_METRICS,				// Uniformity and proportional analysis for driver
	DRIVER_DESTROY,				// final test cleanup and de-allocation for driver
};


/*
 * TP - test parameters
 *
 * Some are NIST defined test parameters, some specific to a particular test, some are
 * were added to allow for better test flexibility.
 */

#   define MIN_PARAM (1)	// minimum -P parameter number
#   define MAX_PARAM (11)	// maximum -P parameter number
#   define MAX_INT_PARAM (9)	// maximum -P parameter that is an integer, beyond this are doubles

enum param {
	PARAM_continue = 0,				// Don't prompt for any more parameters
	PARAM_blockFrequencyBlockLength = 1,		// -P 1=M, Block Frequency Test - block length
	PARAM_nonOverlappingTemplateBlockLength = 2,	// -P 2=m, NonOverlapping Template Test - block length
	PARAM_overlappingTemplateBlockLength = 3,	// -P 3=m, Overlapping Template Test - block length
	PARAM_approximateEntropyBlockLength = 4,	// -P 4=m, Approximate Entropy Test - block length
	PARAM_serialBlockLength = 5,			// -P 5=m, Serial Test - block length
	PARAM_linearComplexitySequenceLength = 6,	// -P 6=M, Linear Complexity Test - block length
	PARAM_numOfBitStreams = 7,			// -P 7=iterations (-i iterations)
	PARAM_uniformity_bins = 8,			// -P 8=bins, uniformity test is thru this many bins
	PARAM_n = 9,					// -P 9=bitcount, Length of a single bit stream
	PARAM_uniformity_level = 10,			// -P 10=uni_level, uniformity errors have values below this
	PARAM_alpha = 11,				// -P 11=alpha, p_value significance level
};

typedef struct _testParameters {
	long int blockFrequencyBlockLength;		// -P 1=M, Block Frequency Test - block length
	long int nonOverlappingTemplateBlockLength;	// -P 2=m, NonOverlapping Template Test - block length
	long int overlappingTemplateBlockLength;	// -P 3=m, Overlapping Template Test - block length
	long int approximateEntropyBlockLength;		// -P 4=m, Approximate Entropy Test - block length
	long int serialBlockLength;			// -P 5=m, Serial Test - block length
	long int linearComplexitySequenceLength;	// -P 6=M, Linear Complexity Test - block length
	long int numOfBitStreams;			// -P 7=iterations (-i iterations)
	long int uniformity_bins;			// -P 8=bins, uniformity test is thru this many bins
	long int n;					// -P 9=bitcount, Length of a single bit stream
	double uniformity_level;			// -P 10=uni_level, uniformity errors have values below this
	double alpha;					// -P 11=alpha, p_value significance level
} TP;


/*
 * test constants
 *
 * These values are initialized by the init() driver function after the command line arguments are parsed,
 * AND after any test parameters are established (by default or via interactive promot),
 * AND before the individual test init functions are called.
 *
 * Once the init() function sets these values, they do not change for the duration of the test.
 *
 * In some cases the constants are simply numeric (such as the square root of 2).
 * In some cases the constants depend on test parameters (such as TP.n or TP.numOfBitStreams, etc.)
 *
 * NOTE: In the comments below, n is TP.n.
 */
typedef struct _const {
	double sqrt2;		// Square root of 2 - used by several tests
	double log2;		// log(2) - used by many tests
	double sqrtn;		// Square root of n - used by TEST_FREQUENCY
	double sqrtn4_095_005;	// Square root of (n / 4.0 * 0.95 * 0.05) - used by TEST_FFT
	double sqrt_log20_n;	// Square root of ln(20) * n - used by TEST_FFT
	double sqrt2n;		// Square root of (2*n) - used by TEST_RUNS
	double two_over_sqrtn;	// 2 / Square root of n - used by TEST_RUNS
	double p_32;		// Probability of rank RANK_ROWS - used by RANK_TEST
	double p_31;		// Probability of rank RANK_ROWS-1 - used by RANK_TEST
	double p_30;		// Probability of rank < RANK_ROWS-1 - used by RANK_TEST
	double logn;		// log(n) - used by many tests
	long int excursion_constraint;	// Number of crossings required to complete the test -
	//    used by TEST_RND_EXCURSION_VAR and TEST_RND_EXCURSION
	long int matrix_count;	// Total possible matrix for a given bit stream length - used by RANK_TEST
} T_CONST;

#   define UNSET_DOUBLE		((double)(0.0))		// unitialized floating point constant
#   define NON_P_VALUE	((double)(-99999999.0))	// never a valid p_value


/*
 * state - execution state, initialized and set up by the command line, augmented by test results
 */

struct state {
	bool batchmode;		// -b: true -> non-interactive execution, false -> classic mode

	bool testVectorFlag;			// if and -t test1[,test2].. was given
	bool testVector[NUMOFTESTS + 1];	// -t test1[,test2]..: tests to invoke
	// -t 0 is a historical alias for all tests

	bool generatorFlag;		// if -g num was given
	enum gen generator;		// -g num: RNG to test

	bool iterationFlag;		// if -i iterations was given
	// iterations is the same as numOfBitStreams, so this value is in tp.numOfBitStreams

	bool reportCycleFlag;		// if -I reportCycle was given
	long int reportCycle;		// -I reportCycle: Report after completion of reportCycle iterations
	//		   (def: 0: do not report)
	bool runModeFlag;		// if -m mode was given
	enum run_mode runMode;		// -m mode: whether gather state files, process state files or both

	bool workDirFlag;		// if -w workDir was given
	char *workDir;			// -w workDir: write experiment results under dir

	bool subDirsFlag;		// if -c was given
	bool subDirs;			// -c: false -> don't create any directories needed for creating files
	//		(def: do create)

	bool resultstxtFlag;		// -n: false -> don't create result.txt, data*.txt, nor stats.txt
	//		(def: do create)

	bool randomDataFlag;		// if -f randdata was given
	char *randomDataPath;		// -f randdata: path to a random data file

	bool dataFormatFlag;		// if -F format was given
	enum format dataFormat;		// -F format: 'r': raw binary, 'a': ASCII '0'/'1' chars

	bool jobnumFlag;		// if -j jobnum was given
	long int jobnum;		// -j jobnum: seek into randdata num*bitcount*iterations bits

	TP tp;				// test parameters
	bool promptFlag;		// -p: true -> in interactive mode (no -b), do not prompt for change of parameters

	T_CONST c;			// test constants
	bool cSetup;			// true --> init() function has initialized the test constants c

	FILE *streamFile;		// if non-NULL, open stream for randomDataPath
	char *finalReptPath;		// if non-NULL, path of finalAnalysisReport.txt
	FILE *finalRept;		// if non-NULL, open stream for finalAnalysisReport.txt
	char *freqFilePath;		// if non-NULL, path of freq.txt
	FILE *freqFile;			// if non-NULL, open stream for freq.txt

	char *generatorDir[NUMOFGENERATORS + 1];// generator names (or -g 0: AlgorithmTesting == read from file)

	char *testNames[NUMOFTESTS + 1];		// names of each test
	char *subDir[NUMOFTESTS + 1];			// NULL or name of working subdirectory (under workDir)
	enum driver_state driver_state[NUMOFTESTS +1];	// driver state for each test

	int partitionCount[NUMOFTESTS + 1];	// Partition the result test i into partitionCount[i] data*.txt files
	char *datatxt_fmt[NUMOFTESTS + 1];	// format of data*.txt filenames or NULL

	struct dyn_array *freq;			// dynamic array frequency results on data for each iteration
	struct dyn_array *stats[NUMOFTESTS + 1];// per test dynamic array of per iteration data (for stats.txt unless -n)
	struct dyn_array *p_val[NUMOFTESTS + 1];// per test dynamic array of p_values and unless -n for results.txt
	// NOTE: NonOverlapping Template Test uses array of struct nonover_stats

	bool is_excursion[NUMOFTESTS + 1];	// true --> test is a form of random excursion

	BitSequence *epsilon;			// bit stream
	BitSequence *tmpepsilon;		// buffer to write to file in dataFormat

	long int count[NUMOFTESTS + 1];		// Count of completed iterations, including tests skipped due to conditions
	long int valid[NUMOFTESTS + 1];		// Count of completed testable iterations, ignores tests skipped due to conditions
	long int success[NUMOFTESTS + 1];	// Count of completed SUCCESS iterations that were testable
	long int failure[NUMOFTESTS + 1];	// Count of completed FAILURE iterations that were testable
	long int valid_p_val[NUMOFTESTS + 1];	// Count of p_values that were [0.0, 1.0] for iterations that were testable

	bool uniformity_failure[NUMOFTESTS + 1];	// true --> uniformity failure for a given test
	bool proportional_failure[NUMOFTESTS + 1];	// true --> proportional failure for a given test

	long int maxGeneralSampleSize;		// largest sample size for a non-excursion test
	long int maxRandomExcursionSampleSize;	// largest sample size for a general (non-random excursion) test

	struct dyn_array *nonovTemplates;		// array of non-overlapping template words for TEST_NONPERIODIC

	double *fft_m;				// test m array for TEST_FFT
	double *fft_X;				// test X array for TEST_FFT
	double *fft_wsave;			// test wsave array for TEST_FFT

	BitSequence **rank_matrix;		// Rank test 32 by 32 matrix for TEST_RANK

	long int *excursion_var_stateX;		// pointer to EXCURSION_VAR_STATES states for TEST_RND_EXCURSION_VAR
	long int *ex_var_partial_sums;		// array of n partial sums for TEST_RND_EXCURSION_VAR

	BitSequence *linear_b;			// LFSR array b for TEST_LINEARCOMPLEXITY
	BitSequence *linear_c;			// LFSR array c for TEST_LINEARCOMPLEXITY
	BitSequence *linear_t;			// LFSR array t for TEST_LINEARCOMPLEXITY
	BitSequence *linear_p;			// LFSR array p (just like b, shifted by 1) for TEST_LINEARCOMPLEXITY

	long int *apen_P;			// Frequency count for TEST_APEN
	long int apen_p_len;			// Number of long ints in apen_P for TEST_APEN

	long int *serial_P;			// Frequency count for TEST_SERIAL
	long int serial_p_len;			// Number of long ints in serial_P for TEST_SERIAL

	BitSequence *nonper_seq;		// special BitSequence for TEST_NONPERIODIC

	long int universal_L;			// Length of each block for TEST_UNIVERSAL
	long int *universal_T;			// working Universal template

	long int *rnd_excursion_S_k;		// Sum of -1/+1 states for TEST_RND_EXCURSION
	long int *rnd_excursion_cycle;		// cycle counts for TEST_RND_EXCURSION
	long int rnd_excursion_cycle_len;	// Length of the rnd_excursion_cycle array for TEST_RND_EXCURSION
	long int *excursion_stateX;		// pointer to EXCURSION_TEST_CNT states for TEST_RND_EXCURSION_VAR

	bool legacy_output;			// true ==> try to mimic output format of legacy code

	long int curIteration;			// Number of iterations on all enabled tests completed so far
};

/* *INDENT-ON* */

/*
 * driver - a driver like API to setup a given test, iterate on bitsteams, analyze test results
 */

extern void init(struct state *state);
extern void iterate(struct state *state);
extern void print(struct state *state);
extern void metrics(struct state *state);
extern void destroy(struct state *state);

extern void Parse_args(struct state *state, int argc, char *argv[]);

#endif				/* DEFS_H */
