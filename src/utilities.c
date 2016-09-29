/*****************************************************************************
 U T I L I T I E S
 *****************************************************************************/

// global capabilities
#define _ATFILE_SOURCE
#define __USE_XOPEN2K8
#define _GNU_SOURCE

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <errno.h>

// for checking dir
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// for stpncpy() and getline()
#include <string.h>
#include <stdio.h>

// sts includes
#include "externs.h"
#include "utilities.h"
#include "generators.h"
#include "stat_fncs.h"


/*
 * forward static function declarations
 */
static void readBinaryDigitsInASCIIFormat(struct state *state);


/*
 * getNumber - get a number from a stream
 *
 * given:
 *      input           FILE stream to read from
 *      output          where to write error messages
 *
 * returns:
 *      parsed long int form a line read from the stream
 *
 * This function does not return on read errors.  Entering a non-numeric line
 * results an error message on output and another read from input.
 */

long int
getNumber(FILE * input, FILE * output)
{
	char *line = NULL;	// != NULL --> malloced / realloced stream buffer
	size_t buflen = 0;	// ignored when line == NULL
	ssize_t linelen;	// length of line read from stream
	long int number;	// value to return

	/*
	 * firewall
	 */
	if (input == NULL) {
		err(1, __FUNCTION__, "input arg is NULL");
	}
	if (output == NULL) {
		err(1, __FUNCTION__, "output arg is NULL");
	}

	/*
	 * keep after the user until they learn to type correctly :-)
	 */
	do {
		/*
		 * read the line
		 */
		linelen = getline(&line, &buflen, input);
		if (line == NULL) {
			errp(4, __FUNCTION__, "line is still NULL after getline call");
		}
		if (linelen < 0) {
			errp(4, __FUNCTION__, "getline retuned: %d", linelen);
		}
		// firewall
		if (line[linelen] != '\0') {
			err(4, __FUNCTION__, "getline did not return a NUL terminated string");
		}

		/*
		 * attempt to convert to a number
		 */
		errno = 0;
		number = strtol(line, NULL, 0);
		if (errno != 0) {
			fprintf(output, "\n\nerror in parsing string to integer, please try again: ");
			fflush(output);
		}
	} while (errno != 0);

	/*
	 * cleanup and report success
	 */
	free(line);
	return number;
}


/*
 * getNumberOrDie - get a number from a stream or die
 *
 * given:
 *      stream          FILE stream to read
 *
 * returns:
 *      Parsed long int form a line read from the stream.
 *
 * This function does not return on error.
 */

long int
getNumberOrDie(FILE * stream)
{
	char *line = NULL;	// != NULL --> malloced / realloced stream buffer
	size_t buflen = 0;	// ignored when line == NULL
	ssize_t linelen;	// length of line read from stream
	long int number;	// value to return

	/*
	 * firewall
	 */
	if (stream == NULL) {
		err(1, __FUNCTION__, "stream arg is NULL");
	}

	/*
	 * read the line
	 */
	linelen = getline(&line, &buflen, stream);
	if (line == NULL) {
		errp(4, __FUNCTION__, "line is still NULL after getline call");
	}
	if (linelen <= 0) {
		errp(4, __FUNCTION__, "getline retuned: %d", linelen);
	}
	if (line[linelen] != '\0') {
		err(4, __FUNCTION__, "getline did not return a NUL terminated string");
	}

	/*
	 * attempt to convert to a number
	 */
	errno = 0;
	number = strtol(line, NULL, 0);
	if (errno != 0) {
		errp(4, __FUNCTION__, "error in parsing string to integer: '%s'", line);
	}

	/*
	 * cleanup and report success
	 */
	free(line);
	return number;
}


/*
 * getString - get a line from a stream
 *
 * given:
 *      stream          FILE stream to read
 *
 * returns:
 *      NUL terminated line read from stream, w/o and trailing newline or return.
 *
 * This function does not return on error.
 */
char *
getString(FILE * stream)
{
	char *line = NULL;	// != NULL --> malloced / realloced stream buffer
	size_t buflen = 0;	// ignored when line == NULL
	ssize_t linelen;	// length of line read from stream
	bool ready = false;	// line is ready to return

	/*
	 * firewall
	 */
	if (stream == NULL) {
		err(1, __FUNCTION__, "stream arg is NULL");
	}

	/*
	 * read the line
	 */
	linelen = getline(&line, &buflen, stdin);
	if (line == NULL) {
		errp(4, __FUNCTION__, "line is still NULL after getline call");
	}
	if (linelen <= 0) {
		errp(4, __FUNCTION__, "getline retuned: %d", linelen);
	}
	if (line[linelen] != '\0') {
		err(4, __FUNCTION__, "getline did not return a NUL terminated string");
	}

	/*
	 * remove any trailing newline, returns, etc.
	 */
	while (linelen > 0 && ready == false) {
		switch (line[linelen - 1]) {
		case '\n':
		case '\r':
			line[linelen - 1] = '\0';
			--linelen;
			break;
		default:
			ready = true;
			break;
		}
	}

	/*
	 * return our line
	 */
	return (line);
}


/*
 * checkWritePermissions - check write permissions of a directory or file
 *
 * given:
 *      path             // path to check
 *
 * retuen
 *      true --> path is writiable by the effecive user
 *      false --> path does not exist, is not writable, or some other error
 *
 * Check the write permissions of a path for the current user group and return
 * whether the path writable or not.
 */
bool
checkWritePermissions(char *path)
{
	bool permissions = false;
	int writable = 0;

	// firewall
	if (path == NULL) {
		warn(__FUNCTION__, "path arg was NULL");
		return false;
	}
	// check if path has write permissions
	writable = faccessat(AT_FDCWD, path, W_OK, AT_EACCESS);
	if (writable == 0) {
		permissions = true;
	} else {
		switch (errno) {
		case EACCES:
			dbg(DBG_VHIGH, "%s: path: %s is not writable", __FUNCTION__, path);
			break;
		case ENOENT:
			dbg(DBG_VHIGH, "%s: path: %s does not exist", __FUNCTION__, path);
			break;
		default:
			dbg(DBG_VHIGH, "%s: faccessat error on %s: %d: %s", __FUNCTION__, path, errno, strerror(errno));
			break;
		}
	}
	return (permissions);
}


/*
 * checkReadPermissions - check read permissions of a directory or file
 *
 * given:
 *      path             // path to check
 *
 * retuen
 *      true --> path is readable by the effecive user
 *      false --> path does not exist, is not readable, or some other error
 *
 * Check the write permissions of a path for the current user group and return
 * whether the path readable or not.
 */
bool
checkReadPermissions(char *path)
{
	bool permissions = false;
	int readable = 0;

	// firewall
	if (path == NULL) {
		warn(__FUNCTION__, "path arg was NULL");
		return false;
	}
	// check if path has write permissions
	readable = faccessat(AT_FDCWD, path, R_OK, AT_EACCESS);
	if (readable == 0) {
		permissions = true;
	} else {
		switch (errno) {
		case EACCES:
			dbg(DBG_VHIGH, "%s: path: %s is not readable", __FUNCTION__, path);
			break;
		case ENOENT:
			dbg(DBG_VHIGH, "%s: path: %s does not exist", __FUNCTION__, path);
			break;
		default:
			dbg(DBG_VHIGH, "%s: faccessat error on %s: %d: %s", __FUNCTION__, path, errno, strerror(errno));
			break;
		}
	}
	return (permissions);
}


/*
 * makePath - create directories along a path
 *
 * given:
 *      dir             // the path that must exist and last component writable
 *
 * Create directories (including intermediate directories if they don't exist) for a path dir for both absolute and
 * reative path names.
 */
void
makePath(char *dir)
{
	// set tmp string to length of dir
	char *tmp;		// copy of dir, //'s turned into /, trailing / removed
	char *p = NULL;		// working pointer
	size_t len;		// length of tmp copy of dir
	struct stat statbuf;	// sub-directory status

	// firewall
	if (dir == NULL) {
		err(10, __FUNCTION__, "dir arg is NULL");
	}
	dbg(DBG_VHIGH, "called %s on path: %s", __FUNCTION__, dir);

	/*
	 * optimization - if dir exists
	 */
	if (stat(dir, &statbuf) == 0) {
		if (S_ISDIR(statbuf.st_mode)) {
			// dir exists
			if (checkWritePermissions(dir)) {
				// all is well, nothing to do
				dbg(DBG_VHIGH, "dir is already a writable directory: %s", dir);
				return;
			} else {
				err(10, __FUNCTION__, "dir exists but is not a writable directory: %s", dir);
			}

		} else {
			// dir is not a directory
			err(10, __FUNCTION__, "dir exists but is not a directory: %s", dir);
		}
	}

	/*
	 * copy dir into tmp converting multiple /'s into a single /
	 */
	// allocate max possible storage for tmp
	len = strlen(dir);
	tmp = malloc(len + 1);	// +1 for paranoia below
	if (tmp == NULL) {
		errp(10, __FUNCTION__, "unable to allocate a string of length %d", len + 1);
	}
	tmp[len] = '\0';	// paranoia
	// copy directory path into tmp, converting multiple /'s into a single /
	for (p = dir, len = 0; *p; ++p) {
		if (*p == '/' && *(p + 1) == '/') {
			++p;
		} else {
			tmp[len++] = *p;
		}
	}
	tmp[len] = '\0';

	// if final character(s) is a / replace it with NUL
	if (len > 0 && tmp[len - 1] == '/') {
		tmp[len--] = '\0';
	}
	dbg(DBG_VHIGH, "will use canonical path: %s", tmp);

	// in case: empty path or just /
	if (len == 0 || (tmp[0] == '/' && len == 1)) {
		// path was empty or just /'s, nothing to do
		free(tmp);
		dbg(DBG_VHIGH, "empty path or just /, nothing to do");
		return;
	}

	/*
	 * iterate through directory string creating each directory
	 *
	 * NOTE: We start with the 2nd character because dir starts with /,
	 *       by definition of the filesystem / exists and does not need
	 *       to be created.  We handled the case where dir is only / above.
	 *       Finally if dir is just a single non-/ character, this loop
	 *       will terminate and will fall thru to the final mkdir at the
	 *       bottom of this function.
	 */
	for (p = tmp + 1; *p; p++) {
		if (*p == '/') {

			// cap directory string at the current directory
			*p = '\0';
			dbg(DBG_VVHIGH, "working sub-path: %s", tmp);

			// optimization - if sub-path already exists
			if (stat(tmp, &statbuf) == 0) {
				if (S_ISDIR(statbuf.st_mode)) {
					// sub-path is a dir, just keep going
					dbg(DBG_VVHIGH, "sub-path already a directory: %s", tmp);
					*p = '/';
					continue;
				} else {
					dbg(DBG_VHIGH, "sub-path exists but not directory: %s", tmp);
					dbg(DBG_VHIGH, "cannot create path: %s", dir);
					free(tmp);
					return;
				}
			}
			// new element in sub-path does not exist, so mkdir it
			dbg(DBG_VVHIGH, "about to mkdir %s", tmp);
			errno = 0;	// paranoia
			if (mkdir(tmp, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) < 0) {
				errp(1, __FUNCTION__, "error creating %s for %s", tmp, dir);
			}
			dbg(DBG_VVHIGH, "just created %s", tmp);

			// restore string for next possible directory
			*p = '/';
		}
	}

	/*
	 * make final directory (wasn't created in loop due to replacing '/' or lack of one to begin with)
	 */
	dbg(DBG_VVHIGH, "about to do the final mkdir %s", tmp);
	errno = 0;		// paranoia
	if (mkdir(tmp, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) < 0) {
		errp(1, __FUNCTION__, "error creating final %s for %s", tmp, dir);
	}
	dbg(DBG_VVHIGH, "just created final %s", tmp);
	dbg(DBG_VHIGH, "directory now exists and is writable: %s", dir);
	free(tmp);
	return;
}


/*
 * precheckPath - ensure that a path exists and is writable as needed
 *
 * given:
 *      state           // run state where subDirs is found
 *      dir             // the path to check
 *
 * If the state->subDirs is true (no -c), then dir will be checked if it exists.
 * If it does not exist, then there will be an attempt to create dir.  If dir cannot
 * be created, then is function will report an error and not return.  If dir exists
 * but is not writiable, then this function will report an error and not return.
 *
 * If the state->subDirs is false (-c), then dir will be checked if is exists.
 * If it does not exist, then this function will report an error and not return. If dir
 * exists but is not writiable, then this function will report an error and not return.
 */
void
precheckPath(struct state *state, char *dir)
{
	/*
	 * firewall
	 */
	if (state == NULL) {
		err(1, __FUNCTION__, "state arg is NULL");
	}
	if (dir == NULL) {
		err(1, __FUNCTION__, "dir arg is NULL");
	}
	dbg(DBG_VHIGH, "called %s on path: %s", __FUNCTION__, dir);

	/*
	 * no -c (state->subDirs is true)
	 *
	 * We are allowed to create missing directories.
	 */
	if (state->subDirs == true) {

		// make the directory if it does not exist (or not writable)
		dbg(DBG_VVHIGH, "no -c, check if %s is a writballe directory", dir);
		if (checkWritePermissions(dir) != true) {
			dbg(DBG_VVHIGH, "it is not, so try to mkdir %s", dir);
			makePath(dir);
			dbg(DBG_VVHIGH, "directory is writable %s", dir);
			return;
		}
	}

	/*
	 * if -c, then verify that directory exists and is writable
	 */
	dbg(DBG_VVHIGH, "with -c, %s must be a writballe directory", dir);
	if (checkWritePermissions(dir) != true) {
		err(10, __FUNCTION__, "directory does not exist or is not writable: %s", dir);
	}
	dbg(DBG_VVHIGH, "directory is writable %s", dir);
	return;
}


/*
 * precheckSubdir - ensure that a workDir sub-directory exists and is writable
 *
 * given:
 *      state           // run state where subDirs is found
 *      subdir          // check subdir under state->workDir
 *
 * returns:
 * 	Malloced path to the given subdir
 *
 * This utility function is used by test initialization functions to ensure
 * that subdir under the working directory (state->workDir) exists and is
 * writable.
 *
 * This function does not return on error.
 */
char *
precheckSubdir(struct state *state, char *subdir)
{
	char *fullpath;		// workDir/subdir
	size_t len;		// length of fullpath

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(1, __FUNCTION__, "state arg is NULL");
	}
	if (subdir == NULL) {
		err(1, __FUNCTION__, "subdir arg is NULL");
	}
	if (state->workDir == NULL) {
		err(1, __FUNCTION__, "state->workDir is NULL");
	}

	/*
	 * allocate full path including subdir
	 */
	len = strlen(state->workDir) + 1 + strlen(subdir) + 1;
	fullpath = malloc(len + 1);	// +1 for later paranoia
	if (fullpath == NULL) {
		errp(1, __FUNCTION__, "cannot malloc %d octets for fullpath", len+1);
	}

	/*
	 * form full path
	 */
	snprintf(fullpath, len, "%s/%s", state->workDir, subdir);
	fullpath[len] = '\0';	// paranoia
	dbg(DBG_VHIGH, "will verify that subdir exists and is writable: %s", fullpath);

	/*
	 * ensure that a workDir sub-directory exists and is writable
	 */
	precheckPath(state, fullpath);

	/*
	 * return malloced fullpath
	 */
	return fullpath;
}


/*
 * str2longint - convert a string into a long int
 *
 * given:
 *      success_p       pointer to bool indicating parse success or failure
 *      string          string to parse
 *
 * returns:
 *      If *success_p == true, returns a parsed long int form string.
 *      If *success_p != true, then it returns errno.
 *
 * This function does not return when given NULL args or on a parse error.
 */

long int
str2longint(bool * success_p, char *string)
{
	long int number;	// value to return
	int saved_errno;	// saved errno from strtol()

	/*
	 * firewall
	 */
	if (success_p == NULL) {
		err(1, __FUNCTION__, "success_p arg is NULL");
	}
	if (string == NULL) {
		err(1, __FUNCTION__, "string arg is NULL");
	}

	/*
	 * attempt to convert to a number
	 */
	errno = 0;
	number = strtol(string, NULL, 0);
	saved_errno = errno;
	if (saved_errno != 0) {

		// cleanup and report failure
		dbg(DBG_LOW, "error in parsing string to integer: '%s'", string);
		*success_p = false;
		return saved_errno;
	}

	/*
	 * cleanup and report success
	 */
	*success_p = true;
	return number;
}

/*
 * str2longint_or_die - get a number from a string or die
 *
 * given:
 *      string          string to parse
 *
 * returns:
 *      Parsed long int form a line read from the string.
 *
 * This function does not return on error.
 */

long int
str2longint_or_die(char *string)
{
	long int number;	// value to return

	/*
	 * firewall
	 */
	if (string == NULL) {
		err(1, __FUNCTION__, "string arg is NULL");
	}

	/*
	 * attempt to convert to a number
	 */
	errno = 0;
	number = strtol(string, NULL, 0);
	if (errno != 0) {
		errp(4, __FUNCTION__, "error in parsing string to integer: '%s'", string);
	}

	/*
	 * cleanup and report success
	 */
	return number;
}


/*
 * generatorOptions - determine generation options
 *
 * given:
 * 	state		// pointer to run state
 *
 * In classic (non-batch) mode we prompt the user for a generator number.  In the
 * case of generator 0 (read from a file) we also ask for the filename and verify
 * that the file is readable.
 *
 * In bagch more we just cheeck if, for generator 0 (read from a file) the filename
 * is readable.
 *
 * This function does not return on error.
 */
void
generatorOptions(struct state *state)
{
	int generator;		// generator number
	bool success;		// if we read a valid file format
	char *src;		// potential src of random file data
	char *line;		// random file data line

	/*
	 * batch mode, nothing to do here other than to verify, for -g 0, randdata is readable
	 */
	if (state->batchmode == true) {

		if (state->generator == 0) {

			// verify the input file is readable
			if (checkReadPermissions(state->randomDataPath) == false) {
				err(10, __FUNCTION__, "input data file not readable: %s", state->randomDataPath);
			}

			// open the input file for reading
			state->streamFile = fopen(state->randomDataPath, "r");
			if (state->streamFile == NULL) {
				errp(10, __FUNCTION__, "unable to open data file to reading: %s", state->randomDataPath);
			}
		}

		// nothing else to do but return the generator number
		return;
	}

	/*
	 * non-batch mode
	 */
	do {
		// if -g was used, do not prompt for generator
		if (state->generatorFlag == true) {

			// use the generator number passed in on the command line
			generator = state->generator;

		// -g was not used, prompt for generator
		} else {
			// prompt
			printf("           G E N E R A T O R    S E L E C T I O N \n");
			printf("           ______________________________________\n\n");
			printf("    [0] Input File                 [1] Linear Congruential\n");
			printf("    [2] Quadratic Congruential I   [3] Quadratic Congruential II\n");
			printf("    [4] Cubic Congruential         [5] XOR\n");
			printf("    [6] Modular Exponentiation     [7] Blum-Blum-Shub\n");
			printf("    [8] Micali-Schnorr             [9] G Using SHA-1\n\n");
			printf("   Enter Choice: ");
			fflush(stdout);

			// read number
			generator = getNumber(stdin, stdout);
			putchar('\n');

			// help if they gave us a wrong nunber
			if (generator < 0 || generator > NUMOFGENERATORS) {
				printf("\ngenerator number must be from 0 to %d inclusive, try again\n\n", NUMOFGENERATORS);
			}
		}

		// for generator 0 (read from a file) and no -f randdata, ask for the filename
		if (generator == 0 && state->randomDataPath == false) {

			// prompt
			printf("\t\tUser Prescribed Input File: ");
			fflush(stdout);

			// read filename
			src = getString(stdin);
			putchar('\n');

			// set the randomData filename
			state->randomDataPath = src;

			// sanity check to see if we can read the file
			if (checkReadPermissions(src) != true) {
				printf("\nfile: %s does not exist or is not readable, try again\n\n", src);
				free(state->randomDataPath);
				continue;
			}
		}

		// for generator 0 (read from a file), open the file
		if (generator == 0) {
			// open the input file for reading
			state->streamFile = fopen(state->randomDataPath, "r");
			if (state->streamFile == NULL) {
				printf("\nunable to open %s of reading, try again\n\n", state->randomDataPath);
				if (state->randomDataPath == false) {
					free(state->randomDataPath);
				}
				continue;
			}

			// promot for input format if -F was NOT used
			if (state->dataFormatFlag == false) {

				// determine generator 0 (read from a file), open the file format
				success = false;
				do {
					// prompt
					printf("   Input File Format:\n");
					printf("    [0] or [a] ASCII - A sequence of ASCII 0's and 1's\n");
					printf("    [1] or [r] Raw binary - Each byte in data file contains 8 bits of data\n\n");
					printf("   Select input mode:  ");
					fflush(stdout);

					// read a line
					line = getString(stdin);
					putchar('\n');

					// parse format
					switch (line[0]) {
					case '0':
						success = true;
						state->dataFormat = FORMAT_ASCII_01;
						break;
					case '1':
						success = true;
						state->dataFormat = FORMAT_RAW_BINARY;
						break;
					case FORMAT_RAW_BINARY:
					case FORMAT_ASCII_01:
						success = true;
						state->dataFormat = (enum format) line[0];
						break;
					default:
						printf("\ninput must be %d of %c, %d or %c\n\n",
						       0, (char) FORMAT_ASCII_01,
						       1, (char) FORMAT_RAW_BINARY);
						break;
					}
				} while (success != true);
				free(line);
			}
		}

	} while (generator < 0 || generator > NUMOFGENERATORS);
	putchar('\n');

	// set the generator option
	dbg(DBG_LOW, "will use generator %s[%d]", state->generatorDir[generator], generator);
	state->generator = generator;
	return;
}


void
chooseTests(struct state *state)
{
	int i;
	int run_all;
	char *run_test;
	bool success;		// if parse in the input line was sucessful

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}

	// If -t was used, tests are already chosen, just return
	if (state->testVectorFlag == true) {
		return;
	}

	// prompt
	printf("                S T A T I S T I C A L   T E S T S\n");
	printf("                _________________________________\n\n");
	printf("    [01] Frequency                       [02] Block Frequency\n");
	printf("    [03] Cumulative Sums                 [04] Runs\n");
	printf("    [05] Longest Run of Ones             [06] Rank\n");
	printf("    [07] Discrete Fourier Transform      [08] Nonperiodic Template Matchings\n");
	printf("    [09] Overlapping Template Matchings  [10] Universal Statistical\n");
	printf("    [11] Approximate Entropy             [12] Random Excursions\n");
	printf("    [13] Random Excursions Variant       [14] Serial\n");
	printf("    [15] Linear Complexity\n\n");
	printf("         INSTRUCTIONS\n");
	printf("            Enter 0 if you DO NOT want to apply all of the\n");
	printf("            statistical tests to each sequence and 1 if you DO.\n\n");
	printf("   Enter Choice: ");
	fflush(stdout);

	// read number
	run_all = getNumber(stdin, stdout);
	putchar('\n');

	state->testVector[0] = ((run_all == 1) ? true : false);
	putchar('\n');
	if (state->testVector[0] == true) {
		for (i = 1; i <= NUMOFTESTS; i++) {
			state->testVector[i] = true;
		}
	} else {
		do {
			// prompt
			printf("         INSTRUCTIONS\n");
			printf("            Enter a 0 or 1 to indicate whether or not the numbered statistical\n");
			printf("            test should be applied to each sequence.\n\n");
			printf("               111111\n");
			printf("      123456789012345\n");
			printf("      ");
			fflush(stdout);

			// read the test string
			run_test = getString(stdin);
			putchar('\n');

			// parse 0's and 1's
			success = true;	// hope for the best
			for (i = 1; i <= NUMOFTESTS; i++) {
				switch (run_test[i - 1]) {
				case '0':
					state->testVector[i] = false;
					break;
				case '1':
					state->testVector[i] = true;
					break;
				case '\0':
					success = false;	// must prompt again
					printf("\nYou must enter a 0 or 1 for all %d tests\n\n", NUMOFTESTS);
					break;
				default:
					success = false;	// must prompt again
					printf("\nUnexpcted character for test %d, enter only 0 or 1 for each test\n", i);
					break;
				}
			}
		} while (success != true);
	}
	return;
}


void
fixParameters(struct state *state)
{
	int counter;
	int testid;

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}

	/*
	 * Check to see if any parameterized tests are selected
	 */
	if ((state->testVector[TEST_BLOCK_FREQUENCY] == false) && (state->testVector[TEST_NONPERIODIC] == false) &&
	    (state->testVector[TEST_OVERLAPPING] == false) && (state->testVector[TEST_APEN] == false) &&
	    (state->testVector[TEST_SERIAL] == false) && (state->testVector[TEST_LINEARCOMPLEXITY] == false))
		return;

	do {
		counter = 1;
		// prompt
		printf("        P a r a m e t e r   A d j u s t m e n t s\n");
		printf("        -----------------------------------------\n");
		if (state->testVector[TEST_BLOCK_FREQUENCY] == true)
			printf("    [%d] Block Frequency Test - block length(M):         %ld\n", counter++,
			       state->tp.blockFrequencyBlockLength);
		if (state->testVector[TEST_NONPERIODIC] == true)
			printf("    [%d] NonOverlapping Template Test - block length(m): %ld\n", counter++,
			       state->tp.nonOverlappingTemplateBlockLength);
		if (state->testVector[TEST_OVERLAPPING] == true)
			printf("    [%d] Overlapping Template Test - block length(m):    %ld\n", counter++,
			       state->tp.overlappingTemplateBlockLength);
		if (state->testVector[TEST_APEN] == true)
			printf("    [%d] Approximate Entropy Test - block length(m):     %ld\n", counter++,
			       state->tp.approximateEntropyBlockLength);
		if (state->testVector[TEST_SERIAL] == true)
			printf("    [%d] Serial Test - block length(m):                  %ld\n", counter++,
			       state->tp.serialBlockLength);
		if (state->testVector[TEST_LINEARCOMPLEXITY] == true)
			printf("    [%d] Linear Complexity Test - block length(M):       %ld\n", counter++,
			       state->tp.linearComplexitySequenceLength);
		putchar('\n');
		printf("   Select Test (0 to continue): ");
		fflush(stdout);

		// read number
		testid = getNumber(stdin, stdout);
		putchar('\n');

		counter = 0;
		if (state->testVector[TEST_BLOCK_FREQUENCY] == true) {
			counter++;
			if (counter == testid) {
				// prompt
				printf("   Enter Block Frequency Test block length: ");
				fflush(stdout);

				// read number
				state->tp.blockFrequencyBlockLength = getNumber(stdin, stdout);
				putchar('\n');
				continue;
			}
		}
		if (state->testVector[TEST_NONPERIODIC] == true) {
			counter++;
			if (counter == testid) {
				// prompt
				printf("   Enter NonOverlapping Template Test block Length: ");
				fflush(stdout);

				// read number
				state->tp.nonOverlappingTemplateBlockLength = getNumber(stdin, stdout);
				putchar('\n');
				continue;
			}
		}
		if (state->testVector[TEST_OVERLAPPING] == true) {
			counter++;
			if (counter == testid) {
				// prompt
				printf("   Enter Overlapping Template Test block Length: ");
				fflush(stdout);

				// read number
				state->tp.overlappingTemplateBlockLength = getNumber(stdin, stdout);
				putchar('\n');
				continue;
			}
		}
		if (state->testVector[TEST_APEN] == true) {
			counter++;
			if (counter == testid) {
				// prompt
				printf("   Enter Approximate Entropy Test block Length: ");
				fflush(stdout);

				// read number
				state->tp.approximateEntropyBlockLength = getNumber(stdin, stdout);
				putchar('\n');
				continue;
			}
		}
		if (state->testVector[TEST_SERIAL] == true) {
			counter++;
			if (counter == testid) {
				// prompt
				printf("   Enter Serial Test block Length: ");
				fflush(stdout);

				// read number
				state->tp.serialBlockLength = getNumber(stdin, stdout);
				putchar('\n');
				continue;
			}
		}
		if (state->testVector[TEST_LINEARCOMPLEXITY] == true) {
			counter++;
			if (counter == testid) {
				// prompt
				printf("   Enter Linear Complexity Test block Length: ");
				fflush(stdout);

				// read number
				state->tp.linearComplexitySequenceLength = getNumber(stdin, stdout);
				putchar('\n');
				continue;
			}
		}
	} while (testid != 0);
}


void
fileBasedBitStreams(struct state *state)
{
	int seekError;
	long int byteCount;		// byte count for a number of consecutive bits, rounded up

	/*
	 * firewall
	 */
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->streamFile == NULL) {
		err(10, __FUNCTION__, "streamFile is NULL");
	}

	/*
	 * parse a set of ASCII '0' and '1' characters
	 */
	if (state->dataFormat == FORMAT_ASCII_01) {

		// set file pointer jobnum chunks into file
		dbg(DBG_LOW, "seeking %ld * %ld * %ld = %ld on %s for ASCII 0/1 format",
			     state->jobnum, state->tp.n, state->tp.numOfBitStreams,
			     (state->jobnum * state->tp.n * state->tp.numOfBitStreams),
			     state->randomDataPath);
		seekError = fseek(state->streamFile,
				  (state->jobnum * state->tp.n * state->tp.numOfBitStreams),
				  SEEK_SET);
		if (seekError != 0) {
			errp(1, __FUNCTION__, "could not seek %ld into file: %s",
					      (state->jobnum * state->tp.n * state->tp.numOfBitStreams),
					      state->randomDataPath);
		}

		// parse data
		readBinaryDigitsInASCIIFormat(state);

		// close file
		fclose(state->streamFile);
		state->streamFile = NULL;

	/*
	 * parse raw 8-bit binary octets
	 */
	} else if (state->dataFormat == FORMAT_RAW_BINARY) {

		/*
		 * bytes that hold a given set of consecutive bits
		 *
		 * We need to round up the byte count to the next whole byte.
		 * However if the bit count is a multiple of 8, then we do
		 * not increase the byte only.  We only increase by one in
		 * the case of a final partial bytw.
		 */
		byteCount = ((state->jobnum * state->tp.n * state->tp.numOfBitStreams)+7)/8;

		// set file pointer jobnum chunks into file
		dbg(DBG_LOW, "seeking %ld * %ld * %ld/8 = %ld on %s for raw binary format",
			     state->jobnum, state->tp.n, state->tp.numOfBitStreams,
			     byteCount, state->randomDataPath);
		seekError = fseek(state->streamFile,
				  ((state->jobnum * state->tp.n * state->tp.numOfBitStreams)+7)/8,
				  SEEK_SET);
		if (seekError != 0) {
			err(1, __FUNCTION__, "could not seek %ld into file: %s", byteCount, state->randomDataPath);
		}

		// parse data
		readHexDigitsInBinaryFormat(state);

		// close file
		fclose(state->streamFile);
		state->streamFile = NULL;

	/*
	 * should not get here
	 */
	} else {
		err(1, __FUNCTION__, "Input file format selection is invalid");
	}
	return;
}


static void
readBinaryDigitsInASCIIFormat(struct state *state)
{
	int i;
	int j;
	int num_0s;
	int num_1s;
	int bitsRead;
	int bit;

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->streamFile == NULL) {
		err(10, __FUNCTION__, "streamFile is NULL");
	}

	if ((epsilon = (BitSequence *) calloc(state->tp.n, sizeof(BitSequence))) == NULL) {
		errp(10, __FUNCTION__, "cannot malloc BitSequence of %d octets", state->tp.n * sizeof(BitSequence));
	}
	for (i = 0; i < state->tp.numOfBitStreams; i++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		for (j = 0; j < state->tp.n; j++) {
			if (fscanf(state->streamFile, "%1d", &bit) == EOF) {
				warn(__FUNCTION__, "Insufficient data in file %s: %d bits were read",
						   state->randomDataPath, bitsRead);
				free(epsilon);
				return;
			} else {
				bitsRead++;
				if (bit == 0) {
					num_0s++;
				} else {
					num_1s++;
				}
				epsilon[j] = bit;
			}
		}
		fprintf(freqfp, "\t\tBITSREAD = %d 0s = %d 1s = %d\n", bitsRead, num_0s, num_1s);
		nist_test_suite(state);
	}
	free(epsilon);
}

void
readHexDigitsInBinaryFormat(struct state *state)
{
	int i;
	int done;
	int num_0s;
	int num_1s;
	int bitsRead;
	BYTE buffer[4];

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	if (state->streamFile == NULL) {
		err(10, __FUNCTION__, "streamFile is NULL");
	}

	if ((epsilon = (BitSequence *) calloc(state->tp.n, sizeof(BitSequence))) == NULL) {
		errp(10, __FUNCTION__, "cannot malloc BitSequence of %d octets", state->tp.n * sizeof(BitSequence));
	}

	for (i = 0; i < state->tp.numOfBitStreams; i++) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		done = 0;
		do {
			if (fread(buffer, sizeof(unsigned char), 4, state->streamFile) != 4) {
				errp(10, __FUNCTION__, "read error or insufficient data in file");
			}
			done = convertToBits(buffer, 32, state->tp.n, &num_0s, &num_1s, &bitsRead);
		} while (!done);
		fprintf(freqfp, "\t\tBITSREAD = %d 0s = %d 1s = %d\n", bitsRead, num_0s, num_1s);

		nist_test_suite(state);

	}
	free(epsilon);
}


int
convertToBits(BYTE * x, int xBitLength, int bitsNeeded, int *num_0s, int *num_1s, int *bitsRead)
{
	int i;
	int j;
	int count;
	int bit;
	BYTE mask;
	int zeros;
	int ones;

	count = 0;
	zeros = ones = 0;
	for (i = 0; i < (xBitLength + 7) / 8; i++) {
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
			epsilon[*bitsRead] = bit;
			(*bitsRead)++;
			if (*bitsRead == bitsNeeded) {
				return 1;
			}
			if (++count == xBitLength) {
				return 0;
			}
		}
	}

	return 0;
}


void
openOutputStreams(struct state *state)
{
	int i;
	int numOfBitStreams;
	int numOfOpenFiles = 0;
	char freqfn[200];
	char summaryfn[200];
	char statsDir[200];
	char resultsDir[200];

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}
	// create the path if required
	if (state->workDirFlag == true) {
		makePath(state->workDir);
	}

	sprintf(freqfn, "%s/freq.txt", state->workDir);
	dbg(DBG_HIGH, "in %s, generator %s[%d] uses freq.txt file: %s", 
		      __FUNCTION__, state->generatorDir[state->generator], state->generator, freqfn);
	if ((freqfp = fopen(freqfn, "w")) == NULL) {
		errp(10, __FUNCTION__, "Could not open freq file: %s", freqfn);
	}
	sprintf(summaryfn, "%s/finalAnalysisReport.txt", state->workDir);
	dbg(DBG_HIGH, "in %s, generator %s[%d] uses finalAnalysisReport.txt.txt file: %s", 
		      __FUNCTION__, state->generatorDir[state->generator], state->generator, summaryfn);
	if ((summary = fopen(summaryfn, "w")) == NULL) {
		errp(10, __FUNCTION__, "Could not open stats file: %s", summaryfn);
	}

	for (i = 1; i <= NUMOFTESTS; i++) {
		if (state->testVector[i] == true) {
			sprintf(statsDir, "%s/stats.txt", state->subDir[i]);
			dbg(DBG_HIGH, "in %s, generator %s[%d] test %s[%d] uses stats.txt file: %s", 
				      __FUNCTION__, state->generatorDir[state->generator], state->generator, 
				      state->testNames[i], i, statsDir);
			sprintf(resultsDir, "%s/results.txt", state->subDir[i]);
			dbg(DBG_HIGH, "in %s, generator %s[%d] test %s[%d] uses results.txt file: %s", 
				      __FUNCTION__, state->generatorDir[state->generator], state->generator, 
				      state->testNames[i], i, resultsDir);
			if ((stats[i] = fopen(statsDir, "w")) == NULL) {	/* STATISTICS LOG */
				errp(10, __FUNCTION__, "unable to open stats.txt file %s for test: %s", statsDir, state->testNames[i]);
			} else {
				numOfOpenFiles++;
			}
			if ((results[i] = fopen(resultsDir, "w")) == NULL) {	/* P_VALUES LOG */
				errp(10, __FUNCTION__, "unable to open results.txt file %s for test: %s", resultsDir, state->testNames[i]);
			} else {
				numOfOpenFiles++;
			}
		}
	}

	// do not perform any interation if batchmode is enabled or -i was given
	if (state->batchmode == false && state->iterationFlag == false) {
		// prompt
		printf("   How many bitstreams? ");
		fflush(stdout);

		// read number
		numOfBitStreams = getNumber(stdin, stdout);
		putchar('\n');
	}
}


void
invokeTestSuite(struct state *state)
{
	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}

	fprintf(freqfp, "________________________________________________________________________________\n\n");
	if (state->generator == 0) {
		fprintf(freqfp, "\t\tFILE = %s\t\tALPHA = %6.4f\n", state->randomDataPath, ALPHA);
	} else {
		fprintf(freqfp, "\t\tFILE = %s\t\tALPHA = %6.4f\n", state->generatorDir[state->generator], ALPHA);
	}
	fprintf(freqfp, "________________________________________________________________________________\n\n");
	if (state->batchmode == true) {
		dbg(DBG_LOW, "     Statistical Testing In Progress.........");
	} else {
		printf("     Statistical Testing In Progress.........\n\n");
	}
	switch (state->generator) {
	case 0:
		fileBasedBitStreams(state);
		break;
	case 1:
		lcg(state);
		break;
	case 2:
		quadRes1(state);
		break;
	case 3:
		quadRes2(state);
		break;
	case 4:
		cubicRes(state);
		break;
	case 5:
		exclusiveOR(state);
		break;
	case 6:
		modExp(state);
		break;
	case 7:
		bbs(state);
		break;
	case 8:
		micali_schnorr(state);
		break;
	case 9:
		SHA1(state);
		break;

	/*
	 * INTRODUCE NEW PSEUDO RANDOM NUMBER GENERATORS HERE
	 */

	default:
		err(10, __FUNCTION__, "Error in invokeTestSuite!");
		break;
	}
	if (state->batchmode == true) {
		dbg(DBG_LOW, "     Statistical Testing Complete!!!!!!!!!!!!");
	} else {
		printf("     Statistical Testing Complete!!!!!!!!!!!!\n");
	}
}


/*
 * nist_test_suite - run an interation for all enabled tests
 *
 * given:
 * 	state		// run state and which tests are enabled
 */
void
nist_test_suite(struct state *state)
{
	time_t seconds;		// seconds since the epoch
	struct tm *loc_ret;	// return value form localtime_r()
	struct tm now;		// the time now
	size_t time_len;	// length of string produced by strftime() or 0
	char buf[BUFSIZ+1];	// time string buffer
	int i;

	// firewall
	if (state == NULL) {
		err(10, __FUNCTION__, "state is NULL");
	}

	/*
	 * perform 1 interation on all enabled tests
	 */
	for (i = 1; i <= NUMOFTESTS; ++i) {

		// call if enabled
		if (state->testVector[i] == true) {
			// interate for a given test
			test[i].iterate(state);
		}
	}

	/*
	 * count the interation and report process if requested
	 */
	++state->curIteration;
	if (state->reportCycle > 0 && (state->curIteration % state->reportCycle) == 0) {

		/*
		 * determine the time for now
		 */
		errno = 0;	// paranoia
		seconds = time(NULL);
		if (seconds < 0) {
			errp(10, __FUNCTION__, "time returned < 0: %ld", seconds);
		}
		loc_ret = localtime_r(&seconds, &now);
		if (loc_ret == NULL) {
			errp(10, __FUNCTION__, "localtime_r returned NULL");
		}
		errno = 0;	// paranoia
		time_len = strftime(buf, BUFSIZ, "%F %T", &now);
		if (time_len == 0) {
			errp(10, __FUNCTION__, "strftime failed");
		}
		buf[BUFSIZ] = '\0';	// paranoia
		
		/*
		 * we are at a multiple of state->reportCycle interations, tell the user
		 */
		msg("completed iteration %ld of %ld at %s", state->curIteration, state->tp.numOfBitStreams, buf);
	}
	return;
}
