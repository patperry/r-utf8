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

#include "rutf8.h"


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
