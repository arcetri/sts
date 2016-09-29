// alloc.c
//
// dynamic array of things allocator

#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"


/*
 * forward static function declarations
 */
static void zero_chunk(enum type type, WORD64 chunk, void *data);
static void grow_dyn_array(struct dyn_array *array, int new_chunks);


/*
 * zero_chunk - zeroize a chunk of dynamic array elements according to type
 *
 * given:
 *      type            // the type of element in dynamic array
 *      chunk           // number of elements to expand by when allocating
 *      data            // pointer to data to zeroize
 *
 * Strictly speaking, zeroizing new chunk data is not needed.  We do it for
 * debugging purposes and as a firewall against going beyond array bounds.
 *
 * This function does not retun on error.
 */
static void
zero_chunk(enum type type, WORD64 chunk, void *data)
{
	ULONG i;

	/*
	 * firewall - sanity check args
	 */
	switch (type) {
	// known types
	case type_double:
	case type_word64:
		break;
	// unknown or invalid types
	case type_unknown:
	default:
		err(50, __FUNCTION__, "attempt to allocated unknown type: %d", type);
		break;
	}
	if (chunk <= 0) {
		err(50, __FUNCTION__, "chunk must be > 0: %u", chunk);
	}
	if (data == NULL) {
		err(50, __FUNCTION__, "data is NULL");
	}

	/*
	 * zerosize allocated data according to type
	 */
	for (i = 0; i < chunk; ++i) {
		switch (type) {
		case type_double:
			*(((double *)data)+i) = 0.0;
			break;
		case type_word64:
			*(((WORD64 *)data)+i) = (WORD64)0;
			break;
		default:
			err(50, __FUNCTION__, "bogus type while zeroing: %d", type);
			break;
		}
	}
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
	ULONG old_allocated;	// old number elements allocated
	ULONG new_allocated;	// new number elements allocated
	size_t old_octets;	// old size of data in dynamic array
	size_t new_octets;	// new size of data in dynamic array after allocation
	BYTE *p;
	ULONG i;

	/*
	 * firewall - sanity check args
	 */
	if (array == NULL) {
		err(51, __FUNCTION__, "array is NULL");
	}
	if (new_chunks <= 0) {
		err(51, __FUNCTION__, "chunks arg must be > 0: %d", new_chunks);
	}

	/*
	 * firewall - sanity check array
	 */
	if (array->name == NULL) {
		err(51, __FUNCTION__, "name in dynamic array is NULL");
	}
	if (array->data == NULL) {
		err(51, __FUNCTION__, "data in dynamic array for %s is NULL", array->name);
	}
	if (array->elm_size <= 0) {
		err(51, __FUNCTION__, "octets in dynamic array for %s must be > 0: %u", array->name, array->elm_size);
	}
	if (array->chunk <= 0) {
		err(51, __FUNCTION__, "chunk in dynamic array for %s must be > 0: %u", array->name, array->chunk);
	}
	if (array->count > array->allocated) {
		err(51, __FUNCTION__, "count: %u in dynamic array for %s must be <= allocated: %u",
		    array->count, array->name, array->allocated);
	}

	/*
	 * reallocate array
	 */
	// firewall - partial check for overflow
	old_allocated = array->allocated;
	new_allocated = old_allocated + (new_chunks * array->chunk);
	if (new_allocated <= old_allocated) {
		err(51, __FUNCTION__, "adding %u new chunks of %d elements would overflow dynamic array %s allocated: %u to %u",
		    new_chunks, array->chunk, array->name, old_allocated, new_allocated);
	}
	// firewall - partial check for size overflow
	old_octets = old_allocated * array->elm_size;
	new_octets = new_allocated * array->elm_size;
	if (new_octets <= old_octets) {
		err(51, __FUNCTION__, "adding %u new chunks of %d elements would overflow dynamic array %s octet size: %u to %u",
		    new_chunks, array->chunk, array->name, old_octets, new_octets);
	}
	data = realloc(array->data, new_octets);
	if (data == NULL) {
		errp(51, __FUNCTION__, "failed to add %u new chunks of %d elements to dynamic array %s octet size %u to %u",
		     new_chunks, array->chunk, array->name, old_octets, new_octets);
	}
	array->data = data;
	array->allocated = new_allocated;
	dbg(DBG_VHIGH, "expanded dynamic array %s of %d elemets of %d octets type %d from %u to %u octet size %u to %u",
	    array->name, array->chunk, array->elm_size, array->type, old_allocated, array->allocated, old_octets, new_octets);

	/*
	 * zerosize new chunks
	 */
	for (i = old_allocated, p = (BYTE *) (array->data) + old_octets;
	     i < new_allocated; i += array->chunk, p += (array->elm_size * array->chunk)) {
		zero_chunk(array->type, array->chunk, p);
	}
	return;
}



/*
 * create_dyn_array - create a dynamic array
 *
 * given:
 *      type            // the type of element in dynamic array
 *      chunk           // number of elements to expand by when allocating
 *      name            // debugging string describing this array
 *
 * returns:
 *      initialized (to zero) empty dynamic array
 *
 * This function does not return on error.
 */
struct dyn_array *
create_dyn_array(enum type type, WORD64 chunk, char *name)
{
	struct dyn_array *ret;	// created dynamic array to return
	ULONG octets;		// bytes in an single element

	/*
	 * firewall - sanity check args
	 *
	 * Also set the octets size.
	 */
	switch (type) {
	case type_double:
		octets = sizeof(double);
		break;
	case type_word64:
		octets = sizeof(ULONG);
		break;
	case type_unknown:
	default:
		octets = 0;
		err(52, __FUNCTION__, "attempt to create a dynamic array of an unknown type: %d", type);
		break;
	}
	if (octets <= 0) {
		err(52, __FUNCTION__, "octets must be > 0: %u", octets);
	}
	if (name == NULL) {
		err(52, __FUNCTION__, "name is NULL");
	}

	/*
	 * allocate new dynamic array
	 */
	ret = malloc(sizeof(struct dyn_array));
	if (ret == NULL) {
		errp(52, __FUNCTION__, "unable to malloc %d octets for dynamic array %s", sizeof(struct dyn_array), name);
	}

	/*
	 * initialize dynamic array
	 *
	 * Start off with an empty dynamic array of just 1 chunk
	 */
	ret->type = type;
	ret->elm_size = octets;
	ret->count = 0;		// allocated array is empty
	ret->allocated = chunk;	// will initially allocate just one chunk
	ret->chunk = chunk;
	ret->name = strdup(name);
	if (ret->name == NULL) {
		errp(52, __FUNCTION__, "unable to strdup %d octets", strlen(name));
	}
	ret->data = malloc(chunk * octets);
	if (ret->data == NULL) {
		errp(52, __FUNCTION__, "unable to initially allocate %d elemets of %d octets for dynamic array %s", chunk, octets,
		     name);
	}

	/*
	 * zerosize allocated data according to type
	 */
	zero_chunk(type, chunk, ret->data);

	/*
	 * return newly allocated array
	 */
	dbg(DBG_HIGH, "initalized empty dynamic array %s of %d elemets of %d octets type %d",
	    ret->name, ret->chunk, ret->elm_size, ret->type);
	return ret;
}


/*
 * append_value - append a value of a given type onto the end of a dynamic array
 *
 * given:
 *      array           // pointer to the dynamic array
 *      type            // the type of element in dynamic array
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
append_value(struct dyn_array *array, enum type type, void *value_p)
{
	BYTE *p;

	/*
	 * firewall - sanity check args
	 */
	if (array == NULL) {
		err(53, __FUNCTION__, "array is NULL");
	}
	if (value_p == NULL) {
		err(53, __FUNCTION__, "value_p is NULL");
	}

	/*
	 * firewall - sanity check array
	 */
	if (array->name == NULL) {
		err(53, __FUNCTION__, "name in dynamic array is NULL");
	}
	if (array->data == NULL) {
		err(53, __FUNCTION__, "data in dynamic array for %s is NULL", array->name);
	}
	if (array->elm_size <= 0) {
		err(53, __FUNCTION__, "octets in dynamic array for %s must be > 0: %u", array->name, array->elm_size);
	}
	if (array->chunk <= 0) {
		err(53, __FUNCTION__, "chunk in dynamic array for %s must be > 0: %u", array->name, array->chunk);
	}
	if (array->count > array->allocated) {
		err(53, __FUNCTION__, "count: %u in dynamic array for %s must be <= allocated: %u",
		    array->count, array->name, array->allocated);
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
	p = (BYTE *) (array->data) + (array->count * array->elm_size);
	switch (array->type) {
	case type_double:
		*(double *) p = *(double *) value_p;
		break;
	case type_word64:
		*(WORD64 *) p = *(WORD64 *) value_p;
		break;
	case type_unknown:
	default:
		err(53, __FUNCTION__, "dynamic array for %s has unknown type: %d", array->name, array->type);
		break;
	}
	++array->count;

	/*
	 * firewall - ensure that the dynamic array is never full
	 */
	if (array->count >= array->allocated) {
		grow_dyn_array(array, 1);
	}
	return;
}


#if defined(STANDALONE)

int debuglevel = DBG_NONE;
const char * const version = "dyn_test";
char * program = "dyn_test";

const char * const testNames[NUMOFTESTS+1] = {
    " ", "Frequency", "BlockFrequency", "CumulativeSums", "Runs", "LongestRun", "Rank",
    "FFT", "NonOverlappingTemplate", "OverlappingTemplate", "Universal",
    "ApproximateEntropy", "RandomExcursions", "RandomExcursionsVariant",
    "Serial", "LinearComplexity"
};


/*ARGSUSED*/
main(int argc, char *argv[])
{
	struct state s;				// running state
	struct dyn_array *array;		// dynatic array to test
	double d;				// test double
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
	for (d=0.0; d < 1000000.0; d += 1.0) {
		append_double(array, d);
	}

	/*
	 * verify values
	 */
	for (i=0; i < 1000000; ++i) {
		if ((double)i != get_double(array, i)) {
			warn(__FUNCTION__, "value mismatch %d != %f", i, get_double(array, i));
		}
	}
	exit(0);
}

#endif /* STANDALONE */
