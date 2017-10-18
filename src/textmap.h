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

#ifndef UTF8LITE_TEXTMAP_H
#define UTF8LITE_TEXTMAP_H

/**
 * \file textmap.h
 *
 * Text normalization map.
 */

#include <stddef.h>
#include <stdint.h>

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
enum utf8lite_textmap_kind {
	UTF8LITE_TEXTMAP_NORMAL = 0, /**< transform to composed normal form */
	UTF8LITE_TEXTMAP_CASE   = (1 << 0), /**< perform case folding */
	UTF8LITE_TEXTMAP_COMPAT = (1 << 1), /**< apply compatibility mappings */
	UTF8LITE_TEXTMAP_QUOTE  = (1 << 2), /**< replace apostrophe with `'` */
	UTF8LITE_TEXTMAP_RMDI   = (1 << 3)  /**< remove default ignorables */
};

/**
 * Type map, for normalizing tokens to types.
 */
struct utf8lite_textmap {
	struct corpus_text type;/**< type of the token given to the most
				  recent typemap_set() call */
	int8_t ascii_map[128];	/**< a lookup table for the mappings of ASCII
				  characters; -1 indicates deletion */
	uint32_t *codes;	/**< buffer for intermediate UTF-32 decoding */
	size_t size_max;	/**< token size maximum; normalizing a larger
				 	token will force a reallocation */
	int kind;		/**< the type map kind descriptor, a bit mask
				  of #corpus_type_kind values */
	int charmap_type;	/**< the unicode map type, a bit mask of
				  #corpus_udecomp_type and
				  #corpus_ucasefold_type values */
};

/**
 * Initialize a new text map of the specified kind.
 *
 * \param map the text map
 * \param kind a bitmask of #utf8lite_textmap_kind values, specifying
 * 	the map kind
 *
 * \returns 0 on success
 */
int utf8lite_textmap_init(struct utf8lite_textmap *map, int kind);

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

#endif /* UTF8LITE_TEXTMAP_H */
