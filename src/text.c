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
#include "text.h"

/* http://stackoverflow.com/a/11986885 */
#define hextoi(ch) ((ch > '9') ? (ch &~ 0x20) - 'A' + 10 : (ch - '0'))

#define UTF8LITE_TEXT_CODE_NONE ((uint32_t)-1)


static void iter_retreat_escaped(struct utf8lite_text_iter *it,
				 const uint8_t *begin);
static void iter_retreat_raw(struct utf8lite_text_iter *it);


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


void utf8lite_text_iter_make(struct utf8lite_text_iter *it,
			   const struct utf8lite_text *text)
{
	it->ptr = text->ptr;
	it->end = it->ptr + UTF8LITE_TEXT_SIZE(text);
	it->text_attr = text->attr;
	it->attr = 0;
	it->current = UTF8LITE_TEXT_CODE_NONE;
}


int utf8lite_text_iter_can_advance(const struct utf8lite_text_iter *it)
{
	return (it->ptr != it->end);
}


int utf8lite_text_iter_advance(struct utf8lite_text_iter *it)
{
	const uint8_t *ptr = it->ptr;
	size_t text_attr = it->text_attr;
	uint32_t code;
	size_t attr;

	if (!utf8lite_text_iter_can_advance(it)) {
		goto at_end;
	}

	attr = 0;
	code = *ptr++;

	if (code == '\\' && (text_attr & UTF8LITE_TEXT_ESC_BIT)) {
		attr = UTF8LITE_TEXT_ESC_BIT;
		utf8lite_decode_escape(&ptr, &code);
		if (code >= 0x80) {
			attr |= UTF8LITE_TEXT_UTF8_BIT;
		}
	} else if (code >= 0x80) {
		attr = UTF8LITE_TEXT_UTF8_BIT;
		ptr--;
		utf8lite_decode_utf8(&ptr, &code);
	}

	it->ptr = ptr;
	it->current = code;
	it->attr = attr;
	return 1;

at_end:
	it->current = UTF8LITE_TEXT_CODE_NONE;
	it->attr = 0;
	return 0;
}


void utf8lite_text_iter_reset(struct utf8lite_text_iter *it)
{
	const size_t size = (it->text_attr & UTF8LITE_TEXT_SIZE_MASK);
	const uint8_t *begin = it->end - size;

	it->ptr = begin;
	it->current = UTF8LITE_TEXT_CODE_NONE;
	it->attr = 0;
}


void utf8lite_text_iter_skip(struct utf8lite_text_iter *it)
{
	it->ptr = it->end;
	it->current = UTF8LITE_TEXT_CODE_NONE;
	it->attr = 0;
}


int utf8lite_text_iter_can_retreat(const struct utf8lite_text_iter *it)
{
	const size_t size = (it->text_attr & UTF8LITE_TEXT_SIZE_MASK);
	const uint8_t *begin = it->end - size;
	const uint8_t *ptr = it->ptr;
	uint32_t code = it->current;
	struct utf8lite_text_iter it2;

	if (ptr > begin + 12) {
		return 1;
	}

	if (ptr == begin) {
		return 0;
	}

	if (!(it->attr & UTF8LITE_TEXT_ESC_BIT)) {
		return (ptr != begin + UTF8LITE_UTF8_ENCODE_LEN(code));
	}

	it2 = *it;
	iter_retreat_escaped(&it2, begin);

	return (it2.ptr != begin);
}


int utf8lite_text_iter_retreat(struct utf8lite_text_iter *it)
{
	const size_t size = (it->text_attr & UTF8LITE_TEXT_SIZE_MASK);
	const uint8_t *begin = it->end - size;
	const uint8_t *ptr = it->ptr;
	const uint8_t *end = it->end;
	uint32_t code = it->current;

	if (ptr == begin) {
		return 0;
	}

	if (it->text_attr & UTF8LITE_TEXT_ESC_BIT) {
		iter_retreat_escaped(it, begin);
	} else {
		iter_retreat_raw(it);
	}

	// we were at the end of the text
	if (code == UTF8LITE_TEXT_CODE_NONE) {
		it->ptr = end;
		return 1;
	}

	// at this point, it->code == code, and it->ptr is the code start
	ptr = it->ptr;

	if (ptr == begin) {
		it->current = UTF8LITE_TEXT_CODE_NONE;
		it->attr = 0;
		return 0;
	}

	// read the previous code
	if (it->text_attr & UTF8LITE_TEXT_ESC_BIT) {
		iter_retreat_escaped(it, begin);
	} else {
		iter_retreat_raw(it);
	}

	// now, it->code is the previous code, and it->ptr is the start
	// of the previous code

	// set the pointer to the end of the previous code
	it->ptr = ptr;
	return 1;
}


void iter_retreat_raw(struct utf8lite_text_iter *it)
{
	const uint8_t *ptr = it->ptr;
	uint32_t code;

	code = *(--ptr);

	if (code < 0x80) {
		it->ptr = (uint8_t *)ptr;
		it->current = code;
		it->attr = 0;
	} else {
		// skip over continuation bytes
		do {
			ptr--;
		} while (*ptr < 0xC0);

		it->ptr = (uint8_t *)ptr;

		utf8lite_decode_utf8(&ptr, &it->current);
		it->attr = UTF8LITE_TEXT_UTF8_BIT;
	}
}


// we are at an escape if we are preceded by an odd number of
// backslash (\) characters
static int at_escape(const uint8_t *begin, const uint8_t *ptr)
{
	int at = 0;
	uint_fast8_t prev;

	while (begin < ptr) {
		prev = *(--ptr);

		if (prev != '\\') {
			goto out;
		}

		at = !at;
	}

out:
	return at;
}


void iter_retreat_escaped(struct utf8lite_text_iter *it, const uint8_t *begin)
{
	const uint8_t *ptr = it->ptr;
	uint32_t code, unesc, hi;
	size_t attr;
	int i;

	code = *(--ptr);
	attr = 0;

	// check for 2-byte escape
	switch (code) {
	case '"':
	case '\\':
	case '/':
		unesc = code;
		break;

	case 'b':
		unesc = '\b';
		break;

	case 'f':
		unesc = '\f';
		break;
	case 'n':
		unesc = '\n';
		break;

	case 'r':
		unesc = '\r';
		break;

	case 't':
		unesc = '\t';
		break;

	default:
		unesc = 0;
		break;
	}

	if (unesc) {
	       if (at_escape(begin, ptr)) {
		       ptr--;
		       code = unesc;
		       attr = UTF8LITE_TEXT_ESC_BIT;
	       }
	       goto out;
	}

	// check for 6-byte escape
	if (isxdigit((int)code)) {
		if (!(begin + 4 < ptr && ptr[-4] == 'u'
				&& at_escape(begin, ptr - 4))) {
			goto out;
		}
		attr = UTF8LITE_TEXT_ESC_BIT;

		code = 0;
		for (i = 0; i < 4; i++) {
			code = (code << 4) + hextoi(ptr[i - 3]);
		}
		ptr -= 5;

		if (code >= 0x80) {
			attr |= UTF8LITE_TEXT_UTF8_BIT;
		}

		if (UTF8LITE_IS_UTF16_LOW(code)) {
			hi = 0;
			for (i = 0; i < 4; i++) {
				hi = (hi << 4) + hextoi(ptr[i - 4]);
			}

			code = UTF8LITE_DECODE_UTF16_PAIR(hi, code);
			ptr -= 6;
		}

		goto out;
	}

	// check for ascii
	if (code < 0x80) {
		goto out;
	}

	// if we got here, then code is a continuation byte

	// skip over preceding continuation bytes
	do {
		ptr--;
	} while (*ptr < 0xC0);

	// decode the utf-8 value
	it->ptr = (uint8_t *)ptr;
	utf8lite_decode_utf8(&ptr, &it->current);
	it->attr = UTF8LITE_TEXT_UTF8_BIT;
	return;

out:
	it->ptr = (uint8_t *)ptr;
	it->current = code;
	it->attr = attr;
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
	return ((text1->attr & ~UTF8LITE_TEXT_UTF8_BIT)
			== (text2->attr & ~UTF8LITE_TEXT_UTF8_BIT)
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
