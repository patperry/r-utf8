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
	int oldflags = r->flags;
	r->flags = flags;
	return oldflags;
}


const char *utf8lite_render_set_tab(struct utf8lite_render *r, const char *tab)
{
	const char *oldtab = r->tab;
	size_t len;
	assert(tab);

	if ((len = strlen(tab)) >= INT_MAX) {
		// tab string length exceeds maximum
		r->error = UTF8LITE_ERROR_OVERFLOW;

	} else {
		r->tab = tab;
		r->tab_length = (int)len;
	}

	return oldtab;
}


const char *utf8lite_render_set_newline(struct utf8lite_render *r,
					const char *newline)
{
	const char *oldnewline = r->newline;
	size_t len;
	assert(newline);

	if ((len = strlen(newline)) >= INT_MAX) {
		// newline string length exceeds maximum
		r->error = UTF8LITE_ERROR_OVERFLOW;
	} else {
		r->newline = newline;
		r->newline_length = (int)len;
	}

	return oldnewline;
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
	char *end;
	unsigned hi, lo;
	int len;

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

	end = r->string + r->length;

	if (ch <= 0xFFFF) {
		sprintf(end, "\\u%04x", (unsigned)ch);
	} else if (r->flags & UTF8LITE_ENCODE_JSON) {
		hi = UTF8LITE_UTF16_HIGH(ch);
		lo = UTF8LITE_UTF16_LOW(ch);
		sprintf(end, "\\u%04x\\u%04x", hi, lo);
	} else {
		sprintf(end, "\\U%08"PRIx32, (uint32_t)ch);
	}

	r->length += len;
	return 0;
}


static int utf8lite_render_ascii(struct utf8lite_render *r, int32_t ch)
{
	char *end;

	// character expansion for a special escape: \X
	utf8lite_render_grow(r, 2);
	CHECK_ERROR(r);

	end = r->string + r->length;

	if ((ch <= 0x1F || ch == 0x7F)
			&& (r->flags & UTF8LITE_ESCAPE_CONTROL)) {
		switch (ch) {
		case '\a':
			if (r->flags & UTF8LITE_ENCODE_JSON) {
				return utf8lite_escape_utf8(r, ch);
			}
			end[0] = '\\';
			end[1] = 'a';
			r->length += 2;
			break;
		case '\b':
			end[0] = '\\';
			end[1] = 'b';
			r->length += 2;
			break;
		case '\f':
			end[0] = '\\';
			end[1] = 'f';
			r->length += 2;
			break;
		case '\n':
			end[0] = '\\';
			end[1] = 'n';
			r->length += 2;
			break;
		case '\r':
			end[0] = '\\';
			end[1] = 'r';
			r->length += 2;
			break;
		case '\t':
			end[0] = '\\';
			end[1] = 't';
			r->length += 2;
			break;
		case '\v':
			if (r->flags & UTF8LITE_ENCODE_JSON) {
				return utf8lite_escape_utf8(r, ch);
			}
			end[0] = '\\';
			end[1] = 'v';
			r->length += 2;
			break;
		default:
			return utf8lite_escape_utf8(r, ch);
		}
	} else {
		switch (ch) {
		case '\"':
			if (r->flags & UTF8LITE_ESCAPE_DQUOTE) {
				end[0] = '\\';
				end[1] = '\"';
				r->length += 2;
				goto exit;
			}
			break;
		case '\'':
			if (r->flags & UTF8LITE_ESCAPE_SQUOTE) {
				end[0] = '\\';
				end[1] = '\'';
				r->length += 2;
				goto exit;
			}
			break;
		case '\\':
			if (r->flags & (UTF8LITE_ESCAPE_CONTROL
						| UTF8LITE_ESCAPE_DQUOTE
						| UTF8LITE_ESCAPE_SQUOTE
						| UTF8LITE_ESCAPE_EXTENDED
						| UTF8LITE_ESCAPE_UTF8)) {
				end[0] = '\\';
				end[1] = '\\';
				r->length += 2;
				goto exit;
			}
			break;
		default:
			break;
		}

		end[0] = (char)ch;
		r->length++;
	}
exit:
	r->string[r->length] = '\0';
	return 0;
}


int utf8lite_render_char(struct utf8lite_render *r, int32_t ch)
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
	} else if (ch > 0xFFFF && (r->flags & UTF8LITE_ESCAPE_EXTENDED)) {
		return utf8lite_escape_utf8(r, ch);
	}

	type = utf8lite_charwidth(ch);
	switch (type) {
	case UTF8LITE_CHARWIDTH_NONE:
		if (r->flags & UTF8LITE_ESCAPE_CONTROL) {
			return utf8lite_escape_utf8(r, ch);
		}
		break;
	case UTF8LITE_CHARWIDTH_IGNORABLE:
		if (r->flags & UTF8LITE_ENCODE_RMDI) {
			return 0;
		}
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
	struct utf8lite_text_iter it;

	CHECK_ERROR(r);

	utf8lite_text_iter_make(&it, text);
	while (utf8lite_text_iter_advance(&it)) {
		utf8lite_render_char(r, it.current);
		CHECK_ERROR(r);
	}

	return 0;
}


static int ascii_width(int32_t ch, int flags)
{
	// handle control characters
	if (ch <= 0x1F || ch == 0x7F) {
		if (!(flags & UTF8LITE_ESCAPE_CONTROL)) {
			return 0;
		}

		switch (ch) {
		case '\a':
		case '\v':
			// \u0007, \u000b (JSON) : \a, \b (C)
			return (flags & UTF8LITE_ENCODE_JSON) ? 6 : 2;
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
			return 2;
		default:
			return 6; // \uXXXX
		}
	}

	// handle printable characters
	switch (ch) {
	case '\"':
		return (flags & UTF8LITE_ESCAPE_DQUOTE) ? 2 : 1;
	case '\'':
		return (flags & UTF8LITE_ESCAPE_SQUOTE) ? 2 : 1;
	case '\\':
		if (flags & (UTF8LITE_ESCAPE_CONTROL
				| UTF8LITE_ESCAPE_DQUOTE
				| UTF8LITE_ESCAPE_SQUOTE
				| UTF8LITE_ESCAPE_EXTENDED
				| UTF8LITE_ESCAPE_UTF8)) {
			return 2;
		} else {
			return 1;
		}
	default:
		return 1;
	}
}


static int utf8_escape_width(int32_t ch, int flags)
{
	if (ch <= 0xFFFF) {
		return 6;
	} else if (flags & UTF8LITE_ENCODE_JSON) {
		return 12;
	} else {
		return 10;
	}
}


static int utf8_width(int32_t ch, int cw, int flags)
{
	int w;

	switch ((enum utf8lite_charwidth_type)cw) {
	case UTF8LITE_CHARWIDTH_NONE:
		if (flags & UTF8LITE_ESCAPE_CONTROL) {
			w = utf8_escape_width(ch, flags);
		} else {
			w = 0;
		}
		break;

	case UTF8LITE_CHARWIDTH_IGNORABLE:
	case UTF8LITE_CHARWIDTH_MARK:
		w = 0;
		break;

	case UTF8LITE_CHARWIDTH_NARROW:
		w = 1;
		break;

	case UTF8LITE_CHARWIDTH_AMBIGUOUS:
		w = (flags & UTF8LITE_ENCODE_AMBIGWIDE) ? 2 : 1;
		break;

	case UTF8LITE_CHARWIDTH_WIDE:
	case UTF8LITE_CHARWIDTH_EMOJI:
		w = 2;
		break;
	}

	return w;
}


int utf8lite_render_measure(const struct utf8lite_render *r,
			    const struct utf8lite_graph *g,
			    int *widthptr)
{
	struct utf8lite_text_iter it;
	int32_t ch;
	int err = 0, cw, w, width;

	width = 0;
	utf8lite_text_iter_make(&it, &g->text);

	while (utf8lite_text_iter_advance(&it)) {
		ch = it.current;

		if (ch <= 0x7F) {
			w = ascii_width(ch, r->flags);
		} else if (r->flags & UTF8LITE_ESCAPE_UTF8) {
			w = utf8_escape_width(ch, r->flags);
		} else if ((r->flags & UTF8LITE_ESCAPE_EXTENDED)
				&& (ch > 0xFFFF)) {
			w = utf8_escape_width(ch, r->flags);
		} else {
			cw = utf8lite_charwidth(ch);
			if (cw == UTF8LITE_CHARWIDTH_EMOJI) {
				width = 2;
				goto exit;
			}
			w = utf8_width(ch, cw, r->flags);
		}

		if (w > INT_MAX - width) {
			width = -1;
			err = UTF8LITE_ERROR_OVERFLOW;
			goto exit;
		}

		width += w;
	}

exit:
	if (widthptr) {
		*widthptr = width;
	}
	return err;
}
