/*****************************************************************************
 D E B U G G I N G  A I D E S
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

#ifndef DEBUG_H
#   define DEBUG_H

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
 * errno makig the calls that perror print incorrect error messages.
 *
 * The DEBUG_LINT assumes the following global variabls is declared elsewhere:
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
 * of those warnings may inidicate the presense of bugs in your code including but
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
#      define usage_err(usage, exitcode, name, ...) (fprintf(stderr, "%s\n%s: ", (usage), (name)), \
					       fprintf(stderr, __VA_ARGS__), \
					       exit(exitcode))
#      define usage_errp(usage, exitcode, name, ...) (fprintf(stderr, "%s\n%s: ", (usage), (name)), \
						fprintf(stderr, __VA_ARGS__), \
						fputc('\n', stderr), perror(__FUNCTION__), \
						exit(exitcode))

#   else			// DEBUG_LINT && __STDC_VERSION__ >= 199901L

extern void msg(char const *fmt, ...);
extern void dbg(int level, char const *fmt, ...);
extern void warn(char const *name, char const *fmt, ...);
extern void warnp(char const *name, char const *fmt, ...);
extern void err(int exitcode, char const *name, char const *fmt, ...);
extern void errp(int exitcode, char const *name, char const *fmt, ...);
extern void usage_err(char const *usage, int exitcode, char const *name, char const *fmt, ...);
extern void usage_errp(char const *usage, int exitcode, char const *name, char const *fmt, ...);

#   endif			// DEBUG_LINT && __STDC_VERSION__ >= 199901L

#   define DBG_NONE (0)		// no debugging
#   define DBG_LOW (1)		// minimal debugging
#   define DBG_MED (3)		// somewhat more debugging
#   define DBG_HIGH (5)		// verbose debugging
#   define DBG_VHIGH (7)	// very verbose debugging
#   define DBG_VVHIGH (9)	// very very verbose debugging
#   define FORCED_EXIT (255)	// exit(255) on bad exit code

#endif				/* DEBUG_H */
