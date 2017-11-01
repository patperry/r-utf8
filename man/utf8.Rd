\name{utf8}
\title{UTF-8 Character Handling}
\alias{utf8}
\alias{as_utf8}
\alias{utf8_normalize}
\alias{utf8_valid}
\description{
    UTF-8 text validation and normalization.
}
\usage{
as_utf8(x, normalize = FALSE)

utf8_valid(x)

utf8_normalize(x, map_case = FALSE, map_compat = FALSE,
               map_quote = FALSE, remove_ignorable = FALSE)
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
}
\value{
    For \code{as_utf8} and \code{utf8_normalize}, the result is a
    character object with the same attributes as \code{x} but with
    \code{Encoding} set to \code{"UTF-8"}.

    For \code{utf8_valid} a logical object with the same \code{names},
    \code{dim}, and \code{dimnames} as \code{x}.
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
}
