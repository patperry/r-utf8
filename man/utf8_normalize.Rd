\name{utf8_normalize}
\title{Text Normalization}
\alias{utf8_normalize}
\description{
    Transform text to normalized form, optionally mapping to lowercase
    and applying compatibility maps.
}
\usage{
utf8_normalize(x, map_case = FALSE, map_compat = FALSE,
               map_quote = FALSE, remove_ignorable = FALSE)
}
\arguments{
    \item{x}{character object.}

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
    \code{utf8_normalize} converts the elements of a character object to
    Unicode normalized composed form (NFC) while applying the character
    maps specified by the \code{map_case}, \code{map_compat},
    \code{map_quote}, and \code{remove_ignorable} arguments.
}
\value{
    The result is a character object with the same attributes as \code{x}
    but with \code{Encoding} set to \code{"UTF-8"}.
}
\seealso{
    \code{\link{as_utf8}}.
}
\examples{
angstrom <- c("\u00c5", "\u0041\u030a", "\u212b")
utf8_normalize(angstrom) == "\u00c5"
}
