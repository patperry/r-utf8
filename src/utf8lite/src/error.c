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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "utf8lite.h"


void utf8lite_message_clear(struct utf8lite_message *msg)
{
	if (msg) {
		msg->string[0] = '\0';
	}
}


void utf8lite_message_set(struct utf8lite_message *msg,
			  const char *fmt, ...)
{
	va_list ap;

	if (msg) {
		va_start(ap, fmt);
		vsnprintf(msg->string, sizeof(msg->string), fmt, ap);
		va_end(ap);
	}
}


void utf8lite_message_append(struct utf8lite_message *msg,
			     const char *fmt, ...)
{
	size_t n, nmax;
	va_list ap;
	       
	if (msg) {
		nmax = sizeof(msg->string);
		n = strlen(msg->string);
		assert(n <= nmax);

		va_start(ap, fmt);
		vsnprintf(msg->string + n, nmax - n, fmt, ap);
		va_end(ap);
	}
}
