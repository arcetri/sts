/*
 * The OggSQUISH functions used by sts
 */

#ifndef DFFT_H
#   define DFFT_H

#   define WORK_ARRAY_LEN (15)

extern void __ogg_fdrffti(int n, double *wsave, long int *ifac);
extern void __ogg_fdrfftf(int n, double *X, double *wsave, long int *ifac);

#endif				/* DFFT_H */
