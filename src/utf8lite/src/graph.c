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
#include "utf8lite.h"

/*
                   width
graph               Ambiguous  Emoji Ignorable Narrow   Mark   None   Wide
  Control                   0      0      3809      0      0   2116      0
  CR                        0      0         0      0      0      1      0
  EBase                     0     98         0      0      0      0      0
  EBaseGAZ                  0      4         0      0      0      0      0
  EModifier                 0      5         0      0      0      0      0
  Extend                    0      0       359     26   1514      0      2
  GlueAfterZWJ              2     20         0      0      0      0      0
  L                         0      0         1      0      0      0    124
  LF                        0      0         0      0      0      1      0
  LV                        0      0         0      0      0      0    399
  LVT                       0      0         0      0      0      0  10773
  Other                   882    996         2  21606      0 971540  99206
  Prepend                   0      0         0      9     10      0      0
  RegionalIndicator         0     26         0      0      0      0      0
  SpacingMark               0      0         0    348      0      0      0
  T                         0      0         0    137      0      0      0
  V                         0      0         1     94      0      0      0
  ZWJ                       0      0         1      0      0      0      0
*/


static int ascii_width(int32_t ch, int flags);
static int utf8_escape_width(int32_t ch, int flags);
static int utf8_width(int32_t ch, int cw, int flags);


int utf8lite_graph_measure(const struct utf8lite_graph *g,
			   int flags, int *widthptr)
{
	struct utf8lite_text_iter it;
	int32_t ch;
	int err = 0, cw, w, width;

	width = 0;
	utf8lite_text_iter_make(&it, &g->text);

	while (utf8lite_text_iter_advance(&it)) {
		ch = it.current;

		if (ch <= 0x7F) {
			w = ascii_width(ch, flags);
		} else if (flags & UTF8LITE_ESCAPE_UTF8) {
			w = utf8_escape_width(ch, flags);
		} else if ((flags & UTF8LITE_ESCAPE_EXTENDED)
				&& (ch > 0xFFFF)) {
			w = utf8_escape_width(ch, flags);
		} else {
			cw = utf8lite_charwidth(ch);
			if (cw == UTF8LITE_CHARWIDTH_EMOJI) {
				width = 2;
				goto exit;
			}
			w = utf8_width(ch, cw, flags);
		}

		if (w < 0) {
			width = w;
			goto exit;
		} else if (w > INT_MAX - width) {
			width = -1;
			err = UTF8LITE_ERROR_OVERFLOW;
			goto exit;
		} else {
			width += w;
		}
	}

exit:
	if (widthptr) {
		*widthptr = width;
	}
	return err;
}


int ascii_width(int32_t ch, int flags)
{
	// handle control characters
	if (ch <= 0x1F || ch == 0x7F) {
		if (!(flags & UTF8LITE_ESCAPE_CONTROL)) {
			return -1;
		}

		switch (ch) {
		case '\a':
		case '\v':
			// \u0007, \u000b (JSON) : \a, \b (C)
			return (flags & UTF8LITE_ENCODE_JSON) ? 6 : 2;
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
			return 2;
		default:
			return 6; // \uXXXX
		}
	}

	// handle printable characters
	switch (ch) {
	case '\"':
		return (flags & UTF8LITE_ESCAPE_DQUOTE) ? 2 : 1;
	case '\'':
		return (flags & UTF8LITE_ESCAPE_SQUOTE) ? 2 : 1;
	case '\\':
		if (flags & (UTF8LITE_ESCAPE_CONTROL
				| UTF8LITE_ESCAPE_DQUOTE
				| UTF8LITE_ESCAPE_SQUOTE
				| UTF8LITE_ESCAPE_EXTENDED
				| UTF8LITE_ESCAPE_UTF8)) {
			return 2;
		} else {
			return 1;
		}
	default:
		return 1;
	}
}


int utf8_width(int32_t ch, int cw, int flags)
{
	int w = -1;

	switch ((enum utf8lite_charwidth_type)cw) {
	case UTF8LITE_CHARWIDTH_NONE:
		if (flags & UTF8LITE_ESCAPE_CONTROL) {
			w = utf8_escape_width(ch, flags);
		} else {
			w = -1;
		}
		break;

	case UTF8LITE_CHARWIDTH_IGNORABLE:
	case UTF8LITE_CHARWIDTH_MARK:
		w = 0;
		break;

	case UTF8LITE_CHARWIDTH_NARROW:
		w = 1;
		break;

	case UTF8LITE_CHARWIDTH_AMBIGUOUS:
		w = (flags & UTF8LITE_ENCODE_AMBIGWIDE) ? 2 : 1;
		break;

	case UTF8LITE_CHARWIDTH_WIDE:
	case UTF8LITE_CHARWIDTH_EMOJI:
		w = 2;
		break;
	}

	return w;
}


int utf8_escape_width(int32_t ch, int flags)
{
	if (ch <= 0xFFFF) {
		return 6;
	} else if (flags & UTF8LITE_ENCODE_JSON) {
		return 12;
	} else {
		return 10;
	}
}
