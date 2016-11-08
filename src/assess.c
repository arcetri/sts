/*
 * Title : The NIST Statistical Test Suite
 *
 * Date : December 1999
 *
 * Programmer : Juan Soto
 *
 * Summary : For use in the evaluation of the randomness of bitstreams produced by cryptographic random number generators.
 *
 * Package : Version 1.0
 *
 * Copyright : (c) 1999 by the National Institute Of Standards & Technology
 *
 * History : Version 1.0 by J. Soto, October 1999 Revised by J. Soto, November 1999 Revised by Larry Bassham, March 2008
 *
 * Keywords : Pseudorandom Number Generator (PRNG), Randomness, Statistical Tests, Complementary Error functions, Incomplete Gamma
 * Function, Random Walks, Rank, Fast Fourier Transform, Template, Cryptographically Secure PRNG (CSPRNG), Approximate Entropy
 * (ApEn), Secure Hash Algorithm (SHA-1), Blum-Blum-Shub (BBS) CSPRNG, Micali-Schnorr (MS) CSPRNG,
 *
 * Source : David Banks, Elaine Barker, James Dray, Allen Heckert, Stefan Leigh, Mark Levenson, James Nechvatal, Andrew Rukhin,
 * Miles Smid, Juan Soto, Mark Vangel, and San Vo.
 *
 * Technical Assistance : Larry Bassham, Ron Boisvert, James Filliben, Daniel Lozier, and Bert Rust.
 *
 * Warning : Portability Issues.
 *
 * Limitation : Amount of memory allocated for workspace.
 *
 * Restrictions: Permission to use, copy, and modify this software without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy or modification of this software and in all copies of
 * the supporting documentation for such software.
 *
 *********************************************************************************************************************************
 *
 * IMPORTANT NOTE:
 *
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
 *
 * Our code review identified more than 67 issues (as of 2015 July) in the original code to which we attempted to fix.
 * A fair number of these issues results in incorrect statistical analysis, core dump / process crashing, or permitted an
 * external user to compromise the integrity of the process (in some cases to the point where a system cracker could
 * force the execution of arbitrary code.  We identified a number of cases where the code assumed a 32-bit machine
 * such that on 64 bit or more systems, the code produced bogus results.  We also identified limitations where testing
 * data sets that approached or exceeded 2^31 bits would completely fail.  And that is just a partial list of some of the
 * significant issues we identified!
 *
 * We attempted to resolve, fix and/or improve as many of these issues as we could.  With this new code, we
 * compared our new results to results of the original code running on a 32-bit machine.  Due to limitations of the original
 * code we could only reliably compare using data sets of at most 2^30 bits (the original code would crash and/or produce
 * bogus results if the test set was larger).
 *
 * When we fixed the implementation of the mathematics, improved / corrected the constants, and performed general bug fixing,
 * the output of the test changed.  In a number of cases we could not simply compare old and new output: especially when the
 * Output difference came as a direct result of a bug fix!  Nevertheless, for all 9 built-in generators the output was
 * equivalent to the old code when the data set was limited to just 2^30 bits, the iteration size was 2^20 bits, the
 * number of iterations were 2^10 and the -O (mimic output format of legacy code) option was used, and result impacting
 * bug fixes were taken into account.
 *
 * We transformed this code in a number of other ways, some that were not strictly related to the above mentioned bug fixes.
 * Our transformation goals were several:
 *
 *      1) Allow to test to be executed without human intervention
 *
 *         We wanted to run tests on batch mode.  The original code prompted input from the user in a way that
 *         was at best awkward to automate.
 *
 *      2) Support 64-bit processors, not 32-bit processors
 *
 *         We made a wide number of changes to support more modern 64-bit CPUs.  In doing so we did not make a
 *         significant effort to preserve support on 32-bit processors.
 *
 *      3) Improve code comments
 *
 *         When we were in doubt, we commented.  Commenting the code helped increase our understanding of what
 *         the original code was attempting to do.  In a number of cases this allowed us to identify problems
 *         with the code.  Not all of the code is commented to the level that we prefer.  No doubt some of our
 *         added comments are wrong or contain typos.  (see the "Send patches" instructions below).
 *
 *      4) Improve the consistency of the code style
 *
 *         The original code suffered at times from an inconsistent coding style.  It would have been better if
 *         the code picked just one style, but it didn't.  This mix of styles made reading and understand of the
 *         code more awkward that is needed to be.  We used the indent tool and the configuration found in the
 *         .indent.pro file found in this directory.  We arbitrarily choose something very close to the original
 *         BSD-style coding style as implemented by the indent tool.  We had to choose to extend the line length
 *         to 132 characters, beyond the usual 80, due to the way the code was originally written.
 *
 *      5) Eliminate as much as possible the use of global variables
 *
 *         The original code relied heavily on variables of global scope.  In some cases those global variables
 *         were used differently by different parts of the test resulting in unexpected side effects based on
 *         how the tests were run.  We changed the code so that the only global scope variables are things
 *         such as the version string constant, the level of debugging and the name of the program.  The
 *         latter 2 variables are set during command line argument parsing and are not changed after that.
 *         We introduced the struct state argument, initialized and/or setup by the command line parsing code
 *         and passed as pointer to nearly all functions throughout this code.
 *
 *      6) Slightly improve code performance
 *
 *         In several places the code is more inefficient than is needs to be.  While maximizing performance
 *         was NOT our goal, we improved the performance of certain code sections where doing so maintained
 *         correct testing and did not overly obscure the code.
 *
 *      7) Pass data in memory instead of via files
 *
 *         The original code, perhaps due to limitations of the platforms on which it may have originally run,
 *         "passed data" between certain stages of the test via files.  Data was written to files in some places,
 *         those files were closed and later on those files were re-opened and read / parsed again.  The parsing
 *         of these data files was too often not very robust.  We modified the code to allocate memory and where
 *         needed, pass pointers to that data via the state structure argument mentioned in (5) above.
 *         We did not attempt to preserve the ability for systems with a tiny amount of memory to perform well.
 *         If you attempt to test a huge amount of data (huge in comparison to the amount of RAM on your system),
 *         expect this code to page/swap or gracefully abort due to malloc failures.
 *
 *      8) Check for errors on return from system functions
 *
 *         The original code was too often naive when it came to handling error generated by system calls.
 *         In too many places error conditions were not checked.  In other cases errors would be detected
 *         but not reported in such a way that the user could tell what went wrong.  Some memory allocations
 *         were not checked resulting in disastrous consequences later on when the allocation error was
 *         ignored.  On other cases multiple error returns produced the same error message.  Long executions
 *         would quit without much or any indication of what went wrong.  We attempted to check all calls
 *         to system functions and report some sort of error when problems were detected.  In cases where
 *         the error was a result of a lack of resources (say an virtual memory allocation error), or an
 *         inability to access a critical resource (say creating a critical file), a fatal error message is
 *         generated and execution is terminated (rather than attempting to limp along in a crippled state).
 *
 *      9) Debugging, notice, warning and fatal error message processing hooks
 *
 *         We developed a consistent method to produce debug messages, notice messages, issue warnings and
 *         fatal error message handling.  Not only does this permit one to set breakpoints to trap, say,
 *         errors and warnings, it allow us to print where applicable, errno related error messages.
 *
 *      10) Introduce a "driver-like" function interface to test functions
 *
 *          We introduced a "driver-like" function interface to the 15 original statistical tests.  Instead of
 *          calling one monolithic function for each test, we broke up the tests into functions that initialize
 *          the test, process a single iteration of data, log test statistics to files, report on the test
 *          result, and destroy the initialization of the test.  One might say this was applying a more
 *          consistent object-method-like interface to these tests.
 *
 *      11) Improve the memory allocation patters of each test
 *
 *          We attempted to load allocation of required data into the initialization function of each test.
 *          In many cases the original code would allocate and free memory on each iteration.  We created
 *          a dynamic array mechanism where the initialization function of a test would allocate the
 *          storage needed for the entire test.  Other functions would append elements to the dynamic
 *          arrays allowing them to grow as needed as iterations proceed.  This minimized memory allocation
 *          thrashing that was present in the original code.  In many cases the original code would calloc()
 *          memory (allocated and then zeroize) during each iteration.  Sometimes the zeroized memory would
 *          later be initialized by values (sometimes re-initialized with a zero value).  We changed the code
 *          so that initialization code allocates.  The iteration code will initialize the allocated memory
 *          that it uses.  This reduces the working set size of the tests.
 *
 *      12) Fix use of uninitialized values
 *
 *          The original code had a number of places where it assumed variables (sometimes global in scope)
 *          contained zero values.  Variable were used without first assigning them a value in some cases.
 *          When initialized data contained non-zero values, unexpected test results were produced,
 *          We made careful use of facilities such as valgrind to catch and correct such bugs as we could find.
 *
 *      13) Fixed the use of test constants
 *
 *          The original code contained a number test parameter constants that were computed with a naive
 *          level of precision.  We attempted to derive such constants using tools such as Mathematica,
 *          and Calc, and update the test constants accordingly.
 *
 *      14) Handle improper test conditions better
 *
 *          Certain tests require minimum conditions in order for their results to be value.  For example, some
 *          tests cannot be performed if the iteration size is too small or if a test parameter is out of range.
 *          We added code to the iteration call of a test that attempts to detect such problems and to disable
 *          the given test (with a suitable warning message explaining why) when it cannot be performed.  In
 *          other cases the data for a given iteration does not permit the test or be performed.  In some of
 *          those cases the iteration can neither be said to pass nor fail.  The original code would sometimes
 *          assign a p_value of 0 to such an iteration.  We established a NON_P_VALUE for such cases that
 *          differentiates between a true 0 p_value and the inability of a given test iteration to generate
 *          a meaningful p_value.  In cases where p_values were written to a file, a NON_P_VALUE was reported
 *          as the string __INVALID__.
 *
 *      15) Minimize the use of magic numbers
 *
 *          The original code was full of cases were numeric values were used without explanation.  The code
 *          might use the value 9.0 without explaining that this was 10-1, converted to a double, that is
 *          needed to compute a p_value for 10 results.  This made it hard to modify test conditions where
 *          to might be desirable to compute a test for more than 10 results.  In some cases changing a test
 *          parameter to a non-default value could produce invalid test results.  Where possible, we replaced
 *          magic integers with #defined constants where the meaning was more clear or by computing the values
 *          on the fly rather than assuming a fixed value.
 *
 *      16) Minimize the use of fixed size arrays
 *
 *          The original code was full of arrays with a fixed size.  The size sometimes appeared to be set
 *          large enough in the hopes that the code would not write off the end of the fixed array.  When
 *          it did, often difficult would follow.  Where possible, we attempted to eliminate the use of
 *          such dangerous fixed sized arrays.  In other cases the array was created with a #defined
 *          size and firewall code was added that attempted to detect (and abort) should the bounds of
 *          the fixed array be violated.
 *
 *      17) Change confusing variable names
 *
 *          In a number of cases the original code used variable names that were misleading (where the
 *          purpose of the variable was not what the variable name implied), or where confusing (such
 *          as using pi for the value of Pi (3.1415926...) or for p[i]), or where the variable did not
 *          match well the mathematical values found in the SP800-22Rev1a document.
 *
 *      18) Better makefile
 *
 *          The makefile supplied with the original code was incomplete and inconsistent.  Moreover it
 *          used non-portable make extensions.  We re-wrote the sts makefile to use both best practices and
 *          to be more portable.
 *
 *      19) Command line usage
 *
 *          In adding various capabilities such as batch processing, we needed to add many more
 *          flags to the command line.  We added a -h flag to print out a usage message.  When
 *          an command line error is detected, a usage message is also produced.
 *
 *      20) Fix compiler warnings
 *
 *          Modern compilers report several warnings with respect to the original code.  Some of these
 *          warnings hint at more serious flaws in that code.  We fixed these warnings, as extended the
 *          code to using the c99 standard and to be pedantic error and warning free.
 *
 *      21) Fixed memory leaks
 *
 *          The original code had arrays that were allocated and lost (data no longer referenced)
 *          or that were not freed.  We used memory allocation checking tools and cleaned up
 *          cases where memory leaks were found.
 *
 *      22) other issues not listed
 *
 *          The above list of 16 goals is incomplete.  In the interest of not extending this long comment
 *          much further, we will just mention that other important goals were attempted to be reached.
 *
 * We are not finished in our code modification and test improvement.  We are in the process of making even more
 * important changes to this code.  In particular we want to:
 *
 *      A) Apply the above numbered transformations in a better and more consistent fashion.
 *
 *         The above numbered transformation goals are in some cases incompletely applied.  In other
 *         cases we need to do a better job of applying them.  On multiple cases we need to extend
 *         the original idea.
 *
 *      B) Speed up results through parallel processing
 *
 *         We want to take advantage of multi-core parallel processing to decrease execution time.
 *         We want to be able to run separate iterations on multiple hosts in parallel.  Much of the
 *         motivation behind changes such as "(5) Eliminate we much as possible, the use of global variables"
 *         and "(1) Allow to test to be executed without human intervention" was to make it possible for
 *         parallel processing that would allow for faster testing and/or testing of even larger
 *         amounts of test data.
 *
 *      C) Evaluate test conditions
 *
 *         We need to take another careful pass to determine when the parameters do not agree with
 *         the test conditions described in the SP800-22Rev1a test document.  We also need to take
 *         a pass to determine when an iteration cannot be used to produce a meaningful p_value.
 *         In some cases the sited papers listed in the SP800-22Rev1a references should be reviewed.
 *
 *      D) Improve test statistics
 *
 *         We started to to better accounting of when test and iterations pass, fail or produce
 *         invalid results.  In a number of cases this test statistics is not reported.  We need to
 *         report these stats.
 *
 *      E) Better test results
 *
 *         The output of the original test code in files such as finalAnalysisReport.txt is not
 *         very clear nor are some of the lines printed in various stats.txt files.  When the -O
 *         flag is NOT given we need to include an easier to understand test results.  We can
 *         also address the "(D) Improve test statistics" issue here as well.
 *
 *      F) Update the SP800-22Rev1a document
 *
 *         While much of the text in the SP800-22Rev1a document still applies, some text needs
 *         to be revised.  For example, language about the new command line needs to be added.
 *         Test constant precision that were improved in the code need to be reflected in that
 *         document.  And in a few cases, bugs, typos or misleading SP800-22Rev1a text needs
 *         to be fixed.
 *
 *      G) Add entropy analysis
 *
 *         The approximate entropy test is of limited value.  Improved entropy tests need to
 *         be added to the existing test suite such as some of those outlined in FIPS 800-90B.
 *
 *********************************************************************************************************************************
 *
 * We DO appreciate the effort of the original code developers.  Without their efforts this modified code would not have
 * been possible.  The NIST document, SP800-22Rev1a, was extremely valuable to us.  And the overall test methodology is sound.
 *
 * The original authors were probably constrained by the machines in which they ran tests.  They did not anticipate our
 * interest in testing large data sets or in testing on parallel processors.  Their original goals were not our goals.
 *
 * We do NOT intend to impugn the professional integrity of the original authors, nor criticize those who modified their
 * code afterwards.  We hope we do not offend them in any way.  If we do, we apologize as this was not in intention.
 *
 * The above partial list of issues as presented to help explain why we extensively modified their original code.
 * We believed we owed the original authors an explanation as to why we do heavily modified their code.
 *
 * Finally we DO THANK the original authors for making their code freely available.  We saw the value of their efforts
 * and set about the tasks of EXTENDING their code to situations the original authors did not intended.
 *
 * Our bug fixes are an expression of gratitude for the original authors efforts.  The bugs we introduced along
 * the way come with our additional apology as well as this disclaimer:
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
 *
 *********************************************************************************************************************************
 *
 * If you wish to be notified of changes of this code, please send Email to:
 *
 *          sts-question at external dot cisco dot com
 *
 * You MUST use the following phrase in the subject of your Email (or risk your Email being ignored):
 *
 *          please subscribe me to the sts-announce list
 *
 * Please include the body of that that EMail:
 *
 *          Your name
 *          Your interest in this sts code
 *
 *********************************************************************************************************************************
 *
 * While we do not promise we will answer all questions about this code, if you wish to ask the maintainer a
 * question, please send Email to:
 *
 *          sts-question at external dot cisco dot com
 *
 * You MUST use the following phrase in the subject of your Email (or risk your Email being ignored):
 *
 *          A question about sts
 *
 * Please include in the body of your Email the version of the code you are asking about.
 * See the version string that is located just a few lines below.
 *
 *********************************************************************************************************************************
 *
 * We have by no means believe we have fixed every last bug in this code.  Moreover we do not doubt that we have introduced
 * new bugs during our transformation.  For any such new bugs, we apologize AND WELCOME patches that fix either old bugs
 * or our newly introduced bugs.
 *
 * Send patches in the form of "diff -u" to:
 *
 *      sts-bug-fix at external dot cisco dot com
 *
 *      NOTE: PLEASE indicate the version against which you are patching!
 *            Include the version string that is located just a few lines below.
 *
 * You MUST use the following phrase in the subject of your Email (or risk your Email being ignored):
 *
 *      sts patch request
 */


// Exit codes: 5 to 9
// NOTE: This code also does an exit(0) on normal completion
// NOTE: 0-4 is used in Parse_args.c

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "defs.h"
#include "utilities.h"
#include "externs.h"
#include "debug.h"

// sts_version-edit_number
const char *const version = "sts-2.1.2.5.cisco";

// our name
char *program = "assess";

// do not debug by default
long int debuglevel = DBG_NONE;


int
main(int argc, char *argv[])
{
	struct state run_state;	/* options set and dynamic arrays for this run */
	int io_ret;		/* I/O return status */

	/*
	 * Set default test parameters and parse command line
	 */
	Parse_args(&run_state, argc, argv);

	/*
	 * Initialize all active tests
	 */
	init(&run_state);

	/*
	 * Report state if debugging
	 */
	if (debuglevel > DBG_NONE) {
		print_option_summary(&run_state, "ready to test state");
	}

	/*
	 * Run test suite iterations
	 */
	invokeTestSuite(&run_state);

	/*
	 * Close down the frequency file
	 */
	if (run_state.freqFile != NULL) {
		errno = 0;	// paranoia
		io_ret = fflush(run_state.freqFile);
		if (io_ret != 0) {
			errp(5, __FUNCTION__, "error flushing freqFile");
		}
		errno = 0;	// paranoia
		io_ret = fclose(run_state.freqFile);
		if (io_ret != 0) {
			errp(5, __FUNCTION__, "error closing freqFile");
		}
	}

	/*
	 * Print results if needed
	 *
	 * or old code: partition results
	 */
	print(&run_state);

	/*
	 * final result processing
	 */
	io_ret = fprintf(run_state.finalRept, "------------------------------------------------------------------------------\n");
	if (io_ret <= 0) {
		errp(5, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, "RESULTS FOR THE UNIFORMITY OF P-VALUES AND THE PROPORTION OF PASSING SEQUENCES\n");
	if (io_ret <= 0) {
		errp(5, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, "------------------------------------------------------------------------------\n");
	if (io_ret <= 0) {
		errp(5, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, "   generator is: %s\n", run_state.generatorDir[run_state.generator]);
	if (io_ret <= 0) {
		errp(5, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, "------------------------------------------------------------------------------\n");
	if (io_ret <= 0) {
		errp(5, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, " C1  C2  C3  C4  C5  C6  C7  C8  C9 C10  P-VALUE  PROPORTION  STATISTICAL TEST\n");
	if (io_ret <= 0) {
		errp(5, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, "------------------------------------------------------------------------------\n");
	if (io_ret <= 0) {
		errp(5, __FUNCTION__, "error in writing to finalRept");
	}

	metrics(&run_state);
	errno = 0;		// paranoia
	io_ret = fflush(run_state.finalRept);
	if (io_ret != 0) {
		errp(5, __FUNCTION__, "error flushing finalRept");
	}
	errno = 0;		// paranoia
	io_ret = fclose(run_state.finalRept);
	if (io_ret != 0) {
		errp(5, __FUNCTION__, "error closing finalRept");
	}

	/*
	 * finish / destroy storage no longer needed
	 */
	destroy(&run_state);

	// All Done!!! -- Jessica Noll, Age 2
	exit(0);
}
