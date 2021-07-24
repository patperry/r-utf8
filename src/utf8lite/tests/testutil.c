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
#include "../src/utf8lite.h"
#include "testutil.h"

static struct utf8lite_text *mktext(const char *str, int flags);


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


struct utf8lite_text *JS(const char *str)
{
	return mktext(str, UTF8LITE_TEXT_UNESCAPE);
}


struct utf8lite_text *S(const char *str)
{
	return mktext(str, 0);
}


struct utf8lite_text *mktext(const char *str, int flags)
{
	struct utf8lite_text *text = alloc(sizeof(*text));
	struct utf8lite_text text2;
	size_t size = strlen(str);
	uint8_t *ptr = alloc(size + 1);
	int err;

	memcpy(ptr, str, size + 1);
	err = utf8lite_text_assign(text, ptr, size, flags, NULL);
	ck_assert(!err);

	ck_assert(!utf8lite_text_assign(&text2, ptr, size,
					flags | UTF8LITE_TEXT_VALID, NULL));
	ck_assert(text->ptr == text2.ptr);
	ck_assert_uint_eq(text->attr, text2.attr);

	return text;
}
