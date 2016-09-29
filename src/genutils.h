
#ifndef _GENUTILS_H_
#define _GENUTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

typedef struct _MP_struct {
	int		size;	/*  in bytes  */
	int		bitlen;	/*  in bits, duh  */
	BYTE	*val;
	} MP;

#define	FREE(A)	if ( (A) ) { free((A)); (A) = NULL; }
#define	ASCII2BIN(ch)	( (((ch) >= '0') && ((ch) <= '9')) ? ((ch) - '0') : (((ch) >= 'A') && ((ch) <= 'F')) ? ((ch) - 'A' + 10) : ((ch) - 'a' + 10) )

#ifndef EXPWD
#define	EXPWD		((DBLWORD)1<<NUMLEN)
#endif

#define	sniff_bit(ptr,mask)		(*(ptr) & mask)

/*
 * Function Declarations
 */
extern int		greater(BYTE *x, BYTE *y, int l);
extern int		less(BYTE *x, BYTE *y, int l);
extern BYTE	bshl(BYTE *x, int l);
extern void	bshr(BYTE *x, int l);
extern int		Mult(BYTE *A, BYTE *B, int LB, BYTE *C, int LC);
extern void	ModSqr(BYTE *A, BYTE *B, int LB, BYTE *M, int LM);
extern void	ModMult(BYTE *A, BYTE *B, int LB, BYTE *C, int LC, BYTE *M, int LM);
extern void	smult(BYTE *A, BYTE b, BYTE *C, int L);
extern void	Square(BYTE *A, BYTE *B, int L);
extern void	ModExp(BYTE *A, BYTE *B, int LB, BYTE *C, int LC, BYTE *M, int LM);
extern int		DivMod(BYTE *x, int lenx, BYTE *n, int lenn, BYTE *quot, BYTE *rem);
extern void	Mod(BYTE *x, int lenx, BYTE *n, int lenn);
extern void	Div(BYTE *x, int lenx, BYTE *n, int lenn);
extern void	sub(BYTE *A, int LA, BYTE *B, int LB);
extern int		negate(BYTE *A, int L);
extern BYTE	add(BYTE *A, int LA, BYTE *B, int LB);
extern void	prettyprintBstr(char *S, BYTE *A, int L);
extern void	byteReverse(ULONG *buffer, int byteCount);
extern void	ahtopb (char *ascii_hex, BYTE *p_binary, int bin_len);

#endif  /* _GENUTILS_H_ */
