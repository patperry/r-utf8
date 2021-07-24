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
#include "utf8lite.h"

/* http://stackoverflow.com/a/11986885 */
#define hextoi(ch) ((ch > '9') ? (ch &~ 0x20) - 'A' + 10 : (ch - '0'))

static void iter_retreat_escaped(struct utf8lite_text_iter *it,
				 const uint8_t *begin);
static void iter_retreat_raw(struct utf8lite_text_iter *it);

void utf8lite_text_iter_make(struct utf8lite_text_iter *it,
			   const struct utf8lite_text *text)
{
	it->ptr = text->ptr;
	it->end = it->ptr + UTF8LITE_TEXT_SIZE(text);
	it->text_attr = text->attr;
	it->current = UTF8LITE_CODE_NONE;
}


int utf8lite_text_iter_advance(struct utf8lite_text_iter *it)
{
	const uint8_t *ptr = it->ptr;
	size_t text_attr = it->text_attr;
	int32_t code;

	if (it->ptr == it->end) {
		goto at_end;
	}

	code = *ptr++;

	if (code == '\\' && (text_attr & UTF8LITE_TEXT_ESC_BIT)) {
		utf8lite_decode_escape(&ptr, &code);
	} else if (code >= 0x80) {
		ptr--;
		utf8lite_decode_utf8(&ptr, &code);
	}

	it->ptr = ptr;
	it->current = code;
	return 1;

at_end:
	it->current = UTF8LITE_CODE_NONE;
	return 0;
}


void utf8lite_text_iter_skip(struct utf8lite_text_iter *it)
{
	it->ptr = it->end;
	it->current = UTF8LITE_CODE_NONE;
}


int utf8lite_text_iter_retreat(struct utf8lite_text_iter *it)
{
	const size_t size = (it->text_attr & UTF8LITE_TEXT_SIZE_MASK);
	const uint8_t *begin = it->end - size;
	const uint8_t *ptr = it->ptr;
	const uint8_t *end = it->end;
	int32_t code = it->current;

	if (ptr == begin) {
		return 0;
	}

	if (it->text_attr & UTF8LITE_TEXT_ESC_BIT) {
		iter_retreat_escaped(it, begin);
	} else {
		iter_retreat_raw(it);
	}

	// we were at the end of the text
	if (code == UTF8LITE_CODE_NONE) {
		it->ptr = end;
		return 1;
	}

	// at this point, it->code == code, and it->ptr is the code start
	ptr = it->ptr;

	if (ptr == begin) {
		it->current = UTF8LITE_CODE_NONE;
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


void utf8lite_text_iter_reset(struct utf8lite_text_iter *it)
{
	const size_t size = (it->text_attr & UTF8LITE_TEXT_SIZE_MASK);
	const uint8_t *begin = it->end - size;

	it->ptr = begin;
	it->current = UTF8LITE_CODE_NONE;
}


void iter_retreat_raw(struct utf8lite_text_iter *it)
{
	const uint8_t *ptr = it->ptr;
	int32_t code;

	code = *(--ptr);

	if (code < 0x80) {
		it->ptr = (uint8_t *)ptr;
		it->current = code;
	} else {
		// skip over continuation bytes
		do {
			ptr--;
		} while (*ptr < 0xC0);

		it->ptr = (uint8_t *)ptr;

		utf8lite_decode_utf8(&ptr, &it->current);
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
	int32_t code, unesc, hi;
	int i;

	code = *(--ptr);

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
	       }
	       goto out;
	}

	// check for 6-byte escape
	if (isxdigit((int)code)) {
		if (!(begin + 4 < ptr && ptr[-4] == 'u'
				&& at_escape(begin, ptr - 4))) {
			goto out;
		}

		code = 0;
		for (i = 0; i < 4; i++) {
			code = (code << 4) + hextoi(ptr[i - 3]);
		}
		ptr -= 5;

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
	return;

out:
	it->ptr = (uint8_t *)ptr;
	it->current = code;
}
