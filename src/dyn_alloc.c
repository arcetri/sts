// alloc.c - dynamic array of things allocator

/*
 * This code has been heavily modified by the following people:
 *
 *      Landon Curt Noll
 *      Tom Gilgan
 *      Riccardo Paccagnella
 *
 * See the README.txt and the initial comment in assess.c for more information.
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


// Exit codes: 60 thru 69

#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "externs.h"
#include "debug.h"
#include "utilities.h"


extern long int debuglevel;	// -v lvl: defines the level of verbosity for debugging


/*
 * Forward static function declarations
 */
static void grow_dyn_array(struct dyn_array *array, long int elms_to_allocate);


/*
 * grow_dyn_array - grow the allocation of a dynamic array by a specified number of elements
 *
 * given:
 *      array           		pointer to the dynamic array
 *      elms_to_allocate	       	number of elements to allocate space for
 *
 * The dynamic array allocation will be expanded by new_elem elements.  That data will
 * be zeroized according to the type.
 *
 * This function does not return on error.
 */
static void
grow_dyn_array(struct dyn_array *array, long int elms_to_allocate)
{
	void *data;			// Reallocated array
	long int old_allocated;		// Old number of elements allocated
	long int new_allocated;		// New number of elements allocated
	long int old_bytes;		// Old size of data in dynamic array
	long int new_bytes;		// New size of data in dynamic array after allocation
	unsigned char *p;		// Pointer to the beginning of the new allocated space

	/*
	 * Check preconditions (firewall) - sanity check args
	 */
	if (array == NULL) {
		err(61, __FUNCTION__, "array arg is NULL");
	}
	if (elms_to_allocate <= 0) {
		err(61, __FUNCTION__, "elms_to_allocate arg must be > 0: %d", elms_to_allocate);
	}

	/*
	 * Check preconditions (firewall) - sanity check array
	 */
	if (array->data == NULL) {
		err(61, __FUNCTION__, "data for dynamic array is NULL");
	}
	if (array->elm_size <= 0) {
		err(61, __FUNCTION__, "elm_size in dynamic array must be > 0: %ld", array->elm_size);
	}
	if (array->chunk <= 0) {
		err(61, __FUNCTION__, "chunk in dynamic array must be > 0: %ld", array->chunk);
	}
	if (array->count > array->allocated) {
		err(61, __FUNCTION__, "count: %ld in dynamic array must be <= allocated: %ld", array->count, array->allocated);
	}

	// firewall - check for overflow
	old_allocated = array->allocated;
	if (sum_will_overflow_long(old_allocated, elms_to_allocate)) {
		err(61, __FUNCTION__, "allocating %ld new elements would overflow the allocated counter (now %ld) of the dynamic "
				"array: %ld + %ld does not fit in a long int",
		    elms_to_allocate, old_allocated, old_allocated, elms_to_allocate);
	}
	new_allocated = old_allocated + elms_to_allocate;

	// firewall - check for size overflow
	old_bytes = old_allocated * array->elm_size;
	if (multiplication_will_overflow_long(new_allocated, array->elm_size)) {
		err(61, __FUNCTION__, "the total number of bytes occupied by %ld elements of size %lu would overflow because"
				" %ld * %lu does not fit in a long int",
		    new_allocated, array->elm_size, new_allocated, array->elm_size);
	}
	new_bytes = new_allocated * array->elm_size;

	// firewall - check if new_bytes fits in a size_t variable
	if (new_bytes > SIZE_MAX) {
		err(61, __FUNCTION__, "the total number of bytes occupied by %ld elements of size %lu is too big and does not fit"
				" the bounds of a size_t variable: requires %ld <= %lu",
		    new_allocated, array->elm_size, new_bytes, SIZE_MAX);
	}

	/*
	 * Reallocate array
	 */
	data = realloc(array->data, (size_t) new_bytes);
	if (data == NULL) {
		errp(61, __FUNCTION__, "failed to reallocate the dynamic array from a size of %ld bytes to a size of %ld bytes",
		     old_bytes, new_bytes);
	}

	array->data = data;
	array->allocated = new_allocated;
	dbg(DBG_VHIGH, "expanded dynamic array with %ld new allocated elements of %lu bytes: the number of allocated elements "
			    "went from %ld to %ld and the size went from %ld to %ld",
	    elms_to_allocate, array->elm_size, old_allocated, array->allocated, old_bytes, new_bytes);

	/*
	 * Zeroize new elements if needed
	 */
	if (array->zeroize == 1) {
		p = (unsigned char *) (array->data) + old_bytes;
		memset(p, 0, elms_to_allocate * array->elm_size);
	}

	return;
}



/*
 * create_dyn_array - create a dynamic array
 *
 * given:
 *      elm_size        // size of an element
 *      chunk           // fixed number of elements to expand by when allocating
 *      start_elm_count // starting number of elements to allocate
 *      zeroize         // 1 --> always zeroize newly allocated elements, 0 --> don't
 *
 * returns:
 *      initialized (to zero) empty dynamic array
 *
 * This function does not return on error.
 */
struct dyn_array *
create_dyn_array(size_t elm_size, long int chunk, long int start_elm_count, int zeroize)
{
	struct dyn_array *ret;		// Created dynamic array to return
	long int number_of_bytes; 	// Total number of bytes occupied by the initialized array

	/*
	 * Check preconditions (firewall) - sanity check args
	 */
	if (elm_size <= 0) {
		err(62, __FUNCTION__, "elm_size must be > 0: %ld", elm_size);
	}
	if (chunk <= 0) {
		err(62, __FUNCTION__, "chunk must be > 0: %ld", chunk);
	}
	if (start_elm_count <= 0) {
		err(62, __FUNCTION__, "start_elm_count must be > 0: %ld", start_elm_count);
	}

	/*
	 * Allocate new dynamic array
	 */
	ret = malloc(sizeof(struct dyn_array));
	if (ret == NULL) {
		errp(62, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for dyn_array",
		     (long int) 1, sizeof(struct dyn_array));
	}

	/*
	 * Initialize empty dynamic array
	 * Start with a dynamic array with allocated enough chunks to hold at least start_elm_count elements
	 */
	ret->elm_size = elm_size;
	ret->zeroize = zeroize;
	ret->count = 0;		// Allocated array is empty
	ret->allocated = chunk * ((start_elm_count + (chunk - 1)) / chunk); // Allocate a number of elements multiple of chunk
	ret->chunk = chunk;

	// firewall - check for size overflow
	if (multiplication_will_overflow_long(ret->allocated, elm_size)) {
		err(61, __FUNCTION__, "the total number of bytes occupied by %ld elements of size %lu would overflow because"
				    " %ld * %lu does not fit in a long int",
		    ret->allocated, elm_size, ret->allocated, elm_size);
	}
	number_of_bytes = ret->allocated * elm_size;
	if (number_of_bytes > SIZE_MAX) {
		err(61, __FUNCTION__, "the total number of bytes occupied by %ld elements of size %lu is too big and does not fit"
				    " the bounds of a size_t variable: requires %ld <= %lu",
		    ret->allocated, elm_size, number_of_bytes, SIZE_MAX);
	}

	ret->data = malloc((size_t) number_of_bytes);
	if (ret->data == NULL) {
		errp(62, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for dyn_array->data", ret->allocated,
		     elm_size);
	}

	/*
	 * Zeroize allocated data according to type
	 */
	if (ret->zeroize == 1) {
		memset(ret->data, 0, ret->allocated * ret->elm_size);
	}

	/*
	 * Return newly allocated array
	 */
	dbg(DBG_HIGH, "initialized empty dynamic array of %ld elements of %ld bytes per element", ret->allocated, ret->elm_size);

	return ret;
}


/*
 * append_value - append a value of a given type onto the end of the dynamic array
 *
 * given:
 *      array           // pointer to the dynamic array
 *      value_to_add    // pointer to the value to add to the end of the dynamic array
 *
 * We will add a value of a given type onto the end of the dynamic array.
 * We will grow the dynamic array if all allocated values are used.
 * If after adding the value, all allocated values are used, we will grow
 * the dynamic array as a firewall.
 *
 * This function does not return on error.
 */
void
append_value(struct dyn_array *array, void *value_to_add)
{
	unsigned char *p;

	/*
	 * Check preconditions (firewall) - sanity check args
	 */
	if (array == NULL) {
		err(63, __FUNCTION__, "array arg is NULL");
	}
	if (value_to_add == NULL) {
		err(63, __FUNCTION__, "value_to_add arg is NULL");
	}

	/*
	 * Check preconditions (firewall) - sanity check array
	 */
	if (array->data == NULL) {
		err(63, __FUNCTION__, "data in dynamic array");
	}
	if (array->elm_size <= 0) {
		err(63, __FUNCTION__, "elm_size in dynamic array must be > 0: %ld", array->elm_size);
	}
	if (array->chunk <= 0) {
		err(63, __FUNCTION__, "chunk in dynamic array must be > 0: %ld", array->chunk);
	}
	if (array->count > array->allocated) {
		err(63, __FUNCTION__, "count: %ld in dynamic array must be <= allocated: %ld", array->count, array->allocated);
	}

	/*
	 * Expand dynamic array if needed
	 */
	if (array->count == array->allocated) {
		grow_dyn_array(array, array->chunk);
	}

	/*
	 * We know the dynamic array has enough room to append, so append the value
	 */
	p = (unsigned char *) (array->data) + (array->count * array->elm_size);
	memcpy(p, value_to_add, array->elm_size);
	++array->count;

	return;
}


/*
 * append_array - append the values of a given array (which are of a given type) onto the end of the dynamic array
 *
 * given:
 *      array           	// pointer to the dynamic array
 *      array_to_add_p 		// pointer to the array to add to the end of the dynamic array
 *      array_to_add_size 	// size of the array to add to the end of the dynamic array
 *
 * We will add the values of the given array (which are of a given type) onto the
 * end of the dynamic array. We will grow the dynamic array if all allocated values are used.
 * If after adding the values of the array, all allocated values are used, we will grow
 * the dynamic array as a firewall.
 *
 * This function does not return on error.
 */
void
append_array(struct dyn_array *array, void *array_to_add_p, long int total_elements_to_add)
{
	long int available_empty_elements;
	long int required_elements_to_allocate;
	unsigned char *p;

	/*
	 * Check preconditions (firewall) - sanity check args
	 */
	if (array == NULL) {
		err(63, __FUNCTION__, "array arg is NULL");
	}
	if (array_to_add_p == NULL) {
		err(63, __FUNCTION__, "array_to_add_p arg is NULL");
	}

	/*
	 * Check preconditions (firewall) - sanity check array
	 */
	if (array->data == NULL) {
		err(63, __FUNCTION__, "data in dynamic array");
	}
	if (array->elm_size <= 0) {
		err(63, __FUNCTION__, "elm_size in dynamic array must be > 0: %ld", array->elm_size);
	}
	if (array->chunk <= 0) {
		err(63, __FUNCTION__, "chunk in dynamic array must be > 0: %ld", array->chunk);
	}
	if (array->count > array->allocated) {
		err(63, __FUNCTION__, "count: %ld in dynamic array must be <= allocated: %ld", array->count, array->allocated);
	}

	/*
	 * Expand dynamic array if needed
	 */
	available_empty_elements = array->allocated - array->count;
	required_elements_to_allocate = array->chunk * ((total_elements_to_add - available_empty_elements + (array->chunk - 1)) / array->chunk);
	if (available_empty_elements <= total_elements_to_add) {
		grow_dyn_array(array, required_elements_to_allocate);
	}

	/*
	 * We know the dynamic array has enough room to append, so append the new array
	 */
	p = (unsigned char *) (array->data) + (array->count * array->elm_size);
	memcpy(p, array_to_add_p, array->elm_size * total_elements_to_add);
	array->count += total_elements_to_add;

	return;
}


/*
 * Free_dyn_array - free a dynamic array
 *
 * given:
 *      array           // pointer to the dynamic array
 */
void
free_dyn_array(struct dyn_array *array)
{
	/*
	 * Check preconditions (firewall) - sanity check args
	 */
	if (array == NULL) {
		err(64, __FUNCTION__, "array arg is NULL");
	}

	/*
	 * Free any storage this dynamic array might have
	 */
	if (array->data != NULL) {
		free(array->data);
		array->data = NULL;
	}

	/*
	 * Zero the count and allocation
	 */
	array->elm_size = 0;
	array->count = 0;
	array->allocated = 0;
	array->chunk = 0;
	return;
}



#if defined(STANDALONE)

int debuglevel = DBG_NONE;
const char *const version = "dyn_test";
char *program = "dyn_test";

const char *const testNames[NUMOFTESTS + 1] = {
	" ", "Frequency", "BlockFrequency", "CumulativeSums", "Runs", "LongestRun", "Rank",
	"FFT", "NonOverlappingTemplate", "OverlappingTemplate", "Universal",
	"ApproximateEntropy", "RandomExcursions", "RandomExcursionsVariant",
	"Serial", "LinearComplexity"
};


 /*ARGSUSED*/
main(int argc, char *argv[])
{
	struct state s;		// running state
	struct dyn_array *array;	// dynatic array to test
	double d;		// test double
	int i;

	/*
	 * parse args
	 */
	Parse_args(&s, argc, argv);

	/*
	 * create dynamic array
	 */
	array = create_dyn_array(type_double, DEFAULT_CHUNK, "double test");

	/*
	 * load a million doubles
	 */
	for (d = 0.0; d < 1000000.0; d += 1.0) {
		append_double(array, d);
	}

	/*
	 * verify values
	 */
	for (i = 0; i < 1000000; ++i) {
		if ((double) i != get_value(array, double, i)) {
			warn(__FUNCTION__, "value mismatch %d != %f", i, get_value(array, double, i));
		}
	}
	exit(0);
}

#endif				/* STANDALONE */
