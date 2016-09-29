
#ifndef _CEPHES_H_
#   define _CEPHES_H_

extern double cephes_igamc(double a, double x);
extern double cephes_igam(double a, double x);
extern double cephes_lgam(double x);
extern double cephes_p1evl(double x, double *coef, int N);
extern double cephes_polevl(double x, double *coef, int N);
extern double cephes_erf(double x);
extern double cephes_erfc(double x);
extern double cephes_normal(double x);

#endif				/* _CEPHES_H_ */
