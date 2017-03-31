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


// Exit codes: 80 thru 89

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <getopt.h>

// forward declarations

static void write_sequence(void);

// defs.h

#   define NUMOFGENERATORS		(10)		// MAX PRNGs
#   define BITS_N_BYTE			(8)					// Number of bits in a byte

/*
 * Compiler independent Bool definitions
 */
#   if !defined(__bool_true_false_are_defined) || __bool_true_false_are_defined==0
#      undef bool
#      undef true
#      undef false
#      if defined(__cplusplus)
#         define bool bool
#         define true true
#         define false false
#      else
#         if __STDC_VERSION__ >= 199901L
#            define bool _Bool
#         else
#            define bool unsigned char
#         endif
#         define true 1
#         define false 0
#      endif
#      define __bool_true_false_are_defined 1
#   endif

typedef unsigned char BitSequence;

enum gen {
	GENERATOR_LCG = 1,              // -g 1, Linear Congruential
	GENERATOR_QCG1 = 2,             // -g 2, Quadratic Congruential I
	GENERATOR_QCG2 = 3,             // -g 3, Quadratic Congruential II
	GENERATOR_CCG = 4,              // -g 4, Cubic Congruential
	GENERATOR_XOR = 5,              // -g 5, XOR
	GENERATOR_MODEXP = 6,           // -g 6, Modular Exponentiation
	GENERATOR_BBS = 7,              // -g 7, Blum-Blum-Shub
	GENERATOR_MS = 8,               // -g 8, Micali-Schnorr
	GENERATOR_SHA1 = 9,             // -g 9, G Using SHA-1
};

// externs.h

/*****************************************************************************
		   G L O B A L  D A T A  S T R U C T U R E S
 *****************************************************************************/

static const char *const version = "3.2.0";	// our version
static char *program = NULL;			// Program name (argv[0])
static long int debuglevel = 0;			// -v lvl: defines the level of verbosity for debugging

// utilities.h

#if 0
extern long int getNumber(FILE * input, FILE * output);
extern double getDouble(FILE * input, FILE * output);
extern char *getString(FILE * stream);
extern void makePath(char *dir);
extern bool checkWritePermissions(char *dir);
extern bool checkReadPermissions(char *dir);
extern FILE *openTruncate(char *filename);
extern char *filePathName(char *head, char *tail);
extern char *data_filename_format(int partitionCount);
extern void precheckPath(struct state *state, char *dir);
extern char *precheckSubdir(struct state *state, char *subdir);
extern long int str2longint(bool * success_p, char *string);
extern void generatorOptions(struct state *state);
extern void chooseTests(struct state *state);
extern void fixParameters(struct state *state);
extern bool copyBitsToEpsilon(struct state *state, long int thread_id, BYTE *x, long int xBitLength, long int *num_0s,
			      long int *num_1s, long int *bitsRead);
extern void invokeTestSuite(struct state *state);
extern void read_from_p_val_file(struct state *state);
extern void write_p_val_to_file(struct state *state);
extern void createDirectoryTree(struct state *state);
extern void print_option_summary(struct state *state, char *where);
extern int sum_will_overflow_long(long int si_a, long int si_b);
extern int multiplication_will_overflow_long(long int si_a, long int si_b);
extern void append_string_to_linked_list(struct Node **head, char* string);
#endif
static void getTimestamp(char *buf, size_t len);

// generators.h

static void lcg(void);
static void quadRes1(void);
static void quadRes2(void);
static void cubicRes(void);
static void exclusiveOR(void);
static void modExp(void);
static void bbs(void);
static void micali_schnorr(void);
static void SHA1(void);

/*
 * The circular shifts.
 */
#   define CS1(x) ((((ULONG)x)<<1)|(((ULONG)x)>>31))
#   define CS5(x)  ((((ULONG)x)<<5)|(((ULONG)x)>>27))
#   define CS30(x)  ((((ULONG)x)<<30)|(((ULONG)x)>>2))

/*
 * K constants
 */

#   define K0  0x5a827999L
#   define K1  0x6ed9eba1L
#   define K2  0x8f1bbcdcL
#   define K3  0xca62c1d6L

#   define f1(x,y,z)   ( (x & (y ^ z)) ^ z )

#   define f3(x,y,z)   ( (x & ( y ^ z )) ^ (z & y) )

#   define f2(x,y,z)   ( x ^ y ^ z )	/* Rounds 20-39 */

#   define  expand(x)  Wbuff[x%16] = CS1(Wbuff[(x - 3)%16 ] ^ Wbuff[(x - 8)%16 ] ^ Wbuff[(x - 14)%16] ^ Wbuff[x%16])

#   define sub1Round1(count) \
	 { \
	 temp = CS5(A) + f1(B, C, D) + E + Wbuff[count] + K0; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	 } \

#   define sub2Round1(count)   \
	 { \
	 expand(count); \
	 temp = CS5(A) + f1(B, C, D) + E + Wbuff[count%16] + K0; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	} \

#   define Round2(count)     \
	 { \
	 expand(count); \
	 temp = CS5( A ) + f2( B, C, D ) + E + Wbuff[count%16] + K1;  \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp;  \
	 } \

#   define Round3(count)    \
	 { \
	 expand(count); \
	 temp = CS5( A ) + f3( B, C, D ) + E + Wbuff[count%16] + K2; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	 }

#   define Round4(count)    \
	 { \
	 expand(count); \
	 temp = CS5( A ) + f2( B, C, D ) + E + Wbuff[count%16] + K3; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	 }

// config.h

/*
 * AUTO DEFINES (DON'T TOUCH!)
 */

#   if __STDC_VERSION__ >= 199901L

#      include <stdint.h>

typedef uint8_t *CSTRTD;	// C strings are usually unsigned bytes
typedef uint8_t *BSTRTD;	// so called sts bit strings are 1 bit per octet
typedef uint8_t BYTE;		// byte as in unsigned 8 bit (octet)
typedef uint32_t UINT;		// force int to be 32 bit unsigned values
typedef uint16_t USHORT;	// force shorts to be 16 bit unsigned values
typedef uint32_t ULONG;		// sts assumes long is a 32 bit unsigned value
typedef uint16_t DIGIT;		// sts prefers to store digits in 16 bit unsigned values
typedef uint32_t DBLWORD;	// sts assumes this is a 32 bit unsigned value
typedef uint64_t WORD64;	// 64 bit unsigned value

#   else			// old compiler

#      ifndef	CSTRTD
	typedef char *CSTRTD;
#      endif
#      ifndef	BSTRTD
	typedef unsigned char *BSTRTD;
#      endif

#      ifndef	BYTE
	typedef unsigned char BYTE;
#      endif
#      ifndef	UINT
	typedef unsigned int UINT;
#      endif
#      ifndef	USHORT
	typedef unsigned short USHORT;
#      endif
#      ifndef	ULONG
	typedef unsigned long ULONG;
#      endif
#      ifndef	DIGIT
	typedef USHORT DIGIT;	/* 16-bit word */
#      endif
#      ifndef	DBLWORD
	typedef ULONG DBLWORD;	/* 32-bit word */
#      endif

#      ifndef	WORD64
	typedef unsigned long long WORD64;	/* 64-bit unsigned word */
#      endif

#      ifndef int64_t
	typedef long long int64_t;	/* 64-bit signed word */
#      endif

#   endif			// __STDC_VERSION__ >= 199901L

/*
 * Compiler independent Bool definitions
 */
#   if !defined(__bool_true_false_are_defined) || __bool_true_false_are_defined==0
#      undef bool
#      undef true
#      undef false
#      if defined(__cplusplus)
#         define bool bool
#         define true true
#         define false false
#      else
#         if __STDC_VERSION__ >= 199901L
#            define bool _Bool
#         else
#            define bool unsigned char
#         endif
#         define true 1
#         define false 0
#      endif
#      define __bool_true_false_are_defined 1
#   endif

// genutils.h


typedef struct _MP_struct {
	int size;		/* in bytes */
	int bitlen;		/* in bits, duh */
	BYTE *val;
} MP;

#   define	FREE(A)	if ( (A) ) { free((A)); (A) = NULL; }
#   define	ASCII2BIN(ch)	( (((ch) >= '0') && ((ch) <= '9')) ? \
				  ((ch) - '0') : \
				  (((ch) >= 'A') && ((ch) <= 'F')) ? ((ch) - 'A' + 10) : ((ch) - 'a' + 10) )

#   ifndef EXPWD
#      define	EXPWD		((DBLWORD)1<<NUMLEN)
#   endif

#   define	sniff_bit(ptr,mask)		(*(ptr) & mask)

/*
 * Function Declarations
 */
static int greater(BYTE * x, BYTE * y, int l);
static int less(BYTE * x, BYTE * y, int l);
static BYTE bshl(BYTE * x, int l);
static void bshr(BYTE * x, int l);
static int Mult(BYTE * A, BYTE * B, int LB, BYTE * C, int LC);
static void ModSqr(BYTE * A, BYTE * B, int LB, BYTE * M, int LM);
static void ModMult(BYTE * A, BYTE * B, int LB, BYTE * C, int LC, BYTE * M, int LM);
static void smult(BYTE * A, BYTE b, BYTE * C, int L);
static void Square(BYTE * A, BYTE * B, int L);
static void ModExp(BYTE * A, BYTE * B, int LB, BYTE * C, int LC, BYTE * M, int LM);
static int DivMod(BYTE * x, int lenx, BYTE * n, int lenn, BYTE * quot, BYTE * rem);
static void Mod(BYTE * x, int lenx, BYTE * n, int lenn);
static void Div(BYTE * x, int lenx, BYTE * n, int lenn);
static void sub(BYTE * A, int LA, BYTE * B, int LB);
static int negate(BYTE * A, int L);
static BYTE add(BYTE * A, int LA, BYTE * B, int LB);
static void prettyprintBstr(char *S, BYTE * A, int L);
static void byteReverse(ULONG * buffer, int byteCount);
static void ahtopb(char *ascii_hex, BYTE * p_binary, int bin_len);

// debug.h

/*****************************************************************************
 D E B U G G I N G  A I D E S
 *****************************************************************************/

/*
 * DEBUG_LINT - if defined, debug calls turn into fprintf to stderr calls
 *
 * The purpose of DEBUG_LINT is to let the C compiler do a fprintf format
 * argument count and type check against the debug function.  With DEBUG_LINT,
 * the debug macros are "faked" as best as we can.
 *
 * NOTE: Use of DEBUG_LINT is intended ONLY for static analysis (ala classic lint)
 * with compiler warnings.  DEBUG_LINT works best with -Wall.  In particular, it
 * won't help of you disable warnings that DEBUG_LINT would otherwise generate.
 *
 * When using DEBUG_LINT, consider compiling with:
 *
 *      -Wall -Werror -pedantic
 *
 * with at least the c99 standard.  As in:
 *
 *      -std=c99 -Wall -Werror -pedantic
 *
 * During DEBUG_LINT, output will be written to stderr.  These macros assume
 * that stderr is unbuffered.
 *
 * No error checking is performed by fprintf and fputc to stderr.  Such errors will overwrite
 * errno making the calls that perror print incorrect error messages.
 *
 * The DEBUG_LINT assumes the following global variables is declared elsewhere:
 *
 *      long int debuglevel;            // dbg() verbosity cutoff level
 *
 * The DEBUG_LINT assumes the following file is included somewhere above the include of this file:
 *
 *      #include <stdlib.h>
 *
 * The DEBUG_LINT only works with compilers newer than 199901L (c99 or newer).
 * Defining DEBUG_LINT on an older compiler will be ignored in this file.
 * However when it comes to compiling debug.c, nothing will happen resulting
 * in a link error.  This is a feature, not a bug.  It tells you that your
 * compiler is too old to make use of DEBUG_LINT so don't use it.
 *
 * IMPORTANT NOTE:
 *
 * Executing code with DEBUG_LINT enabled is NOT recommended.
 * It might work, but don't count in it!
 *
 *  You are better off defining DEBUG_LINT in CFLAGS with, say:
 *
 *      -std=c99 -Wall -Werror -pedantic
 *
 * At a minimum, fix warnings (that turn into compiler errors) related to fprintf()
 * calls * until the program compiles.  For better results, fix ALL warnings as some
 * of those warnings may indicate the presence of bugs in your code including but
 * not limited to the use of debug functions.
 */

#   if defined(DEBUG_LINT) && __STDC_VERSION__ >= 199901L

/*
 * Compiler independent Bool definitions
 */
#      if !defined(__bool_true_false_are_defined) || __bool_true_false_are_defined==0
#         undef bool
#         undef true
#         undef false
#         if defined(__cplusplus)
#            define bool bool
#            define true true
#            define false false
#         else
#            if __STDC_VERSION__ >= 199901L
#               define bool _Bool
#            else
#               define bool unsigned char
#            endif
#            define true 1
#            define false 0
#         endif
#         define __bool_true_false_are_defined 1
#      endif

#      define msg(...) fprintf(stderr, __VA_ARGS__)
#      define dbg(level, ...) ((debuglevel >= (level)) ? printf(__VA_ARGS__) : true)
#      define warn(name, ...) (fprintf(stderr, "%s: ", (name)), \
			 fprintf(stderr, __VA_ARGS__))
#      define warnp(name, ...) (fprintf(stderr, "%s: ", (name)), \
			  fprintf(stderr, __VA_ARGS__), \
			  fputc('\n', stderr), \
			  perror(__FUNCTION__))
#      define err(exitcode, name, ...) (fprintf(stderr, "%s: ", (name)), \
				  fprintf(stderr, __VA_ARGS__), \
				  exit(exitcode))
#      define errp(exitcode, name, ...) (fprintf(stderr, "%s: ", (name)), \
				   fprintf(stderr, __VA_ARGS__), \
				   fputc('\n', stderr), \
				   perror(__FUNCTION__), \
				   exit(exitcode))
#      define usage_err(exitcode, name, ...) (fprintf(stderr, "%s: ", (name)), \
					       fprintf(stderr, __VA_ARGS__), \
					       exit(exitcode))
#      define usage_errp(exitcode, name, ...) (fprintf(stderr, "%s: ", (name)), \
						fprintf(stderr, __VA_ARGS__), \
						fputc('\n', stderr), perror(__FUNCTION__), \
						exit(exitcode))

#   else			// DEBUG_LINT && __STDC_VERSION__ >= 199901L

static void msg(char const *fmt, ...);
static void dbg(int level, char const *fmt, ...);
static void warn(char const *name, char const *fmt, ...);
static void warnp(char const *name, char const *fmt, ...);
static void err(int exitcode, char const *name, char const *fmt, ...);
static void errp(int exitcode, char const *name, char const *fmt, ...);
static void usage_err(int exitcode, char const *name, char const *fmt, ...);
static void usage_errp(int exitcode, char const *name, char const *fmt, ...);

#   endif			// DEBUG_LINT && __STDC_VERSION__ >= 199901L

#   define DBG_NONE (0)		// no debugging
#   define DBG_LOW (1)		// minimal debugging
#   define DBG_MED (3)		// somewhat more debugging
#   define DBG_HIGH (5)		// verbose debugging
#   define DBG_VHIGH (7)	// very verbose debugging
#   define DBG_VVHIGH (9)	// very very verbose debugging
#   define FORCED_EXIT (255)	// exit(255) on bad exit code


// Generator names (or -g 0: AlgorithmTesting == read from file)
char *generatorDir[NUMOFGENERATORS + 1] = {
	 "AlgorithmTesting",		// -g 0, Read from file -- XXX invalid?
	 "LCG",				// -g 1, Linear Congruential
	 "QCG1",			// -g 2, Quadratic Congruential I
	 "QCG2",			// -g 3, Quadratic Congruential II
	 "CCG",				// -g 4, Cubic Congruential
	 "XOR",				// -g 5, XOR
	 "MODEXP",			// -g 6, Modular Exponentiation
	 "BBS",				// -g 7, Blum-Blum-Shub
	 "MS",				// -g 8, Micali-Schnorr
	 "G-SHA1",			// -g 9, G Using SHA-1
};


// Format of data when read from a file
enum format {
	FORMAT_ASCII_01 = 'a',		// Use ascii '0' and '1' chars, 1 bit per octet
	FORMAT_0 = '0',			// Alias for FORMAT_ASCII_01 - redirects to it
	FORMAT_RAW_BINARY = 'r',	// Data in raw binary, 8 bits per octet
	FORMAT_1 = '1',			// Alias for FORMAT_RAW_BINARY - redirects to it
};


/*
 * globals - see also extern.h
 */
long int n;			// Length of a single bit stream
enum gen generator;		// generator to use


/*
 * static variables
 */
static BitSequence *epsilon = NULL;		// Bit stream
static bool legacy_output = false;		// true ==> try to mimic output format of legacy code
static char *freqFilePath = NULL;		// true if non-NULL, path of freq.txt
static FILE *freqFile = NULL;			// true if non-NULL, open stream for freq.txt
static FILE *streamFile = NULL;			// true if non-NULL, open stream for randomDataPath
static BitSequence *tmpepsilon = NULL;		// Buffer to write to file in dataFormat
static enum format dataFormat = FORMAT_RAW_BINARY;	// -F format: 'r': raw binary, 'a': ASCII '0'/'1' chars
static long int numOfBitStreams = 0;		// -P 7=iterations (-i iterations)
static long int iterationsMissing = 0;		// Number of iterations that need to be completed
static long int reportCycle = 0;		// -I reportCycle: Report after completion of reportCycle iterations
static char *randomDataPath = NULL;		// randdata: path to a random data file, NULL (no file)
static long int bitsRead = 0;			// bits read so far


/*
 * getTimestamp - get the time and write it as a string into a buffer
 *
 * given:
 *      buf             // fixed length buffer
 *      len             // length of the buffer in bytes
 *
 * Determine the time as of "now" and format it as follows:
 *
 *      yyyy-mm-dd hh:mm:ss
 *
 * This function does not retun on error.
 */
static void
getTimestamp(char *buf, size_t len)
{
	time_t seconds;		// seconds since the epoch
	struct tm *loc_ret;	// return value form localtime_r()
	struct tm now;		// the time now
	size_t time_len;	// Length of string produced by strftime() or 0

	// firewall
	if (buf == NULL) {
		err(229, __func__, "bug arg is NULL");
	}
	if (len <= 0) {
		err(229, __func__, "len must be > 0: %lu", len);
	}

	/*
	 * Determine the time for now
	 */
	errno = 0;		// paranoia
	seconds = time(NULL);
	if (seconds < 0) {
		errp(229, __func__, "time returned < 0: %ld", seconds);
	}
	loc_ret = localtime_r(&seconds, &now);
	if (loc_ret == NULL) {
		errp(229, __func__, "localtime_r returned NULL");
	}
	errno = 0;		// paranoia
	time_len = strftime(buf, len - 1, "%F %T", &now);
	if (time_len == 0) {
		errp(229, __func__, "strftime failed");
	}
	buf[len] = '\0';	// paranoia
	return;
}


static void
generator_report_iteration(void)
{
	char buf[BUFSIZ + 1];	// time string buffer
	long int iteration_being_done;

	/*
	 * Count the iteration and report process if requested
	 */
	iteration_being_done = numOfBitStreams - iterationsMissing;

	if (reportCycle > 0 && (((iteration_being_done % reportCycle) == 0) ||
				       (iteration_being_done == numOfBitStreams))) {
		getTimestamp(buf, BUFSIZ);
		msg("Completed iteration %ld of %ld at %s", iteration_being_done,
		    numOfBitStreams, buf);
	}

	if (iteration_being_done == numOfBitStreams) {
		dbg(DBG_LOW, "End of iterate phase\n");
	}
}


/*
 * copyBitsToEpsilon - convert binary bytes into the end of an epsilon bit array
 *
 * given:
 *      x               // pointer to an array (even just 1) binary bytes
 *      xBitLength      // Number of bits to convert
 *      bitsNeeded      // Total number of bits we want to convert this run
 *      num_0s          // pointer to number of 0 bits converted so far
 *      num_1s          // pointer to number of 1 bits converted so far
 *
 * returns:
 *      true ==> we have converted enough bits
 *      false ==> we have NOT converted enough bits, yet
 */
static bool
copyBitsToEpsilon(BYTE *x, long int xBitLength, long int *num_0s, long int *num_1s)
{
	long int i;
	long int j;
	long int count;
	int bit;
	BYTE mask;
	long int zeros;
	long int ones;
	long int bitsNeeded;

	/*
	 * Check preconditions (firewall)
	 */
	if (epsilon == NULL) {
		err(227, __func__, "epsilon is NULL");
	}

	bitsNeeded = n;

	count = 0;
	zeros = ones = 0;
	for (i = 0; i < (xBitLength + BITS_N_BYTE - 1) / BITS_N_BYTE; i++) {
		mask = 0x80;
		for (j = 0; j < 8; j++) {
			if (*(x + i) & mask) {
				bit = 1;
				(*num_1s)++;
				ones++;
			} else {
				bit = 0;
				(*num_0s)++;
				zeros++;
			}
			mask >>= 1;
			epsilon[bitsRead] = (BitSequence) bit;
			(bitsRead)++;
			if (bitsRead >= bitsNeeded) {
				return true;
			}
			if (++count == xBitLength) {
				return false;
			}
		}
	}

	return false;
}


static double
lcg_rand(long int N, double SEED, double *DUNIF, long int NDIM)
{
	long int i;
	double DZ;
	double DOVER;
	double DZ1;
	double DZ2;
	double DOVER1;
	double DOVER2;
	double DTWO31;
	double DMDLS;
	double DA1;
	double DA2;

	DTWO31 = 2147483648.0;	/* DTWO31=2**31 */
	DMDLS = 2147483647.0;	/* DMDLS=2**31-1 */
	DA1 = 41160.0;		/* DA1=950706376 MOD 2**16 */
	DA2 = 950665216.0;	/* DA2=950706376-DA1 */

	DZ = SEED;
	if (N > NDIM) {
		N = NDIM;
	}
	for (i = 1; i <= N; i++) {
		DZ = floor(DZ);
		DZ1 = DZ * DA1;
		DZ2 = DZ * DA2;
		DOVER1 = floor(DZ1 / DTWO31);
		DOVER2 = floor(DZ2 / DTWO31);
		DZ1 = DZ1 - DOVER1 * DTWO31;
		DZ2 = DZ2 - DOVER2 * DTWO31;
		DZ = DZ1 + DZ2 + DOVER1 + DOVER2;
		DOVER = floor(DZ / DMDLS);
		DZ = DZ - DOVER * DMDLS;
		DUNIF[i - 1] = DZ / DMDLS;
		SEED = DZ;
	}

	return SEED;
}


static void
lcg(void)
{
	int io_ret;		// I/O return status
	double *DUNIF;
	double SEED;
	long int i;
	unsigned bit;
	long int num_0s;
	long int num_1s;
	long int v;

	/*
	 * Check preconditions (firewall)
	 */
	if (epsilon == NULL) {
		err(80, __func__, "epsilon is NULL");
	}
	if (legacy_output) {
		if (freqFilePath == NULL) {
			err(80, __func__, "freqFilePath is NULL");
		}
		if (freqFile == NULL) {
			err(80, __func__, "freqFile is NULL");
		}
	}

	SEED = 23482349.0;
	DUNIF = calloc((size_t) n, sizeof(double));
	if (DUNIF == NULL) {
		errp(80, __func__, "could not calloc for DUNIF: %ld doubles of %lu bytes each", n, sizeof(double));
	}

	for (v = 0; v < numOfBitStreams; v++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		SEED = lcg_rand(n, SEED, DUNIF, n);
		for (i = 0; i < n; i++) {
			if (DUNIF[i] < 0.5) {
				bit = 0;
				num_0s++;
			} else {
				bit = 1;
				num_1s++;
			}
			bitsRead++;
			epsilon[i] = (BitSequence) bit;
		}

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (legacy_output == true) {
			fprintf(freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(freqFile);
			if (io_ret != 0) {
				errp(80, __func__, "error flushing to: %s", freqFilePath);
			}
		}
		write_sequence();
		generator_report_iteration();
	}

	free(DUNIF);

	return;
}


static void
quadRes1(void)
{
	int io_ret;		// I/O return status
	long int k;
	long int num_0s;
	long int num_1s;
	bool done;		// true ==> enoungh data has been converted
	BYTE p[64];		// XXX - array size uses magic number
	BYTE g[64];		// XXX - array size uses magic number
	BYTE x[128];		// XXX - array size uses magic number

	/*
	 * Check preconditions (firewall)
	 */
	if (legacy_output) {
		if (freqFilePath == NULL) {
			err(80, __func__, "freqFilePath is NULL");
		}
		if (freqFile == NULL) {
			err(80, __func__, "freqFile is NULL");
		}
	}

	ahtopb
	    ("987b6a6bf2c56a97291c445409920032499f9ee7ad128301b5d0254aa1a9633f"
	     "dbd378d40149f1e23a13849f3d45992f5c4c6b7104099bc301f6005f9d8115e1", p, 64);
	ahtopb
	    ("3844506a9456c564b8b8538e0cc15aff46c95e69600f084f0657c2401b3c2447"
	     "34b62ea9bb95be4923b9b7e84eeaf1a224894ef0328d44bc3eb3e983644da3f5", g, 64);

	for (k = 0; k < numOfBitStreams; k++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		do {
			memset(x, 0x00, 128);
			ModMult(x, g, 64, g, 64, p, 64);
			memcpy(g, x + 64, 64);
			done = copyBitsToEpsilon(g, 512, &num_0s, &num_1s);
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (legacy_output == true) {
			fprintf(freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(freqFile);
			if (io_ret != 0) {
				errp(81, __func__, "error flushing to: %s", freqFilePath);
			}
		}
		write_sequence();
		generator_report_iteration();
	}

	return;
}


static void
quadRes2(void)
{
	int io_ret;		// I/O return status
	BYTE g[64];		// XXX - array size uses magic number
	BYTE x[129];		// XXX - array size uses magic number
	BYTE t1[65];		// XXX - array size uses magic number
	BYTE One[1];		// XXX - array size uses magic number
	BYTE Two;
	BYTE Three[1];		// XXX - array size uses magic number
	bool done;		// true ==> enoungh data has been converted
	long int k;
	long int num_0s;
	long int num_1s;

	/*
	 * Check preconditions (firewall)
	 */
	if (legacy_output) {
		if (freqFilePath == NULL) {
			err(80, __func__, "freqFilePath is NULL");
		}
		if (freqFile == NULL) {
			err(80, __func__, "freqFile is NULL");
		}
	}

	One[0] = 0x01;
	Two = 0x02;
	Three[0] = 0x03;

	ahtopb
	    ("7844506a9456c564b8b8538e0cc15aff46c95e69600f084f0657c2401b3c2447"
	     "34b62ea9bb95be4923b9b7e84eeaf1a224894ef0328d44bc3eb3e983644da3f5", g, 64);

	for (k = 0; k < numOfBitStreams; k++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		do {
			memset(t1, 0x00, 65);
			memset(x, 0x00, 129);
			smult(t1, Two, g, 64);	/* 2x */
			add(t1, 65, Three, 1);	/* 2x+3 */
			Mult(x, t1, 65, g, 64);	/* x(2x+3) */
			add(x, 129, One, 1);	/* x(2x+3)+1 */
			memcpy(g, x + 65, 64);
			done = copyBitsToEpsilon(g, 512, &num_0s, &num_1s);
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (legacy_output == true) {
			fprintf(freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(freqFile);
			if (io_ret != 0) {
				errp(82, __func__, "error flushing to: %s", freqFilePath);
			}
		}
		write_sequence();
		generator_report_iteration();
	}

	return;
}


static void
cubicRes(void)
{
	int io_ret;		// I/O return status
	BYTE g[64];		// XXX - array size uses magic number
	BYTE tmp[128];		// XXX - array size uses magic number
	BYTE x[192];		// XXX - array size uses magic number
	bool done;		// true ==> enoungh data has been converted
	long int k;
	long int num_0s;
	long int num_1s;

	/*
	 * Check preconditions (firewall)
	 */
	if (legacy_output) {
		if (freqFilePath == NULL) {
			err(80, __func__, "freqFilePath is NULL");
		}
		if (freqFile == NULL) {
			err(80, __func__, "freqFile is NULL");
		}
	}

	ahtopb
	    ("7844506a9456c564b8b8538e0cc15aff46c95e69600f084f0657c2401b3c2447"
	     "34b62ea9bb95be4923b9b7e84eeaf1a224894ef0328d44bc3eb3e983644da3f5", g, 64);

	for (k = 0; k < numOfBitStreams; k++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		do {
			memset(tmp, 0x00, 128);
			memset(x, 0x00, 192);
			Mult(tmp, g, 64, g, 64);
			Mult(x, tmp, 128, g, 64);	// Don't need to mod by 2^512, just take low 64 bytes
			memcpy(g, x + 128, 64);
			done = copyBitsToEpsilon(x, 512, &num_0s, &num_1s);
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (legacy_output == true) {
			fprintf(freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(freqFile);
			if (io_ret != 0) {
				errp(83, __func__, "error flushing to: %s", freqFilePath);
			}
		}
		write_sequence();
		generator_report_iteration();
	}

	return;
}


static void
exclusiveOR(void)
{
	int io_ret;		// I/O return status
	long int i;
	long int num_0s;
	long int num_1s;
	BYTE bit_sequence[127];	// XXX - array size uses magic number

	/*
	 * Check preconditions (firewall)
	 */
	if (epsilon == NULL) {
		err(80, __func__, "epsilon is NULL");
	}
	if (legacy_output) {
		if (freqFilePath == NULL) {
			err(80, __func__, "freqFilePath is NULL");
		}
		if (freqFile == NULL) {
			err(80, __func__, "freqFile is NULL");
		}
	}

	memcpy(bit_sequence,
	       "0001011011011001000101111001001010011011101101000100000010101111"
	       "111010100100001010110110000000000100110000101110011111111100111", 127);
	num_0s = 0;
	num_1s = 0;
	bitsRead = 0;
	for (i = 0; i < 127; i++) {
		if (bit_sequence[i] == '1') {
			epsilon[bitsRead] = 1;
			num_1s++;
		} else {
			epsilon[bitsRead] = 0;
			num_1s++;
		}
		bitsRead++;
	}
	for (i = 127; i < n * numOfBitStreams; i++) {
		if (bit_sequence[(i - 1) % 127] != bit_sequence[(i - 127) % 127]) {
			bit_sequence[i % 127] = 1;
			epsilon[bitsRead] = 1;
			num_1s++;
		} else {
			bit_sequence[i % 127] = 0;
			epsilon[bitsRead] = 0;
			num_0s++;
		}
		bitsRead++;
		if (bitsRead >= n) {

			/*
			 * Write stats to freq.txt if in legacy_output mode
			 */
			if (legacy_output == true) {
				fprintf(freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
				errno = 0;        // paranoia
				io_ret = fflush(freqFile);
				if (io_ret != 0) {
					errp(84, __func__, "error flushing to: %s", freqFilePath);
				}
			}
			write_sequence();
			generator_report_iteration();

			num_0s = 0;
			num_1s = 0;
			bitsRead = 0;
		}
	}

	return;
}


static void
modExp(void)
{
	int io_ret;		// I/O return status
	long int k;
	long int num_0s;
	long int num_1s;
	bool done;		// true ==> enoungh data has been converted
	BYTE p[64];		// XXX - array size uses magic number
	BYTE g[64];		// XXX - array size uses magic number
	BYTE x[192];		// XXX - array size uses magic number
	BYTE y[20];		// XXX - array size uses magic number

	/*
	 * Check preconditions (firewall)
	 */
	if (legacy_output) {
		if (freqFilePath == NULL) {
			err(80, __func__, "freqFilePath is NULL");
		}
		if (freqFile == NULL) {
			err(80, __func__, "freqFile is NULL");
		}
	}

	ahtopb("7AB36982CE1ADF832019CDFEB2393CABDF0214EC", y, 20);
	ahtopb
	    ("987b6a6bf2c56a97291c445409920032499f9ee7ad128301b5d0254aa1a9633f"
	     "dbd378d40149f1e23a13849f3d45992f5c4c6b7104099bc301f6005f9d8115e1", p, 64);
	ahtopb
	    ("3844506a9456c564b8b8538e0cc15aff46c95e69600f084f0657c2401b3c2447"
	     "34b62ea9bb95be4923b9b7e84eeaf1a224894ef0328d44bc3eb3e983644da3f5", g, 64);

	for (k = 0; k < numOfBitStreams; k++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		do {
			memset(x, 0x00, 128);
			ModExp(x, g, 64, y, 20, p, 64);	/* NOTE: g must be less than p */
			done = copyBitsToEpsilon(x, 512, &num_0s, &num_1s);
			memcpy(y, x + 44, 20);
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (legacy_output == true) {
			fprintf(freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(freqFile);
			if (io_ret != 0) {
				errp(85, __func__, "error flushing to: %s", freqFilePath);
			}
		}
		write_sequence();
		generator_report_iteration();
	}

	return;
}

static void
bbs(void)
{
	int io_ret;		// I/O return status
	long int i;
	long int v;
	BYTE p[64];		// XXX - array size uses magic number
	BYTE q[64];		// XXX - array size uses magic number
	BYTE n_128[128];		// XXX - array size uses magic number
	BYTE s[64];		// XXX - array size uses magic number
	BYTE x[256];		// XXX - array size uses magic number
	long int num_0s;
	long int num_1s;

	/*
	 * Check preconditions (firewall)
	 */
	if (epsilon == NULL) {
		err(80, __func__, "epsilon is NULL");
	}
	if (legacy_output) {
		if (freqFilePath == NULL) {
			err(80, __func__, "freqFilePath is NULL");
		}
		if (freqFile == NULL) {
			err(80, __func__, "freqFile is NULL");
		}
	}

	ahtopb
	    ("E65097BAEC92E70478CAF4ED0ED94E1C94B154466BFB9EC9BE37B2B0FF8526C2"
	     "22B76E0E915017535AE8B9207250257D0A0C87C0DACEF78E17D1EF9DC44FD91F", p, 64);
	ahtopb
	    ("E029AEFCF8EA2C29D99CB53DD5FA9BC1D0176F5DF8D9110FD16EE21F32E37BA8"
	     "6FF42F00531AD5B8A43073182CC2E15F5C86E8DA059E346777C9A985F7D8A867", q, 64);
	memset(n_128, 0x00, 128);
	Mult(n_128, p, 64, q, 64);
	memset(s, 0x00, 64);
	ahtopb
	    ("10d6333cfac8e30e808d2192f7c0439480da79db9bbca1667d73be9a677ed313"
	     "11f3b830937763837cb7b1b1dc75f14eea417f84d9625628750de99e7ef1e976", s, 64);
	memset(x, 0x00, 256);
	ModSqr(x, s, 64, n_128, 128);

	for (v = 0; v < numOfBitStreams; v++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		for (i = 0; i < n; i++) {
			ModSqr(x, x, 128, n_128, 128);
			memcpy(x, x + 128, 128);
			if ((x[127] & 0x01) == 0) {
				num_0s++;
				epsilon[i] = 0;
			} else {
				num_1s++;
				epsilon[i] = 1;
			}
			bitsRead++;
			if ((i % 50000) == 0) {
				printf("\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
				fflush(stdout);
			}
		}

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (legacy_output == true) {
			fprintf(freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(freqFile);
			if (io_ret != 0) {
				errp(86, __func__, "error flushing to: %s", freqFilePath);
			}
		}
		write_sequence();
		generator_report_iteration();
	}

	return;
}


// The exponent, e, is set to 11
// This results in k = 837 and r = 187
static void
micali_schnorr(void)
{
	int io_ret;		// I/O return status
	long int i;
	long int j;
	long int k = 837;
	long int num_0s;
	long int num_1s;
	bool done;		// true ==> enoungh data has been converted
	BYTE p[64];		// XXX - array size uses magic number
	BYTE q[64];		// XXX - array size uses magic number
	BYTE n_128[128];		// XXX - array size uses magic number
	BYTE e[1];		// XXX - array size uses magic number
	BYTE X[128];		// XXX - array size uses magic number
	BYTE Y[384];		// XXX - array size uses magic number
	BYTE Tail[105];		// XXX - array size uses magic number

	/*
	 * Check preconditions (firewall)
	 */
	if (legacy_output) {
		if (freqFilePath == NULL) {
			err(80, __func__, "freqFilePath is NULL");
		}
		if (freqFile == NULL) {
			err(80, __func__, "freqFile is NULL");
		}
	}

	ahtopb
	    ("E65097BAEC92E70478CAF4ED0ED94E1C94B154466BFB9EC9BE37B2B0FF8526C2"
	     "22B76E0E915017535AE8B9207250257D0A0C87C0DACEF78E17D1EF9DC44FD91F", p, 64);
	ahtopb
	    ("E029AEFCF8EA2C29D99CB53DD5FA9BC1D0176F5DF8D9110FD16EE21F32E37BA8"
	     "6FF42F00531AD5B8A43073182CC2E15F5C86E8DA059E346777C9A985F7D8A867", q, 64);
	memset(n_128, 0x00, 128);
	Mult(n_128, p, 64, q, 64);
	e[0] = 0x0b;
	memset(X, 0x00, 128);
	ahtopb("237c5f791c2cfe47bfb16d2d54a0d60665b20904ec822a6", X + 104, 24);

	for (i = 0; i < numOfBitStreams; i++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		do {
			ModExp(Y, X, 128, e, 1, n_128, 128);
			memcpy(Tail, Y + 23, 105);
			for (j = 0; j < 3; j++) {
				bshl(Tail, 105);
			}
			done = copyBitsToEpsilon(Tail, k, &num_0s, &num_1s);
			memset(X, 0x00, 128);
			memcpy(X + 104, Y, 24);
			for (j = 0; j < 5; j++) {
				bshr(X + 104, 24);
			}
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (legacy_output == true) {
			fprintf(freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(freqFile);
			if (io_ret != 0) {
				errp(87, __func__, "error flushing to: %s", freqFilePath);
			}
		}
		write_sequence();
		generator_report_iteration();
	}

	return;
}

// Uses 160 bit Xkey and no XSeed (b=160)
// This is the generic form of the generator found on the last page of the Change Notice for FIPS 186-2
static void
SHA1(void)
{
	int io_ret;		// I/O return status
	ULONG A;
	ULONG B;
	ULONG C;
	ULONG D;
	ULONG E;
	ULONG temp;
	ULONG Wbuff[16];	// XXX - array size uses magic number
	BYTE Xkey[20];		// XXX - array size uses magic number
	BYTE G[20];		// XXX - array size uses magic number
	BYTE M[64];		// XXX - array size uses magic number
	BYTE One[1] = { 0x01 };
	long int i;
	long int num_0s;
	long int num_1s;
	bool done;		// true ==> enough data has been converted
	ULONG tx[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };	// XXX - array size uses magic number

	/*
	 * Check preconditions (firewall)
	 */
	if (legacy_output) {
		if (freqFilePath == NULL) {
			err(80, __func__, "freqFilePath is NULL");
		}
		if (freqFile == NULL) {
			err(80, __func__, "freqFile is NULL");
		}
	}

	ahtopb("ec822a619d6ed5d9492218a7a4c5b15d57c61601", Xkey, 20);
	// ahtopb("E65097BAEC92E70478CAF4ED0ED94E1C94B15446", Xkey, 20);
	// ahtopb("6BFB9EC9BE37B2B0FF8526C222B76E0E91501753", Xkey, 20);
	// ahtopb("5AE8B9207250257D0A0C87C0DACEF78E17D1EF9D", Xkey, 20);
	// ahtopb("D99CB53DD5FA9BC1D0176F5DF8D9110FD16EE21F", Xkey, 20);

	for (i = 0; i < numOfBitStreams; i++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		do {
			memcpy(M, Xkey, 20);
			memset(M + 20, 0x00, 44);

			// Start: SHA Steps A-E
			A = tx[0];
			B = tx[1];
			C = tx[2];
			D = tx[3];
			E = tx[4];

			memcpy((BYTE *) Wbuff, M, 64);
#ifdef LITTLE_ENDIAN
			byteReverse(Wbuff, 20);
#endif
			sub1Round1(0);
			sub1Round1(1);
			sub1Round1(2);
			sub1Round1(3);
			sub1Round1(4);
			sub1Round1(5);
			sub1Round1(6);
			sub1Round1(7);
			sub1Round1(8);
			sub1Round1(9);
			sub1Round1(10);
			sub1Round1(11);
			sub1Round1(12);
			sub1Round1(13);
			sub1Round1(14);
			sub1Round1(15);
			sub2Round1(16);
			sub2Round1(17);
			sub2Round1(18);
			sub2Round1(19);
			Round2(20);
			Round2(21);
			Round2(22);
			Round2(23);
			Round2(24);
			Round2(25);
			Round2(26);
			Round2(27);
			Round2(28);
			Round2(29);
			Round2(30);
			Round2(31);
			Round2(32);
			Round2(33);
			Round2(34);
			Round2(35);
			Round2(36);
			Round2(37);
			Round2(38);
			Round2(39);
			Round3(40);
			Round3(41);
			Round3(42);
			Round3(43);
			Round3(44);
			Round3(45);
			Round3(46);
			Round3(47);
			Round3(48);
			Round3(49);
			Round3(50);
			Round3(51);
			Round3(52);
			Round3(53);
			Round3(54);
			Round3(55);
			Round3(56);
			Round3(57);
			Round3(58);
			Round3(59);
			Round4(60);
			Round4(61);
			Round4(62);
			Round4(63);
			Round4(64);
			Round4(65);
			Round4(66);
			Round4(67);
			Round4(68);
			Round4(69);
			Round4(70);
			Round4(71);
			Round4(72);
			Round4(73);
			Round4(74);
			Round4(75);
			Round4(76);
			Round4(77);
			Round4(78);
			Round4(79);

			A += tx[0];
			B += tx[1];
			C += tx[2];
			D += tx[3];
			E += tx[4];

			memcpy(G, (BYTE *) & A, 4);
			memcpy(G + 4, (BYTE *) & B, 4);
			memcpy(G + 8, (BYTE *) & C, 4);
			memcpy(G + 12, (BYTE *) & D, 4);
			memcpy(G + 16, (BYTE *) & E, 4);
#ifdef LITTLE_ENDIAN
			byteReverse((ULONG *) G, 20);
#endif
			// End: SHA Steps A-E

			done = copyBitsToEpsilon(G, 160, &num_0s, &num_1s);
			add(Xkey, 20, G, 20);
			add(Xkey, 20, One, 1);
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (legacy_output == true) {
			fprintf(freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(freqFile);
			if (io_ret != 0) {
				errp(88, __func__, "error flushing to: %s", freqFilePath);
			}
		}
		write_sequence();
		generator_report_iteration();
	}

	return;
}


/*
 * write_sequence - write the epsilon stream to a randomDataPath
 */
static void
write_sequence(void)
{
	int io_ret;		// I/O return status
	int count;
	long int i;
	int j;

	/*
	 * Check preconditions (firewall)
	 */
	if (tmpepsilon == NULL) {
		err(231, __func__, "tmpepsilon is NULL");
	}
	if (streamFile == NULL) {
		err(231, __func__, "streamFile is NULL");
	}
	if (randomDataPath == NULL) {
		err(231, __func__, "randomDataPath is NULL");
	}

	/*
	 * Write contents of epsilon
	 */
	switch (dataFormat) {
	case FORMAT_RAW_BINARY:

		/*
		 * Store output as binary bits
		 */
		for (i = 0, j = 0, count = 0, tmpepsilon[0] = 0; i < n; i++) {

			// Store bit in current output byte
			if (epsilon[i] == 1) {
				tmpepsilon[j] |= (1 << count);
			} else if (epsilon[i] != 0) {
				err(231, __func__, "epsilon[%ld]: %d is neither 0 nor 1", i, epsilon[i]);
			}
			++count;

			// When an output byte is complete
			if (count >= BITS_N_BYTE) {
				// Prep for next byte
				count = 0;
				if (j >= (n / BITS_N_BYTE)) {
					break;
				}
				++j;
				tmpepsilon[j] = 0;
			}
		}

		/*
		 * Write output buffer
		 */
		errno = 0;	// paranoia
		io_ret = (int) fwrite(tmpepsilon, sizeof(BitSequence), (size_t) j, streamFile);
		if (io_ret < j) {
			errp(231, __func__, "write of %d elements of %ld bytes to %s failed",
			     j, sizeof(BitSequence), randomDataPath);
		}
		errno = 0;	// paranoia
		io_ret = fflush(streamFile);
		if (io_ret == EOF) {
			errp(231, __func__, "flush of %s failed", randomDataPath);
		}
		break;

	case FORMAT_ASCII_01:

		/*
		 * Store output as ASCII string of numbers
		 */
		for (i = 0; i < n; i++) {
			if (epsilon[i] == 0) {
				tmpepsilon[i] = '0';
			} else if (epsilon[i] == 1) {
				tmpepsilon[i] = '1';
			} else {
				err(231, __func__, "epsilon[%ld]: %d is neither 0 nor 1", i, epsilon[i]);
			}
		}

		/*
		 * Write file as ASCII string of numbers
		 */
		errno = 0;	// paranoia
		io_ret = (int) fwrite(tmpepsilon, sizeof(BitSequence), (size_t) n, streamFile);
		if (io_ret < n) {
			errp(231, __func__, "write of %ld elements of %ld bytes to %s failed",
			     n, sizeof(BitSequence), randomDataPath);
		}
		io_ret = fflush(streamFile);
		if (io_ret == EOF) {
			errp(231, __func__, "flush of %s failed", randomDataPath);
		}
		break;

	default:
		err(231, __func__, "Invalid format");
		break;
	}

	return;
}

/*
 * file: mp.c
 *
 * DESCRIPTION
 *
 * These functions comprise a multi-precision integer arithmetic
 * and discrete function package.
 */

#define	MAXPLEN		384


/*****************************************
** greater - Test if x > y               *
**                                       *
** Returns TRUE (1) if x greater than y, *
** otherwise FALSE (0).                  *
**                                       *
** Parameters:                           *
**                                       *
**  x      Address of array x            *
**  y      Address of array y            *
**  l      Length both x and y in bytes  *
**                                       *
******************************************/
static int
greater(BYTE * x, BYTE * y, int l)
{
	int i;

	for (i = 0; i < l; i++) {
		if (x[i] != y[i]) {
			break;
		}
	}

	if (i == l) {
		return 0;
	}

	if (x[i] > y[i]) {
		return 1;
	}

	return 0;
}


/*****************************************
** less - Test if x < y                  *
**                                       *
** Returns TRUE (1) if x less than y,    *
** otherwise FALSE (0).                  *
**                                       *
** Parameters:                           *
**                                       *
**  x      Address of array x            *
**  y      Address of array y            *
**  l      Length both x and y in bytes  *
**                                       *
******************************************/
int
less(BYTE * x, BYTE * y, int l)
{
	int i;

	for (i = 0; i < l; i++) {
		if (x[i] != y[i]) {
			break;
		}
	}

	if (i == l) {
		return 0;
	}

	if (x[i] < y[i]) {
		return 1;
	}

	return 0;
}


/*****************************************
** bshl - shifts array left              *
**                  by one bit.          *
**                                       *
** x = x * 2                             *
**                                       *
** Parameters:                           *
**                                       *
**  x      Address of array x            *
**  l      Length array x in bytes       *
**                                       *
******************************************/
static BYTE
bshl(BYTE * x, int l)
{
	BYTE *p;
	int c1;
	int c2;

	p = x + l - 1;
	c1 = 0;
	c2 = 0;
	while (p != x) {
		if (*p & 0x80) {
			c2 = 1;
		}
		*p <<= 1;	/* shift the word left once (ls bit = 0) */
		if (c1) {
			*p |= 1;
		}
		c1 = c2;
		c2 = 0;
		p--;
	}

	if (*p & 0x80) {
		c2 = 1;
	}
	*p <<= 1;		/* shift the word left once (ls bit = 0) */
	if (c1) {
		*p |= (DIGIT) 1;
	}

	return (BYTE) c2;
}


/*****************************************
** bshr - shifts array right             *
**                   by one bit.         *
**                                       *
** x = x / 2                             *
**                                       *
** Parameters:                           *
**                                       *
**  x      Address of array x            *
**  l      Length array x in bytes       *
**                                       *
******************************************/
static void
bshr(BYTE * x, int l)
{
	BYTE *p;
	int c1;
	int c2;

	p = x;
	c1 = 0;
	c2 = 0;
	while (p != x + l - 1) {
		if (*p & 0x01) {
			c2 = 1;
		}
		*p >>= 1;	/* shift the word right once (ms bit = 0) */
		if (c1) {
			*p |= 0x80;
		}
		c1 = c2;
		c2 = 0;
		p++;
	}

	*p >>= 1;		/* shift the word right once (ms bit = 0) */
	if (c1) {
		*p |= 0x80;
	}
}


/*****************************************
** Mult - Multiply two integers          *
**                                       *
** A = B * C                             *
**                                       *
** Parameters:                           *
**                                       *
**  A      Address of the result         *
**  B      Address of the multiplier     *
**  C      Address of the multiplicand   *
**  LB      Length of B in bytes         *
**  LC      Length of C in bytes         *
**                                       *
**  NOTE:  A MUST be LB+LC in length     *
**                                       *
******************************************/
static int
Mult(BYTE * A, BYTE * B, int LB, BYTE * C, int LC)
{
	int i;
	int j;
	int k;
	DIGIT result;

	for (i = LB - 1; i >= 0; i--) {
		result = 0;
		k = 0;
		for (j = LC - 1; j >= 0; j--) {
			k = i + j + 1;
			result = (DIGIT) A[k] + ((DIGIT) (B[i] * C[j])) + (result >> 8);
			A[k] = (BYTE) result;
		}
		A[--k] = (BYTE) (result >> 8);
	}

	return 0;
}


static void
ModSqr(BYTE * A, BYTE * B, int LB, BYTE * M, int LM)
{

	Square(A, B, LB);
	Mod(A, 2 * LB, M, LM);
}

static void
ModMult(BYTE * A, BYTE * B, int LB, BYTE * C, int LC, BYTE * M, int LM)
{
	Mult(A, B, LB, C, LC);
	Mod(A, (LB + LC), M, LM);
}


/*****************************************
** smult - Multiply array by a scalar.   *
**                                       *
** A = b * C                             *
**                                       *
** Parameters:                           *
**                                       *
**  A      Address of the result         *
**  b      Scalar (1 BYTE)               *
**  C      Address of the multiplicand   *
**  L      Length of C in bytes          *
**                                       *
**  NOTE:  A MUST be L+1 in length       *
**                                       *
******************************************/
static void
smult(BYTE * A, BYTE b, BYTE * C, int L)
{
	int i;
	DIGIT result;

	result = 0;
	for (i = L - 1; i > 0; i--) {
		result = A[i] + ((DIGIT) b * C[i]) + (result >> 8);
		A[i] = (BYTE) (result & 0xff);
		A[i - 1] = (BYTE) (result >> 8);
	}
}

/*****************************************
** Square() - Square an integer          *
**                                       *
** A = B^2                               *
**                                       *
** Parameters:                           *
**                                       *
**  A      Address of the result         *
**  B      Address of the operand        *
**  L      Length of B in bytes          *
**                                       *
**  NOTE:  A MUST be 2*L in length       *
**                                       *
******************************************/
static void
Square(BYTE * A, BYTE * B, int L)
{
	Mult(A, B, L, B, L);
}

/*****************************************
** ModExp - Modular Exponentiation       *
**                                       *
** A = B ** C (MOD M)                    *
**                                       *
** Parameters:                           *
**                                       *
**  A      Address of result             *
**  B      Address of mantissa           *
**  C      Address of exponent           *
**  M      Address of modulus            *
**  LB     Length of B in bytes          *
**  LC     Length of C in bytes          *
**  LM     Length of M in bytes          *
**                                       *
**  NOTE: The integer B must be less     *
**        than the modulus M.      	 *
**  NOTE: A must be at least 3*LM        *
**        bytes long.  However, the      *
**        result stored in A will be     *
**        only LM bytes long.            *
******************************************/
static void
ModExp(BYTE * A, BYTE * B, int LB, BYTE * C, int LC, BYTE * M, int LM)
{
	BYTE wmask;
	int bits;

	bits = LC * 8;
	wmask = 0x80;

	A[LM - 1] = 1;

	while (!sniff_bit(C, wmask)) {
		wmask >>= 1;
		bits--;
		if (!wmask) {
			wmask = 0x80;
			C++;
		}
	}

	while (bits--) {
		memset(A + LM, 0x00, LM * 2);

		/*
		 * temp = A*A (MOD M)
		 */
		ModSqr(A + LM, A, LM, M, LM);

		/*
		 * A = lower L bytes of temp
		 */
		memcpy(A, A + LM * 2, LM);
		memset(A + LM, 0x00, 2 * LM);

		if (sniff_bit(C, wmask)) {
			memset(A + LM, 0x00, (LM + LB));
			ModMult(A + LM, B, LB, A, LM, M, LM);	/* temp = B * A (MOD M) */
			memcpy(A, A + LM + (LM + LB) - LM, LM);	/* A = lower LM bytes of temp */
			memset(A + LM, 0x00, 2 * LM);
		}

		wmask >>= 1;
		if (!wmask) {
			wmask = 0x80;
			C++;
		}
	}
}


/*
 * DivMod: computes: quot = x / n rem = x % n returns: length of "quot" len of rem is lenx+1
 */
static int
DivMod(BYTE * x, int lenx, BYTE * n, int lenn, BYTE * quot, BYTE * rem)
{
	BYTE *tx;
	BYTE *tn;
	BYTE *ttx;
	BYTE *ts;
	BYTE bmult[1];
	int i;
	int shift;
	int lgth_x;
	int lgth_n;
	int t_len;
	int lenq;
	DIGIT tMSn;
	DIGIT mult;
	ULONG tMSx;
	int underflow;

	tx = x;
	tn = n;

	/*
	 * point to the MSD of n
	 */
	for (i = 0, lgth_n = lenn; i < lenn; i++, lgth_n--) {
		if (*tn) {
			break;
		}
		tn++;
	}
	if (!lgth_n) {
		return 0;
	}

	/*
	 * point to the MSD of x
	 */
	for (i = 0, lgth_x = lenx; i < lenx; i++, lgth_x--) {
		if (*tx) {
			break;
		}
		tx++;
	}
	if (!lgth_x) {
		return 0;
	}

	if (lgth_x < lgth_n) {
		lenq = 1;
	} else {
		lenq = lgth_x - lgth_n + 1;
	}
	memset(quot, 0x00, lenq);

	/*
	 * Loop while x > n, WATCH OUT if lgth_x == lgth_n
	 */
	while ((lgth_x > lgth_n) || ((lgth_x == lgth_n) && !less(tx, tn, lgth_n))) {
		shift = 1;
		if (lgth_n == 1) {
			if (*tx < *tn) {
				tMSx = (DIGIT) (((*tx) << 8) | *(tx + 1));
				tMSn = *tn;
				shift = 0;
			} else {
				tMSx = *tx;
				tMSn = *tn;
			}
		} else if (lgth_n > 1) {
			tMSx = (DIGIT) (((*tx) << 8) | *(tx + 1));
			tMSn = (DIGIT) (((*tn) << 8) | *(tn + 1));
			if ((tMSx < tMSn) || ((tMSx == tMSn) && less(tx, tn, lgth_n))) {
				tMSx = (tMSx << 8) | *(tx + 2);
				shift = 0;
			}
		} else {
			tMSx = (DIGIT) (((*tx) << 8) | *(tx + 1));
			tMSn = *tn;
			shift = 0;
		}

		mult = (DIGIT) (tMSx / tMSn);
		if (mult > 0xff) {
			mult = 0xff;
		}
		bmult[0] = mult & 0xff;

		ts = rem;
		do {
			memset(ts, 0x00, lgth_x + 1);
			Mult(ts, tn, lgth_n, bmult, 1);

			underflow = 0;
			if (shift) {
				if (ts[0] != 0) {
					underflow = 1;
				} else {
					for (i = 0; i < lgth_x; i++) {
						ts[i] = ts[i + 1];
					}
					ts[lgth_x] = 0x00;
				}
			}
			if (greater(ts, tx, lgth_x) || underflow) {
				bmult[0]--;
				underflow = 1;
			} else {
				underflow = 0;
			}
		} while (underflow);
		sub(tx, lgth_x, ts, lgth_x);
		if (shift) {
			quot[lenq - (lgth_x - lgth_n) - 1] = bmult[0];
		} else {
			quot[lenq - (lgth_x - lgth_n)] = bmult[0];
		}

		ttx = tx;
		t_len = lgth_x;
		for (i = 0, lgth_x = t_len; i < t_len; i++, lgth_x--) {
			if (*ttx) {
				break;
			}
			ttx++;
		}
		tx = ttx;
	}
	memset(rem, 0x00, lenn);
	if (lgth_x) {
		memcpy(rem + lenn - lgth_x, tx, lgth_x);
	}

	return lenq;
}


/*
 * Mod - Computes an integer modulo another integer
 *
 * x = x (mod n)
 *
 */
static void
Mod(BYTE * x, int lenx, BYTE * n, int lenn)
{
	BYTE quot[MAXPLEN + 1];
	BYTE rem[2 * MAXPLEN + 1];

	memset(quot, 0x00, sizeof(quot));
	memset(rem, 0x00, sizeof(rem));
	if (DivMod(x, lenx, n, lenn, quot, rem)) {
		memset(x, 0x00, lenx);
		memcpy(x + lenx - lenn, rem, lenn);
	}
}

/*
 * Div - Computes the integer division of two numbers
 *
 * x = x / n
 *
 */
static void
Div(BYTE * x, int lenx, BYTE * n, int lenn)
{
	BYTE quot[MAXPLEN + 1];
	BYTE rem[2 * MAXPLEN + 1];
	int lenq;

	memset(quot, 0x00, sizeof(quot));
	memset(rem, 0x00, sizeof(rem));
	if ((lenq = DivMod(x, lenx, n, lenn, quot, rem)) != 0) {
		memset(x, 0x00, lenx);
		memcpy(x + lenx - lenq, quot, lenq);
	}
}


/*****************************************
** sub - Subtract two integers           *
**                                       *
** A = A - B                             *
**                                       *
**                                       *
** Parameters:                           *
**                                       *
**  A      Address of subtrahend integer *
**  B      Address of subtractor integer *
**  L      Length of A and B in bytes    *
**                                       *
**  NOTE: In order to save RAM, B is     *
**        two's complemented twice,      *
**        rather than using a copy of B  *
**                                       *
******************************************/
static void
sub(BYTE * A, int LA, BYTE * B, int LB)
{
	BYTE *tb;

	tb = calloc(LA, 1);
	if (tb == NULL) {
		errp(90, __func__, "cannot calloc for LA: %d bytes", LA);
	}
	memcpy(tb, B, LB);
	negate(tb, LB);
	add(A, LA, tb, LA);

	FREE(tb);
}


/*****************************************
** negate - Negate an integer            *
**                                       *
** A = -A                                *
**                                       *
**                                       *
** Parameters:                           *
**                                       *
**  A      Address of integer to negate  *
**  L      Length of A in bytes          *
**                                       *
******************************************/
static int
negate(BYTE * A, int L)
{
	int i;
	int tL;
	DIGIT accum;

	/*
	 * Take one's complement of A
	 */
	for (i = 0; i < L; i++) {
		A[i] = ~(A[i]);
	}

	/*
	 * Add one to get two's complement of A
	 */
	accum = 1;
	tL = L - 1;
	while (accum && (tL >= 0)) {
		accum += A[tL];
		A[tL--] = (BYTE) (accum & 0xff);
		accum = accum >> 8;
	}

	return accum;
}


/*
 * add()
 *
 * A = A + B
 *
 * LB must be <= LA
 *
 */
static BYTE
add(BYTE * A, int LA, BYTE * B, int LB)
{
	int i;
	int indexA;
	int indexB;
	DIGIT accum;

	indexA = LA - 1;	/* LSD of result */
	indexB = LB - 1;	/* LSD of B */

	accum = 0;
	for (i = 0; i < LB; i++) {
		accum += A[indexA];
		accum += B[indexB--];
		A[indexA--] = (BYTE) (accum & 0xff);
		accum = accum >> 8;
	}

	if (LA > LB) {
		while (accum && (indexA >= 0)) {
			accum += A[indexA];
			A[indexA--] = (BYTE) (accum & 0xff);
			accum = accum >> 8;
		}
	}

	return (BYTE) accum;
}


static void
prettyprintBstr(char *S, BYTE * A, int L)
{
	int i;
	int extra;
	int ctrb;
	int ctrl;

	if (L == 0) {
		printf("%s <empty>", S);
	} else {
		printf("%s\n\t", S);
	}
	extra = L % 24;
	if (extra) {
		ctrb = 0;
		for (i = 0; i < 24 - extra; i++) {
			printf("  ");
			if (++ctrb == 4) {
				printf(" ");
				ctrb = 0;
			}
		}

		for (i = 0; i < extra; i++) {
			printf("%02X", A[i]);
			if (++ctrb == 4) {
				printf(" ");
				ctrb = 0;
			}
		}
		printf("\n\t");
	}

	ctrb = ctrl = 0;
	for (i = extra; i < L; i++) {
		printf("%02X", A[i]);
		if (++ctrb == 4) {
			ctrl++;
			if (ctrl == 6) {
				printf("\n\t");
				ctrl = 0;
			} else {
				printf(" ");
			}
			ctrb = 0;
		}
	}
	printf("\n\n");
	fflush(stdout);
}


/**********************************************************************/
/*
 * Performs byte reverse for PC based implementation (little endian)
 */
/**********************************************************************/
static void
byteReverse(ULONG * buffer, int byteCount)
{
	ULONG value;
	int count;

	byteCount /= sizeof(ULONG);
	for (count = 0; count < byteCount; count++) {
		value = (buffer[count] << 16) | (buffer[count] >> 16);
		buffer[count] = ((value & 0xFF00FF00L) >> 8) | ((value & 0x00FF00FFL) << 8);
	}
}

static void
ahtopb(char *ascii_hex, BYTE * p_binary, int bin_len)
{
	BYTE nibble;
	int i;

	for (i = 0; i < bin_len; i++) {
		nibble = ascii_hex[i * 2];
		if (nibble > 'F') {
			nibble -= 0x20;
		}
		if (nibble > '9') {
			nibble -= 7;
		}
		nibble -= '0';
		p_binary[i] = nibble << 4;

		nibble = ascii_hex[i * 2 + 1];
		if (nibble > 'F') {
			nibble -= 0x20;
		}
		if (nibble > '9') {
			nibble -= 7;
		}
		nibble -= '0';
		p_binary[i] += nibble;
	}
}

// debug.c follows

#ifndef DEBUG_LINT

/*
 * msg - print a generic message
 *
 * given:
 *      fmt     printf format
 *      ...
 *
 * Example:
 *
 *      msg("foobar information");
 *      msg("foo = %d\n", foo);
 */
static void
msg(char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */

	/*
	 * Start the var arg setup and fetch our first arg
	 */
	va_start(ap, fmt);

	/*
	 * Check preconditions (firewall)
	 */
	if (fmt == NULL) {
		warn(__func__, "NULL fmt given to debug");
		fmt = "((NULL fmt))";
	}

	/*
	 * Print the message
	 */
	ret = vfprintf(stderr, fmt, ap);
	if (ret <= 0) {
		fprintf(stderr, "[%s vfprintf returned error: %d]", __func__, ret);
	}
	fputc('\n', stderr);

	/*
	 * Clean up stdarg stuff
	 */
	va_end(ap);
	return;
}


/*
 * dbg - print debug message if we are verbose enough
 *
 * given:
 *      level   print message if >= verbosity level
 *      fmt     printf format
 *      ...
 *
 * Example:
 *
 *      dbg(DBG_MED, "foobar information");
 *      dbg(DBG_LOW, "curds: %s != whey: %d", (char *)curds, whey);
 */
static void
dbg(int level, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */

	/*
	 * Start the var arg setup and fetch our first arg
	 */
	va_start(ap, fmt);

	/*
	 * Check preconditions (firewall)
	 */
	if (fmt == NULL) {
		warn(__func__, "NULL fmt given to debug");
		fmt = "((NULL fmt))";
	}

	/*
	 * Print the debug message (if verbosity level is enough)
	 */
	if (level <= debuglevel) {
		ret = vfprintf(stderr, fmt, ap);
		if (ret <= 0) {
			fprintf(stderr, "[%s vfprintf returned error: %d]", __func__, ret);
		}
		fputc('\n', stderr);
	}

	/*
	 * Clean up stdarg stuff
	 */
	va_end(ap);
	return;
}


/*
 * warn - issue a warning message
 *
 * given:
 *      name    name of function issuing the warning
 *      fmt     format of the warning
 *      ...     optional format args
 *
 * Example:
 *
 *      warn(__func__, "unexpected foobar: %d", value);
 */
static void
warn(char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */

	/*
	 * Start the var arg setup and fetch our first arg
	 */
	va_start(ap, fmt);

	/*
	 * Check preconditions (firewall)
	 */
	if (name == NULL) {
		fprintf(stderr, "Warning: %s called with NULL name\n", __func__);
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		fprintf(stderr, "Warning: %s called with NULL fmt\n", __func__);
		fmt = "((NULL fmt))";
	}

	/*
	 * Issue the warning
	 */
	fprintf(stderr, "Warning: %s: ", name);
	ret = vfprintf(stderr, fmt, ap);
	if (ret <= 0) {
		fprintf(stderr, "[%s vfprintf returned error: %d]", __func__, ret);
	}
	fputc('\n', stderr);

	/*
	 * Clean up stdarg stuff
	 */
	va_end(ap);
	return;
}


/*
 * warn - issue a warning message
 *
 * given:
 *      name    name of function issuing the warning
 *      fmt     format of the warning
 *      ...     optional format args
 *
 * Unlike warn() this function also prints an errno message.
 *
 * Example:
 *
 *      warnp(__func__, "unexpected foobar: %d", value);
 */
static void
warnp(char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */
	int saved_errno;	/* errno at function start */

	/*
	 * Start the var arg setup and fetch our first arg
	 */
	saved_errno = errno;
	va_start(ap, fmt);

	/*
	 * Check preconditions (firewall)
	 */
	if (name == NULL) {
		fprintf(stderr, "Warning: %s called with NULL name\n", __func__);
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		fprintf(stderr, "Warning: %s called with NULL fmt\n", __func__);
		fmt = "((NULL fmt))";
	}

	/*
	 * Issue the warning
	 */
	fprintf(stderr, "Warning: %s: ", name);
	ret = vfprintf(stderr, fmt, ap);
	if (ret <= 0) {
		fprintf(stderr, "[%s vfprintf returned error: %d]", __func__, ret);
	}
	fputc('\n', stderr);
	fprintf(stderr, "errno[%d]: %s\n", saved_errno, strerror(saved_errno));

	/*
	 * Clean up stdarg stuff
	 */
	va_end(ap);
	return;
}


/*
 * err - issue a fatal error message and exit
 *
 * given:
 *      exitcode        value to exit with
 *      name            name of function issuing the warning
 *      fmt             format of the warning
 *      ...             optional format args
 *
 * This function does not return.
 *
 * Example:
 *
 *      err(99, __func__, "bad foobar: %s", message);
 */
static void
err(int exitcode, char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */

	/*
	 * Start the var arg setup and fetch our first arg
	 */
	va_start(ap, fmt);

	/*
	 * Check preconditions (firewall)
	 */
	if (exitcode >= 256) {
		warn(__func__, "called with exitcode >= 256: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__func__, "forcing exit code: %d", exitcode);
	}
	if (exitcode < 0) {
		warn(__func__, "called with exitcode < 0: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__func__, "forcing exit code: %d", exitcode);
	}
	if (name == NULL) {
		warn(__func__, "called with NULL name");
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		warn(__func__, "called with NULL fmt");
		fmt = "((NULL fmt))";
	}

	/*
	 * Issue the fatal error
	 */
	fprintf(stderr, "FATAL: %s: ", name);
	ret = vfprintf(stderr, fmt, ap);
	if (ret <= 0) {
		fprintf(stderr, "[%s vfprintf returned error: %d]", __func__, ret);
	}
	fputc('\n', stderr);

	/*
	 * Clean up stdarg stuff
	 */
	va_end(ap);

	/*
	 * Terminate
	 */
	exit(exitcode);
}


/*
 * errp - issue a fatal error message, errno string and exit
 *
 * given:
 *      exitcode        value to exit with
 *      name            name of function issuing the warning
 *      fmt             format of the warning
 *      ...             optional format args
 *
 * This function does not return.  Unlike err() this function
 * also prints an errno message.
 *
 * Example:
 *
 *      errp(99, __func__, "I/O failure: %s", message);
 */
static void
errp(int exitcode, char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */
	int saved_errno;	/* errno at function start */

	/*
	 * Start the var arg setup and fetch our first arg
	 */
	saved_errno = errno;
	va_start(ap, fmt);

	/*
	 * Check preconditions (firewall)
	 */
	if (exitcode >= 256) {
		warn(__func__, "called with exitcode >= 256: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__func__, "forcing exit code: %d", exitcode);
	}
	if (exitcode < 0) {
		warn(__func__, "called with exitcode < 0: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__func__, "forcing exit code: %d", exitcode);
	}
	if (name == NULL) {
		warn(__func__, "called with NULL name");
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		warn(__func__, "called with NULL fmt");
		fmt = "((NULL fmt))";
	}

	/*
	 * Issue the fatal error
	 */
	fprintf(stderr, "FATAL: %s: ", name);
	ret = vfprintf(stderr, fmt, ap);
	if (ret <= 0) {
		fprintf(stderr, "[%s vfprintf returned error: %d]", __func__, ret);
	}
	fputc('\n', stderr);
	fprintf(stderr, "errno[%d]: %s\n", saved_errno, strerror(saved_errno));

	/*
	 * Clean up stdarg stuff
	 */
	va_end(ap);

	/*
	 * Terminate
	 */
	exit(exitcode);
}


/*
 * usage_err - issue a fatal error message and exit
 *
 * given:
 *      exitcode        value to exit with (must be 0 <= exitcode < 256)
 *                      exitcode == 0 ==> just how to use -h for usage help and exit(0)
 *      name            name of function issuing the warning
 *      fmt             format of the warning
 *      ...             optional format args
 *
 * This function does not return.
 *
 * Example:
 *
 *      usage_err(99, __func__, "bad foobar: %s", message);
 */
static void
usage_err(int exitcode, char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */

	/*
	 * Start the var arg setup and fetch our first arg
	 */
	va_start(ap, fmt);

	/*
	 * Check preconditions (firewall)
	 */
	if (exitcode < 0 || exitcode >= 256) {
		warn(__func__, "exitcode must be >= 0 && < 256: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__func__, "forcing exit code: %d", exitcode);
	}
	if (name == NULL) {
		warn(__func__, "called with NULL name");
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		warn(__func__, "called with NULL fmt");
		fmt = "((NULL fmt))";
	}

	/*
	 * Issue the fatal error
	 */
	if (exitcode > 0) {
		fprintf(stderr, "FATAL: %s: ", name);
		ret = vfprintf(stderr, fmt, ap);
		if (ret <= 0) {
			fprintf(stderr, "%s: vfprintf returned error: %d", __func__, ret);
		}
		fputc('\n', stderr);
	}

	/*
	 * Issue the usage message help
	 */
	if (program == NULL) {
		fprintf(stderr, "For command line usage help, try: sts -h\n");
	} else {
		fprintf(stderr, "For command line usage help, try: %s -h\n", program);
	}
	fprintf(stderr, "Version: %s\n", version);

	/*
	 * Clean up stdarg stuff
	 */
	va_end(ap);

	/*
	 * Terminate
	 */
	exit(exitcode);
}


/*
 * usage_errp - issue a fatal error message, errno string and exit
 *
 * given:
 *      exitcode        value to exit with (must be 0 <= exitcode < 256)
 *                      exitcode == 0 ==> just how to use -h for usage help and exit(0)
 *      name            name of function issuing the warning
 *      fmt             format of the warning
 *      ...             optional format args
 *
 * This function does not return.  Unlike err() this function
 * also prints an errno message.
 *
 * Example:
 *
 *      usage_errp(99, __func__, "bad foobar: %s", message);
 */
static void
usage_errp(int exitcode, char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int saved_errno;	/* errno at function start */
	int ret;		/* return code holder */

	/*
	 * Start the var arg setup and fetch our first arg
	 */
	saved_errno = errno;
	va_start(ap, fmt);

	/*
	 * Check preconditions (firewall)
	 */
	if (exitcode < 0 || exitcode >= 256) {
		warn(__func__, "exitcode must be >= 0 && < 256: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__func__, "forcing exit code: %d", exitcode);
	}
	if (name == NULL) {
		warn(__func__, "called with NULL name");
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		warn(__func__, "called with NULL fmt");
		fmt = "((NULL fmt))";
	}

	/*
	 * Issue the fatal error
	 */
	if (exitcode > 0) {
		fprintf(stderr, "FATAL: %s: ", name);
		ret = vfprintf(stderr, fmt, ap);
		if (ret <= 0) {
			fprintf(stderr, "%s: vfprintf returned error: %d", __func__, ret);
		}
		fputc('\n', stderr);
		fprintf(stderr, "errno[%d]: %s\n", saved_errno, strerror(saved_errno));
	}

	/*
	 * Issue the usage message
	 */
	if (program == NULL) {
		fprintf(stderr, "For command line usage help, try: sts -h\n");
	} else {
		fprintf(stderr, "For command line usage help, try: %s -h\n", program);
	}
	fprintf(stderr, "Version: %s\n", version);

	/*
	 * Clean up stdarg stuff
	 */
	va_end(ap);

	/*
	 * Terminate
	 */
	exit(exitcode);
}

#endif				// DEBUG_LINT

// XXX - write this code

int
main(int argc, char *argv[])
{
	int option;		// getopt() parsed option
	extern char *optarg;	// parsed option argument
	extern int optind;	// index to the next argv element to parse
	extern int opterr;	// 0 ==> disable internal getopt() error messages
	extern int optopt;	// last known option character returned by getopt
	int i;

	/*
	 * parse args
	 */
        program = argv[0];
	while ((option = getopt(argc, argv, "vg:h")) != -1) {
		switch (option) {

		case 'v':	// -v debuglevel
			debuglevel = str2longint(&success, optarg);
                        if (success == false) {
                                usage_errp(usage, 1, __FUNCTION__, "error in parsing -v debuglevel: %s", optarg);
                        } else if (debuglevel < 0) {
                                usage_err(usage, 1, __FUNCTION__, "error debuglevel: %lu must >= 0", debuglevel);
                        }
			break;

		case 'g':       // -g generator
			i = str2longint(&success, optarg);
			if (success == false) {
                               usage_errp(usage, 1, __FUNCTION__, "-g generator must be numeric: %s", optarg);
                        }
                        if (i < 1 || i > NUMOFGENERATORS) {
                                usage_err(usage, 1, __FUNCTION__, "-g generator: %ld must be [1-%d]", i, NUMOFGENERATORS);
                        }
                        generator = (enum gen) i;
                        break;

		case 'h':	// -h (print out help)
			usage_err(usage, 0, __FUNCTION__, "this arg ignored");
			break;

		case '?':
		default:
			if (option == '?') {
                                usage_errp(usage, 1, __FUNCTION__, "getopt returned an error");
                        } else {
                                usage_err(usage, 1, __FUNCTION__, "unknown option: -%c", (char) optopt);
                        }
                        break;
                }
		if (optind >= argc) {
			usage_err(usage, 1, __FUNCTION__, "missing required bitcount argumnt");
		}
		if (optind != argc - 1) {
			usage_err(usage, 1, __FUNCTION__, "unexpected arguments");
		}
	}

	/*
	 * All Done!!! -- Jessica Noll, Age 2
	 */
	exit(0);
}
