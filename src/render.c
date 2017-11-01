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
#include <stdlib.h>
#include "rutf8.h"

#define RENDER_TAG install("utf8::render")

struct rutf8_render {
	struct utf8lite_render render;
	int has_render;
};


void rutf8_free_render(SEXP x)
{
        struct rutf8_render *obj = R_ExternalPtrAddr(x);
        R_SetExternalPtrAddr(x, NULL);
	if (obj) {
		if (obj->has_render) {
			utf8lite_render_destroy(&obj->render);
		}
		free(obj);
	}
}


SEXP rutf8_alloc_render(int flags)
{
	SEXP ans;
	struct rutf8_render *obj;
	int err = 0;

	PROTECT(ans = R_MakeExternalPtr(NULL, RENDER_TAG, R_NilValue));
        R_RegisterCFinalizerEx(ans, rutf8_free_render, TRUE);

	TRY_ALLOC(obj = calloc(1, sizeof(*obj)));
        R_SetExternalPtrAddr(ans, obj);

	TRY(utf8lite_render_init(&obj->render, flags));
	obj->has_render = 1;
exit:
	CHECK_ERROR(err);
	UNPROTECT(1);
	return ans;
}


int rutf8_is_render(SEXP x)
{
	return ((TYPEOF(x) == EXTPTRSXP)
                && (R_ExternalPtrTag(x) == RENDER_TAG));
}


struct utf8lite_render *rutf8_as_render(SEXP x)
{
	struct rutf8_render *obj;

	if (!rutf8_is_render(x)) {
		error("invalid render object");
	}

	obj = R_ExternalPtrAddr(x);
	return (obj->has_render) ? &obj->render : NULL;
}
