#  Copyright 2017 Patrick O. Perry.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

utf8_encode <- function(x, width = 0L, quote = FALSE, justify = "left",
                        escapes = NULL, display = FALSE, utf8 = NULL)
{
    if (is.null(x)) {
        return(NULL)
    }

    if (!is.character(x)) {
        stop("argument is not a character object")
    }

    with_rethrow({
        width <- as_integer_scalar("width", width)
        quote <- as_option("quote", quote)
        justify <- as_justify("justify", justify)
        escapes <- as_style("escapes", escapes)
        display <- as_option("display", display)
        utf8 <- as_output_utf8("utf8", utf8)
    })

    .Call(rutf8_utf8_encode, x, width, quote, justify, escapes, display, utf8)
}
