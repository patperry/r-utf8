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


SEXP rutf8_utf8_width(SEXP sx, SEXP sencode, SEXP squote, SEXP sutf8)
{
	SEXP ans, selt;
	struct rutf8_string elt;
	R_xlen_t i, n;
	int flags, encode, quote, quotes, utf8, w;

	if (sx == R_NilValue) {
		return R_NilValue;
	}
	if (!isString(sx)) {
		error("argument is not a character object");
	}
	n = XLENGTH(sx);
	encode = LOGICAL(sencode)[0] == TRUE;
	quote = LOGICAL(squote)[0] == TRUE;
	utf8 = LOGICAL(sutf8)[0] == TRUE;

	flags = UTF8LITE_ENCODE_C;
	if (encode) {
		flags |= UTF8LITE_ESCAPE_CONTROL;
		if (!utf8) {
			flags |= UTF8LITE_ESCAPE_UTF8;
		}
#if defined(_WIN32) || defined(_WIN64)
		flags |= UTF8LITE_ESCAPE_EXTENDED;
#endif
	}
	if (quote) {
		flags |= UTF8LITE_ESCAPE_DQUOTE;
	}
	quotes = quote ? 2 : 0;

	PROTECT(ans = allocVector(INTSXP, n));
	setAttrib(ans, R_NamesSymbol, getAttrib(sx, R_NamesSymbol));
	setAttrib(ans, R_DimSymbol, getAttrib(sx, R_DimSymbol));
	setAttrib(ans, R_DimNamesSymbol, getAttrib(sx, R_DimNamesSymbol));

	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		PROTECT(selt = STRING_ELT(sx, i));
		rutf8_string_init(&elt, selt);
		
		if (elt.type == RUTF8_STRING_NONE) {
			w = NA_INTEGER;
		} else if (elt.type == RUTF8_STRING_TEXT && !encode && !utf8
				&& !utf8lite_text_isascii(&elt.value.text)) {
			w = NA_INTEGER;
		} else {
			w = rutf8_string_width(&elt, flags);
			if (w < 0) {
				w = NA_INTEGER;
			} else if (w > INT_MAX - quotes) {
				Rf_error("width exceeds maximum (%d)",
					 INT_MAX);
			} else {
				w += quotes;
			}
		}
		INTEGER(ans)[i] = w;
		UNPROTECT(1);
	}

	UNPROTECT(1);
	return ans;
}
