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

#ifndef _CEPHES_H_
#   define _CEPHES_H_

#if defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE) || defined(_ISOC99_SOURCE)
#define HAVE_LGAMMA
#endif

extern double cephes_igamc(double a, double x);
extern double cephes_igam(double a, double x);
#if defined(HAVE_LGAMMA)
#define cephes_lgam(x) (lgamma(x))
#else
extern double cephes_lgam(double x);
#endif /* HAVE_LGAMMA */
#if 0
extern double cephes_erf(double x);
extern double cephes_erfc(double x);
#endif /* 0 */
extern double cephes_normal(double x);

#endif				/* _CEPHES_H_ */
