/*
 * The OggSQUISH functions used by sts
 */

/*
 * This code has been heavily modified by Landon Curt Noll (chongo at cisco dot com) and Tom Gilgan (thgilgan at cisco dot com).
 * See the initial comment in assess.c and the file README.txt for more information.
 *
 * TOM GILGAN AND LANDON CURT NOLL DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL TOM GILGAN NOR LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * chongo (Landon Curt Noll, http://www.isthe.com/chongo/index.html) /\oo/\
 *
 * Share and enjoy! :-)
 */

#ifndef DFFT_H
#   define DFFT_H

#   define WORK_ARRAY_LEN (15)

extern void __ogg_fdrffti(int n, double *wsave, long int *ifac);
extern void __ogg_fdrfftf(int n, double *X, double *wsave, long int *ifac);

#endif				/* DFFT_H */
