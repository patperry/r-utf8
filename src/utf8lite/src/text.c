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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utf8lite.h"


int utf8lite_text_init_copy(struct utf8lite_text *text,
			  const struct utf8lite_text *other)
{
        size_t size = UTF8LITE_TEXT_SIZE(other);
        size_t attr = other->attr;

	if (other->ptr) {
		if (!(text->ptr = malloc(size + 1))) {
			return UTF8LITE_ERROR_NOMEM;
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
#define HASH_SEED 5381
#define HASH_COMBINE(seed, v) (((hash) << 5) + (hash)) ^ ((size_t)(v))


static size_t hash_raw(const struct utf8lite_text *text)
{
	const uint8_t *ptr = text->ptr;
	const uint8_t *end = ptr + UTF8LITE_TEXT_SIZE(text);
	size_t hash = HASH_SEED;
	size_t ch;

	while (ptr != end) {
		ch = *ptr++;
		hash = HASH_COMBINE(hash, ch);
	}

	return hash;
}


size_t utf8lite_text_hash(const struct utf8lite_text *text)
{
	uint8_t buf[4];
	const uint8_t *ptr = text->ptr;
	const uint8_t *end = ptr + UTF8LITE_TEXT_SIZE(text);
	uint8_t *bufptr, *bufend;
	size_t hash = HASH_SEED;
	int32_t code;
	uint_fast8_t ch;

	if (!UTF8LITE_TEXT_HAS_ESC(text)) {
		return hash_raw(text);
	}

	while (ptr != end) {
		ch = *ptr++;
		if (ch == '\\') {
			utf8lite_decode_escape(&ptr, &code);

			bufptr = buf;
			bufend = bufptr;
			utf8lite_encode_utf8(code, &bufend);

			while (bufptr != bufend) {
				ch = *bufptr++;
				hash = HASH_COMBINE(hash, ch);
			}
		} else {
			hash = HASH_COMBINE(hash, ch);
		}
	}

	return hash;
}


int utf8lite_text_equals(const struct utf8lite_text *text1,
			 const struct utf8lite_text *text2)
{
	struct utf8lite_text_iter it1, it2;
	size_t n;

	if (text1->attr == text2->attr) {
		// same bits and size
		n = UTF8LITE_TEXT_SIZE(text1);
		return !memcmp(text1->ptr, text2->ptr, n);
	} else if (UTF8LITE_TEXT_BITS(text1) == UTF8LITE_TEXT_BITS(text2)) {
		// same bits, different size
		return 0;
	} else {
		// different bits or different size
		utf8lite_text_iter_make(&it1, text1);
		utf8lite_text_iter_make(&it2, text2);
		while (utf8lite_text_iter_advance(&it1)) {
			utf8lite_text_iter_advance(&it2);
			if (it1.current != it2.current) {
				return 0;
			}
		}
		return !utf8lite_text_iter_advance(&it2);
	}
}


static int compare_raw(const struct utf8lite_text *text1,
		       const struct utf8lite_text *text2)
{
	size_t n1 = UTF8LITE_TEXT_SIZE(text1);
	size_t n2 = UTF8LITE_TEXT_SIZE(text2);
	size_t n = (n1 < n2) ? n1 : n2;
	int cmp;

	cmp = memcmp(text1->ptr, text2->ptr, n);
	if (cmp == 0) {
		if (n1 < n2) {
			cmp = -1;
		} else if (n1 == n2) {
			cmp = 0;
		} else {
			cmp = +1;
		}
	}
	return cmp;
}


int utf8lite_text_compare(const struct utf8lite_text *text1,
			  const struct utf8lite_text *text2)
{
	struct utf8lite_text_iter it1, it2;

	if (!UTF8LITE_TEXT_HAS_ESC(text1) && !UTF8LITE_TEXT_HAS_ESC(text2)) {
		return compare_raw(text1, text2);
	}

	utf8lite_text_iter_make(&it1, text1);
	utf8lite_text_iter_make(&it2, text2);
	while (utf8lite_text_iter_advance(&it1)) {
		utf8lite_text_iter_advance(&it2);
		if (it1.current < it2.current) {
			return -1;
		} else if (it1.current > it2.current) {
			return +1;
		}
	}

	return utf8lite_text_iter_advance(&it2) ? -1 : 0;
}
