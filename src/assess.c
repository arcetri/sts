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
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "defs.h"
#include "cephes.h"
#include "utilities.h"
#include "stat_fncs.h"
#include "externs.h"
#include "debug.h"

// sts_version-edit_number edit_year-edit_Month-edit_day
const char *const version = "sts-2.1.2-2.cisco";

// our name
char *program = "assess";

// do not debug by default
long int debuglevel = DBG_NONE;


int
main(int argc, char *argv[])
{
	struct state run_state;	/* options set and dynamic arrays for this run */
	int io_ret;		// I/O return status

	/*
	 * set default test parameters and parse command line
	 */
	Parse_args(&run_state, argc, argv);

	/*
	 * initialize all active tests
	 */
	init(&run_state);

	/*
	 * final state report if debugging
	 */
	if (debuglevel > DBG_NONE) {
		print_option_summary(&run_state, "ready to test state");
	}

	/*
	 * run test suite interations
	 */
	invokeTestSuite(&run_state);

	/*
	 * close down the frequency file
	 */
	if (run_state.freqFile != NULL) {
		errno = 0;	// paranoia
		io_ret = fflush(run_state.freqFile);
		if (io_ret != 0) {
			errp(10, __FUNCTION__, "error flushing freqFile");
		}
		errno = 0;	// paranoia
		io_ret = fclose(run_state.freqFile);
		if (io_ret != 0) {
			errp(10, __FUNCTION__, "error closing freqFile");
		}
	}

	/*
	 * print results if needed
	 *
	 * or old code: partition results
	 */
	print(&run_state);

	/*
	 * final result processing
	 */
	io_ret = fprintf(run_state.finalRept, "------------------------------------------------------------------------------\n");
	if (io_ret <= 0) {
		errp(10, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, "RESULTS FOR THE UNIFORMITY OF P-VALUES AND THE PROPORTION OF PASSING SEQUENCES\n");
	if (io_ret <= 0) {
		errp(10, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, "------------------------------------------------------------------------------\n");
	if (io_ret <= 0) {
		errp(10, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, "   generator is: %s\n", run_state.generatorDir[run_state.generator]);
	if (io_ret <= 0) {
		errp(10, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, "------------------------------------------------------------------------------\n");
	if (io_ret <= 0) {
		errp(10, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, " C1  C2  C3  C4  C5  C6  C7  C8  C9 C10  P-VALUE  PROPORTION  STATISTICAL TEST\n");
	if (io_ret <= 0) {
		errp(10, __FUNCTION__, "error in writing to finalRept");
	}
	io_ret = fprintf(run_state.finalRept, "------------------------------------------------------------------------------\n");
	if (io_ret <= 0) {
		errp(10, __FUNCTION__, "error in writing to finalRept");
	}
	metrics(&run_state);
	errno = 0;		// paranoia
	io_ret = fflush(run_state.finalRept);
	if (io_ret != 0) {
		errp(10, __FUNCTION__, "error flushing finalRept");
	}
	errno = 0;		// paranoia
	io_ret = fclose(run_state.finalRept);
	if (io_ret != 0) {
		errp(10, __FUNCTION__, "error closing finalRept");
	}

	/*
	 * finish / destroy storage no longer needed
	 */
	destroy(&run_state);

	// All Done!!! -- Jessica Noll, Age 2
	exit(0);
}
