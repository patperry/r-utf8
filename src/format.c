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


#define ELLIPSIS 0x2026
#define ELLIPSIS_NBYTE 3

enum justify_type {
	JUSTIFY_NONE = 0,
	JUSTIFY_LEFT,
	JUSTIFY_CENTRE,
	JUSTIFY_RIGHT
};

enum text_type { TEXT_CORPUS, TEXT_RAW };

struct text {
	enum text_type type;
	union {
		struct utf8lite_text corpus;
		struct {
			const uint8_t *ptr;
			size_t size;
			cetype_t ce;
		} raw;
	} data;
	cetype_t ce;
	int na;
};

struct text_iter {
	enum text_type type;
	union {
		struct utf8lite_text_iter corpus;
		struct {
			const uint8_t *begin;
			const uint8_t *end;
			const uint8_t *ptr;
			cetype_t ce;
		} raw;
	} data;
	int current;
};


static void text_init_corpus(struct text *text, struct utf8lite_text *corpus)
{
	text->type = TEXT_CORPUS;
	text->ce = CE_UTF8;

	if (corpus->ptr == NULL) {
		text->data.corpus.ptr = (uint8_t *)"NA";
		text->data.corpus.attr = 2;
		text->na = 1;
	} else {
		text->data.corpus = *corpus;
		text->na = 0;
	}
}


static void text_init_charsxp(struct text *text, SEXP charsxp)
{
	const uint8_t *ptr, *ptr2;
	size_t size;
	cetype_t ce;

	text->type = TEXT_RAW;
	if (charsxp == NA_STRING) {
		text->data.raw.ptr = (const uint8_t *)"NA";
		text->data.raw.size = 2;
		text->ce = CE_UTF8;
		text->na = 1;
	} else {
		ce = getCharCE(charsxp);
		ptr = (const uint8_t *)CHAR(charsxp);
		size = (size_t)XLENGTH(charsxp);

		if (!encodes_utf8(ce)) {
			ptr2 = (const uint8_t *)translate_utf8(charsxp);
			if (ptr2 != ptr) {
				ptr = ptr2;
				size = strlen((const char *)ptr);
			}
			ce = CE_UTF8;
		}
		text->data.raw.ptr = ptr;
		text->data.raw.size = size;
		text->ce = (ce == CE_BYTES) ? CE_BYTES : CE_UTF8;
		text->na = 0;
	}
}


static void text_iter_make(struct text_iter *iter, const struct text *text)
{
	const uint8_t *ptr;
	size_t size;

	iter->type = text->type;
	iter->current = -1;

	if (text->type == TEXT_CORPUS) {
		utf8lite_text_iter_make(&iter->data.corpus, &text->data.corpus);
		return;
	}

	ptr = text->data.raw.ptr;
	size = text->data.raw.size;
	iter->data.raw.begin = ptr;
	iter->data.raw.ptr = ptr;
	iter->data.raw.end = ptr + size;
	iter->data.raw.ce = text->ce;
}


static void text_iter_skip(struct text_iter *iter)
{
	const uint8_t *end;

	if (iter->type == TEXT_CORPUS) {
		utf8lite_text_iter_skip(&iter->data.corpus);
	} else {
		end = iter->data.raw.end;
		iter->data.raw.ptr = end;
		iter->current = -1;
	}
}


static int text_iter_advance(struct text_iter *iter)
{
	const uint8_t *start, *ptr, *end;
	int32_t code;
	cetype_t ce;
	int err, ret;

	if (iter->type == TEXT_CORPUS) {
		ret = utf8lite_text_iter_advance(&iter->data.corpus);
		iter->current = iter->data.corpus.current;
		return ret;
	}

	ptr = iter->data.raw.ptr;
	end = iter->data.raw.end;
	ce = iter->data.raw.ce;

	if (ptr == end) {
		ret = 0;
		iter->current = -1;
	} else {
		ret = 1;
		start = ptr;
		if (ce == CE_BYTES) {
			code = *ptr++;
			iter->current = (code < 0x80) ? (int)code : -(int)code;
		} else if ((err = utf8lite_scan_utf8(&start, end, NULL))) {
			iter->current = -(int)*ptr++;
		} else {
			utf8lite_decode_utf8(&ptr, &code);
			iter->current = (int)code;
		}
		iter->data.raw.ptr = ptr;
	}

	return ret;
}


static int text_iter_retreat(struct text_iter *iter)
{
	const uint8_t *start, *begin, *ptr;
	int32_t code;
	int nbyte, err, ret;

	if (iter->type == TEXT_CORPUS) {
		ret = utf8lite_text_iter_retreat(&iter->data.corpus);
		iter->current = iter->data.corpus.current;
		return ret;
	}

	begin = iter->data.raw.begin;
	ptr = iter->data.raw.ptr;

	// at SOT
	if (ptr == begin) {
		iter->current = -1;
		return 0;
	}

	// if not at EOT, move the pointer to the end of
	// the current character
	if (iter->current != -1) {
		if (iter->current < 0) {
			ptr--; // invalid byte
		} else {
			code = (uint32_t)iter->current;
			ptr -= UTF8LITE_UTF8_ENCODE_LEN(code);
		}
	}

	// at this point, ptr is at the end of the code
	iter->data.raw.ptr = ptr;

	// if at SOT, we can't retreat
	if (ptr == begin) {
		iter->current = -1;
		return 0;
	}

	// handle bytes encoding
	if (iter->data.raw.ce == CE_BYTES) {
		code = ptr[-1];
		iter->current = (code < 0x80) ? (int)code : -(int)code;
		return 1;
	}

	// search backwards for the first valid sequence
	for (nbyte = 1; nbyte <= 4; nbyte++) {
		start = ptr - nbyte;
		if (start < begin) {
			break;
		}
		if (!(err = utf8lite_scan_utf8(&start, ptr, NULL))) {
			break;
		}
	}

	// no valid sequence
	if (err) {
		iter->current = -(int)ptr[-1];
	} else {
		start = ptr - nbyte;
		utf8lite_decode_utf8(&start, &code);
		iter->current = (int)code;
	}

	return 1;
}


static int char_width(int code, int type, int quote, int utf8)
{
	if (code < 0) {
		return 4; // \xXX invalid byte
	}

	if (code < 0x80) {
		switch (code) {
		case '\a':
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
		case '\v':
			return 2;
		case '"':
			return quote ? 2 : 1;
		default:
			return isprint(code) ? 1 : 6; // \uXXXX
		}
	}

	if (utf8) {
		switch (type) {
		case UTF8LITE_CHARWIDTH_NONE:
		case UTF8LITE_CHARWIDTH_IGNORABLE:
			return 0;

		case UTF8LITE_CHARWIDTH_NARROW:
		case UTF8LITE_CHARWIDTH_AMBIGUOUS:
			return 1;

		case UTF8LITE_CHARWIDTH_WIDE:
		case UTF8LITE_CHARWIDTH_EMOJI:
			return 2;

		default:
			break;
		}
	}

	return (code <= 0xFFFF) ? 6 : 10; // \uXXXX or \UXXXXYYYY
}


static int text_width(const struct text *text, int limit, int quote, int utf8)
{
	struct text_iter it;
	int32_t code;
	int type, width, w;
	int ellipsis = utf8 ? 1 : 3;

	text_iter_make(&it, text);
	width = quote ? 2 : 0;
	while (text_iter_advance(&it)) {
		code = it.current;
		type = code < 0 ? 0 : charwidth(code);
		w = char_width(code, type, quote, utf8);
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}

	return width;
}


static int text_rwidth(const struct text *text, int limit, int quote, int utf8)
{
	struct text_iter it;
	int code, type, width, w;
	int ellipsis = utf8 ? 1 : 3;

	text_iter_make(&it, text);
	text_iter_skip(&it);
	width = quote ? 2 : 0;
	while (text_iter_retreat(&it)) {
		code = it.current;
		type = code < 0 ? 0 : charwidth(code);
		w = char_width(code, type, quote, utf8);
		if (width > limit - w) {
			return width + ellipsis;
		}
		width += w;
	}

	return width;
}


static void grow_buffer(uint8_t **bufptr, int *nbufptr, int nadd)
{
	uint8_t *buf = *bufptr;
	int nbuf0 = *nbufptr;
	int nbuf = nbuf0;
	int err;

	if ((err = array_size_add(&nbuf, 1, nbuf0, nadd))) {
		error("buffer size (%d + %d bytes)"
		      " exceeds maximum (%d bytes)", nbuf0, nadd,
		      INT_MAX);
	}

	if (nbuf0 > 0) {
		buf = (void *)S_realloc((char *)buf, nbuf, nbuf0, sizeof(uint8_t));
	} else {
		buf = (void *)R_alloc(nbuf, sizeof(uint8_t));
	}

	*bufptr = buf;
	*nbufptr = nbuf;
}

#define ENSURE(nbyte) \
	do { \
		if (dst + (nbyte) > end) { \
			off = (int)(dst - buf); \
			grow_buffer(&buf, &nbuf, nbyte); \
			dst = buf + off; \
			end = buf + nbuf; \
		} \
	} while (0)

static SEXP format_left(const struct text *text, int trim, int chars,
			int width_max, int quote, int utf8, int centre,
			uint8_t **bufptr, int *nbufptr)
{
	uint8_t *buf = *bufptr;
	int nbuf = *nbufptr;
	uint8_t *end = buf + nbuf;
	struct text_iter it;
	uint8_t *dst;
	int i, w, code, trunc, type, nbyte, bfill, fill, len, off,
	    fullwidth, width, quotes;

	dst = buf;

	quotes = quote ? 2 : 0;
	bfill = 0;
	if (centre && !trim) {
		fullwidth = text_width(text, chars, quote, utf8);
		if ((fill = width_max - fullwidth) > 0) {
			bfill = fill / 2;

			ENSURE(bfill);

			for (i = 0; i < bfill; i++) {
				*dst++ = ' ';
			}
		}
	}


	if (quote) {
		ENSURE(1);
		*dst++ = '"';
	}

	width = 0;
	trunc = 0;
	text_iter_make(&it, text);

	while (!trunc && text_iter_advance(&it)) {
		code = it.current;
		type = code < 0 ? 0 : charwidth(code);
		w = char_width(code, type, quote, utf8);

		if (width > chars - w) {
			code = ELLIPSIS;
			w = utf8 ? 1 : 3;
			trunc = 1;
		}

		nbyte = code < 0 ? 1 : UTF8LITE_UTF8_ENCODE_LEN(code);
		ENSURE(nbyte);

		if (trunc) {
			if (utf8) {
				utf8lite_encode_utf8(ELLIPSIS, &dst);
			} else {
				*dst++ = '.';
				*dst++ = '.';
				*dst++ = '.';
			}
		} else if (code < 0) {
			*dst++ = (uint8_t)(-code);
		} else if (code == '"' && quote) {
			ENSURE(2);
			*dst++ = '\\';
			*dst++ = '"';
		} else {
			utf8lite_encode_utf8(code, &dst);
		}
		width += w;
	}

	if (quote) {
		ENSURE(1);
		*dst++ = '"';
	}

	if (!trim && ((fill = width_max - width - quotes - bfill)) > 0) {
		ENSURE(fill);

		while (fill-- > 0) {
			*dst++ = ' ';
		}
	}


	*bufptr = buf;
	*nbufptr = nbuf;

	len = (int)(dst - buf);
	return mkCharLenCE((char *)buf, len, text->ce);
}

#undef ENSURE
#define ENSURE(nbyte) \
	do { \
		if (dst < buf + nbyte) { \
			off = dst - buf; \
			len = nbuf - off; \
			grow_buffer(&buf, &nbuf, nbyte); \
			dst = buf + nbuf - len; \
			memmove(dst, buf + off, len); \
		} \
	} while (0)

static SEXP format_right(const struct text *text, int trim, int chars,
			 int width_max, int quote, int utf8,
			 uint8_t **bufptr, int *nbufptr)
{
	uint8_t *buf = *bufptr;
	int nbuf = *nbufptr;
	struct text_iter it;
	uint8_t *dst;
	int w, code, fill, trunc, type, off, len, nbyte, width, quotes;

	quotes = quote ? 2 : 0;
	dst = buf + nbuf;

	if (quote) {
		ENSURE(1);
		*--dst = '"';
	}

	width = 0;
	trunc = 0;
	text_iter_make(&it, text);
	text_iter_skip(&it);

	while (!trunc && text_iter_retreat(&it)) {
		code = it.current;
		type = code < 0 ? 0 : charwidth(code);
		w = char_width(code, type, quote, utf8);

		if (width > chars - w) {
			code = ELLIPSIS;
			w = utf8 ? 1 : 3;
			trunc = 1;
		}

		nbyte = code < 0 ? 1 : UTF8LITE_UTF8_ENCODE_LEN(code);
		ENSURE(nbyte);

		if (trunc) {
			if (utf8) {
				utf8lite_rencode_utf8(ELLIPSIS, &dst);
			} else {
				// nbyte(ELLIPSIS) == 3 so no need to reserve
				*--dst = '.';
				*--dst = '.';
				*--dst = '.';
			}
		} else if (code < 0) {
			*--dst = (uint8_t)(-code);
		} else if (code == '"' && quote) {
			ENSURE(2);
			*--dst = '"';
			*--dst = '\\';
		} else {
			utf8lite_rencode_utf8(code, &dst);
		}
		width += w;
	}

	if (quote) {
		ENSURE(1);
		*--dst = '"';
	}

	if (!trim && ((fill = width_max - width - quotes)) > 0) {
		ENSURE(fill);

		while (fill-- > 0) {
			*--dst = ' ';
		}
	}

	*bufptr = buf;
	*nbufptr = nbuf;

	off = (int)(dst - buf);
	len = nbuf - off;
	return mkCharLenCE((char *)dst, len, text->ce);
}


SEXP format_text(SEXP sx, SEXP strim, SEXP schars, SEXP sjustify, SEXP swidth,
		 SEXP sna_encode, SEXP squote, SEXP sna_print, SEXP sutf8)
{
	return utf8_format(sx, strim, schars, sjustify, swidth, sna_encode,
			   squote, sna_print, sutf8);
}


SEXP utf8_format(SEXP sx, SEXP strim, SEXP schars, SEXP sjustify, SEXP swidth,
		 SEXP sna_encode, SEXP squote, SEXP sna_print, SEXP sutf8)
{
	SEXP ans, na_print, ans_i;
	enum text_type type;
	enum justify_type justify;
	const char *justify_str;
	uint8_t *buf;
	struct utf8lite_text *text;
	struct text elt, na;
	R_xlen_t i, n;
	int chars, chars_i, ellipsis, width, width_max, nbuf, trim, na_encode,
	    quote, quote_i, quotes, na_width, utf8, nprot;

	nprot = 0;

	if (sx == R_NilValue) {
		return R_NilValue;
	}

	if (!isString(sx)) {
		error("argument is not a character vector");
	}
	type = TEXT_RAW;
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
	text_init_charsxp(&na, na_print);
	na_width = text_width(&na, INT_MAX, 0, utf8);

	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		if (type == TEXT_CORPUS) {
			text_init_corpus(&elt, &text[i]);
		} else {
			text_init_charsxp(&elt, STRING_ELT(sx, i));
		}

		if (elt.na) {
			width = na_encode ? na_width : 0;
		} else if (justify == JUSTIFY_RIGHT) {
			width = text_rwidth(&elt, chars, quote, utf8);
		} else {
			width = text_width(&elt, chars, quote, utf8);
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

		if (type == TEXT_CORPUS) {
			text_init_corpus(&elt, &text[i]);
		} else {
			text_init_charsxp(&elt, STRING_ELT(sx, i));
		}

		if (elt.na) {
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
			ans_i = format_left(&elt, trim, chars_i, width_max,
					    quote_i, utf8, 0, &buf, &nbuf);
			break;

		case JUSTIFY_CENTRE:
			ans_i = format_left(&elt, trim, chars_i, width_max,
					    quote_i, utf8, 1, &buf, &nbuf);
			break;

		case JUSTIFY_RIGHT:
			ans_i = format_right(&elt, trim, chars_i, width_max,
					     quote_i, utf8, &buf, &nbuf);
			break;
		}

		SET_STRING_ELT(ans, i, ans_i);
	}

	UNPROTECT(nprot);
	return ans;
}
