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
 * Our code review identified numerous issues in the original code to which we attempted to fix.
 * A fair number of these issues resulted in incorrect statistical analysis, core dump / process crashing, or permitted an
 * external user to compromise the integrity of the process.  We identified a number of cases where the code assumed a 32-bit
 * machine such that on 64 bit or more systems, the code produced bogus results.  We also identified limitations where testing
 * data sets that approached or exceeded 2^31 bits would completely fail.
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
 * number of iterations were 2^10, the -O (mimic output format of legacy code) option was used and differences due to
 * bug fixes were taken into account.
 *
 * We transformed this code in a number of other ways, some that were not strictly related to the above mentioned bug fixes.
 * Our transformation goals were several:
 *
 *      1) Allow to test to be executed without human intervention
 *
 *         We wanted to run tests on batch mode.  The original code prompted input from the user in a way that
 *         was difficult to automate.
 *
 *      2) Improve support for 64-bit processors
 *
 *         We made a wide number of changes to support more modern 64-bit CPUs.
 *
 *      3) Improve source code documentation
 *
 *         When we were in doubt, we commented.  Commenting the code helped increase our understanding of what
 *         the original code was attempting to do.  In a number of cases this allowed us to identify problems
 *         with the code.
 *
 *      4) Improve the consistency of the coding style
 *
 *         The original code suffered at times from an inconsistent coding style.  We improved this by using something
 *         very close to the original BSD-style coding style. We also used the indent tool with the configuration
 *         found in the .indent.pro file found in this directory.  We had to choose to extend the line
 *         length to 132 characters, beyond the usual 80, due to the way the code was originally written.
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
 *      6) Improve code performance
 *
 *         In several places the code was more inefficient than it needed to be.  We improved the performance
 *         of certain code sections where doing so maintained correct testing and did not overly obscure the code.
 *         The result was a speed up of about 50% of the execution time.
 *
 *      7) Pass data in memory instead of via files
 *
 *         The original code, perhaps due to limitations of the platforms on which it may have originally run,
 *         "passed data" between certain stages of the test via files.  Data was written to files in some places,
 *         those files were closed and later on those files were re-opened and read / parsed again.  We modified
 *         the code to allocate memory and, where needed, pass pointers to that data via the state structure
 *         argument mentioned in (5) above. We did not attempt to preserve the ability for systems with a tiny
 *         amount of memory to perform well. If you attempt to test a huge amount of data (huge in comparison to
 *         the amount of RAM on your system), expect this code to page/swap or gracefully abort due to malloc failures.
 *
 *      8) Check for errors on return from system functions
 *
 *         The original code was often naive when it came to handling error generated by system calls.
 *         In many places error conditions were not checked.  In other cases errors would be detected
 *         but not reported in such a way that the user could tell what went wrong.  Some memory allocations
 *         were not checked resulting in negative consequences later.  On other cases multiple error returns
 *         produced the same error message.  Long executions would quit without much or any indication of what
 *         went wrong.  We attempted to check all calls to system functions and report some sort of error when
 *         problems were detected.  In cases where the error was a result of a lack of resources (say an virtual
 *         memory allocation error), or an inability to access a critical resource (say creating a critical file),
 *         a fatal error message is generated and execution is terminated.
 *
 *      9) Add debugging, notice, warning and fatal error messages
 *
 *         We developed a consistent method to produce debug messages, notice messages, issue warnings and
 *         fatal error message handling.  Not only does this permit one to set breakpoints to trap, say,
 *         errors and warnings, but it also allows us to print, where applicable, errno related error messages.
 *
 *      10) Introduce a "driver-like" function interface to test functions
 *
 *          We introduced a "driver-like" function interface to the 15 original statistical tests.  Instead of
 *          calling one monolithic function for each test, we broke up the tests into functions that initialize
 *          the test, process a single iteration of data, log test statistics to files, report on the test
 *          result, and destroy the initialization of the test.  One might say this was applying a more
 *          consistent object-method-like interface to these tests.
 *
 *      11) Improve the memory allocation patterns of each test
 *
 *          We attempted to load allocation of required data into the initialization function of each test.
 *          In many cases the original code would allocate and free memory on each iteration.  We created
 *          a dynamic array mechanism where the initialization function of a test would allocate the
 *          storage needed for the entire test.  Other functions would append elements to the dynamic
 *          arrays allowing them to grow as needed as iterations proceed.  In many cases the original code
 *          would calloc() memory during each iteration.  We changed the code so that the allocation is done
 *          during initialization code. The iteration code will then initialize the allocated memory that it
 *          uses.  This reduces the working set size of the tests.
 *
 *      12) Fix use of uninitialized values
 *
 *          The original code had a number of places where it assumed variables (sometimes global in scope)
 *          contained zero values.  Variable were used without first assigning them a value in some cases.
 *          When initialized data contained non-zero values, unexpected test results were produced,
 *          We made careful use of facilities such as valgrind to catch and correct such bugs as we could find.
 *
 *      13) Fix the use of test constants
 *
 *          The original code contained a number test parameter constants that were computed with a naive
 *          level of precision.  We attempted to derive such constants using tools such as Mathematica,
 *          and Calc, and update the test constants accordingly.
 *
 *      14) Handle improper test conditions better
 *
 *          Certain tests require minimum conditions in order for their results to be valid.  For example, some
 *          tests cannot be performed if the iteration size is too small or if a test parameter is out of range.
 *          We added code to detect such problems and to disable the given test (with a suitable warning message
 *          explaining why) when it cannot be performed. In other cases the data for a given iteration does not
 *          permit the test or be performed.  In some of those cases the iteration can neither be said to pass
 *          nor fail.  The original code would sometimes assign a p_value of 0 to such an iteration.  We established
 *          a NON_P_VALUE for such cases that differentiates between a true 0 p_value and the inability of a given
 *          test iteration to generate a meaningful p_value.  In cases where p_values were written to a file, a
 *          NON_P_VALUE was reported with the string __INVALID__.
 *
 *      15) Minimize the use of "magic numbers"
 *
 *          The original code was full of cases were numeric values were used without explanation.  The code
 *          might use the value 9.0 without explaining that this was 10-1, converted to a double, that is
 *          needed to compute a p_value for 10 results.  This made it hard to modify test conditions where
 *          to might be desirable to compute a test for more than 10 results.  In some cases changing a test
 *          parameter to a non-default value could produce invalid test results.  Where possible, we replaced
 *          these magic numbers with #defined constants where the meaning was more clear or by computing the values
 *          on the fly rather than assuming a fixed value.
 *
 *      16) Minimize the use of fixed size arrays
 *
 *          The original code was full of arrays with a fixed size.  The size sometimes appeared to be set
 *          large enough in the hopes that the code would not write off the end of the fixed array.  When
 *          it did, often problems would follow.  Where possible, we attempted to eliminate the use of
 *          such dangerous fixed sized arrays.  In other cases the array was created with a #defined
 *          size and preconditions were added that attempt to detect (and abort) if the bounds of
 *          the fixed array are violated.
 *
 *      17) Change confusing variable names
 *
 *          In a number of cases the original code used variable names that were misleading (where the
 *          purpose of the variable was not what the variable name implied), or where confusing (such
 *          as using pi for the value of Pi (3.1415926...) or for p[i]), or where the variable did not
 *          match well the mathematical values found in the SP800-22Rev1a document.
 *
 *      18) Improve makefile
 *
 *          The makefile supplied with the original code was incomplete and inconsistent.  Moreover it
 *          used non-portable make extensions.  We re-wrote the sts makefile to use both best practices and
 *          to be more portable.
 *
 *      19) Improve command line usage
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
 * 	22) Improve the program output
 *
 * 	    The output of the original test code in files such as finalAnalysisReport.txt was not
 *          very clear nor were some of the lines printed in various stats.txt files.  When the -O
 *          flag is NOT given we replaced that with an easier way to understand test results.
 *
 *      23) Speed up results through parallel processing
 *
 *          The original code ran on a single thread and used only a single core of the machine where executed.
 *          We wanted to take advantage of multi-core parallel processing to decrease execution time. We did
 *          so by running separate iterations on multiple cores in parallel through multi-threading.
 *
 *      24) Allow testing of huge datasets in a distributed fashion
 *
 *          In addition to point 23, we added the possibility to test data in multiple computers (distributed mode).
 *          This could be useful when testing huge datasets, where using one computer only might take too much time
 *          even when executed in parallel (with multithreading). The distributed mode allows the user to run sts to
 *          test a specific part of the input data and save the resulting p-values of the testing to an output file.
 *          Thus the user can test different chunks of the input data with different machines and later collect all the
 *          p-values and assess them with one final run of sts in ASSESS_ONLY mode.
 *
 *      24) Other issues not listed
 *
 *          The above list of goals is incomplete.  In the interest of not extending this long comment
 *          much further, we will just mention that other important goals were attempted to be reached.
 *
 * We are not finished in our code modification and test improvement.  We are in the process of making even more
 * important changes to this code.  In particular we are planning to:
 *
 *      A) Visualize test results in a graphical way
 *
 *          We plan to use gnu-plot to visualize the results of a run in a graphical way.
 *          This can be considered an follow-up on the change "(22) Improve the program output".
 *
 *      B) Update the SP800-22Rev1a document
 *
 *          While much of the text in the SP800-22Rev1a document still applies, some text needs
 *          to be revised.  For example, language about the new command line needs to be added.
 *          Test constant precision that were improved in the code need to be reflected in that
 *          document.  And in a few cases, bugs, typos or misleading SP800-22Rev1a text needs
 *          to be fixed.
 *
 *      C) Add entropy analysis
 *
 *          The approximate entropy test is of limited value.  Improved entropy tests need to
 *          be added to the existing test suite such as some of those outlined in FIPS 800-90B.
 *
 *********************************************************************************************************************************
 *
 * Our improvements and bug fixes are an expression of gratitude for the original authors efforts.  The bugs we
 * introduced along the way come with our additional apology as well as this disclaimer:
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


// Exit codes: 5 to 9
// NOTE: This code also does an exit(0) on normal completion
// NOTE: 0-4 is used in parse_args.c

#include <stdlib.h>
#include "constants/defs.h"
#include "utils/utilities.h"
#include "constants/externs.h"
#include "utils/debug.h"


// STS version
const char *const version = "3.1.0";

// Program name
char *program = "sts";

// Do not debug by default
long int debuglevel = DBG_NONE;


int
main(int argc, char *argv[])
{
	struct state run_state;		// Options set and dynamic arrays for this run

	/*
	 * Set default test parameters and parse command line
	 */
	parse_args(&run_state, argc, argv);

	/*
	 * Initialize all active tests
	 */
	init(&run_state);

	/*
	 * Report state if debugging
	 */
	if (debuglevel > DBG_HIGH) {
		print_option_summary(&run_state, "ready to test state");
	}

	/*
	 * Run test suite iterations if needed
	 */
	if (run_state.runMode != MODE_ASSESS_ONLY) {
		invokeTestSuite(&run_state);

		/*
		 * Print p-values and stats of each test in separate files (if needed)
		 */
		if (run_state.resultstxtFlag == true) {
			print(&run_state);
		}
	}

	/*
	 * If only iterations were to be done, save the p-values to file
	 */
	if (run_state.runMode == MODE_ITERATE_ONLY) {
		write_p_val_to_file(&run_state);
	}

	/*
	 * If there were no iterations to do, but only assess, read the data from given files
	 */
	else if (run_state.runMode == MODE_ASSESS_ONLY) {
		read_from_p_val_file(&run_state);
	}

	/*
	 * Perform metrics processing for each test and write final result to file
	 */
	if (run_state.runMode != MODE_ITERATE_ONLY) {
		metrics(&run_state);
	}

	/*
	 * Free memory no longer needed
	 */
	destroy(&run_state);

	/*
	 * Tell user that the execution is completed
	 */
	msg("Execution completed!");
	if (run_state.runMode == MODE_ITERATE_AND_ASSESS || run_state.runMode == MODE_ASSESS_ONLY) {
		if (run_state.legacy_output == true) {
			msg("Check the finalAnalysisReport.txt file for the results");
		} else {
			msg("Check the result.txt file for the results");
		}
	}

	else if (run_state.runMode == MODE_ITERATE_ONLY) {
		msg("A binary file (with extension .pvalues) containing the p-values of the tests has been generated.\n"
				    "You can later assess the results of this and other runs by executing "
				    "sts in '-m a' mode and passing that file(s) as an argument with the '-d' flag.");
	}

	// All Done!!! -- Jessica Noll, Age 2
	exit(0);
}
