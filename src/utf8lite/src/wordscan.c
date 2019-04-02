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
#include "utf8lite.h"
#include "private/emojiprop.h"
#include "private/wordbreak.h"
#include "utf8lite.h"


void utf8lite_wordscan_make(struct utf8lite_wordscan *scan,
			    const struct utf8lite_text *text)
{
	utf8lite_text_iter_make(&scan->iter, text);
	utf8lite_wordscan_reset(scan);
}


#define NEXT() \
	do { \
		follow_zwj = (scan->prop == WORD_BREAK_ZWJ); \
		scan->ptr = scan->iter_ptr; \
		scan->code = scan->iter.current; \
		scan->prop = scan->iter_prop; \
		scan->iter_ptr = scan->iter.ptr; \
		if (utf8lite_text_iter_advance(&scan->iter)) { \
			scan->iter_prop = word_break(scan->iter.current); \
		} else { \
			scan->iter_prop = WORD_BREAK_NONE; \
		} \
	} while (0)

#define EXTEND() \
	do { \
		while (scan->prop == WORD_BREAK_EXTEND \
				|| scan->prop == WORD_BREAK_FORMAT \
				|| scan->prop == WORD_BREAK_ZWJ) { \
			NEXT(); \
		} \
	} while (0)
		

void utf8lite_wordscan_reset(struct utf8lite_wordscan *scan)
{
	scan->current.ptr = NULL;
	scan->current.attr = scan->iter.text_attr & ~UTF8LITE_TEXT_SIZE_MASK;

	utf8lite_text_iter_reset(&scan->iter);
	scan->ptr = scan->iter.ptr;

	if (utf8lite_text_iter_advance(&scan->iter)) {
		scan->code = scan->iter.current;
		scan->prop = word_break(scan->code);

		scan->iter_ptr = scan->iter.ptr;
		if (utf8lite_text_iter_advance(&scan->iter)) {
			scan->iter_prop = word_break(scan->iter.current);
		} else {
			scan->iter_prop = WORD_BREAK_NONE;
		}
	} else {
		scan->code = 0;
		scan->prop = WORD_BREAK_NONE;
		scan->iter_ptr = NULL;
		scan->iter_prop = WORD_BREAK_NONE;
	}
}


static int next_signif_prop(const struct utf8lite_wordscan *scan)
{
	struct utf8lite_text_iter iter;
	int prop;

	switch (scan->iter_prop) {
	case WORD_BREAK_EXTEND:
	case WORD_BREAK_FORMAT:
	case WORD_BREAK_ZWJ:
		break;
	default:
		return scan->iter_prop;
	}

       	iter = scan->iter;
	while (utf8lite_text_iter_advance(&iter)) {
		prop = word_break(iter.current);
		switch (prop) {
		case WORD_BREAK_EXTEND:
		case WORD_BREAK_FORMAT:
		case WORD_BREAK_ZWJ:
			break;
		default:
			return prop;
		}
	}
	return WORD_BREAK_NONE;
}


int utf8lite_wordscan_advance(struct utf8lite_wordscan *scan)
{
	int follow_zwj = 0;
	scan->current.ptr = (uint8_t *)scan->ptr;
	scan->current.attr &= ~UTF8LITE_TEXT_SIZE_MASK;

Start:
	switch ((enum word_break_prop)scan->prop) {
	case WORD_BREAK_NONE:
		// Break at the start and end of text unless the text is empty
		// WB2: Any + eot
		goto Break;

	case WORD_BREAK_CR:
		NEXT();
		goto CR;

	case WORD_BREAK_NEWLINE:
	case WORD_BREAK_LF:
		NEXT();
		goto Newline;

	case WORD_BREAK_WSEGSPACE:
		NEXT();
		goto WSegSpace;

	case WORD_BREAK_ALETTER:
		NEXT();
		goto ALetter;

	case WORD_BREAK_NUMERIC:
		NEXT();
		goto Numeric;

	case WORD_BREAK_EXTENDNUMLET:
		NEXT();
		goto ExtendNumLet;

	case WORD_BREAK_HEBREW_LETTER:
		NEXT();
		goto Hebrew_Letter;

	case WORD_BREAK_KATAKANA:
		NEXT();
		goto Katakana;

	case WORD_BREAK_REGIONAL_INDICATOR:
		NEXT();
		goto Regional_Indicator;

	case WORD_BREAK_DOUBLE_QUOTE:
	case WORD_BREAK_MIDLETTER:
	case WORD_BREAK_MIDNUM:
	case WORD_BREAK_MIDNUMLET:
	case WORD_BREAK_SINGLE_QUOTE:
	case WORD_BREAK_EXTEND: // marks
	case WORD_BREAK_FORMAT: // Cf format controls
	case WORD_BREAK_ZWJ:
	case WORD_BREAK_OTHER:
		NEXT();
		goto Any;
	}

	assert(0 && "Unhandled word break property");
	return 0;
CR:
	if (scan->prop == WORD_BREAK_LF) {
		// Do not break within CRLF
		// WB3: CR * LF
		NEXT();
	}

Newline:
	// Otherwise break after Newlines
	// WB3a: (Newline | CR | LF) +
	goto Break;


WSegSpace:
	// WB3d: Keep horizontal whitespace together.
	if (scan->prop == WORD_BREAK_WSEGSPACE) {
    		NEXT();
		goto WSegSpace;
	}
	EXTEND();
	goto MaybeBreak;

ALetter:
	EXTEND();

	switch (scan->prop) {
	case WORD_BREAK_ALETTER:
		// Do not break between most letters
		// WB5: AHLetter * AHLetter
		NEXT();
		goto ALetter;

	case WORD_BREAK_HEBREW_LETTER:
		// WB5: AHLetter * AHLetter
		NEXT();
		goto Hebrew_Letter;

	case WORD_BREAK_MIDLETTER:
	case WORD_BREAK_MIDNUMLET:
	case WORD_BREAK_SINGLE_QUOTE:
		// Do not break across certain punctuation

		// WB6: AHLetter * (MidLetter | MidNumLetQ) AHLetter

		switch (next_signif_prop(scan)) {
		case WORD_BREAK_ALETTER:
			// WB7: AHLetter (MidLetter | MidNumLetQ) * AHLetter
			NEXT();
			EXTEND();
			NEXT();
			goto ALetter;
		case WORD_BREAK_HEBREW_LETTER:
			// WB7:  AHLetter (MidLetter | MidNumLetQ) * AHLetter
			NEXT();
			EXTEND();
			NEXT();
			goto Hebrew_Letter;
		default:
			goto MaybeBreak;
		}

	case WORD_BREAK_NUMERIC:
		// Do not break within sequences of digits, or digits
		// adjacent to letters (“3a”, or “A3”).
		// WB9: AHLetter * Numeric
		NEXT();
		goto Numeric;

	case WORD_BREAK_EXTENDNUMLET:
		// Do not break from extenders
		// WB13a: AHLetter * ExtendNumLet
		NEXT();
		goto ExtendNumLet;

	default:
		goto MaybeBreak;
	}

Hebrew_Letter:
	EXTEND();

	switch (scan->prop) {
	case WORD_BREAK_ALETTER:
		// Do not break between most letters
		// WB5: AHLetter * AHLetter
		NEXT();
		goto ALetter;

	case WORD_BREAK_HEBREW_LETTER:
		// WB5: AHLetter * AHLetter
		NEXT();
		goto Hebrew_Letter;

	case WORD_BREAK_MIDLETTER:
	case WORD_BREAK_MIDNUMLET:
	case WORD_BREAK_SINGLE_QUOTE:
		// Do not break across certain punctuation

		// WB6: AHLetter * (MidLetter | MidNumLetQ) * AHLetter
		switch (next_signif_prop(scan)) {
		case WORD_BREAK_HEBREW_LETTER:
			// WB7:
			// AHLetter (MidLetter | MidNumLetQ) * AHLetter
			NEXT();
			EXTEND();
			NEXT();
			goto Hebrew_Letter;
		case WORD_BREAK_ALETTER:
			// WB7:
			// AHLetter (MidLetter | MidNumLetQ) * AHLetter
			NEXT();
			EXTEND();
			NEXT();
			goto ALetter;
		default:
			break;
		}

		if (scan->prop == WORD_BREAK_SINGLE_QUOTE) {
			NEXT();
			goto Any;
		}

		goto MaybeBreak;


	case WORD_BREAK_DOUBLE_QUOTE:
		// WB7b: Hebrew_Letter * Double_Quote Hebrew_Letter
		switch (next_signif_prop(scan)) {
		case WORD_BREAK_HEBREW_LETTER:
			// Wb7c:
			//   Hebrew_Letter Double_Quote * Hebrew_Letter
			NEXT();
			EXTEND();
			NEXT();
			goto Hebrew_Letter;
		default:
			goto MaybeBreak;
		}

	case WORD_BREAK_NUMERIC:
		// WB9: AHLetter * Numeric
		NEXT();
		goto Numeric;

	case WORD_BREAK_EXTENDNUMLET:
		// WB13a: AHLetter * ExtendNumLet
		NEXT();
		goto ExtendNumLet;

	default:
		goto MaybeBreak;
	}

Numeric:
	EXTEND();

	switch (scan->prop) {
	case WORD_BREAK_NUMERIC:
		// WB8: Numeric * Numeric
		NEXT();
		goto Numeric;

	case WORD_BREAK_MIDNUMLET:
	case WORD_BREAK_SINGLE_QUOTE:
	case WORD_BREAK_MIDNUM:
		// WB12: Numeric * (MidNum | MidNumLetQ) Numeric
		if (next_signif_prop(scan) == WORD_BREAK_NUMERIC) {
			// WB11: Numeric (MidNum|MidNumLeqQ) * Numeric
			NEXT();
			EXTEND();
			NEXT();
			goto Numeric;
		}
		goto MaybeBreak;

	case WORD_BREAK_EXTENDNUMLET:
		// WB13a: Numeric * ExtendNumLet
		NEXT();
		goto ExtendNumLet;

	case WORD_BREAK_ALETTER:
		// WB10: Numeric * AHLetter
		NEXT();
		goto ALetter;

	case WORD_BREAK_HEBREW_LETTER:
		// WB10: Numeric * AHLetter
		NEXT();
		goto Hebrew_Letter;

	default:
		goto MaybeBreak;
	}

Katakana:
	EXTEND();

	switch (scan->prop) {
	case WORD_BREAK_KATAKANA:
		// WB13: Katakana * Katakana
		NEXT();
		goto Katakana;

	case WORD_BREAK_EXTENDNUMLET:
		// WB13a: Katakana * ExtendNumLet
		NEXT();
		goto ExtendNumLet;

	default:
		goto MaybeBreak;
	}

ExtendNumLet:
	EXTEND();

	switch (scan->prop) {
	case WORD_BREAK_ALETTER:
		// WB13b: ExtendNumLet * AHLetter
		NEXT();
		goto ALetter;

	case WORD_BREAK_NUMERIC:
		// WB13b: ExtendNumLet * Numeric
		NEXT();
		goto Numeric;

	case WORD_BREAK_EXTENDNUMLET:
		// WB13a: ExtendNumLet * ExtendNumLet
		NEXT();
		goto ExtendNumLet;

	case WORD_BREAK_HEBREW_LETTER:
		// WB13b: ExtendNumLet * AHLetter
		NEXT();
		goto Hebrew_Letter;

	case WORD_BREAK_KATAKANA:
		// WB13c: ExtendNumLet * Katakana
		NEXT();
		goto Katakana;

	default:
		goto MaybeBreak;
	}

Regional_Indicator:
	EXTEND();

	//fprintf(stderr, "Regional_Indicator: code = U+%04X\n", code);

	// Do not break within emoji flag sequences. That is, do not break
	// between regional indicator (RI) symbols if there is an odd number
	// of RI characters before the break point

	switch (scan->prop) {
	case WORD_BREAK_REGIONAL_INDICATOR:
		// WB15/16: [^RI] RI * RI
		NEXT();
		EXTEND();
		goto MaybeBreak;

	default:
		// WB15/16: [^RI] RI * RI
		goto MaybeBreak;
	}

Any:
	EXTEND();
	goto MaybeBreak;

MaybeBreak:
	// WB3c: Do not break within emoji zwj sequences.
	if (follow_zwj && (emoji_prop(scan->code)
			   & EMOJI_PROP_EXTENDED_PICTOGRAPHIC)) {
		NEXT();
		goto Start;
	}
	goto Break;
	
Break:
	scan->current.attr |= (size_t)(scan->ptr - scan->current.ptr);
	return (scan->ptr == scan->current.ptr) ? 0 : 1;
}
