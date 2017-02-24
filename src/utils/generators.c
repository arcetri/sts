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
#include "../utils/externs.h"
#include "utilities.h"
#include "generators.h"
#include "genutils.h"
#include "debug.h"


static void
generator_iterate(struct state *state)
{
	long int iteration_being_done = state->tp.numOfBitStreams - state->iterationsMissing;
	state->iterationsMissing -= 1;

	if (iteration_being_done == 0) {
		dbg(DBG_LOW, "Start of iterate phase");
	}

	/*
	 * Create a fake thread for the iteration.
	 * This is done because when the STS generators are used STS currently
	 * supports only 1 thread.
	 */
	struct thread_state fake_thread_state;
	fake_thread_state.global_state = state;
	fake_thread_state.thread_id = 0;
	fake_thread_state.iteration_being_done = iteration_being_done;
	fake_thread_state.mutex = NULL;
	iterate(&fake_thread_state);
}


static void
generator_report_iteration(struct state *state)
{
	char buf[BUFSIZ + 1];	// time string buffer
	long int iteration_being_done;

	/*
	 * Count the iteration and report process if requested
	 */
	iteration_being_done = state->tp.numOfBitStreams - state->iterationsMissing;

	if (state->reportCycle > 0 && (((iteration_being_done % state->reportCycle) == 0) ||
				       (iteration_being_done == state->tp.numOfBitStreams))) {
		getTimestamp(buf, BUFSIZ);
		msg("Completed iteration %ld of %ld at %s", iteration_being_done,
		    state->tp.numOfBitStreams, buf);
	}

	if (iteration_being_done == state->tp.numOfBitStreams) {
		dbg(DBG_LOW, "End of iterate phase\n");
	}
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


void
lcg(struct state *state)
{
	int io_ret;		// I/O return status
	double *DUNIF;
	double SEED;
	long int i;
	unsigned bit;
	long int num_0s;
	long int num_1s;
	long int v;
	long int bitsRead;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(80, __func__, "state arg is NULL");
	}

	SEED = 23482349.0;
	DUNIF = calloc((size_t) state->tp.n, sizeof(double));
	if (DUNIF == NULL) {
		errp(80, __func__, "could not calloc for DUNIF: %ld doubles of %lu bytes each", state->tp.n, sizeof(double));
	}

	for (v = 0; v < state->tp.numOfBitStreams; v++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		SEED = lcg_rand(state->tp.n, SEED, DUNIF, state->tp.n);
		for (i = 0; i < state->tp.n; i++) {
			if (DUNIF[i] < 0.5) {
				bit = 0;
				num_0s++;
			} else {
				bit = 1;
				num_1s++;
			}
			bitsRead++;
			state->epsilon[0][i] = (BitSequence) bit;
		}

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (state->legacy_output == true) {
			fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(state->freqFile);
			if (io_ret != 0) {
				errp(80, __func__, "error flushing to: %s", state->freqFilePath);
			}
		}

		if (state->runMode == MODE_WRITE_ONLY) {
			write_sequence(state);
		} else {
			generator_iterate(state);
		}
		generator_report_iteration(state);
	}

	free(DUNIF);

	return;
}


void
quadRes1(struct state *state)
{
	int io_ret;		// I/O return status
	long int k;
	long int num_0s;
	long int num_1s;
	long int bitsRead;
	bool done;		// true ==> enoungh data has been converted
	BYTE p[64];		// XXX - array size uses magic number
	BYTE g[64];		// XXX - array size uses magic number
	BYTE x[128];		// XXX - array size uses magic number

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(81, __func__, "state arg is NULL");
	}

	ahtopb
	    ("987b6a6bf2c56a97291c445409920032499f9ee7ad128301b5d0254aa1a9633f"
	     "dbd378d40149f1e23a13849f3d45992f5c4c6b7104099bc301f6005f9d8115e1", p, 64);
	ahtopb
	    ("3844506a9456c564b8b8538e0cc15aff46c95e69600f084f0657c2401b3c2447"
	     "34b62ea9bb95be4923b9b7e84eeaf1a224894ef0328d44bc3eb3e983644da3f5", g, 64);

	for (k = 0; k < state->tp.numOfBitStreams; k++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		do {
			memset(x, 0x00, 128);
			ModMult(x, g, 64, g, 64, p, 64);
			memcpy(g, x + 64, 64);
			done = copyBitsToEpsilon(state, 0, g, 512, &num_0s, &num_1s, &bitsRead);
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (state->legacy_output == true) {
			fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(state->freqFile);
			if (io_ret != 0) {
				errp(81, __func__, "error flushing to: %s", state->freqFilePath);
			}
		}

		if (state->runMode == MODE_WRITE_ONLY) {
			write_sequence(state);
		} else {
			generator_iterate(state);
		}
		generator_report_iteration(state);
	}

	return;
}


void
quadRes2(struct state *state)
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
	long int bitsRead;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(82, __func__, "state arg is NULL");
	}

	One[0] = 0x01;
	Two = 0x02;
	Three[0] = 0x03;

	ahtopb
	    ("7844506a9456c564b8b8538e0cc15aff46c95e69600f084f0657c2401b3c2447"
	     "34b62ea9bb95be4923b9b7e84eeaf1a224894ef0328d44bc3eb3e983644da3f5", g, 64);

	for (k = 0; k < state->tp.numOfBitStreams; k++) {
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
			done = copyBitsToEpsilon(state, 0, g, 512, &num_0s, &num_1s, &bitsRead);
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (state->legacy_output == true) {
			fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(state->freqFile);
			if (io_ret != 0) {
				errp(82, __func__, "error flushing to: %s", state->freqFilePath);
			}
		}

		if (state->runMode == MODE_WRITE_ONLY) {
			write_sequence(state);
		} else {
			generator_iterate(state);
		}
		generator_report_iteration(state);
	}

	return;
}


void
cubicRes(struct state *state)
{
	int io_ret;		// I/O return status
	BYTE g[64];		// XXX - array size uses magic number
	BYTE tmp[128];		// XXX - array size uses magic number
	BYTE x[192];		// XXX - array size uses magic number
	bool done;		// true ==> enoungh data has been converted
	long int k;
	long int num_0s;
	long int num_1s;
	long int bitsRead;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(83, __func__, "state arg is NULL");
	}


	ahtopb
	    ("7844506a9456c564b8b8538e0cc15aff46c95e69600f084f0657c2401b3c2447"
	     "34b62ea9bb95be4923b9b7e84eeaf1a224894ef0328d44bc3eb3e983644da3f5", g, 64);

	for (k = 0; k < state->tp.numOfBitStreams; k++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		do {
			memset(tmp, 0x00, 128);
			memset(x, 0x00, 192);
			Mult(tmp, g, 64, g, 64);
			Mult(x, tmp, 128, g, 64);	// Don't need to mod by 2^512, just take low 64 bytes
			memcpy(g, x + 128, 64);
			done = copyBitsToEpsilon(state, 0, x, 512, &num_0s, &num_1s, &bitsRead);
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (state->legacy_output == true) {
			fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(state->freqFile);
			if (io_ret != 0) {
				errp(83, __func__, "error flushing to: %s", state->freqFilePath);
			}
		}

		if (state->runMode == MODE_WRITE_ONLY) {
			write_sequence(state);
		} else {
			generator_iterate(state);
		}
		generator_report_iteration(state);
	}

	return;
}


void
exclusiveOR(struct state *state)
{
	int io_ret;		// I/O return status
	long int i;
	long int num_0s;
	long int num_1s;
	long int bitsRead;
	BYTE bit_sequence[127];	// XXX - array size uses magic number

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(84, __func__, "state arg is NULL");
	}

	memcpy(bit_sequence,
	       "0001011011011001000101111001001010011011101101000100000010101111"
	       "111010100100001010110110000000000100110000101110011111111100111", 127);
	num_0s = 0;
	num_1s = 0;
	bitsRead = 0;
	for (i = 0; i < 127; i++) {
		if (bit_sequence[i] == '1') {
			state->epsilon[0][bitsRead] = 1;
			num_1s++;
		} else {
			state->epsilon[bitsRead] = 0;
			num_1s++;
		}
		bitsRead++;
	}
	for (i = 127; i < state->tp.n * state->tp.numOfBitStreams; i++) {
		if (bit_sequence[(i - 1) % 127] != bit_sequence[(i - 127) % 127]) {
			bit_sequence[i % 127] = 1;
			state->epsilon[0][bitsRead] = 1;
			num_1s++;
		} else {
			bit_sequence[i % 127] = 0;
			state->epsilon[bitsRead] = 0;
			num_0s++;
		}
		bitsRead++;
		if (bitsRead == state->tp.n) {

			/*
			 * Write stats to freq.txt if in legacy_output mode
			 */
			if (state->legacy_output == true) {
				fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
				errno = 0;        // paranoia
				io_ret = fflush(state->freqFile);
				if (io_ret != 0) {
					errp(84, __func__, "error flushing to: %s", state->freqFilePath);
				}
			}

			if (state->runMode == MODE_WRITE_ONLY) {
				write_sequence(state);
			} else {
				generator_iterate(state);
			}
			generator_report_iteration(state);

			num_0s = 0;
			num_1s = 0;
			bitsRead = 0;
		}
	}

	return;
}


void
modExp(struct state *state)
{
	int io_ret;		// I/O return status
	long int k;
	long int num_0s;
	long int num_1s;
	long int bitsRead;
	bool done;		// true ==> enoungh data has been converted
	BYTE p[64];		// XXX - array size uses magic number
	BYTE g[64];		// XXX - array size uses magic number
	BYTE x[192];		// XXX - array size uses magic number
	BYTE y[20];		// XXX - array size uses magic number

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(85, __func__, "state arg is NULL");
	}

	ahtopb("7AB36982CE1ADF832019CDFEB2393CABDF0214EC", y, 20);
	ahtopb
	    ("987b6a6bf2c56a97291c445409920032499f9ee7ad128301b5d0254aa1a9633f"
	     "dbd378d40149f1e23a13849f3d45992f5c4c6b7104099bc301f6005f9d8115e1", p, 64);
	ahtopb
	    ("3844506a9456c564b8b8538e0cc15aff46c95e69600f084f0657c2401b3c2447"
	     "34b62ea9bb95be4923b9b7e84eeaf1a224894ef0328d44bc3eb3e983644da3f5", g, 64);

	for (k = 0; k < state->tp.numOfBitStreams; k++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		do {
			memset(x, 0x00, 128);
			ModExp(x, g, 64, y, 20, p, 64);	/* NOTE: g must be less than p */
			done = copyBitsToEpsilon(state, 0, x, 512, &num_0s, &num_1s, &bitsRead);
			memcpy(y, x + 44, 20);
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (state->legacy_output == true) {
			fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(state->freqFile);
			if (io_ret != 0) {
				errp(85, __func__, "error flushing to: %s", state->freqFilePath);
			}
		}

		if (state->runMode == MODE_WRITE_ONLY) {
			write_sequence(state);
		} else {
			generator_iterate(state);
		}
		generator_report_iteration(state);
	}

	return;
}

void
bbs(struct state *state)
{
	int io_ret;		// I/O return status
	long int i;
	long int v;
	long int bitsRead;
	BYTE p[64];		// XXX - array size uses magic number
	BYTE q[64];		// XXX - array size uses magic number
	BYTE n[128];		// XXX - array size uses magic number
	BYTE s[64];		// XXX - array size uses magic number
	BYTE x[256];		// XXX - array size uses magic number
	long int num_0s;
	long int num_1s;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(86, __func__, "state arg is NULL");
	}

	ahtopb
	    ("E65097BAEC92E70478CAF4ED0ED94E1C94B154466BFB9EC9BE37B2B0FF8526C2"
	     "22B76E0E915017535AE8B9207250257D0A0C87C0DACEF78E17D1EF9DC44FD91F", p, 64);
	ahtopb
	    ("E029AEFCF8EA2C29D99CB53DD5FA9BC1D0176F5DF8D9110FD16EE21F32E37BA8"
	     "6FF42F00531AD5B8A43073182CC2E15F5C86E8DA059E346777C9A985F7D8A867", q, 64);
	memset(n, 0x00, 128);
	Mult(n, p, 64, q, 64);
	memset(s, 0x00, 64);
	ahtopb
	    ("10d6333cfac8e30e808d2192f7c0439480da79db9bbca1667d73be9a677ed313"
	     "11f3b830937763837cb7b1b1dc75f14eea417f84d9625628750de99e7ef1e976", s, 64);
	memset(x, 0x00, 256);
	ModSqr(x, s, 64, n, 128);

	for (v = 0; v < state->tp.numOfBitStreams; v++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		for (i = 0; i < state->tp.n; i++) {
			ModSqr(x, x, 128, n, 128);
			memcpy(x, x + 128, 128);
			if ((x[127] & 0x01) == 0) {
				num_0s++;
				state->epsilon[0][i] = 0;
			} else {
				num_1s++;
				state->epsilon[0][i] = 1;
			}
			bitsRead++;
			if ((i % 50000) == 0) {
				if (state->batchmode == true) {
					dbg(DBG_MED, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld", bitsRead, num_0s, num_1s);
				} else {
					printf("\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
					fflush(stdout);
				}
			}
		}

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (state->legacy_output == true) {
			fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(state->freqFile);
			if (io_ret != 0) {
				errp(86, __func__, "error flushing to: %s", state->freqFilePath);
			}
		}

		if (state->runMode == MODE_WRITE_ONLY) {
			write_sequence(state);
		} else {
			generator_iterate(state);
		}
		generator_report_iteration(state);
	}

	return;
}


// The exponent, e, is set to 11
// This results in k = 837 and r = 187
void
micali_schnorr(struct state *state)
{
	int io_ret;		// I/O return status
	long int i;
	long int j;
	long int k = 837;
	long int num_0s;
	long int num_1s;
	long int bitsRead;
	bool done;		// true ==> enoungh data has been converted
	BYTE p[64];		// XXX - array size uses magic number
	BYTE q[64];		// XXX - array size uses magic number
	BYTE n[128];		// XXX - array size uses magic number
	BYTE e[1];		// XXX - array size uses magic number
	BYTE X[128];		// XXX - array size uses magic number
	BYTE Y[384];		// XXX - array size uses magic number
	BYTE Tail[105];		// XXX - array size uses magic number

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(87, __func__, "state arg is NULL");
	}

	ahtopb
	    ("E65097BAEC92E70478CAF4ED0ED94E1C94B154466BFB9EC9BE37B2B0FF8526C2"
	     "22B76E0E915017535AE8B9207250257D0A0C87C0DACEF78E17D1EF9DC44FD91F", p, 64);
	ahtopb
	    ("E029AEFCF8EA2C29D99CB53DD5FA9BC1D0176F5DF8D9110FD16EE21F32E37BA8"
	     "6FF42F00531AD5B8A43073182CC2E15F5C86E8DA059E346777C9A985F7D8A867", q, 64);
	memset(n, 0x00, 128);
	Mult(n, p, 64, q, 64);
	e[0] = 0x0b;
	memset(X, 0x00, 128);
	ahtopb("237c5f791c2cfe47bfb16d2d54a0d60665b20904ec822a6", X + 104, 24);

	for (i = 0; i < state->tp.numOfBitStreams; i++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		do {
			ModExp(Y, X, 128, e, 1, n, 128);
			memcpy(Tail, Y + 23, 105);
			for (j = 0; j < 3; j++) {
				bshl(Tail, 105);
			}
			done = copyBitsToEpsilon(state, 0, Tail, k, &num_0s, &num_1s, &bitsRead);
			memset(X, 0x00, 128);
			memcpy(X + 104, Y, 24);
			for (j = 0; j < 5; j++) {
				bshr(X + 104, 24);
			}
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (state->legacy_output == true) {
			fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(state->freqFile);
			if (io_ret != 0) {
				errp(87, __func__, "error flushing to: %s", state->freqFilePath);
			}
		}

		if (state->runMode == MODE_WRITE_ONLY) {
			write_sequence(state);
		} else {
			generator_iterate(state);
		}
		generator_report_iteration(state);
	}

	return;
}

// Uses 160 bit Xkey and no XSeed (b=160)
// This is the generic form of the generator found on the last page of the Change Notice for FIPS 186-2
void
SHA1(struct state *state)
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
	long int bitsRead;
	bool done;		// true ==> enough data has been converted
	ULONG tx[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };	// XXX - array size uses magic number

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(88, __func__, "state arg is NULL");
	}

	ahtopb("ec822a619d6ed5d9492218a7a4c5b15d57c61601", Xkey, 20);
	// ahtopb("E65097BAEC92E70478CAF4ED0ED94E1C94B15446", Xkey, 20);
	// ahtopb("6BFB9EC9BE37B2B0FF8526C222B76E0E91501753", Xkey, 20);
	// ahtopb("5AE8B9207250257D0A0C87C0DACEF78E17D1EF9D", Xkey, 20);
	// ahtopb("D99CB53DD5FA9BC1D0176F5DF8D9110FD16EE21F", Xkey, 20);

	for (i = 0; i < state->tp.numOfBitStreams; i++) {
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

			done = copyBitsToEpsilon(state, 0, G, 160, &num_0s, &num_1s, &bitsRead);
			add(Xkey, 20, G, 20);
			add(Xkey, 20, One, 1);
		} while (done == false);

		/*
		 * Write stats to freq.txt if in legacy_output mode
		 */
		if (state->legacy_output == true) {
			fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
			errno = 0;        // paranoia
			io_ret = fflush(state->freqFile);
			if (io_ret != 0) {
				errp(88, __func__, "error flushing to: %s", state->freqFilePath);
			}
		}

		if (state->runMode == MODE_WRITE_ONLY) {
			write_sequence(state);
		} else {
			generator_iterate(state);
		}
		generator_report_iteration(state);
	}

	return;
}
