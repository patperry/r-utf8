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

#ifndef UTF8LITE_H
#define UTF8LITE_H

/**
 * \file utf8lite.h
 *
 * Unicode handling.
 */

#include <stddef.h>
#include <stdint.h>

/** Unicode replacement character */
#define UTF8LITE_REPLACEMENT	0xFFFD

/** Last valid unicode codepoint */
#define UTF8LITE_UNICODE_MAX 0x10FFFF

/** Number of bytes in the UTF-8 encoding of a valid unicode codepoint. */
#define UTF8LITE_UTF8_ENCODE_LEN(u) \
	((u) <= 0x7F     ? 1 : \
	 (u) <= 0x07FF   ? 2 : \
	 (u) <= 0xFFFF   ? 3 : 4)

/** Number of 16-bit code units in the UTF-16 encoding of a valid unicode
 *  codepoint */
#define UTF8LITE_UTF16_ENCODE_LEN(u) \
	((u) <= 0xFFFF ? 1 : 2)

/** High (leading) UTF-16 surrogate for a code point in the supplementary
 *  plane (U+10000 to U+10FFFF). */
#define UTF8LITE_UTF16_HIGH(u) \
	0xD800 | (((u) - 0x010000) >> 10)

/** Low (trailing) UTF-16 surrogate for a code point in the supplementary
 *  plane (U+10000 to U+10FFFF). */
#define UTF8LITE_UTF16_LOW(u) \
	0xDC00 | (((u) - 0x010000) & 0x03FF)

/** Indicates whether a 16-bit code unit is a UTF-16 high surrogate.
 *  High surrogates are in the range 0xD800 `(1101 1000 0000 0000)`
 *  to 0xDBFF `(1101 1011 1111 1111)`. */
#define UTF8LITE_IS_UTF16_HIGH(x) (((x) & 0xFC00) == 0xD800)

/** Indicates whether a 16-bit code unit is a UTF-16 low surrogate.
 *  Low surrogates are in the range 0xDC00 `(1101 1100 0000 0000)`
 *  to 0xDFFF `(1101 1111 1111 1111)`. */
#define UTF8LITE_IS_UTF16_LOW(x) (((x) & 0xFC00) == 0xDC00)

/** Given the high and low UTF-16 surrogates, compute the unicode codepoint. */
#define UTF8LITE_DECODE_UTF16_PAIR(h, l) \
	(((((h) & 0x3FF) << 10) | ((l) & 0x3FF)) + 0x10000)

/** Given the first byte in a valid UTF-8 byte sequence, determine the number of
 *  continuation bytes */
#define UTF8LITE_UTF8_TAIL_LEN(x) \
	(  ((x) & 0x80) == 0x00 ? 0 \
	 : ((x) & 0xE0) == 0xC0 ? 1 \
	 : ((x) & 0xF0) == 0xE0 ? 2 : 3)

/** Maximum number of UTF-8 continuation bytes in a valid encoded character */
#define UTF8LITE_UTF8_TAIL_MAX 3

/** Indicates whether a given unsigned integer is a valid ASCII codepoint */
#define UTF8LITE_IS_ASCII(x) \
	((x) <= 0x7F)

/** Indicates whether a given unsigned integer is a valid unicode codepoint */
#define UTF8LITE_IS_UNICODE(x) \
	(((x) <= UTF8LITE_UNICODE_MAX) \
	 && !UTF8LITE_IS_UTF16_HIGH(x) \
	 && !UTF8LITE_IS_UTF16_LOW(x))

/**
 * Validate the first character in a UTF-8 character buffer.
 *
 * \param bufptr a pointer to the input buffer; on exit, a pointer to
 * 	the end of the first valid UTF-8 character, or the first invalid
 * 	byte in the encoding
 * \param end the end of the input buffer
 *
 * \returns 0 on success, EINVAL on failure
 */
int utf8lite_scan_utf8(const uint8_t **bufptr, const uint8_t *end);

/**
 * Decode the first codepoint from a UTF-8 character buffer.
 * 
 * \param bufptr on input, a pointer to the start of the character buffer;
 * 	on exit, a pointer to the end of the first UTF-8 character in
 * 	the buffer
 * \param codeptr on exit, the first codepoint in the buffer
 */
void utf8lite_decode_utf8(const uint8_t **bufptr, uint32_t *codeptr);

/**
 * Encode a codepoint into a UTF-8 character buffer. The codepoint must
 * be a valid unicode character (according to #UTF8LITE_IS_UNICODE) and the buffer
 * must have space for at least #UTF8LITE_UTF8_ENCODE_LEN bytes.
 *
 * \param code the codepoint
 * \param bufptr on input, a pointer to the start of the buffer;
 * 	on exit, a pointer to the end of the encoded codepoint
 */
void utf8lite_encode_utf8(uint32_t code, uint8_t **bufptr);

/**
 * Encode a codepoint in reverse, at the end of UTF-8 character buffer.
 * The codepoint must be a valid unicode character (according to
 * #UTF8LITE_IS_UNICODE) and the buffer must have space for at least
 * #UTF8LITE_UTF8_ENCODE_LEN bytes.
 *
 * \param code the codepoint
 * \param endptr on input, a pointer to the end of the buffer;
 * 	on exit, a pointer to the start of the encoded codepoint
 */
void utf8lite_rencode_utf8(uint32_t code, uint8_t **endptr);

/**
 * Unicode character decomposition mappings. The compatibility mappings are
 * defined in [UAX #44 Sec. 5.7.3 Character Decomposition Maps]
 * (http://www.unicode.org/reports/tr44/#Character_Decomposition_Mappings).
 */
enum utf8lite_decomp_type {
	UTF8LITE_DECOMP_NORMAL = 0, /**< normalization (required for NFD) */
	UTF8LITE_DECOMP_FONT = (1 << 0),     /**< font variant */
	UTF8LITE_DECOMP_NOBREAK = (1 << 1),  /**< no-break version of a space
					      or hyphen */
	UTF8LITE_DECOMP_INITIAL = (1 << 2),  /**< initial presentation form
					      (Arabic) */
	UTF8LITE_DECOMP_MEDIAL = (1 << 3),   /**< medial presentation form
					      (Arabic) */
	UTF8LITE_DECOMP_FINAL = (1 << 4),    /**< final presentation form
					      (Arabic) */
	UTF8LITE_DECOMP_ISOLATED = (1 << 5), /**< isolated presentation form
					      (Arabic) */
	UTF8LITE_DECOMP_CIRCLE = (1 << 6),   /**< encircled form */
	UTF8LITE_DECOMP_SUPER = (1 << 7),    /**< superscript form */
	UTF8LITE_DECOMP_SUB = (1 << 8),      /**< subscript form */
	UTF8LITE_DECOMP_VERTICAL = (1 << 9), /**< vertical layout presentation
					      form */
	UTF8LITE_DECOMP_WIDE = (1 << 10),    /**< wide (or zenkaku)
					      compatibility */
	UTF8LITE_DECOMP_NARROW = (1 << 11),  /**< narrow (or hankaku)
					      compatibility */
	UTF8LITE_DECOMP_SMALL = (1 << 12),   /**< small variant form
					      (CNS compatibility) */
	UTF8LITE_DECOMP_SQUARE = (1 << 13),  /**< CJK squared font variant */
	UTF8LITE_DECOMP_FRACTION = (1 << 14),/**< vulgar fraction form */
	UTF8LITE_DECOMP_COMPAT = (1 << 15),  /**< unspecified compatibility */

	UTF8LITE_DECOMP_ALL = ((1 << 16) - 1)/**< all decompositions
					      (required for NFKD) */
};

/**
 * Unicode case folding. These are defined in *TR44* Sec. 5.6.
 */
enum utf8lite_casefold_type {
	UTF8LITE_CASEFOLD_NONE = 0,		/**< no case folding */
	UTF8LITE_CASEFOLD_ALL = (1 << 16)	/**< perform case folding */
};

/**
 * Maximum size (in codepoints) of a single code point's decomposition.
 *
 * From *TR44* Sec. 5.7.3: "Compatibility mappings are guaranteed to be no
 * longer than 18 characters, although most consist of just a few characters."
 */
#define UTF8LITE_DECOMP_MAX 18

/**
 * Apply decomposition and/or casefold mapping to a Unicode character,
 * outputting the result to the specified buffer. The output will be at
 * most #UTF8LITE_UNICODE_DECOMP_MAX codepoints.
 *
 * \param type a bitmask composed from #utf8lite_udecomp_type and
 * 	#utf8lite_ucasefold_type values specifying the mapping type
 * \param code the input codepoint
 * \param bufptr on entry, a pointer to the output buffer; on exit,
 * 	a pointer past the last output codepoint
 */
void utf8lite_map(int type, uint32_t code, uint32_t **bufptr);

/**
 * Apply the canonical ordering algorithm to put an array of Unicode
 * codepoints in normal order. See *Unicode* Sec 3.11 and *TR44* Sec. 5.7.4.
 *
 * \param ptr a pointer to the first codepoint
 * \param len the number of codepoints
 */
void utf8lite_order(uint32_t *ptr, size_t len);

/**
 * Apply the canonical composition algorithm to put an array of
 * canonically-ordered Unicode codepoints into composed form.
 *
 * \param ptr a pointer to the first codepoint
 * \param lenptr on entry, a pointer to the number of input codepoints;
 * 	on exit, a pointer to the number of composed codepoints
 */
void utf8lite_compose(uint32_t *ptr, size_t *lenptr);


/**
 * Unicode character width type.
 */
enum utf8lite_charwidth_type {
	UTF8LITE_CHARWIDTH_OTHER = -4,	/**< Control and others:
					  Cc, Cn, Co, Cs, Zl, Zp */
	UTF8LITE_CHARWIDTH_EMOJI = -3,    /**< Emoji, wide in most contexts */
	UTF8LITE_CHARWIDTH_AMBIGUOUS = -2,/**< can be narrow or wide depending
					  on the context */
	UTF8LITE_CHARWIDTH_IGNORABLE = -1,/**< Default ignorables */
	UTF8LITE_CHARWIDTH_NONE = 0,	/**< Combining marks: Mc, Me, Mn */
	UTF8LITE_CHARWIDTH_NARROW = 1,	/**< Most western alphabets */
	UTF8LITE_CHARWIDTH_WIDE = 2	/**< Most ideographs */
};

/**
 * Get the width of a Unicode character, using the East Asian Width table and
 * the Emoji data.
 *
 * \param code the codepoint
 *
 * \returns a #utf8lite_charwidth_type value giving the width
 */
int utf8lite_charwidth(uint32_t code);

/**
 * Get whether a Unicode character is white space.
 *
 * \param code the codepoint
 *
 * \returns 1 if space, 0 otherwise.
 */
int utf8lite_isspace(uint32_t code);

/**
 * Get whether a Unicode character is a default ignorable character.
 *
 * \param code the codepoint
 *
 * \returns 1 if space, 0 otherwise.
 */
int utf8lite_isignorable(uint32_t code);

#endif /* UTF8LITE_H */
