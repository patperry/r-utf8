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

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utf8lite.h"


int utf8lite_text_init_copy(struct utf8lite_text *text,
			  const struct utf8lite_text *other)
{
        size_t size = UTF8LITE_TEXT_SIZE(other);
        size_t attr = other->attr;
	int err;

	if (size) {
		if (!(text->ptr = malloc(size + 1))) {
			err = ENOMEM;
			return err;
		}

		memcpy(text->ptr, other->ptr, size);
		text->ptr[size] = '\0';
	} else {
		text->ptr = NULL;
	}
        text->attr = attr;
        return 0;
}


void utf8lite_text_destroy(struct utf8lite_text *text)
{
        free(text->ptr);
}


int utf8lite_text_isascii(const struct utf8lite_text *text)
{
	struct utf8lite_text_iter it;

	utf8lite_text_iter_make(&it, text);
	while (utf8lite_text_iter_advance(&it)) {
		if (!UTF8LITE_IS_ASCII(it.current)) {
			return 0;
		}
	}
	return 1;
}


// Dan Bernstein's djb2 XOR hash: http://www.cse.yorku.ca/~oz/hash.html
size_t utf8lite_text_hash(const struct utf8lite_text *text)
{
	const uint8_t *ptr = text->ptr;
	const uint8_t *end = ptr + UTF8LITE_TEXT_SIZE(text);
	size_t hash = 5381;
	uint_fast8_t ch;

	while (ptr != end) {
		ch = *ptr++;
		hash = ((hash << 5) + hash) ^ ch;
	}

	return hash;
}


int utf8lite_text_equals(const struct utf8lite_text *text1,
		       const struct utf8lite_text *text2)
{
	return ((text1->attr == text2->attr)
		&& !memcmp(text1->ptr, text2->ptr, UTF8LITE_TEXT_SIZE(text2)));
}


int utf8lite_compare_text(const struct utf8lite_text *text1,
			  const struct utf8lite_text *text2)
{
	size_t n1 = UTF8LITE_TEXT_SIZE(text1);
	size_t n2 = UTF8LITE_TEXT_SIZE(text2);
	size_t n = (n1 < n2) ? n1 : n2;
	return memcmp(text1->ptr, text2->ptr, n);
}
