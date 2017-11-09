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

#include <limits.h>
#include "rutf8.h"


SEXP rutf8_utf8_encode(SEXP sx, SEXP swidth, SEXP squote, SEXP sjustify,
		       SEXP sescapes, SEXP sdisplay, SEXP sutf8)
{
	SEXP ans, selt, ans_i = NA_STRING, srender;
	struct rutf8_string elt;
	struct utf8lite_render *render;
	enum rutf8_justify_type justify;
	const char *escapes;
	R_xlen_t i, n;
	int width, quote, display, utf8;
	int err = 0, nprot = 0, w, quotes, flags;

	if (sx == R_NilValue) {
		return R_NilValue;
	}

	if (!isString(sx)) {
		Rf_error("argument is not a character object");
	}
	n = XLENGTH(sx);

	if (swidth == R_NilValue || INTEGER(swidth)[0] == NA_INTEGER) {
		width = -1;
	} else {
		width = INTEGER(swidth)[0];
	}

	quote = LOGICAL(squote)[0] == TRUE;
	justify = rutf8_as_justify(sjustify);
	escapes = rutf8_as_style(sescapes);
	display = LOGICAL(sdisplay)[0] == TRUE;
	utf8 = LOGICAL(sutf8)[0] == TRUE;

	flags = (UTF8LITE_ESCAPE_CONTROL | UTF8LITE_ENCODE_C);
	if (quote) {
		flags |= UTF8LITE_ESCAPE_DQUOTE;
	}
	if (display) {
		flags |= UTF8LITE_ENCODE_RMDI;
		flags |= UTF8LITE_ENCODE_EMOJIZWSP;
	}
	if (!utf8) {
		flags |= UTF8LITE_ESCAPE_UTF8;
	}
#if defined(_WIN32) || defined(_WIN64)
	flags |= UTF8LITE_ESCAPE_EXTENDED;
#endif
	quotes = quote ? 2 : 0;

	if (justify == RUTF8_JUSTIFY_NONE) {
		width = 0;
	}

	if (width < 0) {
		width = 0;
		for (i = 0; i < n; i++) {
			CHECK_INTERRUPT(i);

			PROTECT(selt = STRING_ELT(sx, i)); nprot++;
			rutf8_string_init(&elt, selt);

			if (elt.type == RUTF8_STRING_NONE) {
				UNPROTECT(1); nprot--;
				continue;
			}

			w = rutf8_string_width(&elt, flags);
			if (w > INT_MAX - quotes) {
				Rf_error("width exceeds maximum (%d)",
					 INT_MAX);
			}
			w += quotes;

			if (w > width) {
				width = w;
			}
			UNPROTECT(1); nprot--;
		}
	}

        PROTECT(srender = rutf8_alloc_render(flags)); nprot++;
	render = rutf8_as_render(srender);
	if (escapes) {
		TRY(utf8lite_render_set_style(render, escapes,
					      RUTF8_STYLE_CLOSE));
	}

	PROTECT(ans = duplicate(sx)); nprot++;

	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		PROTECT(selt = STRING_ELT(sx, i)); nprot++;
		rutf8_string_init(&elt, selt);
		if (elt.type == RUTF8_STRING_NONE) {
			ans_i = NA_STRING;
		} else {
			rutf8_string_render(render, &elt, width, quote,
					    justify);
			ans_i = mkCharLenCE(render->string, render->length,
					    CE_UTF8);
			utf8lite_render_clear(render);
		}

		UNPROTECT(1); nprot--;
		SET_STRING_ELT(ans, i, ans_i);
	}

exit:
	UNPROTECT(nprot);
	CHECK_ERROR(err);
	return ans;
}
