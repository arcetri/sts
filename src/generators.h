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

#ifndef GENERATORS_H
#   define GENERATORS_H

// #include "/sha.h"

extern void lcg(struct state *state);
extern void quadRes1(struct state *state);
extern void quadRes2(struct state *state);
extern void cubicRes(struct state *state);
extern void exclusiveOR(struct state *state);
extern void modExp(struct state *state);
extern void bbs(struct state *state);
extern void micali_schnorr(struct state *state);
extern void SHA1(struct state *state);


/*
 * The circular shifts.
 */
#   define CS1(x) ((((ULONG)x)<<1)|(((ULONG)x)>>31))
#   define CS5(x)  ((((ULONG)x)<<5)|(((ULONG)x)>>27))
#   define CS30(x)  ((((ULONG)x)<<30)|(((ULONG)x)>>2))

/*
 * K constants
 */

#   define K0  0x5a827999L
#   define K1  0x6ed9eba1L
#   define K2  0x8f1bbcdcL
#   define K3  0xca62c1d6L

#   define f1(x,y,z)   ( (x & (y ^ z)) ^ z )

#   define f3(x,y,z)   ( (x & ( y ^ z )) ^ (z & y) )

#   define f2(x,y,z)   ( x ^ y ^ z )	/* Rounds 20-39 */

#   define  expand(x)  Wbuff[x%16] = CS1(Wbuff[(x - 3)%16 ] ^ Wbuff[(x - 8)%16 ] ^ Wbuff[(x - 14)%16] ^ Wbuff[x%16])

#   define sub1Round1(count) \
	 { \
	 temp = CS5(A) + f1(B, C, D) + E + Wbuff[count] + K0; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	 } \

#   define sub2Round1(count)   \
	 { \
	 expand(count); \
	 temp = CS5(A) + f1(B, C, D) + E + Wbuff[count%16] + K0; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	} \

#   define Round2(count)     \
	 { \
	 expand(count); \
	 temp = CS5( A ) + f2( B, C, D ) + E + Wbuff[count%16] + K1;  \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp;  \
	 } \

#   define Round3(count)    \
	 { \
	 expand(count); \
	 temp = CS5( A ) + f3( B, C, D ) + E + Wbuff[count%16] + K2; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	 }

#   define Round4(count)    \
	 { \
	 expand(count); \
	 temp = CS5( A ) + f2( B, C, D ) + E + Wbuff[count%16] + K3; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	 }

#endif				// GENERATORS_H
