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


as_character_scalar <- function(name, value, utf8 = TRUE)
{
    if (is.null(value)) {
        return(NULL)
    }
    value <- as_character_vector(name, value, utf8)
    if (length(value) != 1) {
        stop(sprintf("'%s' must be a scalar character string", name))
    }
    value
}


as_character_vector <- function(name, value, utf8 = TRUE)
{
    if (!(is.null(value) || is.character(value) || is.factor(value)
          || all(is.na(value)))) {
        stop(sprintf("'%s' must be text, a character vector, or NULL", name))
    }
    if (is.null(value)) {
        return(NULL)
    }
    value <- as.character(value)
    if (utf8) {
        value <- as_utf8(value)
    }
    value
}


as_enum <- function(name, value, choices)
{
    if (!(is.character(value) && length(value) == 1 && !is.na(value))) {
        stop(sprintf("'%s' must be a character string", name))
    }

    i <- pmatch(value, choices, nomatch = 0)
    if (all(i == 0)) {
        stop(sprintf("'%s' must be one of the following: ", name),
             paste(dQuote(choices), collapse = ", "))
    }
    i <- i[i > 0]
    choices[[i]]
}


as_integer_scalar <- function(name, value, nonnegative = FALSE)
{
    if (is.null(value)) {
        return(NULL)
    }
    value <- as_integer_vector(name, value, nonnegative)
    if (length(value) != 1) {
        stop(sprintf("'%s' must have length 1", name))
    }
    value
}


as_integer_vector <- function(name, value, nonnegative = FALSE)
{
    if (is.null(value)) {
        return(NULL)
    }

    if (!(is.numeric(value) || all(is.na(value)))) {
        stop(sprintf("'%s' must be integer-valued", name))
    }

    value <- as.integer(value)
    if (nonnegative && any(!is.na(value) & value < 0)) {
        stop(sprintf("'%s' must be non-negative", name))
    }

    value
}


as_na_print <- function(name, value)
{
    if (is.null(value)) {
        return(NULL)
    }
    value <- as_character_scalar(name, value)
    if (is.na(value)) {
        stop(sprintf("'%s' cannot be NA", name))
    }
    value
}


as_nonnegative <- function(name, value)
{
    if (is.null(value)) {
        return(NULL)
    }
    value <- as_integer_scalar(name, value, nonnegative = TRUE)
    if (is.na(value)) {
        stop(sprintf("'%s' cannot be NA", name))
    }
    value
}


as_option <- function(name, value)
{
    if (is.null(value)) {
        return(FALSE)
    }

    if (!(length(value) == 1 && is.logical(value) && !is.na(value))) {
        stop(sprintf("'%s' must be TRUE or FALSE", name))
    }
    as.logical(value)
}

as_chars <- as_nonnegative

as_justify <- function(name, value)
{
    as_enum(name, value, c("left", "right", "centre", "none"))
}

as_max_print <- as_nonnegative

as_print_gap <- function(name, value)
{
    value <- as_nonnegative(name, value)
    if (!is.null(value) && value > 1024) {
        stop(sprintf("'%s' must be less than or equal to 1024", name))
    }
    value
}

as_style <- function(name, value)
{
    value <- as_character_scalar(name, value)
    if (is.null(value) || is.na(value)) {
        return(NULL)
    }
    if (!grepl("^[0-9;]*$", value)) {
        stop(sprintf("'%s' must be a valid ANSI SGR parameter string", name))
    }
    if (nchar(value) >= 128) {
        stop(sprintf("'%s' must have length below 128 characters", name))
    }
    value
}

as_output_utf8 <- function(name, value)
{
    if (is.null(value)) {
        return(output_utf8())
    }
    as_option(name, value)
}
