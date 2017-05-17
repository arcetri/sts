/*****************************************************************************
 U T I L I T I E S
 *****************************************************************************/

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


// Exit codes: 210 thru 234

// global capabilities
#define _ATFILE_SOURCE
#define __USE_XOPEN2K8
#define _GNU_SOURCE

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

// for checking dir
#include <fcntl.h>
#include <sys/stat.h>

// for stpncpy() and getline()
#include <string.h>
#include <stdio.h>

// sts includes
#include "../utils/externs.h"
#include "utilities.h"
#include "debug.h"


/*
 * Forward static function declarations
 */
static double getDouble(FILE * input, FILE * output);
static char * getString(FILE * stream);
static bool checkReadPermissions(char *path);
static void handleFileBasedBitStreams(struct state *state);
static void *testBits(void *thread_args);
static void parseBitsASCIIInput(struct thread_state *thread_state);
static void parseBitsBinaryInput(struct thread_state *thread_state);


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
	size_t buflen = 0;	// Ignored when line == NULL
	ssize_t linelen;	// Length of line read from stream
	long int number;	// value to return

	/*
	 * Check preconditions (firewall)
	 */
	if (input == NULL) {
		err(210, __func__, "input arg is NULL");
	}
	if (output == NULL) {
		err(210, __func__, "output arg is NULL");
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
			errp(210, __func__, "line is still NULL after getline call");
		}
		if (linelen < 0) {
			errp(210, __func__, "getline returned: %ld", linelen);
		}
		// firewall
		if (line[linelen] != '\0') {
			err(210, __func__, "getline did not return a NULL terminated string");
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
 * getDouble - get a floating point double from a stream
 *
 * given:
 *      input           FILE stream to read from
 *      output          where to write error messages
 *
 * returns:
 *      parsed double form a line read from the stream
 *
 * This function does not return on read errors.  Entering a non-numeric line
 * results an error message on output and another read from input.
 */
static double
getDouble(FILE * input, FILE * output)
{
	char *line = NULL;	// != NULL --> malloced / realloced stream buffer
	size_t buflen = 0;	// Ignored when line == NULL
	ssize_t linelen;	// Length of line read from stream
	double number;		// value to return

	/*
	 * Check preconditions (firewall)
	 */
	if (input == NULL) {
		err(211, __func__, "input arg is NULL");
	}
	if (output == NULL) {
		err(211, __func__, "output arg is NULL");
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
			errp(211, __func__, "line is still NULL after getline call");
		}
		if (linelen < 0) {
			errp(211, __func__, "getline returned: %ld", linelen);
		}
		/*
		 * paranoia
		 */
		if (line[linelen] != '\0') {
			err(211, __func__, "getline did not return a NULL terminated string");
		}

		/*
		 * attempt to convert to a number
		 */
		errno = 0;
		number = strtod(line, NULL);
		if (errno != 0) {
			fprintf(output, "\n\nerror in parsing string to floating point value, please try again: ");
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
static char *
getString(FILE * stream)
{
	char *line = NULL;	// != NULL --> malloced / realloced stream buffer
	size_t buflen = 0;	// Ignored when line == NULL
	ssize_t linelen;	// Length of line read from stream
	bool ready = false;	// line is ready to return

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(213, __func__, "stream arg is NULL");
	}

	/*
	 * read the line
	 */
	linelen = getline(&line, &buflen, stream);
	if (line == NULL) {
		errp(213, __func__, "line is still NULL after getline call");
	}
	if (linelen <= 0) {
		errp(213, __func__, "getline returned: %ld", linelen);
	}
	if (line[linelen] != '\0') {
		err(213, __func__, "getline did not return a NUL terminated string");
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
 * return
 *      true --> path is writable by the effective user
 *      false --> path does not exist, is not writable, or some other error
 *
 * Check the write permissions of a path for the current user group and return
 * whether the path writable or not.
 */
bool
checkWritePermissions(char *path)
{
	int writable = 0;

	/*
	 * Check preconditions (firewall)
	 */
	if (path == NULL) {
		warn(__func__, "path arg was NULL");
		return false;
	}

	/*
	 * Check if path has write permissions
	 */
	writable = faccessat(AT_FDCWD, path, W_OK, AT_EACCESS);
	if (writable == 0) {
		return true;
	}

	switch (errno) {
	case EACCES:
		dbg(DBG_VHIGH, "%s: path: %s is not writable", __func__, path);
		break;
	case ENOENT:
		dbg(DBG_VHIGH, "%s: path: %s does not exist", __func__, path);
		break;
	default:
		dbg(DBG_VHIGH, "%s: faccessat error on %s: %d: %s", __func__, path, errno, strerror(errno));
		break;
	}

	return false;
}


/*
 * checkReadPermissions - check read permissions of a directory or file
 *
 * given:
 *      path             // path to check
 *
 * return
 *      true --> path is readable by the effective user
 *      false --> path does not exist, is not readable, or some other error
 *
 * Check the write permissions of a path for the current user group and return
 * whether the path readable or not.
 */
static bool
checkReadPermissions(char *path)
{
	bool permissions = false;
	int readable = 0;

	// firewall
	if (path == NULL) {
		warn(__func__, "path arg was NULL");
		return false;
	}
	// check if path has write permissions
	readable = faccessat(AT_FDCWD, path, R_OK, AT_EACCESS);
	if (readable == 0) {
		permissions = true;
	} else {
		switch (errno) {
		case EACCES:
			dbg(DBG_VHIGH, "%s: path: %s is not readable", __func__, path);
			break;
		case ENOENT:
			dbg(DBG_VHIGH, "%s: path: %s does not exist", __func__, path);
			break;
		default:
			dbg(DBG_VHIGH, "%s: faccessat error on %s: %d: %s", __func__, path, errno, strerror(errno));
			break;
		}
	}
	return (permissions);
}


/*
 * openTruncate - check truncate and write permissions of a file
 *
 * given:
 *      filename         // the filename to check for truncate and write permissions
 *
 * return
 *      non-NULL --> open FILE descritor of a truncted file
 *      NULL --> path was not a file, or could not be created, or was not writable
 *
 * Check the write permissions of a path for the current user group and return
 * whether the file may be truncated and written.
 *
 * If the file existed before this call and is wriable, the file will be truncated to zero length.
 * If the file did not exist before this call and could be created, the file will be created.
 */
FILE *
openTruncate(char *filename)
{
	FILE *stream;		// tunrcated flle or NULL

	/*
	 * Check preconditions (firewall)
	 */
	if (filename == NULL) {
		warn(__func__, "filename arg was NULL");
		return NULL;
	}

	/*
	 * Attempt to truncate file
	 */
	errno = 0;		// paranoia
	stream = fopen(filename, "w");
	if (stream == NULL) {
		warnp(__func__, "could not create/open for writing/truncation: %s", filename);
		return NULL;
	}
	dbg(DBG_HIGH, "created/opened for writing/truncation: %s", filename);
	return stream;
}


/*
 * filePathName - malloc a file pathname given a two parts of the path
 *
 * given:
 *      head            // leading part part of file pathname to form
 *      tail            // trailing part of file pathname to form
 *
 * returns:
 *      Malloced string of head/tail.
 *
 * This function does not return on error.
 */
char *
filePathName(char *head, char *tail)
{
	char *fullpath;		// Path of the dir/file
	size_t len;		// Length of fullpath
	int snprintf_ret;	// snprintf return value

	/*
	 * Check preconditions (firewall)
	 */
	if (head == NULL) {
		err(214, __func__, "head arg is NULL");
	}
	if (tail == NULL) {
		err(214, __func__, "tail arg is NULL");
	}

	/*
	 * Allocate full path including subdir
	 */
	len = strlen(head) + 1 + strlen(tail) + 1;
	fullpath = malloc(len + 1);	// +1 for later paranoia
	if (fullpath == NULL) {
		errp(214, __func__, "cannot malloc of %ld elements of %ld bytes each for fullpath", len + 1,
		     sizeof(fullpath[0]));
	}

	/*
	 * Form full path
	 */
	errno = 0;		// paranoia
	snprintf_ret = snprintf(fullpath, len, "%s/%s", head, tail);
	fullpath[len] = '\0';	// paranoia
	if (snprintf_ret <= 0 || snprintf_ret >= BUFSIZ || errno != 0) {
		errp(214, __func__, "snprintf failed for %ld bytes for %s/%s, returned: %d", len, head, tail, snprintf_ret);
	}

	/*
	 * Return malloced path
	 */
	return fullpath;
}

/*
 * datatxt_fmt - allocate a format string for data*.txt filenames
 *
 * given:
 *      partitionCount          - number of partitions, must be > 0
 *
 * returns:
 *      malloced format of the form %0*d.txt
 *
 * This function does not return in error
 */
char *
data_filename_format(int partitionCount)
{
	char *buf;		// Allocated buffer
	int digits;		// Decimal digits contained in partitionCount
	long int len;		// Length of the format
	int snprintf_ret;	// snprintf return value

	/*
	 * Check preconditions (firewall)
	 */
	if (partitionCount < 1) {
		err(215, __func__, "partitionCount arg: %d must be >= 1", partitionCount);
	}

	/*
	 * Determine the lenth of the allocated buffer
	 */
	if (partitionCount < 2) {
		digits = 0;	// no partitions, just use data.txt
	} else {
		for (digits = MAX_DATA_DIGITS; digits > 0; --digits) {
			if (pow(10.0, digits) < (double) partitionCount) {
				break;
			}
		}
	}
	if (digits < 1) {
		len = (long int) strlen("data.txt") + 1;	// data.txt: + 1 for NUL
	} else if (digits < 10) {
		len = (long int) strlen("data%0") + 1 + strlen("d.txt") + 1;	// data%0[0-9]d.txt: + 1 for NUL
	} else {
		len = (long int) strlen("data%0") + 2 + strlen("d.txt") + 1;	// data%0[0-9][0-9]d.txt: + 1 for NUL
	}

	/*
	 * Allocate the format
	 */
	buf = malloc((size_t) len + 1);	// + 1 for paranoia
	if (buf == NULL) {
		errp(215, __func__, "cannot malloc of %ld elements of %ld bytes each for data%%0*d.txt", len + 1,
		     sizeof(buf[0]));
	}

	/*
	 * Form the format
	 */
	if (digits < 1) {
		strcpy(buf, "data.txt");
	} else {
		errno = 0;	// paranoia
		snprintf_ret = snprintf(buf, len, "data%%0%dd.txt", digits);
		if (snprintf_ret <= 0 || snprintf_ret >= len || errno != 0) {
			errp(215, __func__, "snprintf failed for %ld bytes for data%%0%dd.txt, returned: %d", len, digits,
			     snprintf_ret);
		}
		buf[len] = '\0';	// paranoia
	}

	/*
	 * return allocated buffer
	 */
	return buf;
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
	char *tmp;		// Copy of dir, //'s turned into /, trailing / removed
	char *p = NULL;		// Working pointer
	size_t len;		// Length of tmp copy of dir
	struct stat statbuf;	// Sub-directory status

	/*
	 * Check preconditions (firewall)
	 */
	if (dir == NULL) {
		err(216, __func__, "dir arg is NULL");
	}
	dbg(DBG_VHIGH, "called %s on path: %s", __func__, dir);

	/*
	 * Check if dir exists
	 */
	if (stat(dir, &statbuf) == 0) {
		if (S_ISDIR(statbuf.st_mode)) {
			// dir exists
			if (checkWritePermissions(dir)) {
				// All is good, nothing to do
				dbg(DBG_VHIGH, "dir is already a writable directory: %s", dir);
				return;
			} else {
				err(216, __func__, "dir exists but is not a writable directory: %s", dir);
			}

		} else {
			// dir is not a directory
			err(216, __func__, "dir exists but is not a directory: %s", dir);
		}
	}

	/*
	 * Copy dir into tmp converting multiple /'s into a single /
	 */
	len = strlen(dir); // Allocate max possible storage for tmp
	tmp = malloc(len + 1);	// +1 for paranoia below
	if (tmp == NULL) {
		errp(216, __func__, "unable to allocate a string of length %lu", len + 1);
	}
	tmp[len] = '\0';	// paranoia
	for (p = dir, len = 0; *p; ++p) {
		if (*p == '/' && *(p + 1) == '/') {
			++p;
		} else {
			tmp[len++] = *p;
		}
	}
	tmp[len] = '\0';

	// If final character(s) is a / replace it with NUL
	if (len > 0 && tmp[len - 1] == '/') {
		tmp[len--] = '\0';
	}
	dbg(DBG_VHIGH, "will use canonical path: %s", tmp);

	// In case empty path or just /
	if (len == 0 || (tmp[0] == '/' && len == 1)) {

		// Path was empty or just /'s, nothing to do
		free(tmp);
		dbg(DBG_VHIGH, "empty path or just /, nothing to do");
		return;
	}

	/*
	 * Iterate through directory string creating each directory
	 *
	 * NOTE: We start with the 2nd character because dir starts with /,
	 *       by definition of the filesystem / exists and does not need
	 *       to be created.  We handled the case where dir is only / above.
	 *       Finally if dir is just a single non-/ character, this loop
	 *       will terminate and will fall thru to the final mkdir at the
	 *       bottom of this function.
	 */
	for (p = tmp + 1; *p; p++) {

		// Check if we just passed a directory name in the path
		if (*p == '/') {

			// Terminate directory string at the current directory
			*p = '\0';
			dbg(DBG_VVHIGH, "working sub-path: %s", tmp);

			// Check if sub-path already exists
			if (stat(tmp, &statbuf) == 0) {
				if (S_ISDIR(statbuf.st_mode)) {
					// Sub-path is a dir, just keep going
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

			// New element in sub-path does not exist, so mkdir it
			dbg(DBG_VVHIGH, "about to mkdir %s", tmp);
			errno = 0;	// paranoia
			if (mkdir(tmp, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) < 0) {
				errp(216, __func__, "error creating %s for %s", tmp, dir);
			}
			dbg(DBG_VVHIGH, "just created %s", tmp);

			// Restore string for next possible directory
			*p = '/';
		}
	}

	/*
	 * Make final directory (wasn't created in loop due to replacing '/' or lack of one to begin with)
	 */
	dbg(DBG_VVHIGH, "about to do the final mkdir %s", tmp);
	errno = 0;		// paranoia
	if (mkdir(tmp, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) < 0) {
		errp(216, __func__, "error creating final %s for %s", tmp, dir);
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
 * but is not writable, then this function will report an error and not return.
 *
 * If the state->subDirs is false (-c), then dir will be checked if is exists.
 * If it does not exist, then this function will report an error and not return. If dir
 * exists but is not writable, then this function will report an error and not return.
 */
void
precheckPath(struct state *state, char *dir)
{
	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(217, __func__, "state arg is NULL");
	}
	if (dir == NULL) {
		err(217, __func__, "dir arg is NULL");
	}
	dbg(DBG_VHIGH, "called %s on path: %s", __func__, dir);

	/*
	 * no -c (state->subDirs is true)
	 *
	 * We are allowed to create missing directories.
	 */
	if (state->subDirs == true) {

		// Make the directory if it does not exist (or not writable)
		dbg(DBG_VVHIGH, "no -c, check if %s is a writeable directory", dir);
		if (checkWritePermissions(dir) != true) {
			dbg(DBG_VVHIGH, "it is not, so try to mkdir %s", dir);
			makePath(dir);
			dbg(DBG_VVHIGH, "directory is writable %s", dir);
			return;
		}
	}

	/*
	 * If -c, then verify that directory exists and is writable
	 */
	dbg(DBG_VVHIGH, "with -c, %s must be a writeable directory", dir);
	if (checkWritePermissions(dir) != true) {
		err(217, __func__, "directory does not exist or is not writable: %s", dir);
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
 *      Malloced path to the given subdir
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

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(218, __func__, "state arg is NULL");
	}
	if (subdir == NULL) {
		err(218, __func__, "subdir arg is NULL");
	}
	if (state->workDir == NULL) {
		err(218, __func__, "state->workDir is NULL");
	}

	/*
	 * Form full path
	 */
	fullpath = filePathName(state->workDir, subdir);
	dbg(DBG_VHIGH, "will verify that subdir exists and is writable: %s", fullpath);

	/*
	 * Ensure that a workDir sub-directory exists and is writable
	 */
	precheckPath(state, fullpath);

	/*
	 * Return malloced fullpath
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
	 * Check preconditions (firewall)
	 */
	if (success_p == NULL) {
		err(219, __func__, "success_p arg is NULL");
	}
	if (string == NULL) {
		err(219, __func__, "string arg is NULL");
	}

	/*
	 * Attempt to convert to a number
	 */
	errno = 0;
	number = strtol(string, NULL, 0);
	saved_errno = errno;
	if (saved_errno != 0) {

		// Cleanup and report failure
		dbg(DBG_LOW, "error in parsing string to integer: '%s'", string);
		*success_p = false;
		return saved_errno;
	}

	/*
	 * Cleanup and report success
	 */
	*success_p = true;
	return number;
}


/*
 * generatorOptions - determine generation options
 *
 * given:
 *      state           // pointer to run state
 *
 * In classic (non-batch) mode we prompt the user for a generator number.  In the
 * case of generator 0 (read from a file) we also ask for the filename and verify
 * that the file is readable.
 *
 * In batch more we just check if, for generator 0 (read from a file) the filename
 * is readable.
 *
 * This function does not return on error.
 */
void
generatorOptions(struct state *state)
{
	bool format_success;	// If we read a valid file format
	bool filename_success;	// If we obtains a valid readable file
	char *src;		// Potential src of random file data
	char *line;		// Random file data line

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(221, __func__, "state arg is NULL");
	}

	/*
	 * special processing for reading randdata from stdin
	 */
	if (state->stdinData == true) {

		// setup to read randdata from stdin
		state->streamFile = stdin;
		if (state->streamFile == NULL) {
			err(221, __func__, "stdin is NULL");
		}

		// reading randdata from stdin, nothing more to do here
		return;
	}

	/*
	 * Verify the input file is readable
	 */
	if (checkReadPermissions(state->randomDataPath) == false) {
		err(221, __func__, "input data file not readable: %s", state->randomDataPath);
	}

	/*
	 * Open the input file for reading
	 */
	state->streamFile = fopen(state->randomDataPath, "r");
	if (state->streamFile == NULL) {
		errp(221, __func__, "unable to open data file to reading: %s", state->randomDataPath);
	}

	/*
	 * Batch mode, nothing more to do here
	 */
	if (state->batchmode == true) {
		return;
	}

	/*
	 * Non-batch mode, ask for generator selection
	 */
	filename_success = false;
	do {
		// For generator 0 (read from a file) and no -f randdata, ask for the filename
		if (state->randomDataArg == false) {

			// Ask for the filename
			printf("\t\tUser Prescribed Input File: ");
			fflush(stdout);

			// Read filename
			src = getString(stdin);
			putchar('\n');

			// Sanity check to see if we can read the file
			if (checkReadPermissions(src) != true) {
				printf("\nfile: %s does not exist or is not readable, try again\n\n", src);
				fflush(stdout);
				free(src);
				continue;
			}

			// Set the randomData filename
			state->randomDataPath = src;
		}

		// Open the input file for reading
		state->streamFile = fopen(state->randomDataPath, "r");
		if (state->streamFile == NULL) {
			printf("\nunable to open %s of reading, try again\n\n", state->randomDataPath);
			fflush(stdout);
			if (state->randomDataArg == false) {
				free(state->randomDataPath);
				state->randomDataPath = NULL;
				filename_success = false;
			}
			continue;
		}
		filename_success = true;

		// Ask for input format if -F was NOT used
		if (state->dataFormatFlag == false) {

			// Determine generator 0 (read from a file), open the file format
			format_success = false;
			do {
				// Ask for input file format
				printf("   Input File Format:\n");
				printf("    [0] or [a] ASCII - A sequence of ASCII 0's and 1's\n");
				printf("    [1] or [r] Raw binary - Each byte in data file contains 8 bits of data\n\n");
				printf("   Select input mode:  ");
				fflush(stdout);

				// Read answer
				line = getString(stdin);
				putchar('\n');

				// Parse answer
				switch (line[0]) {
				case '0':
					format_success = true;
					state->dataFormat = FORMAT_ASCII_01;
					break;
				case '1':
					format_success = true;
					state->dataFormat = FORMAT_RAW_BINARY;
					break;
				case FORMAT_RAW_BINARY:
				case FORMAT_ASCII_01:
					format_success = true;
					state->dataFormat = (enum format) line[0];
					break;
				default:
					printf("\ninput must be %d of %c, %d or %c\n\n", 0, (char) FORMAT_ASCII_01, 1,
					       (char) FORMAT_RAW_BINARY);
					fflush(stdout);
					break;
				}
			} while (format_success != true);
			free(line);
		}

	} while (filename_success == false);
	return;
}


/*
 * chooseTests - determine tests to enable
 *
 * given:
 *      state           // pointer to run state
 *
 * Ask the user which tests to enable.
 *
 * This function does not return on error.
 */
void
chooseTests(struct state *state)
{
	long int run_all;	// Whether the
	char *run_test;
	bool success;		// true if parse in the input line was successful
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(223, __func__, "state arg is NULL");
	}

	/*
	 * If -t was used, tests are already chosen, just return
	 * If reading from randata stdin, we cannot ask for tests so just return
	 */
	if (state->testVectorFlag == true || state->stdinData == true) {
		return;
	}

	// Give instructions
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

	do {
		// Ask question
		printf("   Enter Choice: ");
		fflush(stdout);

		// Read numeric answer
		run_all = getNumber(stdin, stdout);
		putchar('\n');

		success = true;	// hope for the best
		switch (run_all) {
		case 0:
			state->testVector[0] = false;
			break;
		case 1:
			state->testVector[0] = true;
			break;
		default:
			success = false;	// must ask again
			printf("\nUnexpected answer, please enter only 0 or 1\n");
			fflush(stdout);
			break;
		}

	} while (success != true);

	putchar('\n');
	if (state->testVector[0] == true) {
		for (i = 1; i <= NUMOFTESTS; i++) {
			state->testVector[i] = true;
		}
	} else {
		do {
			// Ask for individual test numbers
			printf("         INSTRUCTIONS\n");
			printf("            Enter a 0 or 1 to indicate whether or not the numbered statistical\n");
			printf("            test should be applied to each sequence.\n\n");
			printf("               111111\n");
			printf("      123456789012345\n");
			printf("      ");
			fflush(stdout);

			// Read the answer string
			run_test = getString(stdin);
			putchar('\n');

			// Parse 0's and 1's
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
					success = false;	// must ask again
					printf("\nYou must enter a 0 or 1 for all %d tests\n\n", NUMOFTESTS);
					fflush(stdout);
					break;
				default:
					success = false;	// must ask again
					printf("\nUnexpected character for test %d, enter only 0 or 1 for each test\n", i);
					fflush(stdout);
					break;
				}
			}
		} while (success != true);
	}

	return;
}


/*
 * fixParameters - adjust test parameters
 *
 * given:
 *      state           // pointer to run state
 *
 * Ask the user if he wants to update the test parameters.
 *
 * This function does not return on error.
 */
void
fixParameters(struct state *state)
{
	int testid;	// Number of the test whose parameters are being changed

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(223, __func__, "state arg is NULL");
	}

	/*
	 * If reading from randata stdin, we cannot ask for parameters so just return
	 */
	if (state->stdinData == true) {
		return;
	}

	/*
	 * Ask for parameters
	 *
	 * We skip prompting any parameter that is only associated with a non-selected test
	 */
	do {
		/*
		 * Print instructions
		 */
		printf("        P a r a m e t e r   A d j u s t m e n t s\n");
		printf("        -----------------------------------------\n");
		if (state->testVector[TEST_BLOCK_FREQUENCY] == true) {
			printf("    [%d] Block Frequency Test - block length(M):         %ld\n",
			       PARAM_blockFrequencyBlockLength, state->tp.blockFrequencyBlockLength);
		}
		if (state->testVector[TEST_NON_OVERLAPPING] == true) {
			printf("    [%d] NonOverlapping Template Test - block length(m): %ld\n",
			       PARAM_nonOverlappingTemplateBlockLength, state->tp.nonOverlappingTemplateLength);
		}
		if (state->testVector[TEST_OVERLAPPING] == true) {
			printf("    [%d] Overlapping Template Test - block length(m):    %ld\n",
			       PARAM_overlappingTemplateBlockLength, state->tp.overlappingTemplateLength);
		}
		if (state->testVector[TEST_APEN] == true) {
			printf("    [%d] Approximate Entropy Test - block length(m):     %ld\n",
			       PARAM_approximateEntropyBlockLength, state->tp.approximateEntropyBlockLength);
		}
		if (state->testVector[TEST_SERIAL] == true) {
			printf("    [%d] Serial Test - block length(m):                  %ld\n",
			       PARAM_serialBlockLength, state->tp.serialBlockLength);
		}
		if (state->testVector[TEST_LINEARCOMPLEXITY] == true) {
			printf("    [%d] Linear Complexity Test - block length(M):       %ld\n",
			       PARAM_linearComplexitySequenceLength, state->tp.linearComplexitySequenceLength);
		}
		printf("    [%d] bitstream iterations:				%ld\n", PARAM_numOfBitStreams,
		       state->tp.numOfBitStreams);
		printf("    [%d] Uniformity bins:				%ld\n", PARAM_uniformity_bins,
		       state->tp.uniformity_bins);
		printf("    [%d] Length of a single bit stream:			%ld\n", PARAM_n, state->tp.n);
		printf("    [%d] Uniformity Cutoff Level:			%f\n", PARAM_uniformity_level,
		       state->tp.uniformity_level);
		printf("    [%d] Alpha - confidence level:       		%f\n", PARAM_alpha, state->tp.alpha);
		putchar('\n');
		printf("   Select Test (%d to continue): ", PARAM_continue);
		fflush(stdout);

		// Read numeric answer
		testid = (int) getNumber(stdin, stdout);
		putchar('\n');

		/*
		 * Ask for value according to testid
		 */
		switch (testid) {

		case PARAM_continue:
			break;	// Stop asking

		case PARAM_blockFrequencyBlockLength:
			do {
				// Ask for new value
				printf("   Enter Block Frequency Test block length (try: %d): ", DEFAULT_BITCOUNT);
				fflush(stdout);

				// Read numeric answer
				state->tp.blockFrequencyBlockLength = getNumber(stdin, stdout);
				putchar('\n');

				// Check error range
				if (state->tp.blockFrequencyBlockLength <= 0) {
					printf("    Block Frequency Test block length %ld must be > 0, try again\n\n",
					       state->tp.blockFrequencyBlockLength);
				}
			} while (state->tp.blockFrequencyBlockLength <= 0);
			break;

		case PARAM_nonOverlappingTemplateBlockLength:
			do {
				// Ask for new value
				printf("   Enter NonOverlapping Template Test block Length (try: %d): ", DEFAULT_OVERLAPPING);
				fflush(stdout);

				// Read numeric answer
				state->tp.nonOverlappingTemplateLength = getNumber(stdin, stdout);
				putchar('\n');

				// Check error range
				if (state->tp.nonOverlappingTemplateLength < MINTEMPLEN) {
					printf("    NonOverlapping Template Test block Length %ld must be >= %d, try again\n\n",
					       state->tp.nonOverlappingTemplateLength, MINTEMPLEN);
				}
				if (state->tp.nonOverlappingTemplateLength > MAXTEMPLEN) {
					printf("    NonOverlapping Template Test block Length %ld must be <= %d, try again\n\n",
					       state->tp.nonOverlappingTemplateLength, MAXTEMPLEN);
				}
			} while ((state->tp.nonOverlappingTemplateLength < MINTEMPLEN) ||
				 (state->tp.nonOverlappingTemplateLength > MAXTEMPLEN));
			break;

		case PARAM_overlappingTemplateBlockLength:
			do {
				// Ask for new value
				printf("   Enter Overlapping Template Test block Length (try: %d): ", DEFAULT_OVERLAPPING);
				fflush(stdout);

				// Read numeric answer
				state->tp.overlappingTemplateLength = getNumber(stdin, stdout);
				putchar('\n');

				// Check error range
				if (state->tp.overlappingTemplateLength <= 0) {
					printf("    Overlapping Template Test block Length %ld must be > 0, try again\n\n",
					       state->tp.overlappingTemplateLength);
				}
			} while (state->tp.overlappingTemplateLength <= 0);
			break;

		case PARAM_approximateEntropyBlockLength:
			do {
				// Ask for new value
				printf("   Enter Approximate Entropy Test block Length (try: %d): ", DEFAULT_APEN);
				fflush(stdout);

				// Read numeric answer
				state->tp.approximateEntropyBlockLength = getNumber(stdin, stdout);
				putchar('\n');

				// Check error range
				if (state->tp.approximateEntropyBlockLength <= 0) {
					printf("    Approximate Entropy Test block Length %ld must be > 0, try again\n\n",
					       state->tp.approximateEntropyBlockLength);
				}
			} while (state->tp.approximateEntropyBlockLength <= 0);
			break;

		case PARAM_serialBlockLength:
			do {
				// Ask for new value
				printf("   Enter Serial Test block Length (try: %d): ", DEFAULT_SERIAL);
				fflush(stdout);

				// Read numeric answer
				state->tp.serialBlockLength = getNumber(stdin, stdout);
				putchar('\n');

				// Check error range
				if (state->tp.serialBlockLength <= 0) {
					printf("    Serial Test block Length %ld must be > 0, try again\n\n",
					       state->tp.serialBlockLength);
				}
			} while (state->tp.serialBlockLength <= 0);
			break;

		case PARAM_linearComplexitySequenceLength:
			do {
				// Ask for new value
				printf("   Enter Linear Complexity Test block Length (try: %d): ", DEFAULT_LINEARCOMPLEXITY);
				fflush(stdout);

				// Read numeric answer
				state->tp.linearComplexitySequenceLength = getNumber(stdin, stdout);
				putchar('\n');

				// Check error range
				if (state->tp.linearComplexitySequenceLength < MIN_M_LINEARCOMPLEXITY) {
					printf("    linear complexith sequence length(M): %ld must be >= %d, try again\n\n",
					       state->tp.linearComplexitySequenceLength, MIN_M_LINEARCOMPLEXITY);
				} else if (state->tp.linearComplexitySequenceLength > MAX_M_LINEARCOMPLEXITY + 1) {
					printf("    linear complexith sequence length(M): %ld must be <= %d, try again\n\n",
					       state->tp.linearComplexitySequenceLength, MAX_M_LINEARCOMPLEXITY);
				}
			} while ((state->tp.linearComplexitySequenceLength < MIN_M_LINEARCOMPLEXITY) ||
				 (state->tp.linearComplexitySequenceLength > MAX_M_LINEARCOMPLEXITY + 1));
			break;

		case PARAM_numOfBitStreams:
			do {
				// Ask for new value
				printf("   Enter the number bitstream iterations (try: %d): ", DEFAULT_UNIFORMITY_BINS);
				fflush(stdout);

				// Read numeric answer
				state->tp.numOfBitStreams = getNumber(stdin, stdout);
				putchar('\n');

				// Check error range
				if (state->tp.numOfBitStreams <= 0) {
					printf("    Number of uniformity bins %ld must be > 0, try again\n\n",
					       state->tp.numOfBitStreams);
				}
			} while (state->tp.numOfBitStreams <= 0);
			break;

		case PARAM_uniformity_bins:
			do {
				// Ask for new value
				printf("   Enter the number of uniformity bins (try: %d): ", DEFAULT_UNIFORMITY_BINS);
				fflush(stdout);

				// Read numeric answer
				state->tp.uniformity_bins = getNumber(stdin, stdout);
				putchar('\n');

				// Check error range
				if (state->tp.uniformity_bins <= 0) {
					printf("    Number of uniformity bins %ld must be > 0, try again\n\n",
					       state->tp.uniformity_bins);
				}
			} while (state->tp.uniformity_bins <= 0);
			break;

		case PARAM_n:
			do {
				// Ask for new value
				printf("   Enter the length of a single bit stream (try: %d): ", DEFAULT_BITCOUNT);
				fflush(stdout);

				// Read numeric answer
				state->tp.n = getNumber(stdin, stdout);
				putchar('\n');

				// Check error range
				if (state->tp.n < GLOBAL_MIN_BITCOUNT) {
					printf("    Length of a single bit stream %ld must be >= %d, try again\n\n",
					       state->tp.n, GLOBAL_MIN_BITCOUNT);
				}
			} while (state->tp.n < GLOBAL_MIN_BITCOUNT);
			break;

		case PARAM_uniformity_level:
			do {
				// Ask for new value
				printf("   Enter Uniformity Cutoff Level (try: %f): ", DEFAULT_UNIFORMITY_LEVEL);
				fflush(stdout);

				// Read numeric answer
				state->tp.uniformity_level = getDouble(stdin, stdout);
				putchar('\n');

				// Check error range
				if (state->tp.uniformity_level <= 0.0) {
					printf("    Uniformity Cutoff Level must be > 0.0: %f, try again\n\n",
					       state->tp.uniformity_level);
				} else if (state->tp.uniformity_level > 0.1) {
					printf("    Uniformity Cutoff Level must be <= 0.1: %f, try again\n\n",
					       state->tp.uniformity_level);
				}
			} while (state->tp.uniformity_level <= 0.0 || state->tp.uniformity_level > 0.1);
			break;

		// Ask for new value is for any test
		case PARAM_alpha:
			do {
				// Ask for new value
				printf("   Enter Alpha Confidence Level (try: %f): ", DEFAULT_ALPHA);
				fflush(stdout);

				// Read numeric answer
				state->tp.alpha = getDouble(stdin, stdout);
				putchar('\n');

				// Check error range
				if (state->tp.alpha <= 0.0) {
					printf("    Alpha must be > 0.0: %f, try again\n\n", state->tp.alpha);
				} else if (state->tp.alpha > 0.1) {
					printf("    Alpha must be <= 0.1: %f, try again\n\n", state->tp.alpha);
				}
			} while (state->tp.alpha <= 0.0 || state->tp.alpha > 0.1);
			break;

		default:
			printf("   parameter number must be between 0 and %d, try again\n", MAX_PARAM);
			fflush(stdout);
			break;
		}
	} while (testid != PARAM_continue);
	return;
}


void
invokeTestSuite(struct state *state)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(228, __func__, "state arg is NULL");
	}

	/*
	 * Announce test if in legacy output mode
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(state->freqFile,
				 "________________________________________________________________________________\n\n");
		if (io_ret <= 0) {
			errp(228, __func__, "error in writing to %s", state->freqFilePath);
		}

		io_ret = fprintf(state->freqFile, "\t\tFILE = %s\t\tALPHA = %6.4f\n",
						 state->randomDataPath, state->tp.alpha);
		if (io_ret <= 0) {
			errp(228, __func__, "error in writing to %s", state->freqFilePath);
		}
		io_ret = fprintf(state->freqFile,
				 "________________________________________________________________________________\n\n");
		if (io_ret <= 0) {
			errp(228, __func__, "error in writing to %s", state->freqFilePath);
		}
		io_ret = fflush(state->freqFile);
		if (io_ret != 0) {
			errp(228, __func__, "error flushing to %s", state->freqFilePath);
		}
	}

	/*
	 * Test data from a file or from an internal generator
	 * NOTE: Introduce new pseudo random number generators in this switch
	 */
	handleFileBasedBitStreams(state);
}


static void
handleFileBasedBitStreams(struct state *state)
{
	int io_ret;		// I/O return status
	long int i;
	pthread_t thread[state->numberOfThreads];
	pthread_attr_t attr;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	struct thread_state *thread_args = malloc(state->numberOfThreads * sizeof(struct thread_state));
	void *status;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(224, __func__, "state arg is NULL");
	}

	/*
	 * when reading randdata from stdin, we do not seek no matter what our jobnum is
	 */
	if (state->stdinData == true) {
		state->base_seek = 0;
	}

	/*
	 * Compute seek position into the input file according to the jobnum parameter given.
	 *
	 * The position where to seek depends on the data format. If the input is made of
	 * ASCII 0 and 1 characters then we can seek by counting 1 position as 1 bit.
	 */
	else if (state->dataFormat == FORMAT_ASCII_01) {
		state->base_seek = state->jobnum * state->tp.n * state->tp.numOfBitStreams;
	}

	/*
	 * If the input is made of binary data we need to get count that each position is 8 bits.
	 */
	else if (state->dataFormat == FORMAT_RAW_BINARY) {

		/*
		 * Get number of bytes that hold a given set of consecutive bits.
		 *
		 * We need to round up the byte count to the next whole byte.
		 * However if the bit count is a multiple of 8, then we do not increase it.
		 * We only increase by one in the case of a final partial byte.
		 */
		state->base_seek = ((state->jobnum * state->tp.n * state->tp.numOfBitStreams) + BITS_N_BYTE - 1) / BITS_N_BYTE;
	}

	/*
	 * Initialize and set thread detached attribute
	 */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	dbg(DBG_LOW, "Start of iterate phase");

	/*
	 * Run numberOfThreads threads
	 */
	for (i = 0; i < state->numberOfThreads; i++) {
		thread_args[i].global_state = state;
		thread_args[i].thread_id = i;
		thread_args[i].mutex = &mutex;

		io_ret = pthread_create(&thread[i], &attr, testBits, &thread_args[i]);
		if (io_ret != 0) {
			errp(224, __func__, "error on pthread_create()");
		}
	}

	dbg(DBG_HIGH, "All threads created and running. Will wait for them.");

	/*
	 * Free attribute and wait for the threads to finish
	 */
	pthread_attr_destroy(&attr);
	for (i = 0; i < state->numberOfThreads; i++) {
		io_ret = pthread_join(thread[i], &status);
		if (io_ret != 0) {
			errp(224, __func__, "error on pthread_join()");
		}
	}
	pthread_mutex_destroy(&mutex);

	dbg(DBG_LOW, "End of iterate phase\n");

	/*
	 * Close the input file
	 */
	errno = 0;	// paranoia
	io_ret = fclose(state->streamFile);
	if (io_ret != 0) {
		errp(224, __func__, "error closing: %s", state->randomDataPath);
	}
	state->streamFile = NULL;

	return;
}


static void
*testBits(void *thread_args)
{
	struct thread_state *thread_state = (struct thread_state *) thread_args;
	char buf[BUFSIZ + 1];	// time string buffer

	/*
	 * Check preconditions (firewall)
	 */
	if (thread_state == NULL) {
		err(225, __func__, "thread_state arg is NULL");
	}
	struct state *state = thread_state->global_state;
	if (state == NULL) {
		err(225, __func__, "state arg is NULL");
	}

	dbg(DBG_HIGH, "Thread %ld started.", thread_state->thread_id);

	while (1) {
		pthread_mutex_lock(thread_state->mutex);

		if (state->iterationsMissing == 0) {
			pthread_mutex_unlock(thread_state->mutex);
			break;
		}

		thread_state->iteration_being_done = state->tp.numOfBitStreams - state->iterationsMissing;
		state->iterationsMissing -= 1;

		/*
		 * Parse and data for this iteration
		 */
		if (state->dataFormat == FORMAT_ASCII_01) {
			parseBitsASCIIInput(thread_state);
		} else {
			parseBitsBinaryInput(thread_state);
		}

		pthread_mutex_unlock(thread_state->mutex);

		/*
		 * Perform one iteration on the bitstreams read from the streamFile
		 */
		iterate(thread_state);

		/*
		 * Report iteration done (if requested)
		 */
		if (state->reportCycle > 0 && (((thread_state->iteration_being_done % state->reportCycle) == 0) ||
					       (thread_state->iteration_being_done == state->tp.numOfBitStreams))) {
			getTimestamp(buf, BUFSIZ);
			msg("Completed iteration %ld of %ld at %s", thread_state->iteration_being_done + 1,
			    state->tp.numOfBitStreams, buf);
		}
	}

	pthread_exit((void *) thread_state->thread_id);
}


/*
 * parseBitsASCIIInput - read bits from the streamFile and save them into epsilon bit array
 *
 * given:
 *      state           // pointer to run state
 *
 * Given the open steam streamFile, from file state->randomDataPath, convert its ASCII characters
 * into 'bits' for the epsilon bit array.
 */
static void
parseBitsASCIIInput(struct thread_state *thread_state)
{
	long int i;
	long int num_0s;
	long int num_1s;
	long int bitsRead;
	int bit;
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (thread_state == NULL) {
		err(225, __func__, "thread_state arg is NULL");
	}
	struct state *state = thread_state->global_state;
	if (state == NULL) {
		err(225, __func__, "state arg is NULL");
	}
	if (state->streamFile == NULL) {
		err(225, __func__, "streamFile arg is NULL");
	}
	if (state->epsilon[thread_state->thread_id] == NULL) {
		err(227, __func__, "state->epsilon[%ld] is NULL", thread_state->thread_id);
	}

	/*
	 * If not reading randdata from stdin,
	 * Seek to the position of the first bit which has not been copied into the stream yet
	 */
	if (state->stdinData == false &&
	    fseek(state->streamFile, state->base_seek + thread_state->iteration_being_done * state->tp.n, SEEK_SET) != 0) {
		errp(226, __func__, "could not seek %ld further into file: %s",
		     (thread_state->iteration_being_done * state->tp.n), state->randomDataPath);
	}

	/*
	 * Copy the next n bits from the streamFile to epsilon
	 */
	num_0s = 0;
	num_1s = 0;
	bitsRead = 0;
	for (i = 0; i < state->tp.n; i++) {
		io_ret = fscanf(state->streamFile, "%1d", &bit);
		if (io_ret == EOF) {
			warn(__func__, "Insufficient data in file %s: %ld bits were read", state->randomDataPath,
			     bitsRead);
			return;
		} else {
			bitsRead++;
			if (bit == 0) {
				num_0s++;
			} else {
				num_1s++;
			}
			state->epsilon[thread_state->thread_id][i] = (BitSequence) bit;
		}
	}

	/*
	 * Write stats to freq.txt if in legacy_output mode
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
		if (io_ret <= 0) {
			errp(225, __func__, "error in writing to %s", state->freqFilePath);
		}
		io_ret = fflush(state->freqFile);
		if (io_ret != 0) {
			errp(225, __func__, "error flushing to %s", state->freqFilePath);
		}
	}

	return;
}


/*
 * parseBitsBinaryInput - read bits from the streamFile and convert them into epsilon bit array
 *
 * given:
 *      state           // pointer to run state
 *
 * Given the open steam streamFile, from file state->randomDataPath, convert its bytes into 'bits'
 * found in the epsilon bit array.
 */
static void
parseBitsBinaryInput(struct thread_state *thread_state)
{
	long int num_0s;	// Count of 0 bits processed
	long int num_1s;	// Count of 1 bits processed
	long int bitsRead;	// Number of bits to read and process
	bool done;		// true ==> we have converted enough data
	BYTE byte;		// single bite
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (thread_state == NULL) {
		err(225, __func__, "thread_state arg is NULL");
	}
	struct state *state = thread_state->global_state;
	if (state == NULL) {
		err(226, __func__, "state arg is NULL");
	}
	if (state->streamFile == NULL) {
		err(226, __func__, "streamFile arg is NULL");
	}

	/*
	 * If not reading randdata from stdin,
	 * Seek to the position of the first bit which has not been copied into the stream yet
	 */
	if (state->stdinData == false &&
	    fseek(state->streamFile, state->base_seek + thread_state->iteration_being_done * state->tp.n /
							BITS_N_BYTE, SEEK_SET) != 0) {

		errp(226, __func__, "could not seek %ld further into file: %s",
		     thread_state->iteration_being_done * state->tp.n / BITS_N_BYTE, state->randomDataPath);
	}

	/*
	 * Copy the next n bits from the streamFile to epsilon
	 */
	num_0s = 0;
	num_1s = 0;
	bitsRead = 0;
	do {
		/*
		 * Read the next binary octet
		 */
		io_ret = fgetc(state->streamFile);
		if (io_ret < 0) {
			errp(226, __func__, "read error in stream file: %s", state->randomDataPath);
		}
		byte = (BYTE) io_ret;

		/*
		 * Add bits of the octet to the epsilon bit stream
		 */
		done = copyBitsToEpsilon(state, thread_state->thread_id, &byte, BITS_N_BYTE, &num_0s, &num_1s, &bitsRead);
	} while (done == false);

	/*
	 * Write stats to freq.txt if in legacy_output mode
	 */
	if (state->legacy_output == true) {
		io_ret = fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
		if (io_ret <= 0) {
			errp(226, __func__, "error in writing to %s", state->freqFilePath);
		}
		io_ret = fflush(state->freqFile);
		if (io_ret != 0) {
			errp(226, __func__, "error flushing to %s", state->freqFilePath);
		}
	}

	return;
}


/*
 * copyBitsToEpsilon - convert binary bytes into the end of an epsilon bit array
 *
 * given:
 *      state           // pointer to run state
 *      x               // pointer to an array (even just 1) binary bytes
 *      xBitLength      // Number of bits to convert
 *      bitsNeeded      // Total number of bits we want to convert this run
 *      num_0s          // pointer to number of 0 bits converted so far
 *      num_1s          // pointer to number of 1 bits converted so far
 *      bitsRead        // pointer to number of bits converted so far
 *
 * returns:
 *      true ==> we have converted enough bits
 *      false ==> we have NOT converted enough bits, yet
 */
bool
copyBitsToEpsilon(struct state *state, long int thread_id, BYTE *x, long int xBitLength, long int *num_0s, long int *num_1s,
		  long int *bitsRead)
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
	if (state == NULL) {
		err(227, __func__, "state arg is NULL");
	}
	if (state->epsilon[thread_id] == NULL) {
		err(227, __func__, "state->epsilon[%ld] is NULL", thread_id);
	}

	bitsNeeded = state->tp.n;

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
			state->epsilon[thread_id][*bitsRead] = (BitSequence) bit;
			(*bitsRead)++;
			if (*bitsRead == bitsNeeded) {
				return true;
			}
			if (++count == xBitLength) {
				return false;
			}
		}
	}

	return false;
}


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
void
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


void write_p_val_to_file(struct state *state)
{
	long int i, j;
	char *filename, *work_filepath, *final_filepath;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(232, __func__, "state arg was NULL");
	}

	/*
	 * Compute the filename of the working file (.work)
	 */
	asprintf(&filename, "sts.%04ld.%ld.%ld.work", state->jobnum, state->tp.numOfBitStreams, state->tp.n);
	work_filepath = filePathName(state->workDir, filename);
	free(filename);

	/*
	 * Create and open the working binary file
	 */
	FILE *p_val_file;
	p_val_file = fopen(work_filepath, "wb");

	/*
	 * Write the p-values of each test in the working file
	 */
	for (i = 1; i <= NUMOFTESTS; i++) {
		if (state->testVector[i] == true) {

			/*
			 * Write the number of the test
			 */
			fwrite(&i, sizeof(i), 1, p_val_file);

			/*
			 * Write the number of p-values for this test
			 */
			fwrite(&(state->p_val[i]->count), sizeof(state->p_val[i]->count), 1, p_val_file);

			/*
			 * Write the p-values
			 */
			for (j = 0; j < state->p_val[i]->count; j++) {

				double p_val;

				/*
				 * Also when the test is NON_OVERLAPPING, take the p-value only.
				 */
				if (i != TEST_NON_OVERLAPPING) {
					p_val = get_value(state->p_val[i], double, j);
				} else {
					struct nonover_stats *nonov = addr_value(state->p_val[i], struct nonover_stats, j);
					p_val = nonov->p_value;
				}

				fwrite(&p_val, sizeof(p_val), 1, p_val_file);
			}
		}
	}

	/*
	 * Close the "working" file
	 */
	fclose(p_val_file);

	/*
	 * Compute the final filename
	 */
	asprintf(&filename, "sts.%04ld.%ld.%ld.pvalues", state->jobnum, state->tp.numOfBitStreams, state->tp.n);
	final_filepath = filePathName(state->workDir, filename);

	/*
	 * Rename the work file (.work) to have its final filename (.pvalues)
	 */
	if (rename(work_filepath, final_filepath) < 0) {
		errp(232, __func__, "error in renaming %s to %s", work_filepath, final_filepath);
	}

	/*
	 * Free allocated memory
	 */
	free(filename);
	free(work_filepath);
	free(final_filepath);
}


void read_from_p_val_file(struct state *state)
{
	long int test_num, p_val_index;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(232, __func__, "state arg was NULL");
	}

	struct Node *current = state->filenames;
	while (current != NULL) {

		/*
		 * Open the file
		 */
		char *filename = current->filename;
		FILE *p_val_file = fopen(filePathName(state->pvalues_dir, filename), "rb");

		/*
		 * Read the first test number
		 */
		fread(&test_num, sizeof(test_num), 1, p_val_file);

		/*
		 * Read the content of the file
		 */
		while (!feof(p_val_file) && test_num <= NUMOFTESTS) {

			/*
			 * Read number of p-values for the current testnum
			 */
			long int number_of_p_vals;
			fread(&number_of_p_vals, sizeof(number_of_p_vals), 1, p_val_file);

			/*
			 * Read all the p-values for this test
			 */
			for (p_val_index = 0; p_val_index < number_of_p_vals; p_val_index++) {

				double p_val;
				fread(&p_val, sizeof(p_val), 1, p_val_file);

				/*
				 * Append each p-value read to the p-values of this test
				 */
				if (test_num != TEST_NON_OVERLAPPING) {
					append_value(state->p_val[test_num], &p_val);
				} else {
					struct nonover_stats nonov;
					nonov.p_value = p_val;
					append_value(state->p_val[test_num], &nonov);
				}
			}

			/*
			 * Read the next test number
			 */
			fread(&test_num, sizeof(test_num), 1, p_val_file);
		}

		/*
		 * Close the file that has been read
		 */
		fclose(p_val_file);
		current = current->next;
	}
}


/*
 * Appends the given string to the linked list which is pointed to by the given head
 */
void
append_string_to_linked_list(struct Node **head, char* string)
{
	struct Node *current = *head;

	/*
	 * Create the new node to append to the linked list
	 */
	struct Node *new_node = malloc(sizeof(*new_node));
	new_node->filename = strdup(string);
	new_node->next = NULL;

	/*
	 * If the linked list is empty, just make the head to be this new node
	 */
	if (current == NULL)
		*head = new_node;

	/*
	 * Otherwise, go till the last node and append the new node after it
	 */
	else {
		while (current->next != NULL)
			current = current->next;

		current->next = new_node;
	}
}


/*
 * sum_will_overflow_long - check if a sum operation will lead to overflow
 *
 * given:
 *      si_a		// the number to which we want to sum something
 *      si_b		// the addend that we want to add to si_a
 *
 * returns:
 *      1 if the operation would cause number to overflow, 0 otherwise
 */
int
sum_will_overflow_long(long int si_a, long int si_b)
{
	if (((si_b > 0) && (si_a > (LONG_MAX - si_b))) ||
	    ((si_b < 0) && (si_a < (LONG_MIN - si_b)))) {
		return 1;	// will overflow
	}

	return 0;		// will not overflow
}


/*
 * multiplication_will_overflow_long - check if a multiplication operation will lead to overflow
 *
 * given:
 *      si_a		// the number that we want to multiply by something
 *      si_b		// the multiplier by which we want to multiply si_a
 *
 * returns:
 *      1 if the operation would cause number to overflow, 0 otherwise
 */
int
multiplication_will_overflow_long(long int si_a, long int si_b)
{
	if (si_a > 0) {  /* si_a is positive */
		if (si_b > 0) {  /* si_a and si_b are positive */
			if (si_a > (LONG_MAX / si_b)) {
				return 1;	// will overflow
			}
		} else { /* si_a positive, si_b nonpositive */
			if (si_b < (LONG_MIN / si_a)) {
				return 1;	// will overflow
			}
		}
	} else { /* si_a is nonpositive */
		if (si_b > 0) { /* si_a is nonpositive, si_b is positive */
			if (si_a < (LONG_MIN / si_b)) {
				return 1;	// will overflow
			}
		} else { /* si_a and si_b are nonpositive */
			if ( (si_a != 0) && (si_b < (LONG_MAX / si_a))) {
				return 1;	// will overflow
			}
		}
	}

	return 0;		// will not overflow
}
