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
#include "private/charwidth.h"
#include "utf8lite.h"


int utf8lite_charwidth(int32_t code)
{
	int prop = charwidth(code);
	switch(prop) {
	case CHARWIDTH_NONE:
		return UTF8LITE_CHARWIDTH_NONE;
	case CHARWIDTH_IGNORABLE:
		return UTF8LITE_CHARWIDTH_IGNORABLE;
	case CHARWIDTH_MARK:
		return UTF8LITE_CHARWIDTH_MARK;
	case CHARWIDTH_NARROW:
		return UTF8LITE_CHARWIDTH_NARROW;
	case CHARWIDTH_AMBIGUOUS:
		return UTF8LITE_CHARWIDTH_AMBIGUOUS;
	case CHARWIDTH_WIDE:
		return UTF8LITE_CHARWIDTH_WIDE;
	case CHARWIDTH_EMOJI:
		return UTF8LITE_CHARWIDTH_EMOJI;
	default:
		assert(0 && "internal error: unrecognized charwidth property");
		return prop;
	}
}


// TODO: use character class lookup table
int utf8lite_isspace(int32_t code)
{
	if (code <= 0x7F) {
		return (code == 0x20 || (0x09 <= code && code < 0x0E));
	} else if (code <= 0x1FFF) {
		switch (code) {
		case 0x0085:
		case 0x00A0:
		case 0x1680:
			return 1;
		default:
			return 0;
		}
	} else if (code <= 0x200A) {
		return 1;
	} else if (code <= 0x3000) {
		switch (code) {
		case 0x2028:
		case 0x2029:
		case 0x202F:
		case 0x205F:
		case 0x3000:
			return 1;
		default:
			return 0;
		}
	} else {
		return 0;
	}
}


int utf8lite_isignorable(int32_t code)
{
    int prop = utf8lite_charwidth(code);
    return (prop == UTF8LITE_CHARWIDTH_IGNORABLE);
}
