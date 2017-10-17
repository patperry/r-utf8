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

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <check.h>
#include "testutil.h"


static void **allocs;
static int nalloc;


void setup(void)
{
	allocs = NULL;
	nalloc = 0;
}


void teardown(void)
{
	while (nalloc-- > 0) {
		free(allocs[nalloc]);
	}
	free(allocs);
}


void *alloc(size_t size)
{
	void *ptr;

	allocs = realloc(allocs, (size_t)(nalloc + 1) * sizeof(*allocs));
	ck_assert(allocs != NULL);

	ptr = malloc(size);
	ck_assert(ptr != NULL || size == 0);

	allocs[nalloc] = ptr;
	nalloc++;

	return ptr;
}
