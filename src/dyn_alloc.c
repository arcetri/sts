// alloc.c - dynamic array of things allocator

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

// Exit codes: 60 thru 69

#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dyn_alloc.h"
#include "debug.h"


extern long int debuglevel;	// -v lvl: defines the level of verbosity for debugging


/*
 * forward static function declarations
 */
static void zero_chunk(size_t elm_size, long int chunk, void *data);
static void grow_dyn_array(struct dyn_array *array, int new_chunks);


/*
 * zero_chunk - zeroize a chunk of dynamic array elements
 *
 * given:
 *      elm_size        // size of a single element
 *      chunk           // number of elements to expand by when allocating
 *      data            // pointer to data to zeroize
 *
 * Strictly speaking, zeroizing new chunk data is not needed.  We do it for
 * debugging purposes and as a firewall against going beyond array bounds.
 *
 * This function does not retun on error.
 */
static void
zero_chunk(size_t elm_size, long int chunk, void *data)
{
	/*
	 * firewall - sanity check args
	 */
	if (elm_size <= 0) {
		err(60, __FUNCTION__, "elm_size must be > 0: %ld", elm_size);
	}
	if (chunk <= 0) {
		err(60, __FUNCTION__, "chunk must be > 0: %ld", chunk);
	}
	if (data == NULL) {
		err(60, __FUNCTION__, "data arg is NULL");
	}

	/*
	 * zerosize allocated data according to type
	 */
	memset(data, 0, elm_size * chunk);
	return;
}


/*
 * grow_dyn_array - grow the allocation of a dynamic array by a number of chunks
 *
 * given:
 *      array           pointer to the dynamic array
 *      new_chunks      number of chunks to increase
 *
 * The dynanmic array allocation will be expanded by chunks.  That data will
 * be zeroized according to the type.
 *
 * This function does not return on error.
 */
static void
grow_dyn_array(struct dyn_array *array, int new_chunks)
{
	void *data;		// reallocated chunks
	long int old_allocated;	// old number elements allocated
	long int new_allocated;	// new number elements allocated
	size_t old_bytes;	// old size of data in dynamic array
	size_t new_bytes;	// new size of data in dynamic array after allocation
	unsigned char *p;
	long int i;

	/*
	 * firewall - sanity check args
	 */
	if (array == NULL) {
		err(61, __FUNCTION__, "array arg is NULL");
	}
	if (new_chunks <= 0) {
		err(61, __FUNCTION__, "chunks arg must be > 0: %d", new_chunks);
	}

	/*
	 * firewall - sanity check array
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

	/*
	 * reallocate array
	 */
	// firewall - partial check for overflow
	old_allocated = array->allocated;
	new_allocated = old_allocated + (new_chunks * array->chunk);
	if (new_allocated <= old_allocated) {
		err(61, __FUNCTION__, "adding %d new chunks of %ld elements would overflow dynamic array: allocated: %ld to %ld",
		    new_chunks, array->chunk, old_allocated, new_allocated);
	}
	// firewall - partial check for size overflow
	old_bytes = old_allocated * array->elm_size;
	new_bytes = new_allocated * array->elm_size;
	if (new_bytes <= old_bytes) {
		err(61, __FUNCTION__, "adding %d new chunks of %ld elements would overflow dynamic array: octet size: %ld to %ld",
		    new_chunks, array->chunk, old_bytes, new_bytes);
	}
	data = realloc(array->data, new_bytes);
	if (data == NULL) {
		errp(61, __FUNCTION__, "failed to add %d new chunks of %ld elements to dynamic array: octet size %ld to %ld",
		     new_chunks, array->chunk, old_bytes, new_bytes);
	}
	array->data = data;
	array->allocated = new_allocated;
	dbg(DBG_VHIGH, "expanded dynamic array of %ld elemets of size %lu bytes: from %ld to %ld octet size %ld to %ld",
	    array->chunk, array->elm_size, old_allocated, array->allocated, old_bytes, new_bytes);

	/*
	 * zerosize new chunks if needed
	 */
	if (array->zeroize == 1) {
		for (i = old_allocated, p = (unsigned char *) (array->data) + old_bytes;
		     i < new_allocated; i += array->chunk, p += (array->elm_size * array->chunk)) {
			zero_chunk(array->elm_size, array->chunk, p);
		}
	}
	return;
}



/*
 * create_dyn_array - create a dynamic array
 *
 * given:
 *      elm_size        // size of an element
 *      chunk           // number of elements to expand by when allocating
 *      start_chunks    // starting number of chucks to allocate
 *      zeroize         // 1 --> always zero newly allocated chunks, 0 --> don't
 *
 * returns:
 *      initialized (to zero) empty dynamic array
 *
 * This function does not return on error.
 */
struct dyn_array *
create_dyn_array(size_t elm_size, long int chunk, long int start_chunks, int zeroize)
{
	struct dyn_array *ret;	// created dynamic array to return

	/*
	 * firewall - sanity check args
	 *
	 * Also set the byte size.
	 */
	if (elm_size <= 0) {
		err(62, __FUNCTION__, "elm_size must be > 0: %ld", elm_size);
	}
	if (chunk <= 0) {
		err(62, __FUNCTION__, "chunk must be > 0: %ld", chunk);
	}
	if (start_chunks <= 0) {
		err(62, __FUNCTION__, "start_chunks must be > 0: %ld", start_chunks);
	}

	/*
	 * allocate new dynamic array
	 */
	ret = malloc(sizeof(struct dyn_array));
	if (ret == NULL) {
		errp(62, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for dyn_array",
		     (long int) 1, sizeof(struct dyn_array));
	}

	/*
	 * initialize dynamic array
	 *
	 * Start off with an empty dynamic array of just 1 chunk
	 */
	ret->elm_size = elm_size;
	ret->zeroize = zeroize;
	ret->count = 0;		// allocated array is empty
	ret->allocated = start_chunks;
	ret->chunk = chunk;
	ret->data = malloc(ret->allocated * elm_size);
	if (ret->data == NULL) {
		errp(62, __FUNCTION__, "cannot malloc of %ld elements of %ld bytes each for dyn_array->data", start_chunks,
		     elm_size);
	}

	/*
	 * zerosize allocated data according to type
	 */
	if (ret->zeroize == 1) {
		zero_chunk(elm_size, chunk, ret->data);
	}

	/*
	 * return newly allocated array
	 */
	dbg(DBG_HIGH, "initalized empty dynamic array of %ld elemets of %ld bytes per element", ret->chunk, ret->elm_size);
	return ret;
}


/*
 * append_value - append a value of a given type onto the end of a dynamic array
 *
 * given:
 *      array           // pointer to the dynamic array
 *      value_p         // pointer value to add to the end of the dynamic array
 *
 * We will add a value of a given type onto the end of a dynamic array.
 * We will grow the dynamic array if all allocated values are used.
 * If after adding the value, all allocated values are used, we will grow
 * the dynamic array as a firewall.
 *
 * This function does not return on error.
 */
void
append_value(struct dyn_array *array, void *value_p)
{
	unsigned char *p;

	/*
	 * firewall - sanity check args
	 */
	if (array == NULL) {
		err(63, __FUNCTION__, "array arg is NULL");
	}
	if (value_p == NULL) {
		err(63, __FUNCTION__, "value_p arg is NULL");
	}

	/*
	 * firewall - sanity check array
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
	 * expand dynamic array if needed
	 */
	if (array->count == array->allocated) {
		grow_dyn_array(array, 1);
	}

	/*
	 * We know the dynamic array has enough room to append, so append the value
	 */
	p = (unsigned char *) (array->data) + (array->count * array->elm_size);
	memcpy(p, value_p, array->elm_size);
	++array->count;
	return;
}


/*
 * free_dyn_array - free a dynamic array
 *
 * given:
 *      array           // pointer to the dynamic array
 */
void
free_dyn_array(struct dyn_array *array)
{
	/*
	 * firewall - sanity check args
	 */
	if (array == NULL) {
		err(64, __FUNCTION__, "array arg is NULL");
	}

	/*
	 * free any storage this dynamic array might have
	 */
	if (array->data != NULL) {
		free(array->data);
		array->data = NULL;
	}

	/*
	 * zero the count and allocation
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
