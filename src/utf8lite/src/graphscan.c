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
#include <stdio.h>
#include "private/graphbreak.h"
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


#define NEXT() \
	do { \
		scan->current.text.attr |= scan->iter.attr; \
		scan->ptr = scan->iter.ptr; \
		if (utf8lite_text_iter_advance(&scan->iter)) { \
			scan->prop = graph_break(scan->iter.current); \
		} else { \
			scan->prop = -1; \
		} \
	} while (0)


void utf8lite_graphscan_make(struct utf8lite_graphscan *scan,
			     const struct utf8lite_text *text)
{
	utf8lite_text_iter_make(&scan->iter, text);
	utf8lite_graphscan_reset(scan);
}


void utf8lite_graphscan_reset(struct utf8lite_graphscan *scan)
{
	utf8lite_text_iter_reset(&scan->iter);
	scan->current.text.ptr = (uint8_t *)scan->iter.ptr;
	scan->current.text.attr = 0;
	NEXT();
}


int utf8lite_graphscan_advance(struct utf8lite_graphscan *scan)
{
	scan->current.text.ptr = (uint8_t *)scan->ptr;
	scan->current.text.attr = 0;

Start:
	// GB2: Break at the end of text
	if (scan->prop < 0) {
		goto Break;
	}

	switch ((enum graph_break_prop)scan->prop) {
	case GRAPH_BREAK_CR:
		NEXT();
		goto CR;

	case GRAPH_BREAK_CONTROL:
	case GRAPH_BREAK_LF:
		// Break after controls
		// GB4: (Newline | LF) +
		NEXT();
		goto Break;

	case GRAPH_BREAK_L:
		NEXT();
		goto L;

	case GRAPH_BREAK_LV:
	case GRAPH_BREAK_V:
		NEXT();
		goto V;

	case GRAPH_BREAK_LVT:
	case GRAPH_BREAK_T:
		NEXT();
		goto T;

	case GRAPH_BREAK_PREPEND:
		NEXT();
		goto Prepend;

	case GRAPH_BREAK_E_BASE:
	case GRAPH_BREAK_E_BASE_GAZ:
		NEXT();
		goto E_Base;

	case GRAPH_BREAK_ZWJ:
		NEXT();
		goto ZWJ;

	case GRAPH_BREAK_REGIONAL_INDICATOR:
		NEXT();
		goto Regional_Indicator;

	case GRAPH_BREAK_E_MODIFIER:
	case GRAPH_BREAK_GLUE_AFTER_ZWJ:
		NEXT();
		goto MaybeBreak;

	case GRAPH_BREAK_EXTEND:
	case GRAPH_BREAK_SPACINGMARK:
	case GRAPH_BREAK_OTHER:
		NEXT();
		goto MaybeBreak;
	}

	assert(0 && "unhandled grapheme break property");

CR:
	// GB3: Do not break within CRLF
	// GB4: Otherwise break after controls
	if (scan->prop == GRAPH_BREAK_LF) {
		NEXT();
	}
	goto Break;

L:
	// GB6: Do not break Hangul syllable sequences.
	switch (scan->prop) {
	case GRAPH_BREAK_L:
		NEXT();
		goto L;

	case GRAPH_BREAK_V:
	case GRAPH_BREAK_LV:
		NEXT();
		goto V;

	case GRAPH_BREAK_LVT:
		NEXT();
		goto T;

	default:
		goto MaybeBreak;
	}

V:
	// GB7: Do not break Hangul syllable sequences.
	switch (scan->prop) {
	case GRAPH_BREAK_V:
		NEXT();
		goto V;

	case GRAPH_BREAK_T:
		NEXT();
		goto T;

	default:
		goto MaybeBreak;
	}

T:
	// GB8: Do not break Hangul syllable sequences.
	switch (scan->prop) {
	case GRAPH_BREAK_T:
		NEXT();
		goto T;

	default:
		goto MaybeBreak;
	}

Prepend:
	switch (scan->prop) {
	case GRAPH_BREAK_CONTROL:
	case GRAPH_BREAK_CR:
	case GRAPH_BREAK_LF:
		// GB5: break before controls
		goto Break;

	default:
		// GB9b: do not break after Prepend characters.
		goto Start;
	}

E_Base:
	// GB9:  Do not break before extending characters
	// GB10: Do not break within emoji modifier sequences
	while (scan->prop == GRAPH_BREAK_EXTEND) {
		NEXT();
	}
	if (scan->prop == GRAPH_BREAK_E_MODIFIER) {
		NEXT();
	}
	goto MaybeBreak;

ZWJ:
	// Do not break within emoji zwj sequences
	// GB11: ZWJ * (Glue_After_Zwj | EBG)
	switch (scan->prop) {
	case GRAPH_BREAK_GLUE_AFTER_ZWJ:
		NEXT();
		goto MaybeBreak;

	case GRAPH_BREAK_E_BASE_GAZ:
		NEXT();
		goto E_Base;

	default:
		goto MaybeBreak;
	}

Regional_Indicator:
	// Do not break within emoji flag sequences. That is, do not break
	// between regional indicator (RI) symbols if there is an odd number
	// of RI characters before the break point
	if (scan->prop == GRAPH_BREAK_REGIONAL_INDICATOR) {
		// GB12/13: [^RI] RI * RI
		NEXT();
	}
	goto MaybeBreak;

MaybeBreak:
	// GB9: Do not break before extending characters or ZWJ.
	// GB9a: Do not break before SpacingMark [extended grapheme clusters]
	// GB999: Otherwise, break everywhere
	switch (scan->prop) {
	case GRAPH_BREAK_EXTEND:
	case GRAPH_BREAK_SPACINGMARK:
		NEXT();
		goto MaybeBreak;

	case GRAPH_BREAK_ZWJ:
		NEXT();
		goto ZWJ;

	default:
		goto Break;
	}

Break:
	scan->current.text.attr |= (size_t)(scan->ptr - scan->current.text.ptr);
	return (scan->ptr == scan->current.text.ptr) ? 0 : 1;
}
