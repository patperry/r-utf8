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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utf8lite.h"


static void utf8lite_textmap_clear_type(struct utf8lite_textmap *map);
static int utf8lite_textmap_set_type(struct utf8lite_textmap *map, int type);

static int utf8lite_textmap_reserve(struct utf8lite_textmap *map, size_t size);
static int utf8lite_textmap_set_ascii(struct utf8lite_textmap *map,
			     const struct utf8lite_text *text);
static int utf8lite_textmap_set_utf32(struct utf8lite_textmap *map,
				    const int32_t *ptr,
				    const int32_t *end);


int utf8lite_textmap_init(struct utf8lite_textmap *map, int type)
{
	int err;

	map->text.ptr = NULL;
	map->text.attr = 0;
	map->codes = NULL;
	map->size_max = 0;

	utf8lite_textmap_clear_type(map);
	err = utf8lite_textmap_set_type(map, type);
	return err;
}


void utf8lite_textmap_destroy(struct utf8lite_textmap *map)
{
	free(map->codes);
	free(map->text.ptr);
}


void utf8lite_textmap_clear_type(struct utf8lite_textmap *map)
{
	uint_fast8_t ch;

	map->charmap_type = UTF8LITE_DECOMP_NORMAL | UTF8LITE_CASEFOLD_NONE;

	for (ch = 0; ch < 0x80; ch++) {
		map->ascii_map[ch] = (int8_t)ch;
	}

	map->type = 0;
}


int utf8lite_textmap_set_type(struct utf8lite_textmap *map, int type)
{
	int_fast8_t ch;

	if (map->type == type) {
		return 0;
	}

	utf8lite_textmap_clear_type(map);

	if (type & UTF8LITE_TEXTMAP_CASE) {
		for (ch = 'A'; ch <= 'Z'; ch++) {
			map->ascii_map[ch] = ch + ('a' - 'A');
		}

		map->charmap_type |= UTF8LITE_CASEFOLD_ALL;
	}

	if (type & UTF8LITE_TEXTMAP_COMPAT) {
		map->charmap_type = UTF8LITE_DECOMP_ALL;
	}

	map->type = type;

	return 0;
}


int utf8lite_textmap_reserve(struct utf8lite_textmap *map, size_t size)
{
	uint8_t *ptr = map->text.ptr;
	int32_t *codes = map->codes;

	if (map->size_max >= size) {
		return 0;
	}

	if (!(ptr = realloc(ptr, size))) {
		return UTF8LITE_ERROR_NOMEM;
	}
	map->text.ptr = ptr;

	if (size > SIZE_MAX / UTF8LITE_UNICODE_DECOMP_MAX) {
		return UTF8LITE_ERROR_OVERFLOW;
	}

	if (!(codes = realloc(codes, size * UTF8LITE_UNICODE_DECOMP_MAX))) {
		return UTF8LITE_ERROR_NOMEM;
	}
	map->codes = codes;

	map->size_max = size;
	return 0;
}


int utf8lite_textmap_set(struct utf8lite_textmap *map,
		       const struct utf8lite_text *text)
{
	struct utf8lite_text_iter it;
	size_t size = UTF8LITE_TEXT_SIZE(text);
	int32_t *dst;
	int err;

	if (utf8lite_text_isascii(text)) {
		return utf8lite_textmap_set_ascii(map, text);
	}

	// For most inputs, mapping to type reduces or preserves the size.
	// However, for U+0390 and U+03B0, case folding triples the size.
	// (You can verify this with util/compute-typelen.py)
	//
	// Add one for a trailing NUL.
	if (size > ((SIZE_MAX - 1) / 3)) {
		err = UTF8LITE_ERROR_OVERFLOW;
		goto out;
	}

	if ((err = utf8lite_textmap_reserve(map, 3 * size + 1))) {
		goto out;
	}

	dst = map->codes;
	utf8lite_text_iter_make(&it, text);
	while (utf8lite_text_iter_advance(&it)) {
		utf8lite_map(map->charmap_type, it.current, &dst);
	}

	size = (size_t)(dst - map->codes);
	utf8lite_order(map->codes, size);
	utf8lite_compose(map->codes, &size);

	if ((err = utf8lite_textmap_set_utf32(map, map->codes,
					      map->codes + size))) {
		goto out;
	}

out:
	return err;
}


int utf8lite_textmap_set_utf32(struct utf8lite_textmap *map, const int32_t *ptr,
			     const int32_t *end)
{
	int map_quote = map->type & UTF8LITE_TEXTMAP_QUOTE;
	int rm_di = map->type & UTF8LITE_TEXTMAP_RMDI;
	uint8_t *dst = map->text.ptr;
	int32_t code;
	int8_t ch;

	while (ptr != end) {
		code = *ptr++;

		if (code <= 0x7F) {
			ch = map->ascii_map[code];
			if (ch >= 0) {
				*dst++ = (uint8_t)ch;
			}
			continue;
		} else {
			switch (code) {
			case 0x055A: // ARMENIAN APOSTROPHE
			case 0x2018: // LEFT SINGLE QUOTATION MARK
			case 0x2019: // RIGHT SINGLE QUOTATION MARK
			case 0x201B: // SINGLE HIGH-REVERSED-9 QUOTATION MARK
			case 0xFF07: // FULLWIDTH APOSTROPHE
				if (map_quote) {
					code = '\'';
				}
				break;

			default:
				if (rm_di && utf8lite_isignorable(code)) {
					continue;
				}
				break;
			}
		}
		utf8lite_encode_utf8(code, &dst);
	}

	*dst = '\0'; // not necessary, but helps with debugging
	map->text.attr = (UTF8LITE_TEXT_SIZE_MASK
			  & ((size_t)(dst - map->text.ptr)));
	return 0;
}


int utf8lite_textmap_set_ascii(struct utf8lite_textmap *map,
			       const struct utf8lite_text *text)
{
	struct utf8lite_text_iter it;
	size_t size = UTF8LITE_TEXT_SIZE(text);
	int8_t ch;
	uint8_t *dst;
	int err;

	assert(size < SIZE_MAX);

	if ((err = utf8lite_textmap_reserve(map, size + 1))) {
		goto error;
	}

	dst = map->text.ptr;

	utf8lite_text_iter_make(&it, text);
	while (utf8lite_text_iter_advance(&it)) {
		ch = map->ascii_map[it.current];
		if (ch >= 0) {
			*dst++ = (uint8_t)ch;
		}
	}

	*dst = '\0'; // not necessary, but helps with debugging
	map->text.attr = (UTF8LITE_TEXT_SIZE_MASK
			  & ((size_t)(dst - map->text.ptr)));
	return 0;

error:
	return err;
}
