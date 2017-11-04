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
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "utf8lite.h"


static int assign_esc(struct utf8lite_text *text,
		      const uint8_t *ptr, size_t size,
		      struct utf8lite_message *msg);
static void assign_esc_unsafe(struct utf8lite_text *text, const uint8_t *ptr,
			      size_t size);
static int assign_raw(struct utf8lite_text *text, const uint8_t *ptr,
		      size_t size, struct utf8lite_message *msg);
static void assign_raw_unsafe(struct utf8lite_text *text, const uint8_t *ptr,
			      size_t size);

static void append_location(struct utf8lite_message *msg, size_t offset);


int utf8lite_text_assign(struct utf8lite_text *text, const uint8_t *ptr,
			 size_t size, int flags, struct utf8lite_message *msg)
{
	int err = 0;

	if (size > UTF8LITE_TEXT_SIZE_MAX) {
		err = UTF8LITE_ERROR_OVERFLOW;
		utf8lite_message_set(msg, "text size (%"PRIu64" bytes)"
				     " exceeds maximum (%"PRIu64" bytes)",
				     (uint64_t)size,
				     (uint64_t)UTF8LITE_TEXT_SIZE_MAX);
	} else if (flags & UTF8LITE_TEXT_UNESCAPE) {
		if (flags & UTF8LITE_TEXT_VALID) {
			assign_esc_unsafe(text, ptr, size);
		} else {
			err = assign_esc(text, ptr, size, msg);
		}
	} else {
		if (flags & UTF8LITE_TEXT_VALID) {
			assign_raw_unsafe(text, ptr, size);
		} else {
			err = assign_raw(text, ptr, size, msg);
		}
	}

	if (err) {
		text->ptr = NULL;
		text->attr = 0;
	}

	return err;
}


int assign_raw(struct utf8lite_text *text, const uint8_t *ptr, size_t size,
	       struct utf8lite_message *msg)
{
	const uint8_t *input = ptr;
	const uint8_t *end = ptr + size;
	uint_fast8_t ch;
	int err;

	text->ptr = (uint8_t *)ptr;

	while (ptr != end) {
		ch = *ptr++;
		if (ch & 0x80) {
			ptr--;
			if ((err = utf8lite_scan_utf8(&ptr, end, msg))) {
				goto error;
			}
		}
	}

	text->attr = size;
	return 0;

error:
	append_location(msg, (size_t)(ptr - input));
	text->ptr = NULL;
	text->attr = 0;
	return err;
}


int assign_esc(struct utf8lite_text *text, const uint8_t *ptr, size_t size,
	       struct utf8lite_message *msg)
{
	const uint8_t *input = ptr;
	const uint8_t *start;
	const uint8_t *end = ptr + size;
	size_t attr = 0;
	int32_t code;
	uint_fast8_t ch;
	int err;

	text->ptr = (uint8_t *)ptr;

	while (ptr != end) {
		ch = *ptr++;
		if (ch == '\\') {
			attr |= UTF8LITE_TEXT_ESC_BIT;

			start = ptr;
			if ((err = utf8lite_scan_escape(&ptr, end, msg))) {
				goto error;
			}

			utf8lite_decode_escape(&start, &code);
		} else if (ch & 0x80) {
			ptr--;
			if ((err = utf8lite_scan_utf8(&ptr, end, msg))) {
				goto error;
			}
		}
	}

	attr |= size;
	text->attr = attr;
	return 0;

error:
	append_location(msg, (size_t)(ptr - input));
	return err;
}


void assign_raw_unsafe(struct utf8lite_text *text, const uint8_t *ptr,
		       size_t size)
{
	const uint8_t *end = ptr + size;
	uint_fast8_t ch;

	text->ptr = (uint8_t *)ptr;

	while (ptr != end) {
		ch = *ptr++;
		if (ch & 0x80) {
			ptr += UTF8LITE_UTF8_TAIL_LEN(ch);
		}
	}

	text->attr = size;
}



void assign_esc_unsafe(struct utf8lite_text *text, const uint8_t *ptr,
		       size_t size)
{
	const uint8_t *end = ptr + size;
	size_t attr = 0;
	int32_t code;
	uint_fast8_t ch;

	text->ptr = (uint8_t *)ptr;

	while (ptr != end) {
		ch = *ptr++;
		if (ch == '\\') {
			attr |= UTF8LITE_TEXT_ESC_BIT;
			ch = *ptr++;

			switch (ch) {
			case 'u':
				utf8lite_decode_uescape(&ptr, &code);
				break;
			default:
				break;
			}
		} else if (ch & 0x80) {
			ptr += UTF8LITE_UTF8_TAIL_LEN(ch);
		}
	}

	attr |= size;
	text->attr = attr;
}


void append_location(struct utf8lite_message *msg, size_t offset)
{
	utf8lite_message_append(msg, " at position %"PRIu64,
				(uint64_t)(offset + 1));
}
