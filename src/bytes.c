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
#include <ctype.h>
#include <limits.h>
#include "rutf8.h"

static int byte_width(uint8_t byte, int flags);
static void render_byte(struct utf8lite_render *r, uint8_t byte);


int rutf8_bytes_width(const struct rutf8_bytes *bytes, int flags)
{
	const uint8_t *ptr = bytes->ptr;
	const uint8_t *end = ptr + bytes->size;
	uint8_t byte;
	int width, w;

	width = 0;
	while (ptr != end) {
		byte = *ptr++;
		w = byte_width(byte, flags);
		if (w < 0) {
			return -1;
		}
		if (width > INT_MAX - w) {
			Rf_error("width exceeds maximum (%d)", INT_MAX);
		}
		width += w;
	}

	return width;
}


int rutf8_bytes_lwidth(const struct rutf8_bytes *bytes, int flags, int limit)
{
	const uint8_t *ptr = bytes->ptr;
	const uint8_t *end = ptr + bytes->size;
	uint8_t byte;
	int width, w, ellipsis = 3;

	width = 0;
	while (ptr != end) {
		byte = *ptr++;
		w = byte_width(byte, flags);
		assert(w >= 0);
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}

	return width;
}


int rutf8_bytes_rwidth(const struct rutf8_bytes *bytes, int flags, int limit)
{
	const uint8_t *ptr = bytes->ptr;
	const uint8_t *end = ptr + bytes->size;
	uint8_t byte;
	int width, w, ellipsis = 3;

	width = 0;
	while (ptr != end) {
		byte = *ptr++;
		w = byte_width(byte, flags);
		assert(w >= 0);
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}

	return width;
}


static void rutf8_bytes_lrender(struct utf8lite_render *r,
				const struct rutf8_bytes *bytes,
				int width_min, int quote, int centre)
{
	const uint8_t *ptr, *end;
	uint8_t byte;
	int err = 0, w, fullwidth, width, quotes;

	assert(width_min >= 0);

	quotes = quote ? 2 : 0;
	width = 0;

	if (centre && width_min > 0) {
		fullwidth = rutf8_bytes_width(bytes, r->flags);
		if (fullwidth <= width_min - quotes) {
			width = (width_min - fullwidth - quotes) / 2;
			TRY(utf8lite_render_chars(r, ' ', width));
		}
	}

	if (quote) {
		TRY(utf8lite_render_raw(r, "\"", 1));
		assert(width < INT_MAX);
		width++;
	}

	ptr = bytes->ptr;
	end = bytes->ptr + bytes->size;

	while (ptr < end) {
		byte = *ptr++;
		w = byte_width(byte, r->flags);
		render_byte(r, byte);

		assert(w >= 0);
		if (width <= width_min - w) {
			width += w;
		} else {
			width = width_min; // truncate to avoid overflow
		}
	}

	if (quote) {
		TRY(utf8lite_render_raw(r, "\"", 1));
		if (width < width_min) { // avoid overflow
			width++;
		}
	}

	if (width < width_min) {
		TRY(utf8lite_render_chars(r, ' ', width_min - width));
	}
exit:
	CHECK_ERROR(err);
}


static void rutf8_bytes_rrender(struct utf8lite_render *r,
				const struct rutf8_bytes *bytes,
				int width_min, int quote)
{
	const uint8_t *ptr, *end;
	uint8_t byte;
	int err = 0, fullwidth, quotes;

	assert(width_min >= 0);

	quotes = quote ? 2 : 0;

	if (width_min > 0) {
		fullwidth = rutf8_bytes_width(bytes, r->flags);
		// ensure fullwidth + quotes doesn't overflow
		if (fullwidth <= width_min - quotes) {
			fullwidth += quotes;
			TRY(utf8lite_render_chars(r, ' ',
						  width_min - fullwidth));
		}
	}

	if (quote) {
		TRY(utf8lite_render_raw(r, "\"", 1));
	}

	ptr = bytes->ptr;
	end = bytes->ptr + bytes->size;

	while (ptr < end) {
		byte = *ptr++;
		render_byte(r, byte);
	}

	if (quote) {
		TRY(utf8lite_render_raw(r, "\"", 1));
	}
exit:
	CHECK_ERROR(err);
}


void rutf8_bytes_render(struct utf8lite_render *r,
		       const struct rutf8_bytes *bytes,
		       int width, int quote, enum rutf8_justify_type justify)
{
	int centre;
	if (justify == RUTF8_JUSTIFY_RIGHT) {
		rutf8_bytes_rrender(r, bytes, width, quote);
	} else {
		centre = (justify == RUTF8_JUSTIFY_CENTRE);
		rutf8_bytes_lrender(r, bytes, width, quote, centre);
	}
}


static SEXP rutf8_bytes_lformat(struct utf8lite_render *r,
				const struct rutf8_bytes *bytes,
				int trim, int chars, int quote,
				int flags, int width_max, int centre)
{
	SEXP ans = R_NilValue;
	const uint8_t *ptr, *end;
	uint8_t byte;
	int err = 0, w, trunc, bfill, efill, fullwidth, width, quotes;

	quotes = quote ? 2 : 0;

	bfill = 0;
	if (centre && !trim) {
		fullwidth = (rutf8_bytes_lwidth(bytes, flags, chars) + quotes);
		if (fullwidth < width_max) {
			bfill = (width_max - fullwidth) / 2;
			TRY(utf8lite_render_chars(r, ' ', bfill));
		}
	}

	width = 0;
	trunc = 0;
	ptr = bytes->ptr;
	end = bytes->ptr + bytes->size;

	while (!trunc && ptr < end) {
		byte = *ptr++;
		w = byte_width(byte, flags);

		if (width > chars - w) {
			w = 3;
			TRY(utf8lite_render_raw(r, "...", 3));
			trunc = 1;
		} else {
			render_byte(r, byte);
		}

		width += w;
	}

	if (!trim) {
		efill = width_max - width - quotes - bfill;
		TRY(utf8lite_render_chars(r, ' ', efill));
	}

	ans = mkCharLenCE((char *)r->string, r->length, CE_BYTES);
	utf8lite_render_clear(r);
exit:
	CHECK_ERROR(err);
	return ans;
}


static SEXP rutf8_bytes_rformat(struct utf8lite_render *r,
				const struct rutf8_bytes *bytes,
				int trim, int chars, int quote,
				int flags, int width_max)
{
	SEXP ans = R_NilValue;
	const uint8_t *ptr, *end;
	uint8_t byte;
	int err = 0, w, width, quotes, trunc;

	quotes = quote ? 2 : 0;

	end = bytes->ptr + bytes->size;
	ptr = end;
	width = 0;
	trunc = 0;

	while (!trunc && ptr > bytes->ptr) {
		byte = *--ptr;
		w = byte_width(byte, flags);

		if (width > chars - w) {
			w = 3; // ...
			trunc = 1;
		}

		width += w;
	}

	if (!trim) {
		TRY(utf8lite_render_chars(r, ' ', width_max - width - quotes));
	}

	if (trunc) {
		TRY(utf8lite_render_raw(r, "...", 3));
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


SEXP rutf8_bytes_format(struct utf8lite_render *r,
			const struct rutf8_bytes *bytes,
			int trim, int chars, enum rutf8_justify_type justify,
			int quote, int flags, int width_max)
{
	int centre;

	if (justify == RUTF8_JUSTIFY_RIGHT) {
		return rutf8_bytes_rformat(r, bytes, trim, chars, quote,
					   flags, width_max);
	} else {
		centre = (justify == RUTF8_JUSTIFY_CENTRE);
		return rutf8_bytes_lformat(r, bytes, trim, chars, quote,
					   flags, width_max, centre);
	}
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
			return (flags & UTF8LITE_ESCAPE_CONTROL) ? 2 : -1;
		case '\\':
			return (flags & (UTF8LITE_ESCAPE_CONTROL
						| UTF8LITE_ESCAPE_DQUOTE)
					? 2 : 1);
		case '"':
			return (flags & UTF8LITE_ESCAPE_DQUOTE) ? 2 : 1;
		default:
			if (isprint((int)byte)) {
				return 1;
			}
			break;
		}
	}

	// \xXX non-ASCII or non-printable byte
	return (flags & UTF8LITE_ESCAPE_CONTROL) ? 4 : -1;
}


void render_byte(struct utf8lite_render *r, uint8_t byte)
{
	char ch, buf[5];
	int err = 0;

	if (byte <= 0x1f || byte >= 0x7f) {
		if (r->flags & UTF8LITE_ESCAPE_CONTROL) {
			switch (byte) {
			case '\a':
				TRY(utf8lite_render_raw(r, "\\a", 2));
				break;
			case '\b':
				TRY(utf8lite_render_raw(r, "\\b", 2));
				break;
			case '\f':
				TRY(utf8lite_render_raw(r, "\\f", 2));
				break;
			case '\n':
				TRY(utf8lite_render_raw(r, "\\n", 2));
				break;
			case '\r':
				TRY(utf8lite_render_raw(r, "\\r", 2));
				break;
			case '\t':
				TRY(utf8lite_render_raw(r, "\\t", 2));
				break;
			case '\v':
				TRY(utf8lite_render_raw(r, "\\v", 2));
				break;
			default:
				sprintf(buf, "\\x%02x", (unsigned)byte);
				TRY(utf8lite_render_raw(r, buf, 4));
				break;
			}
		} else {
			ch = (char)byte;
			TRY(utf8lite_render_raw(r, &ch, 1));
		}
	} else {
		buf[0] = (char)byte;
		buf[1] = '\0';
		TRY(utf8lite_render_string(r, buf));
	}
exit:
	CHECK_ERROR(err);
}
