/*****************************************************************************
       R A N K  A L G O R I T H M  F U N C T I O N  P R O T O T Y P E S
 *****************************************************************************/

#ifndef MATRIX_H
#   define MATRIX_H

extern int computeRank(int M, int Q, BitSequence ** matrix);
extern void perform_elementary_row_operations(int flag, int i, int M, int Q, BitSequence ** A);
extern int find_unit_element_and_swap(int flag, int i, int M, int Q, BitSequence ** A);
extern int swap_rows(int i, int index, int Q, BitSequence ** A);
extern int determine_rank(int m, int M, int Q, BitSequence ** A);
extern BitSequence **create_matrix(int M, int Q);
extern void def_matrix(struct state *state, int M, int Q, BitSequence ** m, int k);
extern void delete_matrix(int M, BitSequence ** matrix);

#endif				/* MATRIX_H */
