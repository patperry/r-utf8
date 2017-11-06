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
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "rutf8.h"

struct context {
	struct utf8lite_textmap map;
	int has_map;
};


static void context_init(struct context *ctx, SEXP map_case, SEXP map_compat,
			 SEXP map_quote, SEXP remove_ignorable)
{
	int err = 0, type;

	type = UTF8LITE_TEXTMAP_NORMAL;

	if (LOGICAL(map_case)[0] == TRUE) {
		type |= UTF8LITE_TEXTMAP_CASE;
	}

	if (LOGICAL(map_compat)[0] == TRUE) {
		type |= UTF8LITE_TEXTMAP_COMPAT;
	}

	if (LOGICAL(map_quote)[0] == TRUE) {
		type |= UTF8LITE_TEXTMAP_QUOTE;
	}

	if (LOGICAL(remove_ignorable)[0] == TRUE) {
		type |= UTF8LITE_TEXTMAP_RMDI;
	}

	TRY(utf8lite_textmap_init(&ctx->map, type));
	ctx->has_map = 1;
exit:
	CHECK_ERROR(err);
}


static void context_destroy(void *obj)
{
	struct context *ctx = obj;

	if (ctx->has_map) {
		utf8lite_textmap_destroy(&ctx->map);
	}
}


SEXP rutf8_utf8_normalize(SEXP x, SEXP map_case, SEXP map_compat,
			  SEXP map_quote, SEXP remove_ignorable)
{
	SEXP ans, sctx, elt;
	struct context *ctx;
	struct utf8lite_text text;
	const uint8_t *ptr;
	size_t size;
	R_xlen_t i, n;
	int err = 0, nprot = 0;

	if (x == R_NilValue) {
		return R_NilValue;
	}

	PROTECT(sctx = rutf8_alloc_context(sizeof(*ctx), context_destroy));
	nprot++;
        ctx = rutf8_as_context(sctx);
	context_init(ctx, map_case, map_compat, map_quote, remove_ignorable);

	PROTECT(ans = duplicate(x)); nprot++;
	n = XLENGTH(ans);

	for (i = 0; i < n; i++) {
		CHECK_INTERRUPT(i);

		PROTECT(elt = STRING_ELT(ans, i)); nprot++;
		if (elt == NA_STRING) {
			UNPROTECT(1); nprot--;
			continue;
		}

		ptr = (const uint8_t *)rutf8_translate_utf8(elt);
		size = strlen((const char *)ptr);
		TRY(utf8lite_text_assign(&text, ptr, size, 0, NULL));
		TRY(utf8lite_textmap_set(&ctx->map, &text));

		ptr = ctx->map.text.ptr;
		size = UTF8LITE_TEXT_SIZE(&ctx->map.text);
		TRY(size > INT_MAX ? UTF8LITE_ERROR_OVERFLOW : 0);

		elt = mkCharLenCE((const char *)ptr, (int)size, CE_UTF8);
		SET_STRING_ELT(ans, i, elt);
		UNPROTECT(1); nprot--;
	}

exit:
	CHECK_ERROR(err);
	rutf8_free_context(sctx);
	UNPROTECT(nprot);
	return ans;
}
