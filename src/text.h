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

#ifndef UTF8LITE_TEXT_H
#define UTF8LITE_TEXT_H

/**
 * \file text.h
 *
 * UTF-8 encoded text, optionally with JSON-compatible backslash (`\`) escapes.
 */

#include <limits.h>
#include <stddef.h>
#include <stdint.h>


/**
 * An iterator over the decoded UTF-32 characters in a text.
 */
struct utf8lite_text_iter {
	const uint8_t *ptr;	/**< current position in the text buffer*/
	const uint8_t *end;	/**< end of the text buffer */
	size_t text_attr;	/**< text attributes */
	uint32_t current;	/**< current character (UTF-32) */
	size_t attr;		/**< current character attributes */
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
 * Check whether a text iterator can advance.
 *
 * \param it the text iterator
 *
 * \returns non-zero if the iterator can advance, zero otherwise
 */
int utf8lite_text_iter_can_advance(const struct utf8lite_text_iter *it);

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
 * Check whether a text iterator can retreat.
 *
 * \param it the text iterator
 *
 * \returns non-zero if the iterator can retreat, zero otherwise
 */
int utf8lite_text_iter_can_retreat(const struct utf8lite_text_iter *it);

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

#endif /* UTF8LITE_TEXT_H */
