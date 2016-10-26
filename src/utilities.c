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
#include "debug.h"
#include "defs.h"


/*
 * Forward static function declarations
 */
static void readBinaryDigitsInASCIIFormat(struct state *state);
static void getTimestamp(char *buf, size_t len);


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
		err(210, __FUNCTION__, "input arg is NULL");
	}
	if (output == NULL) {
		err(210, __FUNCTION__, "output arg is NULL");
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
			errp(210, __FUNCTION__, "line is still NULL after getline call");
		}
		if (linelen < 0) {
			errp(210, __FUNCTION__, "getline returned: %ld", linelen);
		}
		// firewall
		if (line[linelen] != '\0') {
			err(210, __FUNCTION__, "getline did not return a NULL terminated string");
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

double
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
		err(211, __FUNCTION__, "input arg is NULL");
	}
	if (output == NULL) {
		err(211, __FUNCTION__, "output arg is NULL");
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
			errp(211, __FUNCTION__, "line is still NULL after getline call");
		}
		if (linelen < 0) {
			errp(211, __FUNCTION__, "getline returned: %ld", linelen);
		}
		/*
		 * paranoia
		 */
		if (line[linelen] != '\0') {
			err(211, __FUNCTION__, "getline did not return a NULL terminated string");
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
	size_t buflen = 0;	// Ignored when line == NULL
	ssize_t linelen;	// Length of line read from stream
	long int number;	// value to return

	/*
	 * Check preconditions (firewall)
	 */
	if (stream == NULL) {
		err(212, __FUNCTION__, "stream arg is NULL");
	}

	/*
	 * read the line
	 */
	linelen = getline(&line, &buflen, stream);
	if (line == NULL) {
		errp(212, __FUNCTION__, "line is still NULL after getline call");
	}
	if (linelen <= 0) {
		errp(212, __FUNCTION__, "getline returned: %ld", linelen);
	}
	if (line[linelen] != '\0') {
		err(212, __FUNCTION__, "getline did not return a NULL terminated string");
	}

	/*
	 * attempt to convert to a number
	 */
	errno = 0;
	number = strtol(line, NULL, 0);
	if (errno != 0) {
		errp(212, __FUNCTION__, "error in parsing string to integer: '%s'", line);
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
 *      NULL terminated line read from stream, w/o and trailing newline or return.
 *
 * This function does not return on error.
 */
char *
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
		err(213, __FUNCTION__, "stream arg is NULL");
	}

	/*
	 * read the line
	 */
	linelen = getline(&line, &buflen, stream);
	if (line == NULL) {
		errp(213, __FUNCTION__, "line is still NULL after getline call");
	}
	if (linelen <= 0) {
		errp(213, __FUNCTION__, "getline returned: %ld", linelen);
	}
	if (line[linelen] != '\0') {
		err(213, __FUNCTION__, "getline did not return a NULL terminated string");
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
 * openTruncate - check truncate and write permissions of a file
 *
 * given:
 *      filename         // the filename to check for truncate and write permissions
 *
 * retuen
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
		warn(__FUNCTION__, "filename arg was NULL");
		return NULL;
	}

	/*
	 * attempt to truncate file
	 */
	errno = 0;		// paranoia
	stream = fopen(filename, "w");
	if (stream == NULL) {
		warnp(__FUNCTION__, "could not create/open for writing/truncation: %s", filename);
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
	char *fullpath;		// dir/file
	size_t len;		// Length of fullpath
	int snprintf_ret;	// snprintf return value

	/*
	 * Check preconditions (firewall)
	 */
	if (head == NULL) {
		err(214, __FUNCTION__, "head arg is NULL");
	}
	if (tail == NULL) {
		err(214, __FUNCTION__, "tail arg is NULL");
	}

	/*
	 * Allocate full path including subdir
	 */
	len = strlen(head) + 1 + strlen(tail) + 1;
	fullpath = malloc(len + 1);	// +1 for later paranoia
	if (fullpath == NULL) {
		errp(214, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for fullpath", len + 1,
		     sizeof(fullpath[0]));
	}

	/*
	 * form full path
	 */
	errno = 0;		// paranoia
	snprintf_ret = snprintf(fullpath, len, "%s/%s", head, tail);
	fullpath[len] = '\0';	// paranoia
	if (snprintf_ret <= 0 || snprintf_ret >= BUFSIZ || errno != 0) {
		errp(214, __FUNCTION__, "snprintf failed for %ld bytes for %s/%s, returned: %d", len, head, tail, snprintf_ret);
	}

	/*
	 * return malloced path
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
	char *buf;		// allocated buffer
	int digits;		// decimal digits contained in partitionCount
	int len;		// Length of the format
	int snprintf_ret;	// snprintf return value

	/*
	 * Check preconditions (firewall)
	 */
	if (partitionCount < 1) {
		err(215, __FUNCTION__, "partitionCount arg: %d must be >= 1", partitionCount);
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
		len = strlen("data.txt") + 1;	// data.txt: + 1 for NUL
	} else if (digits < 10) {
		len = strlen("data%0") + 1 + strlen("d.txt") + 1;	// data%0[0-9]d.txt: + 1 for NUL
	} else {
		len = strlen("data%0") + 2 + strlen("d.txt") + 1;	// data%0[0-9][0-9]d.txt: + 1 for NUL
	}

	/*
	 * Allocate the format
	 */
	buf = malloc(len + 1);	// + 1 for paranoia
	if (buf == NULL) {
		errp(215, __FUNCTION__, "cannot malloc of %d elements of %ld bytes each for data%%0*d.txt", len + 1,
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
			errp(215, __FUNCTION__, "snprintf failed for %d bytes for data%%0%dd.txt, returned: %d", len, digits,
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
	// set tmp string to length of dir
	char *tmp;		// copy of dir, //'s turned into /, trailing / removed
	char *p = NULL;		// working pointer
	size_t len;		// Length of tmp copy of dir
	struct stat statbuf;	// sub-directory status

	// firewall
	if (dir == NULL) {
		err(216, __FUNCTION__, "dir arg is NULL");
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
				err(216, __FUNCTION__, "dir exists but is not a writable directory: %s", dir);
			}

		} else {
			// dir is not a directory
			err(216, __FUNCTION__, "dir exists but is not a directory: %s", dir);
		}
	}

	/*
	 * copy dir into tmp converting multiple /'s into a single /
	 */
	// allocate max possible storage for tmp
	len = strlen(dir);
	tmp = malloc(len + 1);	// +1 for paranoia below
	if (tmp == NULL) {
		errp(216, __FUNCTION__, "unable to allocate a string of length %lu", len + 1);
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
				errp(216, __FUNCTION__, "error creating %s for %s", tmp, dir);
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
		errp(216, __FUNCTION__, "error creating final %s for %s", tmp, dir);
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
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(217, __FUNCTION__, "state arg is NULL");
	}
	if (dir == NULL) {
		err(217, __FUNCTION__, "dir arg is NULL");
	}
	dbg(DBG_VHIGH, "called %s on path: %s", __FUNCTION__, dir);

	/*
	 * no -c (state->subDirs is true)
	 *
	 * We are allowed to create missing directories.
	 */
	if (state->subDirs == true) {

		// make the directory if it does not exist (or not writable)
		dbg(DBG_VVHIGH, "no -c, check if %s is a writeable directory", dir);
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
	dbg(DBG_VVHIGH, "with -c, %s must be a writeable directory", dir);
	if (checkWritePermissions(dir) != true) {
		err(217, __FUNCTION__, "directory does not exist or is not writable: %s", dir);
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
		err(218, __FUNCTION__, "state arg is NULL");
	}
	if (subdir == NULL) {
		err(218, __FUNCTION__, "subdir arg is NULL");
	}
	if (state->workDir == NULL) {
		err(218, __FUNCTION__, "state->workDir is NULL");
	}

	/*
	 * form full path
	 */
	fullpath = filePathName(state->workDir, subdir);
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
	 * Check preconditions (firewall)
	 */
	if (success_p == NULL) {
		err(219, __FUNCTION__, "success_p arg is NULL");
	}
	if (string == NULL) {
		err(219, __FUNCTION__, "string arg is NULL");
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
	 * Check preconditions (firewall)
	 */
	if (string == NULL) {
		err(220, __FUNCTION__, "string arg is NULL");
	}

	/*
	 * attempt to convert to a number
	 */
	errno = 0;
	number = strtol(string, NULL, 0);
	if (errno != 0) {
		errp(220, __FUNCTION__, "error in parsing string to integer: '%s'", string);
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
	long int generator;	// generator number
	bool success;		// if we read a valid file format
	char *src;		// potential src of random file data
	char *line;		// random file data line

	/*
	 * case: write to file (-m w)
	 */
	if (state->runMode == MODE_WRITE_ONLY) {
		// Open the input file for writing
		state->streamFile = fopen(state->randomDataPath, "w");
		if (state->streamFile == NULL) {
			errp(221, __FUNCTION__, "unable to open data file to writing: %s", state->randomDataPath);
		}
	}

	/*
	 * case: read from file
	 */
	else if (state->generator == GENERATOR_FROM_FILE) {
		// verify the input file is readable
		if (checkReadPermissions(state->randomDataPath) == false) {
			err(221, __FUNCTION__, "input data file not readable: %s", state->randomDataPath);
		}
		// Open the input file for reading
		state->streamFile = fopen(state->randomDataPath, "r");
		if (state->streamFile == NULL) {
			errp(221, __FUNCTION__, "unable to open data file to reading: %s", state->randomDataPath);
		}
	}

	/*
	 * batch mode, nothing to do here
	 */
	if (state->batchmode == true) {
		// nothing else to do
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
			printf("	       G E N E R A T O R    S E L E C T I O N \n");
			printf("	       ______________________________________\n\n");
			if (state->runMode == MODE_WRITE_ONLY) {
				printf("				   [1] Linear Congruential\n");
			} else {
				printf("    [0] Input File		   [1] Linear Congruential\n");
			}
			printf("	[2] Quadratic Congruential I   [3] Quadratic Congruential II\n");
			printf("	[4] Cubic Congruential	       [5] XOR\n");
			printf("	[6] Modular Exponentiation     [7] Blum-Blum-Shub\n");
			printf("	[8] Micali-Schnorr	       [9] G Using SHA-1\n\n");
			printf("   Enter Choice: ");
			fflush(stdout);

			// read number
			generator = getNumber(stdin, stdout);
			putchar('\n');

			// help if they gave us a wrong number
			if (generator < 0 || generator > NUMOFGENERATORS) {
				printf("\ngenerator number must be from 0 to %d inclusive, try again\n\n", NUMOFGENERATORS);
				fflush(stdout);
				continue;
			}
		}

		// cannot use read from file when in -m w runMode
		if (state->runMode == MODE_WRITE_ONLY && generator == GENERATOR_FROM_FILE) {
			printf("\nbecause -m w (write-only) was given, generator 0 (read from file) cannot be used, try again\n\n");
			fflush(stdout);
			continue;
		}
		// for non-zero generator and -m w runMode, ask for the filename
		if (state->runMode == MODE_WRITE_ONLY && generator != GENERATOR_FROM_FILE) {

			// if no -f randdata, ask for the filename
			if (state->randomDataFlag == false) {

				printf("\t\tOutput generator file: ");
				fflush(stdout);

				// read filename
				src = getString(stdin);
				putchar('\n');

				// Open the file for writing/truncation
				state->streamFile = openTruncate(src);
				if (state->streamFile == NULL) {
					printf("\ncould not create/open for writing/truncation: %s, try again\n\n", src);
					fflush(stdout);
					free(src);
					continue;
				}
				// set the randomData filename
				state->randomDataPath = src;

				// we have -f randdata, open randdata for writing/truncation
			} else {
				state->streamFile = openTruncate(state->randomDataPath);
				if (state->streamFile == NULL) {
					printf("\ncould not create/open for writing/truncation: %s, try again\n\n",
					       state->randomDataPath);
					fflush(stdout);
					continue;
				}
			}
		}
		// for generator 0 (read from a file) and no -f randdata, ask for the filename
		if (generator == GENERATOR_FROM_FILE && state->randomDataFlag == false) {

			// prompt
			printf("\t\tUser Prescribed Input File: ");
			fflush(stdout);

			// read filename
			src = getString(stdin);
			putchar('\n');

			// sanity check to see if we can read the file
			if (checkReadPermissions(src) != true) {
				printf("\nfile: %s does not exist or is not readable, try again\n\n", src);
				fflush(stdout);
				free(src);
				continue;
			}
			// set the randomData filename
			state->randomDataPath = src;
		}
		// for generator 0 (read from a file), open the file
		if (generator == GENERATOR_FROM_FILE) {
			// Open the input file for reading
			state->streamFile = fopen(state->randomDataPath, "r");
			if (state->streamFile == NULL) {
				printf("\nunable to open %s of reading, try again\n\n", state->randomDataPath);
				fflush(stdout);
				if (state->randomDataFlag == false) {
					free(state->randomDataPath);
					state->randomDataPath = NULL;
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
					printf("	[0] or [a] ASCII - A sequence of ASCII 0's and 1's\n");
					printf
					    ("	[1] or [r] Raw binary - Each byte in data file contains 8 bits of data\n\n");
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
						printf("\ninput must be %d of %c, %d or %c\n\n", 0, (char) FORMAT_ASCII_01, 1,
						       (char) FORMAT_RAW_BINARY);
						fflush(stdout);
						break;
					}
				} while (success != true);
				free(line);
			}
		}

	} while (generator < 0 || generator > NUMOFGENERATORS);

	// set the generator option
	dbg(DBG_LOW, "will use generator %s[%ld]", state->generatorDir[generator], generator);
	state->generator = (enum gen) generator;
	return;
}


void
chooseTests(struct state *state)
{
	long int run_all;
	char *run_test;
	bool success;		// if parse in the input line was successful
	int i;

	// firewall
	if (state == NULL) {
		err(222, __FUNCTION__, "state arg is NULL");
	}
	// If -t was used, tests are already chosen, just return
	if (state->testVectorFlag == true) {
		return;
	}
	// prompt
	printf("		    S T A T I S T I C A L   T E S T S\n");
	printf("		    _________________________________\n\n");
	printf("	[01] Frequency			     [02] Block Frequency\n");
	printf("	[03] Cumulative Sums		     [04] Runs\n");
	printf("	[05] Longest Run of Ones	     [06] Rank\n");
	printf("	[07] Discrete Fourier Transform	     [08] Nonperiodic Template Matchings\n");
	printf("	[09] Overlapping Template Matchings  [10] Universal Statistical\n");
	printf("	[11] Approximate Entropy	     [12] Random Excursions\n");
	printf("	[13] Random Excursions Variant	     [14] Serial\n");
	printf("	[15] Linear Complexity\n\n");
	printf("	     INSTRUCTIONS\n");
	printf("		Enter 0 if you DO NOT want to apply all of the\n");
	printf("		statistical tests to each sequence and 1 if you DO.\n\n");
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
			printf("	     INSTRUCTIONS\n");
			printf("		Enter a 0 or 1 to indicate whether or not the numbered statistical\n");
			printf("		test should be applied to each sequence.\n\n");
			printf("		   111111\n");
			printf("	  123456789012345\n");
			printf("	  ");
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
					fflush(stdout);
					break;
				default:
					success = false;	// must prompt again
					printf("\nUnexpcted character for test %d, enter only 0 or 1 for each test\n", i);
					fflush(stdout);
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
	int testid;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(223, __FUNCTION__, "state arg is NULL");
	}

	/*
	 * prompt for parameters
	 *
	 * We skip prompting any parameter that is only associated with a non-selected test
	 */
	do {
		/*
		 * prompt
		 */
		printf("	P a r a m e t e r   A d j u s t m e n t s\n");
		printf("	-----------------------------------------\n");
		if (state->testVector[TEST_BLOCK_FREQUENCY] == true) {
			printf("	[%d] Block Frequency Test - block length(M):	     %ld\n",
			       PARAM_blockFrequencyBlockLength, state->tp.blockFrequencyBlockLength);
		}
		if (state->testVector[TEST_NON_OVERLAPPING] == true) {
			printf("	[%d] NonOverlapping Template Test - block length(m): %ld\n",
			       PARAM_nonOverlappingTemplateBlockLength, state->tp.nonOverlappingTemplateLength);
		}
		if (state->testVector[TEST_OVERLAPPING] == true) {
			printf("	[%d] Overlapping Template Test - block length(m):    %ld\n",
			       PARAM_overlappingTemplateBlockLength, state->tp.overlappingTemplateLength);
		}
		if (state->testVector[TEST_APEN] == true) {
			printf("	[%d] Approximate Entropy Test - block length(m):     %ld\n",
			       PARAM_approximateEntropyBlockLength, state->tp.approximateEntropyBlockLength);
		}
		if (state->testVector[TEST_SERIAL] == true) {
			printf("	[%d] Serial Test - block length(m):		     %ld\n",
			       PARAM_serialBlockLength, state->tp.serialBlockLength);
		}
		if (state->testVector[TEST_LINEARCOMPLEXITY] == true) {
			printf("	[%d] Linear Complexity Test - block length(M):	     %ld\n",
			       PARAM_linearComplexitySequenceLength, state->tp.linearComplexitySequenceLength);
		}
		printf("    [%d] bitstream iterations:				%ld\n", PARAM_numOfBitStreams,
		       state->tp.numOfBitStreams);
		printf("    [%d] Uniformity bins:				%ld\n", PARAM_uniformity_bins,
		       state->tp.uniformity_bins);
		printf("    [%d] Length of a single bit stream:			%ld\n", PARAM_n, state->tp.n);
		printf("    [%d] Uniformity Cutoff Level:			%f\n", PARAM_uniformity_level,
		       state->tp.uniformity_level);
		printf("    [%d] Alpha - confidence level:			%f\n", PARAM_alpha, state->tp.alpha);
		putchar('\n');
		printf("   Select Test (%d to continue): ", PARAM_continue);
		fflush(stdout);

		// read number
		testid = getNumber(stdin, stdout);
		putchar('\n');

		/*
		 * prompt for value according to testid
		 */
		switch (testid) {

		case PARAM_continue:
			break;	// stop prompting

		case PARAM_blockFrequencyBlockLength:
			do {
				// prompt
				printf("   Enter Block Frequency Test block length (try: %d): ", DEFAULT_BITCOUNT);
				fflush(stdout);

				// read number
				state->tp.blockFrequencyBlockLength = getNumber(stdin, stdout);
				putchar('\n');

				// error range check
				if (state->tp.blockFrequencyBlockLength <= 0) {
					printf("	Block Frequency Test block length %ld must be > 0, try again\n\n",
					       state->tp.blockFrequencyBlockLength);
				}
			} while (state->tp.blockFrequencyBlockLength <= 0);
			break;

		case PARAM_nonOverlappingTemplateBlockLength:
			do {
				// prompt
				printf("   Enter NonOverlapping Template Test block Length (try: %d): ", DEFAULT_OVERLAPPING);
				fflush(stdout);

				// read number
				state->tp.nonOverlappingTemplateLength = getNumber(stdin, stdout);
				putchar('\n');

				// error range check
				if (state->tp.nonOverlappingTemplateLength < MINTEMPLEN) {
					printf("	NonOverlapping Template Test block Length %ld must be >= %d, try again\n\n",
					       state->tp.nonOverlappingTemplateLength, MINTEMPLEN);
				}
				if (state->tp.nonOverlappingTemplateLength > MAXTEMPLEN) {
					printf("	NonOverlapping Template Test block Length %ld must be <= %d, try again\n\n",
					       state->tp.nonOverlappingTemplateLength, MAXTEMPLEN);
				}
			} while ((state->tp.nonOverlappingTemplateLength < MINTEMPLEN) ||
				 (state->tp.nonOverlappingTemplateLength > MAXTEMPLEN));
			break;

		case PARAM_overlappingTemplateBlockLength:
			do {
				// prompt
				printf("   Enter Overlapping Template Test block Length (try: %d): ", DEFAULT_OVERLAPPING);
				fflush(stdout);

				// read number
				state->tp.overlappingTemplateLength = getNumber(stdin, stdout);
				putchar('\n');

				// error range check
				if (state->tp.overlappingTemplateLength < MINTEMPLEN) {
					printf("	Overlapping Template Test block Length %ld must be >= %d, try again\n\n",
					       state->tp.overlappingTemplateLength, MINTEMPLEN);
				}
				if (state->tp.overlappingTemplateLength > MAXTEMPLEN) {
					printf("	Overlapping Template Test block Length %ld must be <= %d, try again\n\n",
					       state->tp.overlappingTemplateLength, MAXTEMPLEN);
				}
			} while ((state->tp.overlappingTemplateLength < MINTEMPLEN) ||
				 (state->tp.overlappingTemplateLength > MAXTEMPLEN));
			break;

		case PARAM_approximateEntropyBlockLength:
			do {
				// prompt
				printf("   Enter Approximate Entropy Test block Length (try: %d): ", DEFAULT_APEN);
				fflush(stdout);

				// read number
				state->tp.approximateEntropyBlockLength = getNumber(stdin, stdout);
				putchar('\n');

				// error range check
				if (state->tp.approximateEntropyBlockLength <= 0) {
					printf("	Approximate Entropy Test block Length %ld must be > 0, try again\n\n",
					       state->tp.approximateEntropyBlockLength);
				}
			} while (state->tp.approximateEntropyBlockLength <= 0);
			break;

		case PARAM_serialBlockLength:
			do {
				// prompt
				printf("   Enter Serial Test block Length (try: %d): ", DEFAULT_SERIAL);
				fflush(stdout);

				// read number
				state->tp.serialBlockLength = getNumber(stdin, stdout);
				putchar('\n');

				// error range check
				if (state->tp.serialBlockLength <= 0) {
					printf("	Serial Test block Length %ld must be > 0, try again\n\n",
					       state->tp.serialBlockLength);
				}
			} while (state->tp.serialBlockLength <= 0);
			break;

		case PARAM_linearComplexitySequenceLength:
			do {
				// prompt
				printf("   Enter Linear Complexity Test block Length (try: %d): ", DEFAULT_LINEARCOMPLEXITY);
				fflush(stdout);

				// read number
				state->tp.linearComplexitySequenceLength = getNumber(stdin, stdout);
				putchar('\n');

				// error range check
				if (state->tp.linearComplexitySequenceLength < MIN_LINEARCOMPLEXITY) {
					printf("	linear complexith sequence length(M): %ld must be >= %d, try again\n\n",
					       state->tp.linearComplexitySequenceLength, MIN_LINEARCOMPLEXITY);
				} else if (state->tp.linearComplexitySequenceLength > MAX_LINEARCOMPLEXITY + 1) {
					printf("	linear complexith sequence length(M): %ld must be <= %d, try again\n\n",
					       state->tp.linearComplexitySequenceLength, MAX_LINEARCOMPLEXITY);
				}
			} while ((state->tp.linearComplexitySequenceLength < MIN_LINEARCOMPLEXITY) ||
				 (state->tp.linearComplexitySequenceLength > MAX_LINEARCOMPLEXITY + 1));
			break;

		case PARAM_numOfBitStreams:
			do {
				// prompt
				printf("   Enter the number bitstream iterations (try: %d): ", DEFAULT_UNIFORMITY_BINS);
				fflush(stdout);

				// read number
				state->tp.numOfBitStreams = getNumber(stdin, stdout);
				putchar('\n');

				// error range check
				if (state->tp.numOfBitStreams <= 0) {
					printf("	Number of uniformity bins %ld must be > 0, try again\n\n",
					       state->tp.numOfBitStreams);
				}
			} while (state->tp.numOfBitStreams <= 0);
			break;

		case PARAM_uniformity_bins:
			do {
				// prompt
				printf("   Enter the number of uniformity bins (try: %d): ", DEFAULT_UNIFORMITY_BINS);
				fflush(stdout);

				// read number
				state->tp.uniformity_bins = getNumber(stdin, stdout);
				putchar('\n');

				// error range check
				if (state->tp.uniformity_bins <= 0) {
					printf("	Number of uniformity bins %ld must be > 0, try again\n\n",
					       state->tp.uniformity_bins);
				}
			} while (state->tp.uniformity_bins <= 0);
			break;

		case PARAM_n:
			do {
				// prompt
				printf("   Enter the length of a single bit stream (try: %d): ", DEFAULT_BITCOUNT);
				fflush(stdout);

				// read number
				state->tp.n = getNumber(stdin, stdout);
				putchar('\n');

				// error range check
				if (state->tp.n < MIN_BITCOUNT) {
					printf("	Length of a single bit stream %ld must be >= %d, try again\n\n",
					       state->tp.n, MIN_BITCOUNT);
				} else if (state->tp.n > MAX_BITCOUNT) {
					printf("	Length of a single bit stream %ld must be <= %d, try again\n\n",
					       state->tp.n, MAX_BITCOUNT);
				}
			} while ((state->tp.n < MIN_BITCOUNT) || (state->tp.n > MAX_BITCOUNT));
			break;

		case PARAM_uniformity_level:
			do {
				// prompt
				printf("   Enter Uniformity Cutoff Level (try: %f): ", DEFAULT_UNIFORMITY_LEVEL);
				fflush(stdout);

				// read number
				state->tp.uniformity_level = getDouble(stdin, stdout);
				putchar('\n');

				// error range check
				if (state->tp.uniformity_level <= 0.0) {
					printf("	Uniformity Cutoff Level must be > 0.0: %f, try again\n\n",
					       state->tp.uniformity_level);
				} else if (state->tp.uniformity_level > 0.1) {
					printf("	Uniformity Cutoff Level must be <= 0.1: %f, try again\n\n",
					       state->tp.uniformity_level);
				}
			} while (state->tp.uniformity_level <= 0.0 || state->tp.uniformity_level > 0.1);
			break;

			// prompt is for any test
		case PARAM_alpha:
			do {
				// prompt
				printf("   Enter Alpha Confidence Level (try: %f): ", DEFAULT_ALPHA);
				fflush(stdout);

				// read number
				state->tp.alpha = getDouble(stdin, stdout);
				putchar('\n');

				// error range check
				if (state->tp.alpha <= 0.0) {
					printf("	Alpha must be > 0.0: %f, try again\n\n", state->tp.alpha);
				} else if (state->tp.alpha > 0.1) {
					printf("	Alpha must be <= 0.1: %f, try again\n\n", state->tp.alpha);
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
fileBasedBitStreams(struct state *state)
{
	long int byteCount;	// byte count for a number of consecutive bits, rounded up
	int seekError;		// error during fseek()
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(224, __FUNCTION__, "state arg is NULL");
	}
	if (state->streamFile == NULL) {
		err(224, __FUNCTION__, "streamFile arg is NULL");
	}

	/*
	 * case: parse a set of ASCII '0' and '1' characters
	 */
	if (state->dataFormat == FORMAT_ASCII_01) {

		/*
		 * set file pointer after jobnum chunks into file
		 */
		dbg(DBG_LOW, "seeking %ld * %ld * %ld = %ld on %s for ASCII 0/1 format",
		    state->jobnum, state->tp.n, state->tp.numOfBitStreams,
		    (state->jobnum * state->tp.n * state->tp.numOfBitStreams), state->randomDataPath);
		seekError = fseek(state->streamFile, (state->jobnum * state->tp.n * state->tp.numOfBitStreams), SEEK_SET);
		if (seekError != 0) {
			errp(224, __FUNCTION__, "could not seek %ld into file: %s",
			     (state->jobnum * state->tp.n * state->tp.numOfBitStreams), state->randomDataPath);
		}
		/*
		 * parse data
		 */
		readBinaryDigitsInASCIIFormat(state);

		/*
		 * close file
		 */
		errno = 0;	// paranoia
		io_ret = fclose(state->streamFile);
		if (io_ret != 0) {
			errp(224, __FUNCTION__, "error closing: %s", state->randomDataPath);
		}
		state->streamFile = NULL;

	}

	/*
	 * case: parse raw 8-bit binary bytes
	 */
	else if (state->dataFormat == FORMAT_RAW_BINARY) {

		/*
		 * bytes that hold a given set of consecutive bits
		 *
		 * We need to round up the byte count to the next whole byte.
		 * However if the bit count is a multiple of 8, then we do
		 * not increase it.  We only increase by one in
		 * the case of a final partial byte.
		 */
		byteCount = ((state->jobnum * state->tp.n * state->tp.numOfBitStreams) + 7) / 8;

		/*
		 * set file pointer after jobnum chunks into file
		 */
		dbg(DBG_LOW, "seeking %ld * %ld * %ld/8 = %ld on %s for raw binary format",
		    state->jobnum, state->tp.n, state->tp.numOfBitStreams, byteCount, state->randomDataPath);
		seekError = fseek(state->streamFile, ((state->jobnum * state->tp.n * state->tp.numOfBitStreams) + 7) / 8, SEEK_SET);
		if (seekError != 0) {
			err(224, __FUNCTION__, "could not seek %ld into file: %s", byteCount, state->randomDataPath);
		}
		/*
		 * parse data
		 */
		readHexDigitsInBinaryFormat(state);

		/*
		 * close file
		 */
		errno = 0;	// paranoia
		io_ret = fclose(state->streamFile);
		if (io_ret != 0) {
			errp(224, __FUNCTION__, "error closing: %s", state->randomDataPath);
		}
		state->streamFile = NULL;
	}

	/*
	 * case: should not get here
	 */
	else {
		err(224, __FUNCTION__, "Input file format selection is invalid");
	}

	return;
}


static void
readBinaryDigitsInASCIIFormat(struct state *state)
{
	long int i;
	long int j;
	long int num_0s;
	long int num_1s;
	long int bitsRead;
	long int bit;
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(225, __FUNCTION__, "state arg is NULL");
	}
	if (state->streamFile == NULL) {
		err(225, __FUNCTION__, "streamFile arg is NULL");
	}

	/*
	 * iterate reading bitstream sized binary digits in ASCII
	 */
	dbg(DBG_LOW, "start of iterate phase");
	for (i = 0; i < state->tp.numOfBitStreams; i++) {

		/*
		 * read octets from the bitstream and count 0 and 1 bits
		 */
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		for (j = 0; j < state->tp.n; j++) {
			io_ret = fscanf(state->streamFile, "%ld", &bit);
			if (io_ret == EOF) {
				warn(__FUNCTION__, "Insufficient data in file %s: %ld bits were read", state->randomDataPath,
				     bitsRead);
				return;
			} else {
				bitsRead++;
				if (bit == 0) {
					num_0s++;
				} else {
					num_1s++;
				}
				state->epsilon[j] = bit;
			}
		}

		/*
		 * Write stats to freq.txt
		 */
		io_ret = fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
		if (io_ret <= 0) {
			errp(225, __FUNCTION__, "error in writing to %s", state->freqFilePath);
		}
		io_ret = fflush(state->freqFile);
		if (io_ret != 0) {
			errp(225, __FUNCTION__, "error flushing to %s", state->freqFilePath);
		}

		/*
		 * Perform statistical tests for this iteration
		 */
		nist_test_suite(state);
	}

	dbg(DBG_LOW, "end of iterate phase");
	return;
}


/*
 * readHexDigitsInBinaryFormat - read bits from the streamFile and convert them into epsilon bit array
 *
 * given:
 *      state           // pointer to run state
 *
 * Given the open steam streamFile, form file state->randomDataPath, convert them into 'bits'
 * found in the epsilon bit array.
 */
void
readHexDigitsInBinaryFormat(struct state *state)
{
	long int num_0s;	// Count of 0 bits processed
	long int num_1s;	// Count of 1 bits processed
	long int bitsRead;	// Number of bits to read and process
	bool done;		// true ==> we have converted enough data
	BYTE byte;		// single bite
	int io_ret;		// I/O return status
	long int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(226, __FUNCTION__, "state arg is NULL");
	}
	if (state->streamFile == NULL) {
		err(226, __FUNCTION__, "streamFile arg is NULL");
	}

	/*
	 * iterate reading bitstream in octets
	 */
	dbg(DBG_LOW, "start of iterate phase");
	for (i = 0; i < state->tp.numOfBitStreams; i++) {

		/*
		 * read octets from the bitstream and count 0 and 1 bits
		 */
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		do {
			/*
			 * read the next binary octet
			 */
			io_ret = fgetc(state->streamFile);
			if (io_ret < 0) {
				errp(226, __FUNCTION__, "read error in stream file: %s", state->randomDataPath);
			}
			byte = (BYTE) io_ret;

			/*
			 * add bits to the epsilon bit stream
			 */
			done = convertToBits(state, &byte, BITS_N_BYTE, state->tp.n, &num_0s, &num_1s, &bitsRead);
		} while (done == false);

		/*
		 * Write stats to freq.txt
		 */
		io_ret = fprintf(state->freqFile, "\t\tBITSREAD = %ld 0s = %ld 1s = %ld\n", bitsRead, num_0s, num_1s);
		if (io_ret <= 0) {
			errp(226, __FUNCTION__, "error in writing to %s", state->freqFilePath);
		}
		io_ret = fflush(state->freqFile);
		if (io_ret != 0) {
			errp(226, __FUNCTION__, "error flushing to %s", state->freqFilePath);
		}

		/*
		 * Perform statistical tests for this iteration
		 */
		nist_test_suite(state);

	}
	dbg(DBG_LOW, "end of iterate phase");
	return;
}


/*
 * convertToBits - convert binary bytes into the end of an epsilon bit array
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
convertToBits(struct state * state, BYTE * x, long int xBitLength, long int bitsNeeded, long int *num_0s, long int *num_1s,
	      long int *bitsRead)
{
	long int i;
	long int j;
	long int count;
	long int bit;
	BYTE mask;
	long int zeros;
	long int ones;

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(227, __FUNCTION__, "state arg is NULL");
	}
	if (state->epsilon == NULL) {
		err(227, __FUNCTION__, "state->epsilon is NULL");
	}

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
			state->epsilon[*bitsRead] = bit;
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


void
invokeTestSuite(struct state *state)
{
	int io_ret;		// I/O return status

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(228, __FUNCTION__, "state arg is NULL");
	}

	/*
	 * case: prep for writing to the datafile // TODO understand these following 2 cases
	 */
	if (state->runMode == MODE_WRITE_ONLY) {

		/*
		 * Allocate the write buffer
		 */
		switch (state->dataFormat) {
		case FORMAT_RAW_BINARY:
			/* Compression BITS_N_BYTE:1 */
			state->tmpepsilon = malloc(((state->tp.n / BITS_N_BYTE) + 1) * sizeof(state->tmpepsilon[0]));
			if (state->tmpepsilon == NULL) {
				errp(228, __FUNCTION__, "cannot allocate %ld elements of %ld bytes each",
				     (state->tp.n / BITS_N_BYTE) + 1, sizeof(state->tmpepsilon[0]));
			}
			state->tmpepsilon[(state->tp.n / BITS_N_BYTE) + 1] = '\0';	// paranoia
			break;

		case FORMAT_ASCII_01:
			state->tmpepsilon = malloc((state->tp.n + 1) * sizeof(state->tmpepsilon[0]));
			if (state->tmpepsilon == NULL) {
				errp(228, __FUNCTION__, "cannot allocate %ld elements of %ld bytes each",
				     state->tp.n + 1, sizeof(state->tmpepsilon[0]));
			}
			state->tmpepsilon[state->tp.n + 1] = '\0';	// paranoia
			break;

		default:
			err(228, __FUNCTION__, "Invalid format");
			break;
		}
	}

	/*
	 * case: prep for reading from a file or from a generator
	 */
	else {
		/*
		 * Announce test
		 */
		io_ret = fprintf(state->freqFile,
			    "________________________________________________________________________________\n\n");
		if (io_ret <= 0) {
			errp(228, __FUNCTION__, "error in writing to %s", state->freqFilePath);
		}
		if (state->generator == 0) {
			io_ret =
			    fprintf(state->freqFile, "\t\tFILE = %s\t\tALPHA = %6.4f\n", state->randomDataPath, state->tp.alpha);
			if (io_ret <= 0) {
				errp(228, __FUNCTION__, "error in writing to %s", state->freqFilePath);
			}
		} else {
			io_ret = fprintf(state->freqFile, "\t\tFILE = %s\t\tALPHA = %6.4f\n",
					 state->generatorDir[state->generator], state->tp.alpha);
			if (io_ret <= 0) {
				errp(228, __FUNCTION__, "error in writing to %s", state->freqFilePath);
			}
		}
		io_ret = fprintf(state->freqFile,
			    "________________________________________________________________________________\n\n");
		if (io_ret <= 0) {
			errp(228, __FUNCTION__, "error in writing to %s", state->freqFilePath);
		}
		io_ret = fflush(state->freqFile);
		if (io_ret != 0) {
			errp(228, __FUNCTION__, "error flushing to %s", state->freqFilePath);
		}
		if (state->batchmode == true) {
			dbg(DBG_LOW, "     Statistical Testing In Progress.........");
		} else {
			printf("	 Statistical Testing In Progress.........\n\n");
			fflush(stdout);
		}
	}

	/*
	 * Test data from a file or from an internal generator
	 * NOTE: Introduce new pseudo random number generators in this switch
	 */
	switch (state->generator) {

	case GENERATOR_FROM_FILE:
		fileBasedBitStreams(state);
		break;
	case GENERATOR_LCG:
		lcg(state);
		break;
	case GENERATOR_QCG1:
		quadRes1(state);
		break;
	case GENERATOR_QCG2:
		quadRes2(state);
		break;
	case GENERATOR_CCG:
		cubicRes(state);
		break;
	case GENERATOR_XOR:
		exclusiveOR(state);
		break;
	case GENERATOR_MODEXP:
		modExp(state);
		break;
	case GENERATOR_BBS:
		bbs(state);
		break;
	case GENERATOR_MS:
		micali_schnorr(state);
		break;
	case GENERATOR_SHA1:
		SHA1(state);
		break;

	default:
		err(228, __FUNCTION__, "Error in invokeTestSuite!");
		break;
	}

	/*
	 * -m w: only write generated data to -f randata
	 *
	 * Data already written, nothing else do to.
	 */
	if (state->runMode == MODE_WRITE_ONLY) {

		/*
		 * Announce end of write only run
		 */
		if (state->batchmode == true) {
			dbg(DBG_LOW, "     Exiting, completed writes to %s", state->randomDataPath);
		} else {
			printf("	 Exiting completed writes to %s\n", state->randomDataPath);
			fflush(stdout);
		}
		destroy(state);
		exit(0);
	}

	/*
	 * -m i: iterate only, write state to -s statePath
	 *
	 * State already written, nothing else do to.
	 */
	else if (state->runMode == MODE_ITERATE_ONLY) {

		/*
		 * Announce end of iteration only run
		 */
		if (state->batchmode == true) {
			dbg(DBG_LOW, "     Exiting iterate only");
		} else {
			printf("	 Exiting iterate only\n");
			fflush(stdout);
		}
		destroy(state);
		exit(0);
	}

	/*
	 * -m b: iterate and then assess
	 *
	 * Iterations complete, move on to assess
	 */
	else if (state->runMode == MODE_ITERATE_AND_ASSESS) {
		if (state->batchmode == true) {
			dbg(DBG_LOW, "     Statistical Testing Complete!!!!!!!!!!!!");
		} else {
			printf("	 Statistical Testing Complete!!!!!!!!!!!!\n");
			fflush(stdout);
		}


	}

	/*
	 * -m a: assess only, read state from *.state files under -r stateDir and asses results
	 * -m b: iterate and then assess
	 */
	if (state->batchmode == true) {
		dbg(DBG_LOW, "	   About to start assessment");
	} else {
		printf("     About to start assessment\n");
		fflush(stdout);
	}
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
static void
getTimestamp(char *buf, size_t len)
{
	time_t seconds;		// seconds since the epoch
	struct tm *loc_ret;	// return value form localtime_r()
	struct tm now;		// the time now
	size_t time_len;	// Length of string produced by strftime() or 0

	// firewall
	if (buf == NULL) {
		err(229, __FUNCTION__, "bug arg is NULL");
	}
	if (len <= 0) {
		err(229, __FUNCTION__, "len must be > 0: %lu", len);
	}

	/*
	 * Determine the time for now
	 */
	errno = 0;		// paranoia
	seconds = time(NULL);
	if (seconds < 0) {
		errp(229, __FUNCTION__, "time returned < 0: %ld", seconds);
	}
	loc_ret = localtime_r(&seconds, &now);
	if (loc_ret == NULL) {
		errp(229, __FUNCTION__, "localtime_r returned NULL");
	}
	errno = 0;		// paranoia
	time_len = strftime(buf, len - 1, "%F %T", &now);
	if (time_len == 0) {
		errp(229, __FUNCTION__, "strftime failed");
	}
	buf[len] = '\0';	// paranoia
	return;
}


/*
 * nist_test_suite - run an iteration for all enabled tests
 *
 * given:
 *      state           // run state and which tests are enabled
 */
void
nist_test_suite(struct state *state)
{
	char buf[BUFSIZ + 1];	// time string buffer

	/*
	 * Check preconditions (firewall)
	 */
	if (state == NULL) {
		err(230, __FUNCTION__, "state arg is NULL");
	}

	/*
	 * if you "like to watch", report the very beginning
	 */
	if (state->reportCycle > 0 && state->curIteration == 0) {
		getTimestamp(buf, BUFSIZ);
		msg("starting first iteration of %ld at %s", state->tp.numOfBitStreams, buf);
	}

	/*
	 * Perform iteration on all enabled tests
	 */
	iterate(state);

	/*
	 * Count the iteration and report process if requested
	 */
	++state->curIteration;
	if (state->reportCycle > 0 && (((state->curIteration % state->reportCycle) == 0) ||
				       (state->curIteration == state->tp.numOfBitStreams))) {
		getTimestamp(buf, BUFSIZ);
		if (state->curIteration == state->tp.numOfBitStreams) {
			msg("completed last iteration of %ld at %s", state->tp.numOfBitStreams, buf);
		} else {
			msg("completed iteration %ld of %ld at %s", state->curIteration, state->tp.numOfBitStreams, buf);
		}
	}

	return;
}


/*
 * write_sequence - write the epsilon stream to a randomDataPath
 *
 * given:
 *      state           // run state to write epsilon to randomDataPath
 */
void
write_sequence(struct state *state)
{
	char buf[BUFSIZ + 1];	// time string buffer
	int io_ret;		// I/O return status
	int count;
	long int i;
	int j;

	// firewall
	if (state == NULL) {
		err(231, __FUNCTION__, "state arg was NULL");
	}
	if (state->tmpepsilon == NULL) {
		err(231, __FUNCTION__, "state->tmpepsilon is NULL");
	}
	if (state->streamFile == NULL) {
		err(231, __FUNCTION__, "state->streamFile is NULL");
	}

	/*
	 * Write contents of epsilon
	 */
	switch (state->dataFormat) {
	case FORMAT_RAW_BINARY:

		/*
		 * store output as binary bits
		 */
		for (i = 0, j = 0, count = 0, state->tmpepsilon[0] = 0; i < state->tp.n; i++) {

			// store bit in current output byte
			if (state->epsilon[i] == 1) {
				state->tmpepsilon[j] |= (1 << count);
			} else if (state->epsilon[i] != 0) {
				err(231, __FUNCTION__, "epsilon[%ld]: %d is neither 0 nor 1", i, state->epsilon[i]);
			}
			++count;

			// When an output byte is complete
			if (count >= BITS_N_BYTE) {
				// prep for next byte
				count = 0;
				if (j >= (state->tp.n / BITS_N_BYTE)) {
					break;
				}
				++j;
				state->tmpepsilon[j] = 0;
			}
		}

		/*
		 * Write output buffer
		 */
		errno = 0;	// paranoia
		io_ret = fwrite(state->tmpepsilon, sizeof(BitSequence), j, state->streamFile);
		if (io_ret < j) {
			errp(231, __FUNCTION__, "write of %d elements of %ld bytes to %s failed",
			     j, sizeof(BitSequence), state->randomDataPath);
		}
		errno = 0;	// paranoia
		io_ret = fflush(state->streamFile);
		if (io_ret == EOF) {
			errp(231, __FUNCTION__, "flush of %s failed", state->randomDataPath);
		}
		break;

	case FORMAT_ASCII_01:

		/*
		 * store output as ASCII string of numbers
		 */
		for (i = 0; i < state->tp.n; i++) {
			if (state->epsilon[i] == 0) {
				state->tmpepsilon[i] = '0';
			} else if (state->epsilon[i] == 1) {
				state->tmpepsilon[i] = '1';
			} else {
				err(231, __FUNCTION__, "epsilon[%ld]: %d is neither 0 nor 1", i, state->epsilon[i]);
			}
		}

		/*
		 * Write file as ASCII string of numbers
		 */
		errno = 0;	// paranoia
		io_ret = fwrite(state->tmpepsilon, sizeof(BitSequence), state->tp.n, state->streamFile);
		if (io_ret < state->tp.n) {
			errp(231, __FUNCTION__, "write of %ld elements of %ld bytes to %s failed",
			     state->tp.n, sizeof(BitSequence), state->randomDataPath);
		}
		io_ret = fflush(state->streamFile);
		if (io_ret == EOF) {
			errp(231, __FUNCTION__, "flush of %s failed", state->randomDataPath);
		}
		break;

	default:
		err(231, __FUNCTION__, "Invalid format");
		break;
	}

	/*
	 * Count the iteration and report process if requested
	 */
	++state->curIteration;
	if (state->reportCycle > 0 && (((state->curIteration % state->reportCycle) == 0) ||
				       (state->curIteration == state->tp.numOfBitStreams))) {
		getTimestamp(buf, BUFSIZ);
		if (state->curIteration == state->tp.numOfBitStreams) {
			msg("completed last iteration of %ld at %s", state->tp.numOfBitStreams, buf);
		} else {
			msg("completed iteration %ld of %ld at %s", state->curIteration, state->tp.numOfBitStreams, buf);
		}
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
