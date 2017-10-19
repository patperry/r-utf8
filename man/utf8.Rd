\name{utf8}
\title{UTF-8 Character Handling}
\alias{utf8}
\alias{as_utf8}
\alias{utf8_encode}
\alias{utf8_format}
\alias{utf8_normalize}
\alias{utf8_print}
\alias{utf8_valid}
\alias{utf8_width}
\description{
    UTF-8 text validation, normalization, formatting, and printing.
}
\usage{
as_utf8(x, normalize = FALSE)

utf8_valid(x)

utf8_normalize(x, map_case = FALSE, map_compat = FALSE,
               map_quote = FALSE, remove_ignorable = FALSE)

utf8_encode(x, display = FALSE)

utf8_format(x, trim = FALSE, chars = NULL, justify = "left",
            width = NULL, na.encode = TRUE, quote = FALSE,
            na.print = NULL, print.gap = NULL, ...)

utf8_print(x, chars = NULL, quote = TRUE, na.print = NULL,
           print.gap = NULL, right = FALSE, max = NULL,
           display = TRUE, ...)

utf8_width(x, encode = TRUE, quote = FALSE)
}
\arguments{
    \item{x}{character object.}

    \item{normalize}{a logical value indicating whether to convert to
        Unicode composed normal form (NFC).}

    \item{map_case}{a logical value indicating whether to apply Unicode
        case mapping to the text. For most languages, this transformation
        changes uppercase characters to their lowercase equivalents.}

    \item{map_compat}{a logical value indicating whether to apply
        Unicode compatibility mappings to the characters, those required
        for NFKC and NFKD normal forms.}

    \item{map_quote}{a logical value indicating whether to replace curly
        single quotes and Unicode apostrophe characters with ASCII
        apostrophe (U+0027).}

    \item{remove_ignorable}{a logical value indicating whether to remove
        Unicode "default ignorable" characters like zero-width spaces
        and soft hyphens.}

    \item{display}{logical scalar indicating whether to optimize the
        encoding for display, not byte-for-byte data transmission.}

    \item{trim}{logical scalar indicating whether to suppress
        padding spaces around elements.}

    \item{chars}{integer scalar indicating the maximum number of
        character units to display.  Wide characters like emoji take
        two character units; combining marks and default ignorables
        take none. Longer strings get truncated and suffixed or prefixed
        with an ellipsis (\code{"..."} in C locale, \code{"\u2026"} in
        others). Set to \code{NULL} to limit output to the line width
        as determined by \code{getOption("width")}.}

    \item{justify}{justification; one of \code{"left"}, \code{"right"},
       \code{"centre"}, or \code{"none"}. Can be abbreviated.}

    \item{width}{the minimum field width; set to \code{NULL} or
        \code{0} for no restriction.}

    \item{na.encode}{logical scalar indicating whether to encode
        \code{NA} values as character strings.}

    \item{quote}{logical scalar indicating whether to put surrounding
        double-quotes (\code{'"'}) around character strings and escape
        internal double-quotes.}

    \item{na.print}{character string (or \code{NULL}) indicating
        the encoding for \code{NA} values. Ignored when
        \code{na.encode} is \code{FALSE}.}

    \item{print.gap}{non-negative integer (or \code{NULL}) giving the
        number of spaces in gaps between columns; set to \code{NULL}
        or \code{1} for a single space.}

    \item{right}{logical scalar indicating whether to right-justify
        character strings.}

    \item{max}{non-negative integer (or \code{NULL}) indicating the
        maximum number of elements to print; set to
        \code{getOption("max.print")} if argument is \code{NULL}.}

    \item{encode}{whether to encode the object before measuring its
        width.}

    \item{...}{further arguments passed from other methods. Ignored.}
}
\details{
    \code{as_utf8} converts a character object from its declared encoding
    to a valid UTF-8 character object, or throws an error if no conversion
    is possible. If \code{normalize = TRUE}, then the text gets
    transformed to Unicode composed normal form (NFC) after conversion
    to UTF-8.

    \code{utf8_valid} tests whether the elements of a character object
    can be translated to valid UTF-8 strings.

    \code{utf8_normalize} converts the elements of a character object to
    Unicode normalized composed form (NFC) while applying the character
    maps specified by the \code{map_case}, \code{map_compat},
    \code{map_quote}, and \code{remove_ignorable} arguments.

    \code{utf8_encode} encodes a character object for printing on a UTF-8
    device by escaping controls characters and other non-printable
    characters. When \code{display = TRUE}, the function optimizes the
    encoding for display by removing default ignorable characters (soft
    hyphens, zero-width spaces, etc.) and placing zero-width spaces after
    wide emoji. When \code{LC_CTYPE = "C"}, the function escapes all
    non-ASCII characters and gives the same results on all platforms.

    \code{utf8_format} formats a character object for printing, optionally
    truncating long character strings.

    \code{utf8_print} prints a character object after formatting it with
    \code{utf8_format}.

    \code{utf8_width} returns the printed widths of the elements of
    a character object on a UTF-8 device or, when \code{LC_CTYPE = "C"},
    on an ASCII device. If the string is not printable on the device,
    for example if it contains a control code like \code{"\n"}, then
    the result is \code{NA}. If \code{encode = TRUE}, the default,
    then the function returns the widths of the encoded elements
    (via \code{utf8_encode}); otherwise, the function returns the
    widths of the original elements. If \code{quote = TRUE}, then
    \code{utf8_width} returns the widths of the quoted values
    (enclosing the argument in double quotes, and replacing internal
    quotes with \code{\"}).
}
\value{
    For \code{as_utf8}, \code{utf8_normalize}, and \code{utf8_encode},
    the result is a character object with the same attributes as
    \code{x} but with \code{Encoding} set to \code{"UTF-8"}.

    For \code{utf8_print}, the function returns \code{x} invisibly.

    For \code{utf8_valid} or \code{utf8_width}, a logical or integer
    object, respectively, with the same \code{names}, \code{dim}, and
    \code{dimnames} as \code{x}.
}
\seealso{
    \code{\link{iconv}}.
}
\examples{
# the second element is encoded in latin-1, but declared as UTF-8
x <- c("fa\u00E7ile", "fa\xE7ile", "fa\xC3\xA7ile")
Encoding(x) <- c("UTF-8", "UTF-8", "bytes")

# attempt to convert to UTF-8 (fails)
\dontrun{as_utf8(x)}

y <- x
Encoding(y[2]) <- "latin1" # mark the correct encoding
as_utf8(y) # succeeds

# test for valid UTF-8
utf8_valid(x)

# normalize text
angstrom <- c("\u00c5", "\u0041\u030a", "\u212b")
utf8_normalize(angstrom) == "\u00c5"

# encoding
utf8_encode(x)

# formatting
utf8_format(x, chars = 3)
utf8_format(x, chars = 3, justify = "centre", width = 10)
utf8_format(x, chars = 3, justify = "right")

# get widths
utf8_width(x)
utf8_width(x, encode = FALSE)
utf8_width('"')
utf8_width('"', quote = TRUE)

# printing (assumes that output is capable of displaying Unicode 10.0.0)
print(intToUtf8(0x1F600 + 0:79)) # with default R print function
utf8_print(intToUtf8(0x1F600 + 0:79)) # with utf8_print, truncates line
utf8_print(intToUtf8(0x1F600 + 0:79), chars = 1000) # higher character limit

# in C locale, output ASCII (same results on all platforms)
oldlocale <- Sys.getlocale("LC_CTYPE")
invisible(Sys.setlocale("LC_CTYPE", "C")) # switch to C locale
utf8_print(intToUtf8(0x1F600 + 0:79))
invisible(Sys.setlocale("LC_CTYPE", oldlocale)) # switch back to old locale
}
