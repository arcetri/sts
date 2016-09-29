// Parse_args.c
// Parse command line arguments for batch automation of assess.c

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "externs.h"


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
void
msg(char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */

	/*
	 * start the var arg setup and fetch our first arg
	 */
	va_start(ap, fmt);

	/*
	 * firewall
	 */
	if (fmt == NULL) {
		warn(__FUNCTION__, "NULL fmt given to debug");
		fmt = "((NULL fmt))";
	}

	/*
	 * print the message
	 */
	ret = vfprintf(stderr, fmt, ap);
	if (ret <= 0) {
		fprintf(stderr, "[%s vfprintf returned error: %d]", __FUNCTION__, ret);
	}
	fputc('\n', stderr);

	/*
	 * clean up stdarg stuff
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
void
dbg(int level, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */

	/*
	 * start the var arg setup and fetch our first arg
	 */
	va_start(ap, fmt);

	/*
	 * firewall
	 */
	if (fmt == NULL) {
		warn(__FUNCTION__, "NULL fmt given to debug");
		fmt = "((NULL fmt))";
	}

	/*
	 * if verbose enough, print the debug message
	 */
	if (level <= debuglevel) {
		ret = vfprintf(stderr, fmt, ap);
		if (ret <= 0) {
			fprintf(stderr, "[%s vfprintf returned error: %d]", __FUNCTION__, ret);
		}
		fputc('\n', stderr);
	}

	/*
	 * clean up stdarg stuff
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
 *      warn(__FUNCTION__, "unexpected foobar: %d", value);
 */
void
warn(char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */

	/*
	 * start the var arg setup and fetch our first arg
	 */
	va_start(ap, fmt);

	/*
	 * firewall
	 */
	if (name == NULL) {
		fprintf(stderr, "Warning: %s called with NULL name\n", __FUNCTION__);
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		fprintf(stderr, "Warning: %s called with NULL fmt\n", __FUNCTION__);
		fmt = "((NULL fmt))";
	}

	/*
	 * issue the warning
	 */
	fprintf(stderr, "Warning: %s: ", name);
	ret = vfprintf(stderr, fmt, ap);
	if (ret <= 0) {
		fprintf(stderr, "[%s vfprintf returned error: %d]", __FUNCTION__, ret);
	}
	fputc('\n', stderr);

	/*
	 * clean up stdarg stuff
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
 *      warnp(__FUNCTION__, "unexpected foobar: %d", value);
 */
void
warnp(char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */
	int saved_errno;	/* errno at function start */

	/*
	 * start the var arg setup and fetch our first arg
	 */
	saved_errno = errno;
	va_start(ap, fmt);

	/*
	 * firewall
	 */
	if (name == NULL) {
		fprintf(stderr, "Warning: %s called with NULL name\n", __FUNCTION__);
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		fprintf(stderr, "Warning: %s called with NULL fmt\n", __FUNCTION__);
		fmt = "((NULL fmt))";
	}

	/*
	 * issue the warning
	 */
	fprintf(stderr, "Warning: %s: ", name);
	ret = vfprintf(stderr, fmt, ap);
	if (ret <= 0) {
		fprintf(stderr, "[%s vfprintf returned error: %d]", __FUNCTION__, ret);
	}
	fputc('\n', stderr);
	fprintf(stderr, "errno[%d]: %s\n", saved_errno, strerror(saved_errno));

	/*
	 * clean up stdarg stuff
	 */
	va_end(ap);
	return;
}


/*
 * err - issue a fatal error message and exit
 *
 * given:
 *      exitcode        value to exit with, <0 ==> do not exit
 *      name            name of function issuing the warning
 *      fmt             format of the warning
 *      ...             optional format args
 *
 * This function does not return.
 *
 * Example:
 *
 *      err(1, __FUNCTION__, "bad foobar: %s", message);
 */
void
err(int exitcode, char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */

	/*
	 * start the var arg setup and fetch our first arg
	 */
	va_start(ap, fmt);

	/*
	 * firewall
	 */
	if (exitcode >= 256) {
		warn(__FUNCTION__, "called with exitcode >= 256: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__FUNCTION__, "forcing exit code: %d", exitcode);
	}
	if (exitcode < 0) {
		warn(__FUNCTION__, "called with exitcode < 0: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__FUNCTION__, "forcing exit code: %d", exitcode);
	}
	if (name == NULL) {
		warn(__FUNCTION__, "called with NULL name");
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		warn(__FUNCTION__, "called with NULL fmt");
		fmt = "((NULL fmt))";
	}

	/*
	 * issue the fatal error
	 */
	fprintf(stderr, "FATAL: %s: ", name);
	ret = vfprintf(stderr, fmt, ap);
	if (ret <= 0) {
		fprintf(stderr, "[%s vfprintf returned error: %d]", __FUNCTION__, ret);
	}
	fputc('\n', stderr);

	/*
	 * clean up stdarg stuff
	 */
	va_end(ap);

	/*
	 * terminate unless exit code is negative
	 */
	exit(exitcode);
}


/*
 * errp - issue a fatal error message, errno string and exit
 *
 * given:
 *      exitcode        value to exit with, <0 ==> do not exit
 *      name            name of function issuing the warning
 *      fmt             format of the warning
 *      ...             optional format args
 *
 * This function does not return.  Unlike err() this function
 * also prints an errno message.
 *
 * Example:
 *
 *      errp(1, __FUNCTION__, "I/O failure: %s", message);
 */
void
errp(int exitcode, char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */
	int saved_errno;	/* errno at function start */

	/*
	 * start the var arg setup and fetch our first arg
	 */
	saved_errno = errno;
	va_start(ap, fmt);

	/*
	 * firewall
	 */
	if (exitcode >= 256) {
		warn(__FUNCTION__, "called with exitcode >= 256: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__FUNCTION__, "forcing exit code: %d", exitcode);
	}
	if (exitcode < 0) {
		warn(__FUNCTION__, "called with exitcode < 0: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__FUNCTION__, "forcing exit code: %d", exitcode);
	}
	if (name == NULL) {
		warn(__FUNCTION__, "called with NULL name");
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		warn(__FUNCTION__, "called with NULL fmt");
		fmt = "((NULL fmt))";
	}

	/*
	 * issue the fatal error
	 */
	fprintf(stderr, "FATAL: %s: ", name);
	ret = vfprintf(stderr, fmt, ap);
	if (ret <= 0) {
		fprintf(stderr, "[%s vfprintf returned error: %d]", __FUNCTION__, ret);
	}
	fputc('\n', stderr);
	fprintf(stderr, "errno[%d]: %s\n", saved_errno, strerror(saved_errno));

	/*
	 * clean up stdarg stuff
	 */
	va_end(ap);

	/*
	 * terminate unless exit code is negative
	 */
	exit(exitcode);
}


/*
 * usage_err - issue a fatal error message, usage message, and exit
 *
 * given:
 *      usage_msg       command line help
 *      exitcode        value to exit with (must be 0 <= exitcode < 256)
 *                      exitcode == 0 ==> just print usage and exit(0)
 *      name            name of function issuing the warning
 *      fmt             format of the warning
 *      ...             optional format args
 *
 * This function does not return.
 *
 * Example:
 *
 *      usage_err(usage, 1, __FUNCTION__, "bad foobar: %s", message);
 */
void
usage_err(char const *usage, int exitcode, char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int ret;		/* return code holder */

	/*
	 * start the var arg setup and fetch our first arg
	 */
	va_start(ap, fmt);

	/*
	 * firewall
	 */
	if (usage == NULL) {
		warn(__FUNCTION__, "called with NULL usage");
		usage = "((NULL usage))";
	}
	if (exitcode < 0 || exitcode >= 256) {
		warn(__FUNCTION__, "exitcode must be >= 0 && < 256: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__FUNCTION__, "forcing exit code: %d", exitcode);
	}
	if (name == NULL) {
		warn(__FUNCTION__, "called with NULL name");
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		warn(__FUNCTION__, "called with NULL fmt");
		fmt = "((NULL fmt))";
	}

	/*
	 * issue the fatal error
	 */
	if (exitcode > 0) {
		fprintf(stderr, "FATAL: %s: ", name);
		ret = vfprintf(stderr, fmt, ap);
		if (ret <= 0) {
			fprintf(stderr, "%s: vfprintf returned error: %d", __FUNCTION__, ret);
		}
		fputc('\n', stderr);
	}

	/*
	 * issue the usage message
	 */
	if (program == NULL) {
		fprintf(stderr, "usage: assess %s\n", usage);
	} else {
		fprintf(stderr, "usage: %s %s\n", program, usage);
	}
	fprintf(stderr, "%s\n", version);

	/*
	 * clean up stdarg stuff
	 */
	va_end(ap);

	/*
	 * terminate
	 */
	exit(exitcode);
}


/*
 * usage_errp - issue a fatal error message, errno string, usage message, and exit
 *
 * given:
 *      usage_msg       command line help
 *      exitcode        value to exit with (must be 0 <= exitcode < 256)
 *                      exitcode == 0 ==> just print usage and exit(0)
 *      name            name of function issuing the warning
 *      fmt             format of the warning
 *      ...             optional format args
 *
 * This function does not return.  Unlike err() this function
 * also prints an errno message.
 *
 * Example:
 *
 *      usage_errp(usage, 1, __FUNCTION__, "bad foobar: %s", message);
 */
void
usage_errp(char const *usage, int exitcode, char const *name, char const *fmt, ...)
{
	va_list ap;		/* argument pointer */
	int saved_errno;	/* errno at function start */
	int ret;		/* return code holder */

	/*
	 * start the var arg setup and fetch our first arg
	 */
	saved_errno = errno;
	va_start(ap, fmt);

	/*
	 * firewall
	 */
	if (usage == NULL) {
		warn(__FUNCTION__, "called with NULL usage");
		usage = "((NULL usage))";
	}
	if (exitcode < 0 || exitcode >= 256) {
		warn(__FUNCTION__, "exitcode must be >= 0 && < 256: %d", exitcode);
		exitcode = FORCED_EXIT;
		warn(__FUNCTION__, "forcing exit code: %d", exitcode);
	}
	if (name == NULL) {
		warn(__FUNCTION__, "called with NULL name");
		name = "((NULL name))";
	}
	if (fmt == NULL) {
		warn(__FUNCTION__, "called with NULL fmt");
		fmt = "((NULL fmt))";
	}

	/*
	 * issue the fatal error
	 */
	if (exitcode > 0) {
		fprintf(stderr, "FATAL: %s: ", name);
		ret = vfprintf(stderr, fmt, ap);
		if (ret <= 0) {
			fprintf(stderr, "%s: vfprintf returned error: %d", __FUNCTION__, ret);
		}
		fputc('\n', stderr);
		fprintf(stderr, "errno[%d]: %s\n", saved_errno, strerror(saved_errno));
	}

	/*
	 * issue the usage message
	 */
	if (program == NULL) {
		fprintf(stderr, "usage: assess %s\n", usage);
	} else {
		fprintf(stderr, "usage: %s %s\n", program, usage);
	}
	fprintf(stderr, "%s\n", version);

	/*
	 * clean up stdarg stuff
	 */
	va_end(ap);

	/*
	 * terminate
	 */
	exit(exitcode);
}
