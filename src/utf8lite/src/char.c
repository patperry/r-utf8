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


// TODO use lookup table
int utf8lite_isignorable(int32_t code)
{
	// Default_Ignorable_Code_Point = Yes
	if (code <= 0x7F) {
		return 0;
	}

	switch (code) {
	case 0x00AD:
	case 0x034F:
	case 0x061C:
	case 0x115F:
	case 0x1160:
	case 0x17B4:
	case 0x17B5:
	case 0x180B:
	case 0x180C:
	case 0x180D:
	case 0x180E:
	case 0x200B:
	case 0x200C:
	case 0x200D:
	case 0x200E:
	case 0x200F:
	case 0x202A:
	case 0x202B:
	case 0x202C:
	case 0x202D:
	case 0x202E:
	case 0x2060:
	case 0x2061:
	case 0x2062:
	case 0x2063:
	case 0x2064:
	case 0x2065:
	case 0x2066:
	case 0x2067:
	case 0x2068:
	case 0x2069:
	case 0x206A:
	case 0x206B:
	case 0x206C:
	case 0x206D:
	case 0x206E:
	case 0x206F:
	case 0x3164:
	case 0xFE00:
	case 0xFE01:
	case 0xFE02:
	case 0xFE03:
	case 0xFE04:
	case 0xFE05:
	case 0xFE06:
	case 0xFE07:
	case 0xFE08:
	case 0xFE09:
	case 0xFE0A:
	case 0xFE0B:
	case 0xFE0C:
	case 0xFE0D:
	case 0xFE0E:
	case 0xFE0F:
	case 0xFEFF:
	case 0xFFA0:
	case 0xFFF0:
	case 0xFFF1:
	case 0xFFF2:
	case 0xFFF3:
	case 0xFFF4:
	case 0xFFF5:
	case 0xFFF6:
	case 0xFFF7:
	case 0xFFF8:
	case 0x1BCA0:
	case 0x1BCA1:
	case 0x1BCA2:
	case 0x1BCA3:
	case 0x1D173:
	case 0x1D174:
	case 0x1D175:
	case 0x1D176:
	case 0x1D177:
	case 0x1D178:
	case 0x1D179:
	case 0x1D17A:
		return 1;
	default:
		return (0xDFFFF < code && code <= 0xE0FFF);
	}
}
