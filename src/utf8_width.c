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


int charsxp_width(SEXP charsxp, int quote, int utf8)
{
	const uint8_t *ptr = (const uint8_t *)CHAR(charsxp);
	const uint8_t *start;
	const uint8_t *end = ptr + XLENGTH(charsxp);
	int32_t code;
	int width, cw, err;

	width = quote ? 2 : 0;

	while (ptr != end) {
		start = ptr;
		if ((err = utf8lite_scan_utf8(&start, end, NULL))) {
			return NA_INTEGER;
		}

		utf8lite_decode_utf8(&ptr, &code);
		if (code < 0x80) {
			switch (code) {
			case '\a':
			case '\b':
			case '\f':
			case '\n':
			case '\r':
			case '\t':
			case '\v':
				return NA_INTEGER;
			case '"':
				width += (quote ? 2 : 1);
				continue;
			default:
				if (!isprint((int)code)) {
					return NA_INTEGER;
				}
				width += 1;
				continue;
			}
		} else if (!utf8) {
			return NA_INTEGER;
		}

		cw = charwidth(code);

		switch (cw) {
		case UTF8LITE_CHARWIDTH_NONE:
		case UTF8LITE_CHARWIDTH_IGNORABLE:
			break;

		case UTF8LITE_CHARWIDTH_NARROW:
		case UTF8LITE_CHARWIDTH_AMBIGUOUS:
			width += 1;
			break;

		case UTF8LITE_CHARWIDTH_WIDE:
		case UTF8LITE_CHARWIDTH_EMOJI:
			width += 2;
			break;

		default: // UTF8LITE_CHARWIDTH_OTHER
			return NA_INTEGER;
		}
	}

	return width;
}


SEXP rutf8_utf8_width(SEXP sx, SEXP squote, SEXP sutf8)
{
	SEXP ans, elt;
	R_xlen_t i, n;
	int quote, utf8, w;

	if (sx == R_NilValue) {
		return R_NilValue;
	}
	if (!isString(sx)) {
		error("argument is not a character object");
	}
	n = XLENGTH(sx);
	quote = LOGICAL(squote)[0] == TRUE;
	utf8 = LOGICAL(sutf8)[0] == TRUE;

	PROTECT(ans = allocVector(INTSXP, n));
	setAttrib(ans, R_NamesSymbol, getAttrib(sx, R_NamesSymbol));
	setAttrib(ans, R_DimSymbol, getAttrib(sx, R_DimSymbol));
	setAttrib(ans, R_DimNamesSymbol, getAttrib(sx, R_DimNamesSymbol));

	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		elt = STRING_ELT(sx, i);
		if (elt == NA_STRING) {
			w = NA_INTEGER;
		} else {
			w = charsxp_width(elt, quote, utf8);
		}
		INTEGER(ans)[i] = w;
	}

	UNPROTECT(1);
	return ans;
}
