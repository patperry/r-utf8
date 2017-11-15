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


# converts a character vector from its declared encoding to UTF-8
as_utf8 <- function(x, normalize = FALSE)
{
    ans <- .Call(rutf8_as_utf8, x)
    if (normalize) {
        ans <- utf8_normalize(ans)
    }
    ans
}

# test whether the elements can be converted to valid UTF-8
utf8_valid <- function(x)
{
    .Call(rutf8_utf8_valid, x)
}

# gets the width; NA for invalid or nonprintable sequences
utf8_width <- function(x, encode = TRUE, quote = FALSE, utf8 = NULL)
{
    with_rethrow({
        encode <- as_option("encode", encode)
        quote <- as_option("quote", quote)
        utf8 <- as_output_utf8("utf8", utf8)
    })
    .Call(rutf8_utf8_width, x, encode, quote, utf8)
}

utf8_normalize <- function(x, map_case = FALSE, map_compat = FALSE,
                           map_quote = FALSE, remove_ignorable = FALSE)
{
    with_rethrow({
        x <- as_utf8(x, normalize = FALSE)
        map_case <- as_option("map_case", map_case)
        map_compat <- as_option("map_compat", map_compat)
        map_quote <- as_option("map_quote", map_quote)
        remove_ignorable <- as_option("remove_ignorable", remove_ignorable)
    })

    .Call(rutf8_utf8_normalize, x, map_case, map_compat, map_quote,
          remove_ignorable)
}
