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

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef _CONFIG_H_
#   define	_CONFIG_H_

// #define WINDOWS32
// #define PROTOTYPES
// #define LITTLE_ENDIAN
// #define LOWHI

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

#endif				/* _CONFIG_H_ */

#if defined(__cplusplus)
}
#endif
