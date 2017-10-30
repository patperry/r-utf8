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
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "rutf8.h"


#define ELLIPSIS "\xE2\x80\xA6" // U+2026, width 1

enum justify_type {
	JUSTIFY_NONE = 0,
	JUSTIFY_LEFT,
	JUSTIFY_CENTRE,
	JUSTIFY_RIGHT
};


struct bytes {
	const uint8_t *ptr;
	size_t size;
};


enum text_type { TEXT_NONE = 0, TEXT_BYTES, TEXT_UTF8 };

struct text {
	union {
		struct utf8lite_text utf8;
		struct bytes bytes;
	} value;
	enum text_type type;
};

static void bytes_init(struct bytes *bytes, SEXP charsxp)
{
	assert(charsxp != NA_STRING);

	bytes->ptr = (const uint8_t *)CHAR(charsxp);
	bytes->size = (size_t)XLENGTH(charsxp);
}


static int utf8_init(struct utf8lite_text *text, SEXP charsxp)
{
	const uint8_t *ptr;
	size_t size;
	cetype_t ce;
	int err = 0;

	assert(charsxp != NA_STRING);

	ce = getCharCE(charsxp);
	if (encodes_utf8(ce)) {
		ptr = (const uint8_t *)CHAR(charsxp);
		size = (size_t)XLENGTH(charsxp);
	} else if (ce == CE_LATIN1 || ce == CE_NATIVE) {
		ptr = (const uint8_t *)translate_utf8(charsxp);
		size = strlen((const char *)ptr);
	} else {
		err = UTF8LITE_ERROR_INVAL; // bytes or other encoding
		goto exit;
	}

	TRY(utf8lite_text_assign(text, ptr, size, 0, NULL));
exit:
	return err;
}


static void text_init(struct text *text, SEXP charsxp)
{
	if (charsxp == NA_STRING) {
		text->type = TEXT_NONE;
	} else if (!utf8_init(&text->value.utf8, charsxp)) {
		text->type = TEXT_UTF8;
	} else {
		bytes_init(&text->value.bytes, charsxp);
		text->type = TEXT_BYTES;
	}
}


static int byte_width(uint8_t byte, int flags)
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


static int utf8_width(const struct utf8lite_text *text, int limit,
		      int ellipsis, int flags)
{
	struct utf8lite_graphscan scan;
	int err = 0, width, w;

	utf8lite_graphscan_make(&scan, text);
	width = 0;
	while (utf8lite_graphscan_advance(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, flags, &w));
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}

exit:
	CHECK_ERROR(err);
	return width;
}


static int bytes_width(const struct bytes *bytes, int limit, int ellipsis,
		       int flags)
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


static int utf8_rwidth(const struct utf8lite_text *text, int limit,
		       int ellipsis, int flags)
{
	struct utf8lite_graphscan scan;
	int err = 0, width, w;

	utf8lite_graphscan_make(&scan, text);
	utf8lite_graphscan_skip(&scan);
	width = 0;
	while (utf8lite_graphscan_retreat(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, flags, &w));
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}

exit:
	return width;
}


static int bytes_rwidth(const struct bytes *bytes, int limit, int ellipsis,
			int flags)
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


static int text_width(const struct text *text, int limit, int ellipsis,
		      int flags)
{
	switch (text->type) {
	case TEXT_UTF8:
		return utf8_width(&text->value.utf8, limit, ellipsis, flags);
	case TEXT_BYTES:
		return bytes_width(&text->value.bytes, limit, ellipsis, flags);
	default:
		return -1;
	}
}


static int text_rwidth(const struct text *text, int limit, int ellipsis,
		       int flags)
{
	switch (text->type) {
	case TEXT_UTF8:
		return utf8_rwidth(&text->value.utf8, limit, ellipsis, flags);
	case TEXT_BYTES:
		return bytes_rwidth(&text->value.bytes, limit, ellipsis, flags);
	default:
		return -1;
	}
}


static int centre_pad_begin(struct utf8lite_render *r, int width_max,
			    int fullwidth)
{
	int i, fill, bfill = 0;
	int err = 0;

	if ((fill = width_max - fullwidth) > 0) {
		bfill = fill / 2;

		for (i = 0; i < bfill; i++) {
			TRY(utf8lite_render_string(r, " "));
		}
	}
exit:
	CHECK_ERROR(err);
	return bfill;
}


static void pad_spaces(struct utf8lite_render *r, int nspace)
{
	int err = 0;
	while (nspace-- > 0) {
		TRY(utf8lite_render_string(r, " "));
	}
exit:
	CHECK_ERROR(err);
}


static void render_byte(struct utf8lite_render *r, uint8_t byte)
{
	int err = 0;

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
		case '"':
			TRY(utf8lite_render_printf(r, "%c", (char)byte));
			goto exit;
		default:
			if (isprint((int)byte)) {
				TRY(utf8lite_render_printf(r, "%c",
							   (char)byte));
				goto exit;
			}
			break;
		}
	}

	TRY(utf8lite_render_printf(r, "\\x%02x", (unsigned)byte));
exit:
	CHECK_ERROR(err);
}


static SEXP bytes_format(struct utf8lite_render *r,
			 const struct bytes *bytes,
			 int trim, int chars, int width_max,
			 int quote, int utf8, int centre)
{
	SEXP ans = R_NilValue;
	const uint8_t *ptr, *end;
	const char *ellipsis_str;
	uint8_t byte;
	int err = 0, w, trunc, bfill, fullwidth, width, quotes, ellipsis;

	quotes = quote ? 2 : 0;
	ellipsis = utf8 ? 1 : 3;
	ellipsis_str = utf8 ? ELLIPSIS : "...";

	bfill = 0;
	if (centre && !trim) {
		fullwidth = (bytes_width(bytes, chars, ellipsis, r->flags)
			     + quotes);
		bfill = centre_pad_begin(r, width_max, fullwidth);
	}

	if (quote) {
		TRY(utf8lite_render_string(r, "\""));
	}

	width = 0;
	trunc = 0;
	ptr = bytes->ptr;
	end = bytes->ptr + bytes->size;

	while (!trunc && ptr < end) {
		byte = *ptr++;
		w = byte_width(byte, r->flags);

		if (width > chars - w) {
			w = ellipsis;
			TRY(utf8lite_render_string(r, ellipsis_str));
			trunc = 1;
		} else {
			render_byte(r, byte);
		}

		width += w;
	}

	if (quote) {
		TRY(utf8lite_render_string(r, "\""));
	}

	if (!trim) {
		pad_spaces(r, width_max - width - quotes - bfill);
	}

	ans = mkCharLenCE((char *)r->string, r->length,
			  utf8 ? CE_UTF8 : CE_ANY);
	utf8lite_render_clear(r);
exit:
	CHECK_ERROR(err);
	return ans;
}


static SEXP utf8_format(struct utf8lite_render *r,
			const struct utf8lite_text *text,
			int trim, int chars, int width_max,
			int quote, int utf8, int centre)
{
	SEXP ans = R_NilValue;
	struct utf8lite_graphscan scan;
	const char *ellipsis_str;
	int err = 0, w, trunc, bfill, fullwidth, width, quotes, ellipsis;

	quotes = quote ? 2 : 0;
	ellipsis = utf8 ? 1 : 3;
	ellipsis_str = utf8 ? ELLIPSIS : "...";

	bfill = 0;
	if (centre && !trim) {
		fullwidth = (utf8_width(text, chars, ellipsis, r->flags)
			     + quotes);
		bfill = centre_pad_begin(r, width_max, fullwidth);
	}

	if (quote) {
		TRY(utf8lite_render_string(r, "\""));
	}

	width = 0;
	trunc = 0;
	utf8lite_graphscan_make(&scan, text);

	while (!trunc && utf8lite_graphscan_advance(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, r->flags, &w));

		if (width > chars - w) {
			w = ellipsis;
			TRY(utf8lite_render_string(r, ellipsis_str));
			trunc = 1;
		} else {
			TRY(utf8lite_render_graph(r, &scan.current));
		}

		width += w;
	}

	if (quote) {
		TRY(utf8lite_render_string(r, "\""));
	}

	if (!trim) {
		pad_spaces(r, width_max - width - quotes - bfill);
	}

	ans = mkCharLenCE((char *)r->string, r->length,
			  utf8 ? CE_UTF8 : CE_ANY);
	utf8lite_render_clear(r);
exit:
	CHECK_ERROR(err);
	return ans;
}


static SEXP text_format(struct utf8lite_render *r,
			const struct text *text,
			int trim, int chars, int width_max,
			int quote, int utf8, int centre)
{
	switch (text->type) {
	case TEXT_UTF8:
		return utf8_format(r, &text->value.utf8, trim, chars,
				   width_max, quote, utf8, centre);
	case TEXT_BYTES:
		return bytes_format(r, &text->value.bytes, trim, chars,
				    width_max, quote, utf8, centre);
	default:
		return NA_STRING;
	}
}


static SEXP bytes_rformat(struct utf8lite_render *r,
			  const struct bytes *bytes, int trim, int chars,
			  int width_max, int quote, int utf8)
{
	SEXP ans = R_NilValue;
	const uint8_t *ptr, *end;
	const char *ellipsis_str;
	uint8_t byte;
	int err = 0, w, width, quotes, ellipsis, trunc;

	quotes = quote ? 2 : 0;
	ellipsis = utf8 ? 1 : 3;
	ellipsis_str = utf8 ? ELLIPSIS : "...";

	end = bytes->ptr + bytes->size;
	ptr = end;
	width = 0;
	trunc = 0;

	while (!trunc && ptr > bytes->ptr) {
		byte = *--ptr;
		w = byte_width(byte, r->flags);

		if (width > chars - w) {
			w = ellipsis;
			trunc = 1;
		}

		width += w;
	}

	if (!trim) {
		pad_spaces(r, width_max - width - quotes);
	}

	if (quote) {
		TRY(utf8lite_render_string(r, "\""));
	}

	if (trunc) {
		TRY(utf8lite_render_string(r, ellipsis_str));
	}

	while (ptr < end) {
		byte = *ptr++;
		render_byte(r, byte);
	}

	if (quote) {
		TRY(utf8lite_render_string(r, "\""));
	}

	ans = mkCharLenCE((char *)r->string, r->length,
			  utf8 ? CE_UTF8 : CE_ANY);
	utf8lite_render_clear(r);
exit:
	CHECK_ERROR(err);
	return ans;
}


static SEXP utf8_rformat(struct utf8lite_render *r,
			 const struct utf8lite_text *text, int trim, int chars,
			 int width_max, int quote, int utf8)
{
	SEXP ans = R_NilValue;
	struct utf8lite_graphscan scan;
	const char *ellipsis_str;
	int err = 0, w, width, quotes, ellipsis, trunc;

	quotes = quote ? 2 : 0;
	ellipsis = utf8 ? 1 : 3;
	ellipsis_str = utf8 ? ELLIPSIS : "...";

	utf8lite_graphscan_make(&scan, text);
	utf8lite_graphscan_skip(&scan);
	width = 0;
	trunc = 0;

	while (!trunc && utf8lite_graphscan_retreat(&scan)) {
		TRY(utf8lite_graph_measure(&scan.current, r->flags, &w));

		if (width > chars - w) {
			w = ellipsis;
			trunc = 1;
		}

		width += w;
	}

	if (!trim) {
		pad_spaces(r, width_max - width - quotes);
	}

	if (quote) {
		TRY(utf8lite_render_string(r, "\""));
	}

	if (trunc) {
		TRY(utf8lite_render_string(r, ellipsis_str));
	}

	while (utf8lite_graphscan_advance(&scan)) {
		TRY(utf8lite_render_graph(r, &scan.current));
	}

	if (quote) {
		TRY(utf8lite_render_string(r, "\""));
	}

	ans = mkCharLenCE((char *)r->string, r->length,
			  utf8 ? CE_UTF8 : CE_ANY);
	utf8lite_render_clear(r);
exit:
	CHECK_ERROR(err);
	return ans;
}


static SEXP text_rformat(struct utf8lite_render *r,
			 const struct text *text,
			 int trim, int chars, int width_max,
			 int quote, int utf8)
{
	switch (text->type) {
	case TEXT_UTF8:
		return utf8_rformat(r, &text->value.utf8, trim, chars,
				    width_max, quote, utf8);
	case TEXT_BYTES:
		return bytes_rformat(r, &text->value.bytes, trim, chars,
				     width_max, quote, utf8);
	default:
		return NA_STRING;
	}
}


struct context {
	struct utf8lite_render render;
	int has_render;
};

static void context_init(struct context *ctx)
{
	int err = 0;
	TRY(utf8lite_render_init(&ctx->render, 0));
	ctx->has_render = 1;
exit:
	CHECK_ERROR(err);
}

static void context_destroy(void *obj)
{
	struct context *ctx = obj;
	if (ctx->has_render) {
		utf8lite_render_destroy(&ctx->render);
	}
}

SEXP rutf8_utf8_format(SEXP sx, SEXP strim, SEXP schars, SEXP sjustify,
		       SEXP swidth, SEXP sna_encode, SEXP squote,
		       SEXP sna_print, SEXP sutf8)
{
	SEXP ans, sctx, na_print, ans_i;
	struct context *ctx;
	enum justify_type justify;
	const char *justify_str;
	uint8_t *buf;
	struct utf8lite_text *text;
	struct text elt, na;
	R_xlen_t i, n;
	int chars, chars_i, ellipsis, width, width_max, nbuf, trim, na_encode,
	    quote, quote_i, quotes, na_width, utf8, nprot, flags;

	nprot = 0;

	if (sx == R_NilValue) {
		return R_NilValue;
	}

	if (!isString(sx)) {
		error("argument is not a character vector");
	}
	text = NULL;
	PROTECT(ans = duplicate(sx)); nprot++;
	n = XLENGTH(ans);

	PROTECT(strim = coerceVector(strim, LGLSXP)); nprot++;
	trim = (LOGICAL(strim)[0] == TRUE);

	PROTECT(squote = coerceVector(squote, LGLSXP)); nprot++;
	quote = (LOGICAL(squote)[0] == TRUE);
	quotes = quote ? 2 : 0;

	PROTECT(strim = coerceVector(sutf8, LGLSXP)); nprot++;
	utf8 = (LOGICAL(sutf8)[0] == TRUE);
	ellipsis = utf8 ? 1 : 3;

	if (schars == R_NilValue) {
		chars = NA_INTEGER;
	} else {
		PROTECT(schars = coerceVector(schars, INTSXP)); nprot++;
		chars = INTEGER(schars)[0];
	}

	if (chars == NA_INTEGER || chars > INT_MAX - ellipsis - quotes) {
		chars = INT_MAX - ellipsis - quotes;
	} else if (chars < 0) {
		chars = 0;
	}

	justify_str = CHAR(STRING_ELT(sjustify, 0));
	if (strcmp(justify_str, "left") == 0) {
		justify = JUSTIFY_LEFT;
	} else if (strcmp(justify_str, "right") == 0) {
		justify = JUSTIFY_RIGHT;
	} else if (strcmp(justify_str, "centre") == 0) {
		justify = JUSTIFY_CENTRE;
	} else {
		justify = JUSTIFY_NONE;
		trim = 1;
	}

	if (swidth == R_NilValue) {
		width_max = 0;
	} else {
		PROTECT(swidth = coerceVector(swidth, INTSXP)); nprot++;
		width_max = INTEGER(swidth)[0];
		if (width_max == NA_INTEGER || width_max < 0) {
			width_max = 0;
		}
	}

	PROTECT(sna_encode = coerceVector(sna_encode, LGLSXP)); nprot++;
	na_encode = (LOGICAL(sna_encode)[0] == TRUE);

	if (sna_print == R_NilValue) {
		PROTECT(na_print = mkChar(quote ? "NA" : "<NA>")); nprot++;
	} else {
		na_print = STRING_ELT(sna_print, 0);
	}
	text_init(&na, na_print);
	na_width = text_width(&na, INT_MAX, 0, utf8);

	flags = (UTF8LITE_ESCAPE_CONTROL | UTF8LITE_ENCODE_C);
	if (quote) {
		flags |= UTF8LITE_ESCAPE_DQUOTE;
	}
	if (!utf8) {
		flags |= UTF8LITE_ESCAPE_UTF8;
	}
#if defined(_WIN32) || defined(_WIN64)
	flags |= UTF8LITE_ESCAPE_EXTENDED;
#endif

        PROTECT(sctx = alloc_context(sizeof(*ctx), context_destroy)); nprot++;
	ctx = as_context(sctx);
	context_init(ctx);

	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		text_init(&elt, STRING_ELT(sx, i));

		if (elt.type == TEXT_NONE) {
			width = na_encode ? na_width : 0;
		} else if (justify == JUSTIFY_RIGHT) {
			width = (text_rwidth(&elt, chars, ellipsis, flags)
					+ quotes);
		} else {
			width = (text_width(&elt, chars, ellipsis, flags)
					+ quotes);
		}

		if (width > width_max) {
			width_max = width;
		}

		if (width_max >= chars + ellipsis + quotes) {
			width_max = chars + ellipsis + quotes;
			break;
		}
	}

	nbuf = width_max;
	buf = (void *)R_alloc(nbuf, sizeof(uint8_t));

	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		text_init(&elt, STRING_ELT(sx, i));

		if (elt.type == TEXT_NONE) {
			if (na_encode) {
				elt = na;
				chars_i = na_width;
				quote_i = 0;
			} else {
				SET_STRING_ELT(ans, i, NA_STRING);
				continue;
			}
		} else {
			chars_i = chars;
			quote_i = quote;
		}

		switch (justify) {
		case JUSTIFY_LEFT:
		case JUSTIFY_NONE:
			ans_i = text_format(&ctx->render, &elt, trim,
					    chars_i, width_max, quote_i,
					    utf8, 0);
			break;

		case JUSTIFY_CENTRE:
			ans_i = text_format(&ctx->render, &elt, trim,
					    chars_i, width_max, quote_i,
					    utf8, 1);
			break;

		case JUSTIFY_RIGHT:
			ans_i = text_rformat(&ctx->render, &elt, trim,
					     chars_i, width_max, quote_i,
					     utf8);
			break;
		}

		SET_STRING_ELT(ans, i, ans_i);
	}

	UNPROTECT(nprot);
	return ans;
}
