\name{as_utf8}
\title{UTF-8 Character Encoding}
\alias{as_utf8}
\alias{utf8_valid}
\description{
    UTF-8 text encoding and validation.
}
\usage{
as_utf8(x, normalize = FALSE)

utf8_valid(x)
}
\arguments{
    \item{x}{character object.}

    \item{normalize}{a logical value indicating whether to convert to
        Unicode composed normal form (NFC).}
}
\details{
    \code{as_utf8} converts a character object from its declared encoding
    to a valid UTF-8 character object, or throws an error if no conversion
    is possible. If \code{normalize = TRUE}, then the text gets
    transformed to Unicode composed normal form (NFC) after conversion
    to UTF-8.

    \code{utf8_valid} tests whether the elements of a character object
    can be translated to valid UTF-8 strings.
}
\value{
    For \code{as_utf8}, the result is a character object with the same
    attributes as \code{x} but with \code{Encoding} set to \code{"UTF-8"}.

    For \code{utf8_valid} a logical object with the same \code{names},
    \code{dim}, and \code{dimnames} as \code{x}.
}
\seealso{
    \code{\link{utf8_normalize}}, \code{\link{iconv}}.
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
}
