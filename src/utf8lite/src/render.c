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
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "private/array.h"
#include "utf8lite.h"

#define CHECK_ERROR(r) \
	if (r->error) { \
		return r->error; \
	}

enum code_type {
	CODE_ASCII = 0,
	CODE_UTF8 = (1 << 0),
	CODE_EXTENDED = (1 << 1),
	CODE_EMOJI = (1 << 2)
};

/**
 * Render a single code. If any render escape flags are set, filter
 * the character through the appropriate escaping.
 *
 * \param r the render object
 * \param ch the character (UTF-32)
 * \param attrptr on exit, a bit mask of #code_type flags
 *
 * \returns 0 on success
 */
static int utf8lite_render_code(struct utf8lite_render *r, int32_t ch,
				int *attrptr);

static int utf8lite_render_grow(struct utf8lite_render *r, int nadd)
{
	void *base = r->string;
	int size = r->length_max + 1;
	int err;

	if (nadd <= 0 || r->length_max - nadd > r->length) {
		return 0;
	}

	if ((err = utf8lite_array_grow(&base, &size, sizeof(*r->string),
				       r->length + 1, nadd))) {
		r->error = err;
		return r->error;
	}

	r->string = base;
	r->length_max = size - 1;
	return 0;
}


static int style_open(struct utf8lite_render *r)
{
	if (!r->style_open_length) {
		return 0;
	}

	return utf8lite_render_raw(r, r->style_open, r->style_open_length);
}


static int style_close(struct utf8lite_render *r)
{
	if (!r->style_close_length) {
		return 0;
	}

	return utf8lite_render_raw(r, r->style_close, r->style_close_length);
}


int utf8lite_render_init(struct utf8lite_render *r, int flags)
{
	int err;

	r->string = malloc(1);
	if (!r->string) {
		err = UTF8LITE_ERROR_NOMEM;
		return err;
	}

	r->length = 0;
	r->length_max = 0;
	r->flags = flags;

	r->tab = "\t";
	r->tab_length = (int)strlen(r->tab);

	r->newline = "\n";
	r->newline_length = (int)strlen(r->newline);

	r->style_open = NULL;
	r->style_open_length = 0;
	r->style_close = NULL;
	r->style_close_length = 0;

	utf8lite_render_clear(r);

	return 0;
}


void utf8lite_render_destroy(struct utf8lite_render *r)
{
	free(r->string);
}


void utf8lite_render_clear(struct utf8lite_render *r)
{
	r->string[0] = '\0';
	r->length = 0;
	r->indent = 0;
	r->needs_indent = 1;
	r->error = 0;
}


int utf8lite_render_set_flags(struct utf8lite_render *r, int flags)
{
	r->flags = flags;
	return 0;
}


int utf8lite_render_set_tab(struct utf8lite_render *r, const char *tab)
{
	size_t len;
	assert(tab);

	if ((len = strlen(tab)) >= INT_MAX) {
		// tab string length exceeds maximum
		r->error = UTF8LITE_ERROR_OVERFLOW;
		return r->error;
	} else {
		r->tab = tab;
		r->tab_length = (int)len;
		return 0;
	}
}


int utf8lite_render_set_newline(struct utf8lite_render *r, const char *newline)
{
	size_t len;
	assert(newline);

	if ((len = strlen(newline)) >= INT_MAX) {
		// newline string length exceeds maximum
		r->error = UTF8LITE_ERROR_OVERFLOW;
		return r->error;
	} else {
		r->newline = newline;
		r->newline_length = (int)len;
		return 0;
	}
}


int utf8lite_render_set_style(struct utf8lite_render *r,
			      const char *open, const char *close)
{
	size_t open_len = 0, close_len = 0;

	CHECK_ERROR(r);
	if (open) {
		if ((open_len = strlen(open)) >= INT_MAX) {
			r->error = UTF8LITE_ERROR_OVERFLOW;
			return r->error;
		}
	}
	if (close) {
		if ((close_len = strlen(close)) >= INT_MAX) {
			r->error = UTF8LITE_ERROR_OVERFLOW;
			return r->error;
		}
	}
	r->style_open = open;
	r->style_close = close;
	r->style_open_length = (int)open_len;
	r->style_close_length = (int)close_len;
	return 0;
}


int utf8lite_render_indent(struct utf8lite_render *r, int nlevel)
{
	CHECK_ERROR(r);

	if (nlevel > INT_MAX - r->indent) {
		r->error = UTF8LITE_ERROR_OVERFLOW;
	} else {
		r->indent += nlevel;
		if (r->indent < 0) {
			r->indent = 0;
		}
	}
	return r->error;
}


int utf8lite_render_newlines(struct utf8lite_render *r, int nline)
{
	char *end;
	int i;

	CHECK_ERROR(r);

	for (i = 0; i < nline; i++) {
		utf8lite_render_grow(r, r->newline_length);
		CHECK_ERROR(r);

		end = r->string + r->length;
		memcpy(end, r->newline, r->newline_length + 1); // include '\0'
		r->length += r->newline_length;
		r->needs_indent = 1;
	}

	return 0;
}


static int maybe_indent(struct utf8lite_render *r)
{
	int ntab = r->indent;
	char *end;
	int i;

	if (!r->needs_indent) {
		return 0;
	}

	for (i = 0; i < ntab; i++) {
		utf8lite_render_grow(r, r->tab_length);
		CHECK_ERROR(r);

		end = r->string + r->length;
		memcpy(end, r->tab, r->tab_length + 1); // include '\0'
		r->length += r->tab_length;
	}

	r->needs_indent = 0;
	return 0;
}


static int utf8lite_escape_utf8(struct utf8lite_render *r, int32_t ch)
{
	unsigned hi, lo;
	int len;

	style_open(r);
	CHECK_ERROR(r);

	if (ch <= 0xFFFF) {
		// \uXXXX
		len = 6;
	} else if (r->flags & UTF8LITE_ENCODE_JSON) {
		// \uXXXX\uYYYY
		len = 12;
	} else {
		// \UXXXXYYYY
		len = 10;
	}

	utf8lite_render_grow(r, len);
	CHECK_ERROR(r);

	if (ch <= 0xFFFF) {
		r->length += sprintf(&r->string[r->length],
				     "\\u%04x", (unsigned)ch);
	} else if (r->flags & UTF8LITE_ENCODE_JSON) {
		hi = UTF8LITE_UTF16_HIGH(ch);
		lo = UTF8LITE_UTF16_LOW(ch);
		r->length += sprintf(&r->string[r->length],
				     "\\u%04x\\u%04x", hi, lo);
	} else {
		r->length += sprintf(&r->string[r->length],
				     "\\U%08"PRIx32, (uint32_t)ch);
	}

	style_close(r);
	CHECK_ERROR(r);

	return 0;
}


static int utf8lite_escape_ascii(struct utf8lite_render *r, int32_t ch)
{
	style_open(r);
	CHECK_ERROR(r);

	// character expansion for a special escape: \uXXXX or \X
	utf8lite_render_grow(r, 6);
	CHECK_ERROR(r);

	if (ch <= 0x1F || ch == 0x7F) {
		switch (ch) {
		case '\a':
			if (r->flags & UTF8LITE_ENCODE_JSON) {
				r->length += sprintf(&r->string[r->length],
						     "\\u%04x", (unsigned)ch);
			} else {
				r->string[r->length++] = '\\';
				r->string[r->length++] = 'a';
				r->string[r->length] = '\0';
			}
			break;
		case '\b':
			r->string[r->length++] = '\\';
			r->string[r->length++] = 'b';
			r->string[r->length] = '\0';
			break;
		case '\f':
			r->string[r->length++] = '\\';
			r->string[r->length++] = 'f';
			r->string[r->length] = '\0';
			break;
		case '\n':
			r->string[r->length++] = '\\';
			r->string[r->length++] = 'n';
			r->string[r->length] = '\0';
			break;
		case '\r':
			r->string[r->length++] = '\\';
			r->string[r->length++] = 'r';
			r->string[r->length] = '\0';
			break;
		case '\t':
			r->string[r->length++] = '\\';
			r->string[r->length++] = 't';
			r->string[r->length] = '\0';
			break;
		case '\v':
			if (r->flags & UTF8LITE_ENCODE_JSON) {
				r->length += sprintf(&r->string[r->length],
						     "\\u%04x", (unsigned)ch);
			} else {
				r->string[r->length++] = '\\';
				r->string[r->length++] = 'v';
				r->string[r->length] = '\0';
			}
			break;
		default:
			r->length += sprintf(&r->string[r->length],
					     "\\u%04x", (unsigned)ch);
			break;
		}

		style_close(r);
		CHECK_ERROR(r);
	} else {
		r->string[r->length++] = '\\';
		r->string[r->length] = '\0';
		style_close(r);
		CHECK_ERROR(r);

		utf8lite_render_grow(r, 1);
		CHECK_ERROR(r);
		r->string[r->length++] = (char)ch;
		r->string[r->length] = '\0';
	}

	return 0;
}


static int utf8lite_render_ascii(struct utf8lite_render *r, int32_t ch)
{
	if ((ch <= 0x1F || ch == 0x7F)
			&& (r->flags & UTF8LITE_ESCAPE_CONTROL)) {
		return utf8lite_escape_ascii(r, ch);
	}

	switch (ch) {
	case '\"':
		if (r->flags & UTF8LITE_ESCAPE_DQUOTE) {
			return utf8lite_escape_ascii(r, ch);
		}
		break;
	case '\'':
		if (r->flags & UTF8LITE_ESCAPE_SQUOTE) {
			return utf8lite_escape_ascii(r, ch);
		}
		break;
	case '\\':
		if (r->flags & (UTF8LITE_ESCAPE_CONTROL
					| UTF8LITE_ESCAPE_DQUOTE
					| UTF8LITE_ESCAPE_SQUOTE
					| UTF8LITE_ESCAPE_EXTENDED
					| UTF8LITE_ESCAPE_UTF8)) {
			return utf8lite_escape_ascii(r, ch);
		}
		break;
	default:
		break;
	}

	utf8lite_render_grow(r, 1);
	CHECK_ERROR(r);

	r->string[r->length++] = (char)ch;
	r->string[r->length] = '\0';
	return 0;
}


int utf8lite_render_code(struct utf8lite_render *r, int32_t ch, int *attrptr)
{
	char *end;
	uint8_t *uend;
	int type;

	CHECK_ERROR(r);

	maybe_indent(r);
	CHECK_ERROR(r);

	// maximum character expansion: \uXXXX\uXXXX
	utf8lite_render_grow(r, 12);
	CHECK_ERROR(r);

	end = r->string + r->length;
	if (UTF8LITE_IS_ASCII(ch)) {
		return utf8lite_render_ascii(r, ch);
	} else if (r->flags & UTF8LITE_ESCAPE_UTF8) {
		return utf8lite_escape_utf8(r, ch);
	}

	if (ch > 0xFFFF) {
		if (r->flags & UTF8LITE_ESCAPE_EXTENDED) {
			return utf8lite_escape_utf8(r, ch);
		} else {
			*attrptr |= CODE_EXTENDED;
		}
	}
	*attrptr |= CODE_UTF8;

	type = utf8lite_charwidth(ch);
	switch (type) {
	case UTF8LITE_CHARWIDTH_NONE:
		if (r->flags & UTF8LITE_ESCAPE_CONTROL) {
			return utf8lite_escape_utf8(r, ch);
		}
		break;

	case UTF8LITE_CHARWIDTH_IGNORABLE:
		if ((r->flags & UTF8LITE_ENCODE_RMDI)
				&& (!(*attrptr & CODE_EMOJI))) {
			return 0;
		}
		break;

	case UTF8LITE_CHARWIDTH_EMOJI:
		*attrptr |= CODE_EMOJI;
		break;

	default:
		break;
	}

	uend = (uint8_t *)end;
	utf8lite_encode_utf8(ch, &uend);
	*uend = '\0';
	r->length += UTF8LITE_UTF8_ENCODE_LEN(ch);
	return 0;
}


int utf8lite_render_char(struct utf8lite_render *r, int32_t ch)
{
	uint8_t buf[5];
	uint8_t *ptr = buf;

	// decode to string, then render. 'render_string' handles
	// escaping, removing default ignorables, etc. if necessary
	utf8lite_encode_utf8(ch, &ptr);
	*ptr = '\0';
	return utf8lite_render_string(r, (const char *)buf);
}


int utf8lite_render_chars(struct utf8lite_render *r, int32_t ch, int nchar)
{
	CHECK_ERROR(r);

	while (nchar-- > 0) {
		utf8lite_render_char(r, ch);
		CHECK_ERROR(r);
	}

	return 0;
}


int utf8lite_render_string(struct utf8lite_render *r, const char *str)
{
	struct utf8lite_text text;
	const uint8_t *ptr;
	size_t len;

	CHECK_ERROR(r);

	ptr = (const uint8_t *)str;
	len = strlen(str);
	r->error = utf8lite_text_assign(&text, ptr, len, 0, NULL);
	CHECK_ERROR(r);

	return utf8lite_render_text(r, &text);
}


int utf8lite_render_printf(struct utf8lite_render *r, const char *format, ...)
{
	char *buffer;
	va_list ap, ap2;
	int len;

	CHECK_ERROR(r);

	va_start(ap, format);
	va_copy(ap2, ap);

	len = vsnprintf(NULL, 0, format, ap);
	if (len < 0) {
		// printf formatting error
		r->error = UTF8LITE_ERROR_OS;
		goto exit;
	}

	if (!(buffer = malloc((size_t)len + 1))) {
		r->error = UTF8LITE_ERROR_NOMEM;
		goto exit;
	}

	vsprintf(buffer, format, ap2);
	utf8lite_render_string(r, buffer);
	free(buffer);

exit:
	va_end(ap);
	va_end(ap2);
	return r->error;
}


int utf8lite_render_text(struct utf8lite_render *r,
			 const struct utf8lite_text *text)
{
	struct utf8lite_graphscan scan;

	CHECK_ERROR(r);

	utf8lite_graphscan_make(&scan, text);
	while (utf8lite_graphscan_advance(&scan)) {
		utf8lite_render_graph(r, &scan.current);
		CHECK_ERROR(r);
	}

	return 0;
}


int utf8lite_render_graph(struct utf8lite_render *r,
			 const struct utf8lite_graph *g)
{
	struct utf8lite_text_iter it;
	int attr = CODE_ASCII;

	CHECK_ERROR(r);

	utf8lite_text_iter_make(&it, &g->text);
	while (utf8lite_text_iter_advance(&it)) {
		utf8lite_render_code(r, it.current, &attr);
		CHECK_ERROR(r);
	}

	if (attr & CODE_EMOJI && (r->flags & UTF8LITE_ENCODE_EMOJIZWSP)) {
		utf8lite_render_raw(r, "\xE2\x80\x8B", 3); // U+200B, ZWSP
		CHECK_ERROR(r);
	}

	return 0;
}


int utf8lite_render_raw(struct utf8lite_render *r, const char *bytes,
			size_t size)
{
	if (size > INT_MAX) {
		r->error = UTF8LITE_ERROR_OVERFLOW;
		return r->error;
	}
	utf8lite_render_grow(r, (int)size);
	CHECK_ERROR(r);

	memcpy(r->string + r->length, bytes, size);
	r->length += (int)size;
	r->string[r->length] = '\0';
	return 0;
}
