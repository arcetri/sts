/*
 * The OggSQUISH functions used by sts
 */

#ifndef DFFT_H
#define DFFT_H

extern void __ogg_fdrffti(int n, double *wsave, int *ifac);
extern void __ogg_fdrfftf(int n, double *X, double *wsave, int *ifac);

#endif /* DFFT_H */
