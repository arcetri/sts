#ifndef DEFS_H
#   define DEFS_H

#   include "config.h"


/*****************************************************************************
 D E B U G G I N G  A I D E S
 *****************************************************************************/

extern void msg(char const *fmt, ...);
extern void dbg(int level, char const *fmt, ...);
extern void warn(char const *name, char const *fmt, ...);
extern void warnp(char const *name, char const *fmt, ...);
extern void err(int exitcode, char const *name, char const *fmt, ...);
extern void errp(int exitcode, char const *name, char const *fmt, ...);
extern void usage_err(char const *usage, int exitcode, char const *name, char const *fmt, ...);
extern void usage_errp(char const *usage, int exitcode, char const *name, char const *fmt, ...);

#   define DBG_NONE (0)		// no debugging
#   define DBG_LOW (1)		// minimal debugging
#   define DBG_MED (3)		// somewhat more debugging
#   define DBG_HIGH (5)		// verbose debugging
#   define DBG_VHIGH (7)	// very verbose debugging
#   define DBG_VVHIGH (9)	// very very verbose debugging
#   define FORCED_EXIT (255)	// exit(255) on bad exit code


/*****************************************************************************
 M A C R O S
 *****************************************************************************/

#   define MAX(x,y)             ((x) <  (y)  ? (y)  : (x))
#   define MIN(x,y)             ((x) >  (y)  ? (y)  : (x))
#   define isNonPositive(x)     ((x) <= 0.e0 ?   1  : 0)
#   define isPositive(x)        ((x) >  0.e0 ?   1 : 0)
#   define isNegative(x)        ((x) <  0.e0 ?   1 : 0)
#   define isGreaterThanOne(x)  ((x) >  1.e0 ?   1 : 0)
#   define isZero(x)            ((x) == 0.e0 ?   1 : 0)
#   define isOne(x)             ((x) == 1.e0 ?   1 : 0)


/*****************************************************************************
 G L O B A L  C O N S T A N T S
 *****************************************************************************/

#   define ALPHA				(0.01)	// SIGNIFICANCE LEVEL
#   define MAXNUMOFTEMPLATES			(148)	// APERIODIC TEMPLATES: 148=>temp_length=9
#   define NUMOFTESTS				(15)	// MAX TESTS DEFINED - must match max enum test value below
#   define NUMOFGENERATORS			(10) 	// MAX PRNGs
#   define MAXFILESPERMITTEDFORPARTITION	(148)	// maximum value in default strcut state.numOfFiles[i]

#   define DEFAULT_BITCOUNT			(1000000)	// -P 0=bitcount, Length of a single bit stream
#   define DEFAULT_BLOCK_FREQUENCY		(128)		// -P 1=M, Block Frequency Test - block length
#   define DEFAULT_NONPERIODIC			(9)		// -P 2=m, NonOverlapping Template Test - block length
#   define DEFAULT_OVERLAPPING			(9)		// -P 3=m, Overlapping Template Test - block length
#   define DEFAULT_APEN				(10)		// -P 4=m, Approximate Entropy Test - block length
#   define DEFAULT_SERIAL			(16)		// -P 5=m, Serial Test - block length
#   define DEFAULT_LINEARCOMPLEXITY		(500)		// -P 6=M, Linear Complexity Test - block length
#   define DEFAULT_ITERATIONS			(1)		// -P 7=iterations (-i iterations)

// test(s) to perform
enum test {
	TEST_ALL = 0,			// converence for indicating run all tests
	TEST_FREQUENCY = 1,		// Frequency test (frequency.c)
	TEST_BLOCK_FREQUENCY = 2,	// Block Frequency test (blockFrequency.c)
	TEST_CUSUM = 3,			// Cumluative Sums test (cusum.c)
	TEST_RUNS = 4,			// Runs test (runs.c)
	TEST_LONGEST_RUN = 5,		// Longest Runs test (longestRunOfOnes.c)
	TEST_RANK = 6,			// Rank test (rank.c)
	TEST_FFT = 7,			// Discrete Fourier Transform test (discreteFourierTransform.c)
	TEST_NONPERIODIC = 8,		// Nonoverlapping Template test (nonOverlappingTemplateMatchings.c)
	TEST_OVERLAPPING = 9,		// Overlapping Template test (overlappingTemplateMatchings.c)
	TEST_UNIVERSAL = 10,		// Universal test (universal.c)
	TEST_APEN = 11,			// Aproximate Entrooy test (approximateEntropy.c)
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


/*****************************************************************************
 Dynamic Arrays
 *****************************************************************************/

#   define DEFAULT_CHUNK (1024)	// by default, allocate DEFAULT_CHUNK at a time

/*
 * array types - info on what the array elements consist of
 *
 * When you add types to this enum, you also need to add code to handle the types in:
 *
 *      zero_chunk(enum type type, WORD64 chunk, void *data)
 *      create_dyn_array(enum type type, WORD64 chunk, char *name)
 *      void append_value(struct dyn_array *array, enum type type, void *value_p)
 *
 * in the file "dyn_alloc.c".  Look for the switch statements where the case values are
 * the enum type values below.  That is where you need to add code to handle a new type.
 *
 * You should also create a convenence utilities for adding values of the new type
 * in the next section of this file.
 */
enum type {
	type_unknown = 0,	// no assigned value
	type_double,		// double - double precision floating point
	type_word64,		// WORD64 - 64-bit unsigned value
};


/*
 * convenence utilities for adding or reading values of a given type to a dynamic array
 *
 *      array_p                 // pointer to a struct dyn_array
 *      value                   // value to append
 *      index                   // index of data to fetch (no bounds checking)
 */

#   define append_double(array_p, value) append_value((array_p), type_double, &(value))
#   define append_word64(array_p, value) append_value((array_p), type_word64, &(value))

#   define get_double(array_p, index) (((double *)(((struct dyn_array *)(array_p))->data))[(index)])
#   define get_word64(array_p, index) (((WORD64 *)(((struct dyn_array *)(array_p))->data))[(index)])


/*
 * array - a dynamic aray of idential things
 *
 * Rather than write valus, such as floating point values
 * as ASCII formatted numbers into a file and then later
 * same in the run (after closing the file) reopen and reparse
 * those same files, we will append them to an array
 * and return that pointer to the array.  Then later
 * in the same run, the array can be written as ASCII formatted
 * numbers to a file if needed.
 */
struct dyn_array {
	enum type type;		// the type of element in dynamic array
	ULONG elm_size;		// number of octets for a single element
	WORD64 count;		// number of elements in use
	WORD64 allocated;	// number of elements allocated (>= count)
	WORD64 chunk;		// number of elements to expand by when allocating
	char *name;		// debugging string describing this array or NULL
	void *data;		// allocated dynamic array of idential things or NULL
};


/*
 * external allocation functions
 */
extern struct dyn_array *create_dyn_array(enum type type, WORD64 chunk, char *name);
extern void append_value(struct dyn_array *array, enum type type, void *value_p);


/*****************************************************************************
 G L O B A L   D A T A  S T R U C T U R E S
 *****************************************************************************/

typedef unsigned char BitSequence;

/*
 * TP - NIST defined test parameters, some specific to a particular test
 */

typedef struct _testParameters {
	long int n;					// -P 0=bitcount, Length of a single bit stream
	long int blockFrequencyBlockLength;		// -P 1=M, Block Frequency Test - block length
	long int nonOverlappingTemplateBlockLength;	// -P 2=m, NonOverlapping Template Test - block length
	long int overlappingTemplateBlockLength;	// -P 3=m, Overlapping Template Test - block length
	long int approximateEntropyBlockLength;		// -P 4=m, Approximate Entropy Test - block length
	long int serialBlockLength;			// -P 5=m, Serial Test - block length
	long int linearComplexitySequenceLength;	// -P 6=M, Linear Complexity Test - block length
	long int numOfBitStreams;			// -P 7=iterations (-i iterations)
} TP;

/*
 * state - execution state, initalized and set up by the command line, augmented by test results
 */

struct state {
	bool batchmode;		// -b: true -> non-interactie execution, false -> classic mode

	bool testVectorFlag;	// if and -t test1[,test2].. was given
	bool testVector[NUMOFTESTS + 1];	// -t test1[,test2]..: tests to invoke
	// -t 0 is a historical alias for all tests

	bool generatorFlag;	// if -g num was given
	long int generator;	// -g num: RNG to test, 0 -> use file, must use -f flag (randomDataPath)

	bool iterationFlag;	// if -i iterations was given
	// interactions is the same as numOfBitStreams, so this value is in tp.numOfBitStreams

	bool reportCycleFlag;	// if -I reportCycle was given
	long int reportCycle;	// -I reportCycle: Report after completion of reportCycle iterations (def: 0: do not report)

	bool statePathFlag;	// if -s statePath was given
	char *statePath;	// write state into statePath.state (def: don't)

	bool processStateDirFlag;	// if -p stateDir was given
	char *stateDir;			// -p stateDir: process state files in stateDir (def: under .)

	bool recursive;		// -r: true -> perform a recursive search of stateDir

	bool assessmodeFlag;	// if -m mode was given
	char assessmode;	// -m mode: whether gather state files, process state files or both

	bool workDirFlag;	// if -w workDir was given
	char *workDir;		// -w workDir: write experiment results under dir

	bool subDirsFlag;	// if -c was given
	bool subDirs;		// -c: false -> don't create any directories needed for creating files (def: do create)

	bool resultstxtFlag;	// -n: false -> don't create result.txt, data*.txt, nor stats.txt (def: do create)

	bool randomDataFlag;	// if -f randdata was given
	char *randomDataPath;	// -f randdata: path to a random data file

	bool dataFormatFlag;	// if -F format was given
	enum format dataFormat;	// -F format: 'r': raw binary, 'a': ASCII '0'/'1' chars

	bool jobnumFlag;	// if -j jobnum was given
	long int jobnum;	// -j jobnum: seek into randdata num*bitcount*iterations bits

	bool dataDirFlag;	// if -d dataDir was given
	char *dataDir;		// -d dataDir: data and templates found under dataDir

	TP tp;			// test parameters
	bool promptFlag;	// -p: true -> in interactive mode (no -b), do not prompt for change of parameters

	FILE *streamFile;	// if non-NULL, open stream for randomDataPath

	char *generatorDir[NUMOFGENERATORS + 1];	// generator names (or -g 0: AlgorithmTesting == read from file)

	char *testNames[NUMOFTESTS + 1];		// names of each test
	char *subDir[NUMOFTESTS + 1];			// NULL or name of working subdirectory (under workDir)

	int numOfFiles[NUMOFTESTS + 1];			// Partition the result test i into numOfFiles[i] data*.txt files

	struct dyn_array *freq;				// dynamic array frequency results for bitstream interations
	struct dyn_array *stats[NUMOFTESTS + 1];	// per test dynamic array for stats.txt per interation data
	struct dyn_array *p_val[NUMOFTESTS + 1];	// per test dynamic array for p_values (results.txt / data*.txt)

	long int curIteration;	// number of interations on all enabled tests completed so far
};

/*
 * driver - a driver like API to setup a given test, interate on bitsteams, analyze test results
 */

struct driver {
	void (*init) (struct state * state);	// initialize test
	void (*iterate) (struct state * state);	// perform a single interation of a bitstream
};

extern void Parse_args(struct state *state, int argc, char *argv[]);

#endif				/* DEFS_H */
