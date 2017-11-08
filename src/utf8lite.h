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
 * Lightweight UTF-8 processing.
 */

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/**
 * \defgroup error Error handling
 * @{
 */

/** Maximum error message length, in bytes, not including the trailing NUL */
#define UTF8LITE_MESSAGE_MAX 255

/**
 * Error code.
 */
enum utf8lite_error_type {
	UTF8LITE_ERROR_NONE = 0,/**< no error */
	UTF8LITE_ERROR_INVAL,	/**< invalid input */
	UTF8LITE_ERROR_NOMEM,	/**< out of memory */
	UTF8LITE_ERROR_OS,	/**< operating system error */
	UTF8LITE_ERROR_OVERFLOW,/**< size exceeds maximum */
	UTF8LITE_ERROR_DOMAIN,	/**< input is out of domain */
	UTF8LITE_ERROR_RANGE,	/**< output is out of range */
	UTF8LITE_ERROR_INTERNAL	/**< internal error */
};

/**
 * Message buffer.
 */
struct utf8lite_message {
	char string[UTF8LITE_MESSAGE_MAX + 1]; /**< NUL-terminated message */
};

/**
 * Set a message to the empty string.
 *
 * \param msg message, or NULL
 */
void utf8lite_message_clear(struct utf8lite_message *msg);

/**
 * Set a message to a formatted string.
 *
 * \param msg message, or NULL
 * \param fmt format string
 * \param ... format arguments
 */
void utf8lite_message_set(struct utf8lite_message *msg, const char *fmt, ...)
#if defined(_WIN32) || defined(_WIN64)
	;
#else
	__attribute__ ((format (printf, 2, 3)));
#endif

/**
 * Append to a message.
 *
 * \param msg message, or NULL
 * \param fmt format string
 * \param ... format arguments
 */
void utf8lite_message_append(struct utf8lite_message *msg, const char *fmt, ...)
#if defined(_WIN32) || defined(_WIN64)
	;
#else
	__attribute__ ((format (printf, 2, 3)));
#endif

/**@}*/

/**
 * \defgroup char Unicode characters
 * @{
 */

/** Missing Unicode value */
#define UTF8LITE_CODE_NONE -1

/** Unicode replacement character */
#define UTF8LITE_CODE_REPLACEMENT 0xFFFD

/** Last valid unicode codepoint */
#define UTF8LITE_CODE_MAX 0x10FFFF

/** Number of bits required to encode a codepoint */
#define UTF8LITE_CODE_BITS 21

/** Indicates whether a given unsigned integer is a valid ASCII codepoint */
#define UTF8LITE_IS_ASCII(x) \
	((x) <= 0x7F)

/** Indicates whether a given unsigned integer is a valid unicode codepoint */
#define UTF8LITE_IS_UNICODE(x) \
	(((x) <= UTF8LITE_CODE_MAX) \
	 && !UTF8LITE_IS_UTF16_HIGH(x) \
	 && !UTF8LITE_IS_UTF16_LOW(x))

/**
 * Unicode character width type.
 */
enum utf8lite_charwidth_type {
	UTF8LITE_CHARWIDTH_NONE = 0,	/**< Control or and other */
	UTF8LITE_CHARWIDTH_IGNORABLE,	/**< Default ignorable */
	UTF8LITE_CHARWIDTH_MARK,	/**< Zero-width mark or format */
	UTF8LITE_CHARWIDTH_NARROW,	/**< Most western alphabets */
	UTF8LITE_CHARWIDTH_AMBIGUOUS,	/**< Width depends on context */
	UTF8LITE_CHARWIDTH_WIDE,	/**< Most ideographs */
	UTF8LITE_CHARWIDTH_EMOJI	/**< Emoji presentation */
};

/**
 * Get the width of a Unicode character, using the East Asian Width table and
 * the Emoji data.
 *
 * \param code the codepoint
 *
 * \returns a #utf8lite_charwidth_type value giving the width
 */
int utf8lite_charwidth(int32_t code);

/**
 * Get whether a Unicode character is white space.
 *
 * \param code the codepoint
 *
 * \returns 1 if space, 0 otherwise.
 */
int utf8lite_isspace(int32_t code);

/**
 * Get whether a Unicode character is a default ignorable character.
 *
 * \param code the codepoint
 *
 * \returns 1 if space, 0 otherwise.
 */
int utf8lite_isignorable(int32_t code);

/**@}*/

/**
 * \defgroup encode Encoding
 * @{
 */

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
	0xD800 | (((unsigned)(u) - 0x010000) >> 10)

/** Low (trailing) UTF-16 surrogate for a code point in the supplementary
 *  plane (U+10000 to U+10FFFF). */
#define UTF8LITE_UTF16_LOW(u) \
	0xDC00 | (((unsigned)(u) - 0x010000) & 0x03FF)

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

/**
 * Validate the first character in a UTF-8 character buffer.
 *
 * \param bufptr a pointer to the input buffer; on exit, a pointer to
 * 	the end of the first valid UTF-8 character, or the first invalid
 * 	byte in the encoding
 * \param end the end of the input buffer
 * \param msg an error message buffer
 *
 * \returns 0 on success
 */
int utf8lite_scan_utf8(const uint8_t **bufptr, const uint8_t *end,
		       struct utf8lite_message *msg);

/**
 * Decode the first codepoint from a UTF-8 character buffer.
 * 
 * \param bufptr on input, a pointer to the start of the character buffer;
 * 	on exit, a pointer to the end of the first UTF-8 character in
 * 	the buffer
 * \param codeptr on exit, the first codepoint in the buffer
 */
void utf8lite_decode_utf8(const uint8_t **bufptr, int32_t *codeptr);

/**
 * Encode a codepoint into a UTF-8 character buffer. The codepoint must
 * be a valid unicode character (according to #UTF8LITE_IS_UNICODE) and the buffer
 * must have space for at least #UTF8LITE_UTF8_ENCODE_LEN bytes.
 *
 * \param code the codepoint
 * \param bufptr on input, a pointer to the start of the buffer;
 * 	on exit, a pointer to the end of the encoded codepoint
 */
void utf8lite_encode_utf8(int32_t code, uint8_t **bufptr);

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
void utf8lite_rencode_utf8(int32_t code, uint8_t **endptr);

/**@}*/

/**
 * \defgroup escape Escape code handling
 * @{
 */

/**
 * Scan a JSON-style backslash (\\) escape.
 *
 * \param bufptr on input, a pointer to the byte after the backslash;
 * 	on output, a pointer to the byte after the escape
 * \param end pointer to the end of the buffer
 * \param msg error message buffer
 *
 * \returns 0 on success
 */
int utf8lite_scan_escape(const uint8_t **bufptr, const uint8_t *end,
			 struct utf8lite_message *msg);

/**
 * Scan a JSON-style backslash-u (\\u) escape.
 *
 * \param bufptr on input, a pointer to the byte after the 'u';
 * 	on output, a pointer to the byte after the escape
 * \param end pointer to the end of the buffer
 * \param msg error message buffer
 *
 * \returns 0 on success
 */
int utf8lite_scan_uescape(const uint8_t **bufptr, const uint8_t *end,
			  struct utf8lite_message *msg);

/**
 * Decode a JSON-style backslash (\\) escape.
 *
 * \param bufptr on input, a pointer to the byte after the backslash;
 * 	on output, a pointer to the byte after the escape
 * \param codeptr on output, a pointer to the decoded UTF-32 character
 */
void utf8lite_decode_escape(const uint8_t **bufptr, int32_t *codeptr);

/**
 * Scan a JSON-style backslash-u (\\u) escape.
 *
 * \param bufptr on input, a pointer to the byte after the 'u';
 * 	on output, a pointer to the byte after the escape
 * \param codeptr on output, a pointer to the decoded UTF-32 character
 */
void utf8lite_decode_uescape(const uint8_t **bufptr, int32_t *codeptr);

/**@}*/

/**
 * \defgroup normalize Normalization
 * @{
 */

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
#define UTF8LITE_UNICODE_DECOMP_MAX 18

/**
 * Apply decomposition and/or casefold mapping to a Unicode character,
 * outputting the result to the specified buffer. The output will be at
 * most #UTF8LITE_UNICODE_DECOMP_MAX codepoints.
 *
 * \param type a bitmask composed from #utf8lite_decomp_type and
 * 	#utf8lite_casefold_type values specifying the mapping type
 * \param code the input codepoint
 * \param bufptr on entry, a pointer to the output buffer; on exit,
 * 	a pointer past the last output codepoint
 */
void utf8lite_map(int type, int32_t code, int32_t **bufptr);

/**
 * Apply the canonical ordering algorithm to put an array of Unicode
 * codepoints in normal order. See *Unicode* Sec 3.11 and *TR44* Sec. 5.7.4.
 *
 * \param ptr a pointer to the first codepoint
 * \param len the number of codepoints
 */
void utf8lite_order(int32_t *ptr, size_t len);

/**
 * Apply the canonical composition algorithm to put an array of
 * canonically-ordered Unicode codepoints into composed form.
 *
 * \param ptr a pointer to the first codepoint
 * \param lenptr on entry, a pointer to the number of input codepoints;
 * 	on exit, a pointer to the number of composed codepoints
 */
void utf8lite_compose(int32_t *ptr, size_t *lenptr);

/**@}*/

/**
 * \defgroup text UTF-8 encoded text
 * @{
 */

/** Whether the text might contain a backslash (`\`) that should be
 * interpreted as an escape */
#define UTF8LITE_TEXT_ESC_BIT	((size_t)1 << (CHAR_BIT * sizeof(size_t) - 1))

/** Size of the encoded text, in bytes; (decoded size) <= (encoded size) */
#define UTF8LITE_TEXT_SIZE_MASK	((size_t)SIZE_MAX >> 1)

/** Maximum size of encode text, in bytes. */
#define UTF8LITE_TEXT_SIZE_MAX	UTF8LITE_TEXT_SIZE_MASK

/** The encoded size of the text, in bytes */
#define UTF8LITE_TEXT_SIZE(text) ((text)->attr & UTF8LITE_TEXT_SIZE_MASK)

/** The text attribute bits */
#define UTF8LITE_TEXT_BITS(text) ((text)->attr & ~UTF8LITE_TEXT_SIZE_MASK)

/** Indicates whether the text might contain a backslash (`\`) that should
 *  be interpreted as an escape code */
#define UTF8LITE_TEXT_HAS_ESC(text) \
	(((text)->attr & UTF8LITE_TEXT_ESC_BIT) ? 1 : 0)

/**
 * Flags for utf8lite_text_assign().
 */
enum utf8lite_text_flag {
	/** validate the input */
	UTF8LITE_TEXT_UNKNOWN = 0,

	/** do not perform any validation on the input */
	UTF8LITE_TEXT_VALID = (1 << 0),

	/** interpret backslash (`\`) as an escape */
	UTF8LITE_TEXT_UNESCAPE = (1 << 1)
};

/**
 * UTF-8 encoded text, possibly containing JSON-compatible backslash (`\`)
 * escape codes which should be interpreted as such. The client assumes
 * all responsibility for managing the memory for the underlying UTF8-data.
 */
struct utf8lite_text {
	uint8_t *ptr;	/**< pointer to valid UTF-8 data */
	size_t attr;	/**< text attributes */
};

/**
 * Assign a text value to point to data in the specified memory location
 * after validating the input data.
 *
 * \param text the text value
 * \param ptr a pointer to the underlying memory buffer
 * \param size the number of bytes in the underlying memory buffer
 * \param flags #utf8lite_text_flag bitmask specifying input type
 * \param msg an error message buffer, or NULL
 *
 * \returns 0 on success
 */
int utf8lite_text_assign(struct utf8lite_text *text,
			 const uint8_t *ptr, size_t size, int flags,
			 struct utf8lite_message *msg);

/**
 * Initialize a new text object by allocating space for and copying
 * the encoded characters from another text object.
 *
 * \param text the object to initialize
 * \param other the object to copy
 *
 * \returns 0 on success, or non-zero on memory allocation failure
 */
int utf8lite_text_init_copy(struct utf8lite_text *text,
			    const struct utf8lite_text *other);

/** Indicates whether the text definitely decodes to ASCII. For this to be true,
 *  the text must be encoded in ASCII and not have any escapes that decode to
 *  non-ASCII codepoints.
 */
int utf8lite_text_isascii(const struct utf8lite_text *text);

/**
 * Free the resources associated with a text object.
 *
 * \param text the text object
 */
void utf8lite_text_destroy(struct utf8lite_text *text);

/**
 * Compute a hash code from a text.
 *
 * \param text the text
 *
 * \returns the hash code.
 */
size_t utf8lite_text_hash(const struct utf8lite_text *text);

/**
 * Test whether two texts are equal (bitwise). Bitwise equality is more
 * stringent than decoding to the same value.
 *
 * \param text1 the first text
 * \param text2 the second text
 *
 * \returns non-zero if the tokens are equal, zero otherwise
 */
int utf8lite_text_equals(const struct utf8lite_text *text1,
			 const struct utf8lite_text *text2);

/**
 * Compare two texts.
 *
 * \param text1 the first text
 * \param text2 the second text
 *
 * \returns zero if the two encoded texts are identical; a negative value
 * 	if the first value is less than the second; a positive value
 * 	if the first value is greater than the second
 */
int utf8lite_text_compare(const struct utf8lite_text *text1,
			  const struct utf8lite_text *text2);
/**@}*/


/**
 * \defgroup textiter Text iteration
 * @{
 */

/**
 * An iterator over the decoded UTF-32 characters in a text.
 */
struct utf8lite_text_iter {
	const uint8_t *ptr;	/**< current position in the text buffer*/
	const uint8_t *end;	/**< end of the text buffer */
	size_t text_attr;	/**< text attributes */
	int32_t current;	/**< current character (UTF-32) */
};

/**
 * Initialize a text iterator to start at the beginning of a text.
 *
 * \param it the iterator
 * \param text the text
 */
void utf8lite_text_iter_make(struct utf8lite_text_iter *it,
			     const struct utf8lite_text *text);

/**
 * Advance to the next character in a text.
 *
 * \param it the text iterator
 *
 * \returns non-zero if the iterator successfully advanced; zero if
 * 	the iterator has passed the end of the text
 */
int utf8lite_text_iter_advance(struct utf8lite_text_iter *it);

/**
 * Retreat to the previous character in a text.
 *
 * \param it the text iterator
 *
 * \returns non-zero if the iterator successfully backed up; zero if
 * 	the iterator has passed the start of the text.
 */
int utf8lite_text_iter_retreat(struct utf8lite_text_iter *it);

/**
 * Reset an iterator to the start of the text.
 *
 * \param it the text iterator
 */
void utf8lite_text_iter_reset(struct utf8lite_text_iter *it);

/**
 * Skip an iterator to the end of the text.
 *
 * \param it the text iterator
 */
void utf8lite_text_iter_skip(struct utf8lite_text_iter *it);

/**@}*/

/**
 * \defgroup textmap Text normalization map
 * @{
 */

/**
 * Map descriptor. At a minimum, convert the text to
 * composed normal form (NFC). Optionally, apply compatibility maps for
 * NFKC normal and/or apply other transformations:
 *
 *  + #UTF8LITE_TEXTMAP_CASE: perform case folding, in most languages (including
 *  	English) mapping uppercase characters to their lowercase equivalents,
 *  	but also performing other normalizations like mapping the
 *  	German Eszett (&szlig;) to "ss"; see
 *  	_The Unicode Standard_ Sec. 5.18 "Case Mappings"
 *  	and the
 *  	[Case Mapping FAQ](http://unicode.org/faq/casemap_charprop.html)
 *  	for more information
 *
 *  + #UTF8LITE_TEXTMAP_COMPAT: apply all compatibility maps required for
 *  	[NFKC normal form](http://unicode.org/reports/tr15/#Norm_Forms)
 *
 *  + #UTF8LITE_TEXTMAP_QUOTE: quote fold, replace single quotes and
 *      Unicode apostrophe with ASCII apostrophe (U+0027)
 *
 *  + #UTF8LITE_TEXTMAP_RMDI: remove default ignorables (DI) like soft
 *      hyphens and zero-width spaces, anything with the
 *  	[Default_Ignorable_Code_Point=Yes]
 *  	(http://www.unicode.org/reports/tr44/#Default_Ignorable_Code_Point)
 *  	property
 */
enum utf8lite_textmap_type {
	UTF8LITE_TEXTMAP_NORMAL = 0, /**< transform to composed normal form */
	UTF8LITE_TEXTMAP_CASE   = (1 << 0), /**< perform case folding */
	UTF8LITE_TEXTMAP_COMPAT = (1 << 1), /**< apply compatibility mappings */
	UTF8LITE_TEXTMAP_QUOTE  = (1 << 2), /**< replace apostrophe with `'` */
	UTF8LITE_TEXTMAP_RMDI   = (1 << 3)  /**< remove default ignorables */
};

/**
 * Text normalization map.
 */
struct utf8lite_textmap {
	struct utf8lite_text text;/**< result of the most recent call to
				    utf8lite_textmap_set() */
	int8_t ascii_map[128];	/**< a lookup table for the mappings of ASCII
				  characters; -1 indicates deletion */
	int32_t *codes;		/**< buffer for intermediate UTF-32 decoding */
	size_t size_max;	/**< text size maximum; normalizing a larger
				 	text will force a reallocation */
	int type;		/**< the map type descriptor, a bit mask
				  of #utf8lite_textmap_type values */
	int charmap_type;	/**< the unicode map type, a bit mask of
				  #utf8lite_decomp_type and
				  #utf8lite_casefold_type values */
};

/**
 * Initialize a new text map of the specified kind.
 *
 * \param map the text map
 * \param type a bitmask of #utf8lite_textmap_type values, specifying
 * 	the map type
 *
 * \returns 0 on success
 */
int utf8lite_textmap_init(struct utf8lite_textmap *map, int type);

/**
 * Release the resources associated with a text map.
 * 
 * \param map the text map
 */
void utf8lite_textmap_destroy(struct utf8lite_textmap *map);

/**
 * Given input text, set a map to the corresponding output text.
 *
 * \param map the text map
 * \param text the text
 *
 * \returns 0 on success
 */
int utf8lite_textmap_set(struct utf8lite_textmap *map,
			 const struct utf8lite_text *text);

/**@}*/

/**
 * \defgroup graphscan Character graphemes
 * @{
 */

/**
 * Grapheme cluster.
 */
struct utf8lite_graph {
	struct utf8lite_text text;	/**< grapheme code sequence */
};

/**
 * Grapheme scanner, for iterating over the graphemes in a text. Grapheme
 * boundaries are determined according to
 * [UAX #29, Unicode Text Segmentation][uax29],
 * using the extended grapheme cluster rules.
 *
 * [uax29]: http://unicode.org/reports/tr29/
 */
struct utf8lite_graphscan {
	struct utf8lite_text_iter iter;	/**< iterator pointed at next code */
	const uint8_t *ptr;		/**< next code's start */
	int prop;			/**< next code's break property */
	struct utf8lite_graph current;	/**< current grapheme */
};

/**
 * Create a grapheme scanner over a text object.
 *
 * \param scan the scanner to initialize
 * \param text the text
 */
void utf8lite_graphscan_make(struct utf8lite_graphscan *scan,
			     const struct utf8lite_text *text);

/**
 * Advance a scanner to the next grapheme.
 *
 * \param scan the scanner
 *
 * \returns nonzero on success, zero if at the end of the text
 */
int utf8lite_graphscan_advance(struct utf8lite_graphscan *scan);

/**
 * Retreat a scanner to the previous grapheme.
 *
 * \param scan the scanner
 *
 * \returns non-zero on success, zero if at the start of the text
 */
int utf8lite_graphscan_retreat(struct utf8lite_graphscan *scan);

/**
 * Reset a scanner to the beginning of the text.
 *
 * \param scan the scanner
 */
void utf8lite_graphscan_reset(struct utf8lite_graphscan *scan);

/**
 * Skip a scanner at the end of the text.
 *
 * \param scan the scanner
 */
void utf8lite_graphscan_skip(struct utf8lite_graphscan *scan);

/**@}*/

/**
 * \defgroup render Text rendering
 * @{
 */

/**
 * Render escaping type. Specifies that certain code-points require
 * special handling.
 */
enum utf8lite_escape_type {
	UTF8LITE_ESCAPE_NONE = 0,		/**< no special escaping */
	UTF8LITE_ESCAPE_CONTROL = (1 << 0),	/**< control and other codes */
	UTF8LITE_ESCAPE_DQUOTE = (1 << 1),	/**< ASCII double quote */
	UTF8LITE_ESCAPE_SQUOTE =  (1 << 2),	/**< ASCII single quote */
	UTF8LITE_ESCAPE_EXTENDED = (1 << 3),	/**< extended-plane UTF-8 */
	UTF8LITE_ESCAPE_UTF8 = (1 << 4)		/**< non-ASCII UTF-8 */
};

/**
 * Render encoding type.
 */
enum utf8lite_encode_type {
	UTF8LITE_ENCODE_C = 0,		/**< C-compatible escapes */
	UTF8LITE_ENCODE_JSON = (1 << 5),/**< JSON-compatible escapes */
	UTF8LITE_ENCODE_EMOJIZWSP = (1 << 6),/**< put ZWSP after emoji */
	UTF8LITE_ENCODE_RMDI = (1 << 7),/**< remove default ignorables */
	UTF8LITE_ENCODE_AMBIGWIDE = (1 << 8)/**< assume that ambiguous-width
					       characters are wide */
};

/**
 * Get the width of a grapheme under the specified render settings. If
 * the grapheme contains a non-escaped control character, report the width
 * as -1.
 *
 * \param g the grapheme
 * \param flags a bitmask of #utf8lite_escape_type and #utf8lite_encode_type
 * 			values specifying the encoding settings
 * \param widthptr if non-NULL, a pointer to store the width on exit
 * 	(0 if the grapheme is empty or a non-escaped control)
 *
 * \returns 0 on success
 */
int utf8lite_graph_measure(const struct utf8lite_graph *g, int flags,
			   int *widthptr);

/**
 * Renderer, for printing objects as strings.
 */
struct utf8lite_render {
	char *string;		/**< the rendered string (null terminated) */
	int length;		/**< the length of the rendered string, not
				  including the null terminator */
	int length_max;		/**< the maximum capacity of the rendered
				  string before requiring reallocation, not
				  including the null terminator */
	int flags;		/**< the flags, a bitmask of
				  #utf8lite_escape_type and
				  #utf8lite_encode_type values,
				  specifying escaping behavior */

	const char *tab;	/**< the tab string, for indenting */
	int tab_length;		/**< the length in bytes of the tab string,
				  not including the null terminator */

	const char *newline;	/**< the newline string, for advancing
				  to the next line */
	int newline_length;	/**< the length in bytes of the newline string,
				  not including the null terminator */

	const char *style_open;	/**< the escape style graphic parameters,
				  for styling backslash escapes */
	const char *style_close;/**< the escape style graphic parameters,
				  for restoring state after styling a
				  backslash escapes */
	int style_open_length;	/**< length in bytes of the style_open string,
				  not including the null terminator */
	int style_close_length;	/**< length in bytes of the style_close string,
				  not including the null terminator */

	int indent;		/**< the current indent level */
	int needs_indent;	/**< whether to indent before the next
				  character */
	int error;		/**< the code for the last error that
				  occurred, or zero if none */
};

/**
 * Initialize a new render object.
 *
 * \param r the render object
 * \param flags a bitmask of #utf8lite_escape_type and #utf8lite_encode_type
 * 	values specifying escaping behavior
 *
 * \returns 0 on success
 */
int utf8lite_render_init(struct utf8lite_render *r, int flags);

/**
 * Release a render object's resources.
 *
 * \param r the render object
 */
void utf8lite_render_destroy(struct utf8lite_render *r);

/**
 * Reset the render object to the empty string and set the indent level to 0.
 * Leave the escape flags, the tab, and the newline string at their current
 * values.
 *
 * \param r the render object
 */
void utf8lite_render_clear(struct utf8lite_render *r);

/**
 * Set the escaping behavior.
 *
 * \param r the render object
 * \param flags a bit mask of #utf8lite_escape_type values
 *
 * \returns 0 on success
 */
int utf8lite_render_set_flags(struct utf8lite_render *r, int flags);

/**
 * Set the tab string. The client must not free the passed-in tab
 * string until either the render object is destroyed or a new tab
 * string gets set.
 *
 * \param r the render object
 * \param tab the tab string (null terminated)
 *
 * \returns 0 on success
 */
int utf8lite_render_set_tab(struct utf8lite_render *r, const char *tab);

/**
 * Set the new line string.  The client must not free the passed-in newline
 * string until either the render object is destroyed or a new newline
 * string gets set.
 *
 * \param r the render object
 * \param newline the newline string (null terminated)
 *
 * \returns 0 on success
 */
int utf8lite_render_set_newline(struct utf8lite_render *r, const char *newline);

/**
 * Set the escape style strings. The client must not free the passed
 * in strings until the render object is destroyed or new style
 * strings get set.
 *
 * \param r the render object
 * \param open the string to render before a backslash escape.
 * \param close the string to render after a backslash escape.
 *
 * \returns 0 on success
 */
int utf8lite_render_set_style(struct utf8lite_render *r,
			      const char *open, const char *close);

/**
 * Increase or decrease the indent level.
 *
 * \param r the render object
 * \param nlevel the number of levels add or subtract to the indent
 *
 * \returns 0 on success
 */
int utf8lite_render_indent(struct utf8lite_render *r, int nlevel);

/**
 * Add new lines.
 *
 * \param r the render object
 * \param nline the number of new lines to add
 *
 * \returns 0 on success
 */
int utf8lite_render_newlines(struct utf8lite_render *r, int nline);

/**
 * Render a character grapheme. If any render escape flags are set, filter
 * the grapheme through the appropriate escaping and encoding.
 *
 * \param r the render object
 * \param g the grapheme
 *
 * \returns 0 on success
 */
int utf8lite_render_graph(struct utf8lite_render *r,
			  const struct utf8lite_graph *g);

/**
 * Render a single character, treating it as a grapheme cluster. If any
 * render escape flags are set, filter the character through the
 * appropriate escaping and encoding.
 *
 * \param r the render object
 * \param ch the character
 *
 * \returns 0 on success
 */
int utf8lite_render_char(struct utf8lite_render *r, int32_t ch);

/**
 * Render multiple copies of a character, treating each as a grapheme
 * cluster.
 *
 * \param r the render object
 * \parma ch the character
 * \param nchar the number of copies to render
 *
 * \returns 0 on success
 */
int utf8lite_render_chars(struct utf8lite_render *r, int32_t ch, int nchar);

/**
 * Render a string. If any render escape flags are set, filter
 * all character graphemes through the appropriate escaping.
 *
 * \param r the render object
 * \param str the string, valid UTF-8
 *
 * \returns 0 on success
 */
int utf8lite_render_string(struct utf8lite_render *r, const char *str);

/**
 * Render formatted text. If any render escape flags are set, filter
 * all character graphemes through the appropriate escaping.
 *
 * \param r the render object
 * \param format the format string
 */
int utf8lite_render_printf(struct utf8lite_render *r, const char *format, ...)
#if defined(_WIN32) || defined(_WIN64)
	;
#else
	__attribute__ ((format (printf, 2, 3)));
#endif

/**
 * Render a text object. If any render escape flags are set, filter
 * all character graphemes through the appropriate escaping.
 *
 * \param r the render object
 * \param text the text object
 *
 * \returns 0 on success
 */
int utf8lite_render_text(struct utf8lite_render *r,
			 const struct utf8lite_text *text);

/**
 * Append a sequence of raw bytes to the render buffer. Ignore any special
 * handling specified by the render flags.
 *
 * \param r the render object
 * \param bytes the byte array
 * \param size the number of bytes
 *
 * \returns 0 on success.
 */
int utf8lite_render_raw(struct utf8lite_render *r, const char *bytes,
			size_t size);

/**@}*/

#endif /* UTF8LITE_H */
