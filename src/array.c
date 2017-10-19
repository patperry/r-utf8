/*
 * Copyright 2017 Patrick O. Perry.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include "rutf8.h"


/* Default initial size for nonempty dynamic arrays. Must be positive. */
#define ARRAY_SIZE_INIT	32

/* Growth factor for dynamic arrays. Must be greater than 1.
 *
 *     https://en.wikipedia.org/wiki/Dynamic_array#Growth_factor
 */
#define ARRAY_GROW 1.618 /* Golden Ratio, (1 + sqrt(5)) / 2 */


int bigarray_size_add(size_t *sizeptr, size_t width, size_t count, size_t nadd)
{
	size_t size = *sizeptr;
	size_t size_min;
	int err;
	double n1;

	if (width == 0) {
		return 0;
	}

	if (count > (SIZE_MAX - nadd) / width) {
		err = EINVAL;
		Rf_error("array size (%"PRIu64" + %"PRIu64
			 " elements of %"PRIu64" bytes each)"
			 " exceeds maximum (%"PRIu64" elements)",
			 (uint64_t)count, (uint64_t)nadd,
			 (uint64_t)width, (uint64_t)SIZE_MAX);
		return err;
	}

	size_min = count + nadd;
	if (size >= size_min) {
		return 0;
	}

	assert(ARRAY_SIZE_INIT > 0);
	assert(ARRAY_GROW > 1);

	if (size < ARRAY_SIZE_INIT && size_min > 0) {
		size = ARRAY_SIZE_INIT;
	}

	while (size < size_min) {
		n1 = ARRAY_GROW * size;
		if (n1 > SIZE_MAX / width) {
			size = SIZE_MAX / width;
		} else {
			size = (size_t)n1;
		}
	}

	*sizeptr = size;
	return 0;
}


int array_size_add(int *sizeptr, size_t width, int count, int nadd)
{
	size_t size, size_min, size_max;
	int err;

	assert(*sizeptr >= 0);
	assert(count >= 0);
	assert(nadd >= 0);

	if (width == 0) {
		return 0;
	}

	size = (size_t)*sizeptr;
	if ((err = bigarray_size_add(&size, width, (size_t)count,
					    (size_t)nadd))) {
		return err;
	}
	size_max = (size_t)INT_MAX / width;
	if (size > size_max) {
		size = size_max;
		size_min = (size_t)count + (size_t)nadd;
		if (size < size_min) {
			err = EINVAL;
			Rf_error("array size (%"PRIu64
				 " elements of %"PRIu64" bytes each)"
				 " exceeds maximum (%"PRIu64" elements)",
				 (uint64_t)size_min, (uint64_t)width,
				 (uint64_t)size_max);
			return err;
		}
	}

	*sizeptr = (int)size;
	return 0;
}
