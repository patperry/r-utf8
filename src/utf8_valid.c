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

#include <stddef.h>
#include <stdint.h>
#include "rutf8.h"


SEXP rutf8_utf8_valid(SEXP sx)
{
	SEXP ans, sstr;
	const uint8_t *str;
	struct utf8lite_text text;
	cetype_t ce;
	size_t size;
	R_xlen_t i, n;
	int raw, val;

	if (sx == R_NilValue) {
		return R_NilValue;
	}
	if (!isString(sx)) {
		error("argument is not a character object");
	}

	n = XLENGTH(sx);
	PROTECT(ans = allocVector(LGLSXP, n));
	setAttrib(ans, R_NamesSymbol, getAttrib(sx, R_NamesSymbol));
	setAttrib(ans, R_DimSymbol, getAttrib(sx, R_DimSymbol));
	setAttrib(ans, R_DimNamesSymbol, getAttrib(sx, R_DimNamesSymbol));

	n = XLENGTH(sx);
	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		PROTECT(sstr = STRING_ELT(sx, i));
		if (sstr == NA_STRING) {
			LOGICAL(ans)[i] = NA_LOGICAL;
			UNPROTECT(1);
			continue;
		}

		ce = getCharCE(sstr);
		raw = rutf8_encodes_utf8(ce) || ce == CE_BYTES;

		if (raw) {
			str = (const uint8_t *)CHAR(sstr);
			size = (size_t)XLENGTH(sstr);
		} else {
			str = (const uint8_t *)rutf8_translate_utf8(sstr);
			size = strlen((const char *)str);
		}

		if (utf8lite_text_assign(&text, str, size, 0, NULL)) {
			val = FALSE;
		} else {
			val = TRUE;
		}
		LOGICAL(ans)[i] = val;
		UNPROTECT(1);
	}

	UNPROTECT(1);
	return ans;
}
