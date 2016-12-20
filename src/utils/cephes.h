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

#ifndef _CEPHES_H_
#   define _CEPHES_H_

#   if defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE) || defined(_ISOC99_SOURCE)
#      define HAVE_LGAMMA
#   endif

extern double cephes_igamc(double a, double x);
extern double cephes_igam(double a, double x);
#   if defined(HAVE_LGAMMA)
#      define cephes_lgam(x) (lgamma(x))
#   else
extern double cephes_lgam(double x);
#   endif			/* HAVE_LGAMMA */
#   if 0
extern double cephes_erf(double x);
extern double cephes_erfc(double x);
#   endif			/* 0 */
extern double cephes_normal(double x);

#endif				/* _CEPHES_H_ */

extern const double MACHEP;	// 2**-53
extern const double MAXLOG;	// ln(2**1024*(1-MACHEP))
extern const double MAXNUM;	// 2**1024*(1-MACHEP)
