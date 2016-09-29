#ifndef DYN_ALLOC_H
#   define DYN_ALLOC_H

#   define DEFAULT_CHUNK (1024)	// by default, allocate DEFAULT_CHUNK at a time


/*
 * convenence utilities for adding or reading values of a given type to a dynamic array
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
 * array - a dynamic aray of idential things
 *
 * Rather than write valus, such as floating point values
 * as ASCII formatted numbers into a file and then later
 * same in the run (after closing the file) reopen and reparse
 * those same files, we will append them to an array
 * and return that pointer to the array.  Then later
 * in the same run, the array can be written as ASCII formatted
 * numbers to a file if needed.
 */
struct dyn_array {
	size_t elm_size;	// number of bytes for a single element
	int zeroize;		// 1 --> always zero newly allocated chunks, 0 --> don't
	long int count;		// number of elements in use
	long int allocated;	// number of elements allocated (>= count)
	long int chunk;		// number of elements to expand by when allocating
	void *data;		// allocated dynamic array of idential things or NULL
};


/*
 * external allocation functions
 */
struct dyn_array *create_dyn_array(size_t elm_size, long int chunk, long int start_chunks, int zeroize);
extern void append_value(struct dyn_array *array, void *value_p);
extern void free_dyn_array(struct dyn_array *array);

#endif				// DYN_ALLOC_H
