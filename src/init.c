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
#include <Rdefines.h>
#include <R_ext/Rdynload.h>
#include "rutf8.h"

#define CALLDEF(name, n)  {#name, (DL_FUNC) &name, n}

static const R_CallMethodDef CallEntries[] = {
	CALLDEF(rutf8_as_utf8, 1),
        CALLDEF(rutf8_render_table, 14),
	CALLDEF(rutf8_utf8_encode, 7),
	CALLDEF(rutf8_utf8_format, 11),
	CALLDEF(rutf8_utf8_normalize, 5),
	CALLDEF(rutf8_utf8_valid, 1),
	CALLDEF(rutf8_utf8_width, 4),
        {NULL, NULL, 0}
};

void R_init_utf8(DllInfo *dll)
{
	R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
	R_useDynamicSymbols(dll, FALSE);
	R_forceSymbols(dll, TRUE);
}
