#' The utf8 Package
#'
#' UTF-8 Text Processing
#'
#' Functions for manipulating and printing UTF-8 encoded text:
#'
#' \itemize{ \item [as_utf8()] attempts to convert character data to
#' UTF-8, throwing an error if the data is invalid;
#'
#' \item [utf8_valid()] tests whether character data is valid
#' according to its declared encoding;
#'
#' \item [utf8_normalize()] converts text to Unicode composed normal
#' form (NFC), optionally applying case-folding and compatibility maps;
#'
#' \item [utf8_encode()] encodes a character string, escaping all
#' control characters, so that it can be safely printed to the screen;
#'
#' \item [utf8_format()] formats a character vector by truncating to
#' a specified character width limit or by left, right, or center justifying;
#'
#' \item [utf8_print()] prints UTF-8 character data to the screen;
#'
#' \item [utf8_width()] measures the display width of UTF-8 character
#' strings (many emoji and East Asian characters are twice as wide as other
#' characters);
#'
#' \item [output_ansi()] and [output_utf8()] test for the
#' output connections capabilities. }
#'
#' For a complete list of functions, use `library(help = "utf8")`.
#'
#' @useDynLib utf8, .registration = TRUE
"_PACKAGE"
