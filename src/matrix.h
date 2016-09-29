/*****************************************************************************
       R A N K  A L G O R I T H M  F U N C T I O N  P R O T O T Y P E S
 *****************************************************************************/

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
