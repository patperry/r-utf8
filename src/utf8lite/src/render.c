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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "private/array.h"
#include "utf8lite.h"


static void utf8lite_render_grow(struct utf8lite_render *r, int nadd)
{
	void *base = r->string;
	int size = r->length_max + 1;
	int err;

	if (r->error) {
		return;
	}

	if (nadd <= 0 || r->length_max - nadd > r->length) {
		return;
	}

	if ((err = utf8lite_array_grow(&base, &size, sizeof(*r->string),
				       r->length + 1, nadd))) {
		r->error = err;
		return;
	}

	r->string = base;
	r->length_max = size - 1;
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


void utf8lite_render_indent(struct utf8lite_render *r, int nlevel)
{
	r->indent += nlevel;
	assert(r->indent >= 0);
}


void utf8lite_render_newlines(struct utf8lite_render *r, int nline)
{
	char *end;
	int i;

	if (r->error) {
		return;
	}

	for (i = 0; i < nline; i++) {
		utf8lite_render_grow(r, r->newline_length);
		if (r->error) {
			return;
		}

		end = r->string + r->length;
		memcpy(end, r->newline, r->newline_length + 1); // include '\0'
		r->length += r->newline_length;
		r->needs_indent = 1;
	}
}


static void maybe_indent(struct utf8lite_render *r)
{
	int ntab = r->indent;
	char *end;
	int i;

	if (r->error) {
		return;
	}

	if (!r->needs_indent) {
		return;
	}

	for (i = 0; i < ntab; i++) {
		utf8lite_render_grow(r, r->tab_length);
		if (r->error) {
			return;
		}

		end = r->string + r->length;
		memcpy(end, r->tab, r->tab_length + 1); // include '\0'
		r->length += r->tab_length;
	}

	r->needs_indent = 0;
}


static void utf8lite_escape_utf8(struct utf8lite_render *r, int32_t ch)
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
	if (r->error) {
		return;
	}
	end = r->string + r->length;

	if (ch <= 0xFFFF) {
		sprintf(end, "\\u%04x", (unsigned)ch);
	} else if (r->flags & UTF8LITE_ENCODE_JSON) {
		hi = UTF8LITE_UTF16_HIGH(ch);
		lo = UTF8LITE_UTF16_LOW(ch);
		sprintf(end, "\\u%04x\\u%04x", hi, lo);
	} else {
		sprintf(end, "\\U%08x", (unsigned)ch);
	}

	r->length += len;
}


static void utf8lite_render_ascii(struct utf8lite_render *r, int32_t ch)
{
	char *end;

	// character expansion for a special escape: \X
	utf8lite_render_grow(r, 2);
	if (r->error) {
		return;
	}
	end = r->string + r->length;

	if ((ch <= 0x1F || ch == 0x7F)
			&& (r->flags & UTF8LITE_ESCAPE_CONTROL)) {
		switch (ch) {
		case '\a':
			if (r->flags & UTF8LITE_ENCODE_JSON) {
				utf8lite_escape_utf8(r, ch);
			} else {
				end[0] = '\\';
				end[1] = 'a';
				end[2] = '\0';
				r->length += 2;
			}
			break;
		case '\b':
			end[0] = '\\'; end[1] = 'b'; end[2] = '\0';
			r->length += 2;
			break;
		case '\f':
			end[0] = '\\'; end[1] = 'f'; end[2] = '\0';
			r->length += 2;
			break;
		case '\n':
			end[0] = '\\'; end[1] = 'n'; end[2] = '\0';
			r->length += 2;
			break;
		case '\r':
			end[0] = '\\'; end[1] = 'r'; end[2] = '\0';
			r->length += 2;
			break;
		case '\t':
			end[0] = '\\'; end[1] = 't'; end[2] = '\0';
			r->length += 2;
			break;
		case '\v':
			if (r->flags & UTF8LITE_ENCODE_JSON) {
				utf8lite_escape_utf8(r, ch);
			} else {
				end[0] = '\\';
				end[1] = 'v';
				end[2] = '\0';
				r->length += 2;
			}
			break;
		default:
			utf8lite_escape_utf8(r, ch);
			break;
		}
	} else {
		end[0] = (char)ch;
		end[1] = '\0';
		r->length++;
	}
}


void utf8lite_render_char(struct utf8lite_render *r, int32_t ch)
{
	char *end;
	uint8_t *uend;
	int type;

	if (r->error) {
		return;
	}

	maybe_indent(r);
	if (r->error) {
		return;
	}

	// maximum character expansion:
	// \uXXXX\uXXXX
	// 123456789012
	utf8lite_render_grow(r, 12);
	if (r->error) {
		return;
	}

	end = r->string + r->length;
	if (UTF8LITE_IS_ASCII(ch)) {
		utf8lite_render_ascii(r, ch);
		return;
	}

	type = utf8lite_charwidth(ch);
	switch (type) {
	case UTF8LITE_CHARWIDTH_OTHER:
		if (r->flags & UTF8LITE_ESCAPE_CONTROL) {
			utf8lite_escape_utf8(r, ch);
			return;
		}
		break;
	default:
		break;
	}

	uend = (uint8_t *)end;
	utf8lite_encode_utf8(ch, &uend);
	*uend = '\0';
	r->length += UTF8LITE_UTF8_ENCODE_LEN(ch);
}


void utf8lite_render_string(struct utf8lite_render *r, const char *str)
{
	const uint8_t *ptr = (const uint8_t *)str;
	int32_t ch;

	if (r->error) {
		return;
	}

	while (1) {
		utf8lite_decode_utf8(&ptr, &ch);
		if (ch == 0) {
			return;
		}
		utf8lite_render_char(r, ch);
		if (r->error) {
			return;
		}
	}
}


void utf8lite_render_printf(struct utf8lite_render *r, const char *format, ...)
{
	va_list ap, ap2;
	int err, len;

	if (r->error) {
		return;
	}

	va_start(ap, format);
	va_copy(ap2, ap);

	len = vsnprintf(NULL, 0, format, ap);
	if (len < 0) {
		// printf formatting error
		err = UTF8LITE_ERROR_OS;
		goto out;
	}

	utf8lite_render_grow(r, len);
	if (r->error) {
		goto out;
	}

	vsprintf(r->string + r->length, format, ap2);
	r->length += len;

out:
	va_end(ap);
	va_end(ap2);
}


void utf8lite_render_text(struct utf8lite_render *r,
			const struct utf8lite_text *text)
{
	struct utf8lite_text_iter it;

	if (r->error) {
		return;
	}

	utf8lite_text_iter_make(&it, text);
	while (utf8lite_text_iter_advance(&it)) {
		utf8lite_render_char(r, it.current);
		if (r->error) {
			return;
		}
	}
}
