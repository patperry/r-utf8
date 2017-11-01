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
	enum rutf8_justify_type justify;
	struct utf8lite_text *text;
	struct rutf8_string elt, na;
	R_xlen_t i, n;
	int chars, chars_i, ellipsis, width, width_max, trim, na_encode,
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

	PROTECT(sutf8 = coerceVector(sutf8, LGLSXP)); nprot++;
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

	justify = rutf8_as_justify(sjustify);
	if (justify == RUTF8_JUSTIFY_NONE) {
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

	rutf8_string_init(&na, na_print);
	na_width = rutf8_string_width(&na, flags);

        PROTECT(sctx = rutf8_alloc_context(sizeof(*ctx), context_destroy));
	nprot++;
	ctx = rutf8_as_context(sctx);
	context_init(ctx);

	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		rutf8_string_init(&elt, STRING_ELT(sx, i));

		if (elt.type == RUTF8_STRING_NONE) {
			width = na_encode ? na_width : 0;
		} else if (justify == RUTF8_JUSTIFY_RIGHT) {
			width = (rutf8_string_rwidth(&elt, flags, chars,
						     ellipsis)
				 + quotes);
		} else {
			width = (rutf8_string_lwidth(&elt, flags, chars,
						     ellipsis)
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

	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		rutf8_string_init(&elt, STRING_ELT(sx, i));

		if (elt.type == RUTF8_STRING_NONE) {
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
		case RUTF8_JUSTIFY_LEFT:
		case RUTF8_JUSTIFY_NONE:
			ans_i = rutf8_string_lformat(&ctx->render, &elt,
						     trim, chars_i, width_max,
						     quote_i, utf8, flags, 0);
			break;

		case RUTF8_JUSTIFY_CENTRE:
			ans_i = rutf8_string_lformat(&ctx->render, &elt, trim,
						     chars_i, width_max,
						     quote_i, utf8, flags, 1);
			break;

		case RUTF8_JUSTIFY_RIGHT:
			ans_i = rutf8_string_rformat(&ctx->render, &elt, trim,
						     chars_i, width_max,
						     quote_i, utf8, flags);
			break;
		}

		SET_STRING_ELT(ans, i, ans_i);
	}

	rutf8_free_context(sctx);
	UNPROTECT(nprot);
	return ans;
}