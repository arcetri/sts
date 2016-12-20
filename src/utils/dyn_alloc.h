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

#ifndef DYN_ALLOC_H
#   define DYN_ALLOC_H

#   define DEFAULT_CHUNK (1024)	// by default, allocate DEFAULT_CHUNK at a time


/*
 * convenience utilities for adding or reading values of a given type to a dynamic array
 *
 *      array_p                 // pointer to a struct dyn_array
 *      type                    // type of element held in the dyn_array->data
 *      index                   // index of data to fetch (no bounds checking)
 *
 * Example:
 *      p_value = get_value(state->p_val[test_num], double, i);
 *      stat = addr_value(state->stats[test_num], struct private_stats, i);
 */
#   define get_value(array_p, type, index) (((type *)(((struct dyn_array *)(array_p))->data))[(index)])
#   define addr_value(array_p, type, index) (((type *)(((struct dyn_array *)(array_p))->data))+(index))


/*
 * array - a dynamic array of identical things
 *
 * Rather than write values, such as floating point values
 * as ASCII formatted numbers into a file and then later
 * same in the run (after closing the file) reopen and re-parse
 * those same files, we will append them to an array
 * and return that pointer to the array.  Then later
 * in the same run, the array can be written as ASCII formatted
 * numbers to a file if needed.
 */
struct dyn_array {
	size_t elm_size;	// Number of bytes for a single element
	int zeroize;		// 1 --> always zero newly allocated chunks, 0 --> don't
	long int count;		// Number of elements in use
	long int allocated;	// Number of elements allocated (>= count)
	long int chunk;		// Number of elements to expand by when allocating
	void *data;		// allocated dynamic array of identical things or NULL
};


/*
 * external allocation functions
 */
struct dyn_array *create_dyn_array(size_t elm_size, long int chunk, long int start_elm_count, int zeroize);
extern void append_value(struct dyn_array *array, void *value_to_add);
extern void append_array(struct dyn_array *array, void *array_to_add_p, long int total_elements_to_add);
extern void free_dyn_array(struct dyn_array *array);
extern void clear_dyn_array(struct dyn_array *array);

#endif				// DYN_ALLOC_H
