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
#include "rutf8.h"

static int byte_width(uint8_t byte, int flags);
static void render_byte(struct utf8lite_render *r, uint8_t byte);


int rutf8_bytes_width(const struct rutf8_bytes *bytes, int limit,
		      int ellipsis, int flags)
{
	const uint8_t *ptr = bytes->ptr;
	const uint8_t *end = ptr + bytes->size;
	uint8_t byte;
	int width, w;

	width = 0;
	while (ptr != end) {
		byte = *ptr++;
		w = byte_width(byte, flags);
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}

	return width;
}


int rutf8_bytes_rwidth(const struct rutf8_bytes *bytes, int limit,
		       int ellipsis, int flags)
{
	const uint8_t *ptr = bytes->ptr;
	const uint8_t *end = ptr + bytes->size;
	uint8_t byte;
	int width, w;

	width = 0;
	while (ptr != end) {
		byte = *ptr++;
		w = byte_width(byte, flags);
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}

	return width;
}


SEXP rutf8_bytes_format(struct utf8lite_render *r,
			const struct rutf8_bytes *bytes,
			int trim, int chars, int width_max,
			int quote, int utf8, int flags, int centre)
{
	SEXP ans = R_NilValue;
	const uint8_t *ptr, *end;
	const char *ellipsis_str;
	uint8_t byte;
	int err = 0, w, trunc, bfill, fullwidth, width, quotes, ellipsis;

	quotes = quote ? 2 : 0;
	ellipsis = utf8 ? 1 : 3;
	ellipsis_str = utf8 ? RUTF8_ELLIPSIS : "...";

	bfill = 0;
	if (centre && !trim) {
		fullwidth = (rutf8_bytes_width(bytes, chars, ellipsis, flags)
			     + quotes);
		bfill = centre_pad_begin(r, width_max, fullwidth);
	}

	width = 0;
	trunc = 0;
	ptr = bytes->ptr;
	end = bytes->ptr + bytes->size;

	while (!trunc && ptr < end) {
		byte = *ptr++;
		w = byte_width(byte, flags);

		if (width > chars - w) {
			w = ellipsis;
			TRY(utf8lite_render_string(r, ellipsis_str));
			trunc = 1;
		} else {
			render_byte(r, byte);
		}

		width += w;
	}

	if (!trim) {
		pad_spaces(r, width_max - width - quotes - bfill);
	}

	ans = mkCharLenCE((char *)r->string, r->length, CE_BYTES);
	utf8lite_render_clear(r);
exit:
	CHECK_ERROR(err);
	return ans;
}


SEXP rutf8_bytes_rformat(struct utf8lite_render *r,
			 const struct rutf8_bytes *bytes,
			 int trim, int chars, int width_max, int quote,
			 int utf8, int flags)
{
	SEXP ans = R_NilValue;
	const uint8_t *ptr, *end;
	const char *ellipsis_str;
	uint8_t byte;
	int err = 0, w, width, quotes, ellipsis, trunc;

	quotes = quote ? 2 : 0;
	ellipsis = utf8 ? 1 : 3;
	ellipsis_str = utf8 ? RUTF8_ELLIPSIS : "...";

	end = bytes->ptr + bytes->size;
	ptr = end;
	width = 0;
	trunc = 0;

	while (!trunc && ptr > bytes->ptr) {
		byte = *--ptr;
		w = byte_width(byte, flags);

		if (width > chars - w) {
			w = ellipsis;
			trunc = 1;
		}

		width += w;
	}

	if (!trim) {
		pad_spaces(r, width_max - width - quotes);
	}

	if (trunc) {
		TRY(utf8lite_render_string(r, ellipsis_str));
	}

	while (ptr < end) {
		byte = *ptr++;
		render_byte(r, byte);
	}

	ans = mkCharLenCE((char *)r->string, r->length, CE_BYTES);
	utf8lite_render_clear(r);
exit:
	CHECK_ERROR(err);
	return ans;
}


int byte_width(uint8_t byte, int flags)
{
	if (byte < 0x80) {
		switch (byte) {
		case '\a':
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
		case '\v':
		case '\\':
			return 2;
		case '"':
			return (flags & UTF8LITE_ESCAPE_DQUOTE) ? 2 : 1;
		default:
			if (isprint((int)byte)) {
				return 1;
			}
			break;
		}
	}

	return 4; // \xXX non-ASCII or non-printable byte
}


void render_byte(struct utf8lite_render *r, uint8_t byte)
{
	int err = 0;
	char ch;

	ch = (char)byte;
	TRY(utf8lite_render_bytes(r, &ch, 1));

exit:
	CHECK_ERROR(err);
}
