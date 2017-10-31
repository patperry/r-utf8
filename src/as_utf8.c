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
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "rutf8.h"


static const char *encoding_name(cetype_t ce)
{
	switch (ce) {
	case CE_LATIN1:
		return "latin1";
	case CE_UTF8:
		return "UTF-8";
	case CE_SYMBOL:
		return "symbol";
	case CE_BYTES:
		return "bytes";
	case CE_ANY:
	case CE_NATIVE:
	default:
		return "unknown";
	}
}


static int is_valid(const uint8_t *str, size_t size, size_t *errptr)
{
	const uint8_t *end = str + size;
	const uint8_t *start;
	const uint8_t *ptr = str;
	size_t err = (size_t)-1;
	int valid;

	valid = 1;
	while (ptr != end) {
		start = ptr;
		if (utf8lite_scan_utf8(&ptr, end, NULL)) {
			err = (size_t)(start - str);
			valid = 0;
			goto out;
		}
	}

out:
	if (!valid && errptr) {
		*errptr = err;
	}

	return valid;
}


SEXP rutf8_utf8_coerce(SEXP sx)
{
	SEXP ans, sstr;
	const uint8_t *str;
	cetype_t ce;
	size_t size, err;
	R_xlen_t i, n;
	int raw, duped;;

	if (sx == R_NilValue) {
		return R_NilValue;
	}
	if (!isString(sx)) {
		error("argument is not a character object");
	}

	ans = sx;
	duped = 0;

	n = XLENGTH(sx);
	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		sstr = STRING_ELT(sx, i);
		if (sstr == NA_STRING) {
			continue;
		}

		ce = getCharCE(sstr);
		raw = encodes_utf8(ce) || ce == CE_BYTES;

		if (raw) {
			str = (const uint8_t *)CHAR(sstr);
			size = (size_t)XLENGTH(sstr);
		} else {
			str = (const uint8_t *)translate_utf8(sstr);
			size = strlen((const char *)str);
		}

		if (!is_valid(str, size, &err)) {
			if (ce == CE_BYTES) {
				error("argument entry %"PRIu64
				      " cannot be converted"
				      " from \"bytes\" to \"UTF-8\";"
				      " it contains an invalid byte"
				      " in position %"PRIu64
				      " (\\x%0x)",
				      (uint64_t)i + 1,
				      (uint64_t)err + 1,
				      (unsigned)str[err]);
			} else if (raw) {
				error("argument entry %"PRIu64
				      " is marked as \"UTF-8\""
				      " but contains an invalid byte"
				      " in position %"PRIu64
				      " (\\x%0x)",
				      (uint64_t)i + 1,
				      (uint64_t)err + 1,
				      (unsigned)str[err]);
			} else {
				error("argument entry %"PRIu64
				      " cannot be converted"
				      " from \"%s\" encoding to \"UTF-8\";"
				      " calling 'enc2utf8' on that entry"
				      " returns a string with an invalid"
				      " byte in position %"PRIu64,
				      " (\\x%0x)",
				      (uint64_t)i + 1,
				      encoding_name(ce),
				      (uint64_t)err + 1,
				      (unsigned)str[err]);
			}
		}

		if (!raw || ce == CE_BYTES || ce == CE_NATIVE) {
			if (!duped) {
				PROTECT(ans = duplicate(ans));
				duped = 1;
			}
			SET_STRING_ELT(ans, i,
				       mkCharLenCE((const char *)str,
					           size, CE_UTF8));
		}
	}

	if (duped) {
		UNPROTECT(1);
	}

	return ans;
}


SEXP rutf8_utf8_valid(SEXP sx)
{
	SEXP ans, sstr;
	const uint8_t *str;
	cetype_t ce;
	size_t size, err;
	R_xlen_t i, n;
	int raw, val;;

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

		sstr = STRING_ELT(sx, i);
		if (sstr == NA_STRING) {
			LOGICAL(ans)[i] = NA_LOGICAL;
			continue;
		}

		ce = getCharCE(sstr);
		raw = encodes_utf8(ce) || ce == CE_BYTES;

		if (raw) {
			str = (const uint8_t *)CHAR(sstr);
			size = (size_t)XLENGTH(sstr);
		} else {
			str = (const uint8_t *)translate_utf8(sstr);
			size = strlen((const char *)str);
		}

		val = is_valid(str, size, &err) ? TRUE : FALSE;
		LOGICAL(ans)[i] = val;
	}

	UNPROTECT(1);
	return ans;
}
