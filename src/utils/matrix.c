/*****************************************************************************
 R A N K   A L G O R I T H M   R O U T I N E S
 *****************************************************************************/

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


// Exit codes: 120 thru 129

#include <stdio.h>
#include <stdlib.h>
#include "../utils/externs.h"
#include "matrix.h"
#include "debug.h"


#define	MATRIX_FORWARD_ELIMINATION	0
#define	MATRIX_BACKWARD_ELIMINATION	1

int
computeRank(int M, int Q, BitSequence **matrix)
{
	int i;
	int rank;
	int m = MIN(M, Q);

	/*
	 * Forward application of elementary row operations
	 */
	for (i = 0; i < m - 1; i++) {
		if (matrix[i][i] == 1) {
			perform_elementary_row_operations(MATRIX_FORWARD_ELIMINATION, i, M, Q, matrix);
		} else if (matrix[i][i] == 0) {
			if (find_unit_element_and_swap(MATRIX_FORWARD_ELIMINATION, i, M, Q, matrix) == 1) {
				perform_elementary_row_operations(MATRIX_FORWARD_ELIMINATION, i, M, Q, matrix);
			}
		} else {
			err(122, __func__, "matrix[%d][%d] == %u (should contain 1 or 0).", i, i, matrix[i][i]);
		}
	}

	/*
	 * Backward application of elementary row operations
	 */
	for (i = m - 1; i > 0; i--) {
		if (matrix[i][i] == 1) {
			perform_elementary_row_operations(MATRIX_BACKWARD_ELIMINATION, i, M, Q, matrix);
		} else if (matrix[i][i] == 0) {
			if (find_unit_element_and_swap(MATRIX_BACKWARD_ELIMINATION, i, M, Q, matrix) == 1) {
				perform_elementary_row_operations(MATRIX_BACKWARD_ELIMINATION, i, M, Q, matrix);
			}
		} else {
			err(122, __func__, "matrix[%d][%d] == %u (should contain 1 or 0).", i, i, matrix[i][i]);
		}
	}

	rank = determine_rank(m, M, Q, matrix);

	return rank;
}

void
perform_elementary_row_operations(int flag, int i, int M, int Q, BitSequence ** A)
{
	int j;
	int k;

	if (flag == MATRIX_FORWARD_ELIMINATION) {
		for (j = i + 1; j < M; j++) {
			if (A[j][i] == 1) {
				for (k = i; k < Q; k++) {
					A[j][k] = (BitSequence) ((A[j][k] + A[i][k]) % 2);
				}
			}
		}
	} else {
		for (j = i - 1; j >= 0; j--) {
			if (A[j][i] == 1) {
				for (k = 0; k < Q; k++) {
					A[j][k] = (BitSequence) ((A[j][k] + A[i][k]) % 2);
				}
			}
		}
	}
}

int
find_unit_element_and_swap(int flag, int i, int M, int Q, BitSequence ** A)
{
	int index;
	int row_op = 0;

	if (flag == MATRIX_FORWARD_ELIMINATION) {
		index = i + 1;
		while ((index < M) && (A[index][i] == 0)) {
			index++;
		}
		if (index < M) {
			row_op = swap_rows(i, index, Q, A);
		}
	} else {
		index = i - 1;
		while ((index >= 0) && (A[index][i] == 0)) {
			index--;
		}
		if (index >= 0) {
			row_op = swap_rows(i, index, Q, A);
		}
	}

	return row_op;
}

int
swap_rows(int index_first_row, int index_second_row, int Q, BitSequence ** A)
{
	int p;
	BitSequence temp;

	for (p = 0; p < Q; p++) {
		temp = A[index_first_row][p];
		A[index_first_row][p] = A[index_second_row][p];
		A[index_second_row][p] = temp;
	}

	return 1;
}

int
determine_rank(int m, int M, int Q, BitSequence ** A)
{
	int i;
	int j;
	int rank;
	int allZeroes;

	/*
	 * DETERMINE RANK, THAT IS, COUNT THE NUMBER OF NONZERO ROWS
	 */

	rank = m;
	for (i = 0; i < M; i++) {
		allZeroes = 1;
		for (j = 0; j < Q; j++) {
			if (A[i][j] == 1) {
				allZeroes = 0;
				break;
			}
		}
		if (allZeroes == 1) {
			rank--;
		}
	}

	return rank;
}


/*
 * create_matrix - allocate a 2D matrix of BitSequence values
 *
 * given:
 *      M       // Number of rows in the matrix
 *      Q       // Number of columns in each matrix row
 *
 * returns:
 *      An allocated 2D matrix of BitSequence values.
 *
 * NOTE: This function does NOT return on error.
 *
 * NOTE: Unlike older versions of this function, create_matrix()
 *       does not zeroize the matrix.
 */
BitSequence **
create_matrix(int M, int Q)
{
	BitSequence **matrix;	// matrix top return
	int i;

	/*
	 * Check preconditions (firewall)
	 */
	if (M < 0) {
		err(120, __func__, "number of rows: %d must be > 0", M);
	}
	if (Q < 0) {
		err(120, __func__, "number of columns per rows: %d must be > 0", Q);
	}

	/*
	 * Allocate array of matrix rows
	 */
	matrix = malloc(M * sizeof(matrix[0]));
	if (matrix == NULL) {
		errp(120, __func__, "cannot malloc of %ld elements of %ld bytes each for matrix rows",
		     (long int) M, sizeof(BitSequence *));
	}

	/*
	 * Allocate the columns for each matrix row
	 */
	for (i = 0; i < M; i++) {
		matrix[i] = malloc(Q * sizeof(matrix[0][0]));
		if (matrix[i] == NULL) {
			errp(120, __func__, "cannot malloc of %ld elements of %ld bytes each for matrix[%d] column",
			     (long int) Q, sizeof(BitSequence), i);
		}
	}

	return matrix;
}

/*
 * def_matrix - fills the given matrix m with consecutive bits from the sequence.
 *              MUST be called after create_matrix function has allocated memory for m.
 *
 * given:
 *      M       // Number of rows in the matrix m
 *      Q       // Number of columns in each row of the matrix m
 *      m       // allocated 2D matrix of BitSequence values
 *      k       // offset for the bits to copy to this matrix (counts the matrices that were already filled)
 */
void
def_matrix(struct thread_state *thread_state, int M, int Q, BitSequence ** m, long int k)
{
	int i;
	int j;

	/*
	 * Check preconditions (firewall)
	 */
	if (thread_state == NULL) {
		err(225, __func__, "thread_state arg is NULL");
	}
	struct state *state = thread_state->global_state;
	if (state == NULL) {
		err(121, __func__, "state arg is NULL");
	}
	if (state->epsilon == NULL) {
		err(121, __func__, "state->epsilon is NULL");
	}
	if (state->epsilon[thread_state->thread_id] == NULL) {
		err(151, __func__, "state->epsilon[%ld] is NULL", thread_state->thread_id);
	}
	if (M < 0) {
		err(121, __func__, "number of rows: %d must be > 0", M);
	}
	if (Q < 0) {
		err(121, __func__, "number of columns per rows: %d must be > 0", Q);
	}
	if (k < 0) {
		err(121, __func__, "offset for the values to copy from the sequence to to m: %d must be > 0", Q);
	}

	for (i = 0; i < M; i++) {
		for (j = 0; j < Q; j++) {
			m[i][j] = state->epsilon[thread_state->thread_id][k * (M * Q) + j + i * M];
		}
	}
}

void
delete_matrix(int M, BitSequence ** matrix)
{
	int i;

	for (i = 0; i < M; i++)
		free(matrix[i]);

	free(matrix);
}
