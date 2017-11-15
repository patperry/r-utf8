\name{output_utf8}
\title{Output Capabilities}
\alias{output_ansi}
\alias{output_utf8}
\description{
    Test whether the output connection has ANSI style escape support
    or UTF-8 support.
}
\usage{
output_ansi()

output_utf8()
}
\details{
\code{output_ansi} tests whether the output connection supports ANSI
style escapes. This is \code{TRUE} if the connection is a terminal
and not the Windows GUI. Otherwise, it is true if running in
RStudio 1.1 or later with ANSI escapes enabled, provided \code{stdout()}
has not been redirected to another connection by \code{sink()}.

\code{output_utf8} tests whether the output connection supports UTF-8. For
most platforms \code{l10n_info()$`UTF-8`} gives this information, but this
does not give an accurate result for Windows GUIs. To work around this,
we proceed as follows:
\itemize{
  \item if the character locale (\code{LC_CTYPE}) is \code{"C"}, then
      the result is \code{FALSE};

  \item otherwise, if \code{l10n_info()$`UTF-8`} is \code{TRUE},
      then the result is \code{TRUE};

  \item if running on Windows in RGui or RStudio, then the result
    is \code{TRUE} unless \code{stdout()} has been redirected;
    
  \item in all other cases the result is \code{FALSE}.
}
}
\value{
A logical scalar indicating whether the output connection supports the
given capability.
}
\seealso{
    \code{\link{.Platform}}, \code{\link{isatty}}, \code{\link{l10n_info}},
    \code{\link{Sys.getlocale}}
}
\examples{
# test whether ANSI style escapes or UTF-8 output are supported
cat("ANSI:", output_ansi(), "\n")
cat("UTF8:", output_utf8(), "\n")

# switch to C locale
Sys.setlocale("LC_CTYPE", "C")
cat("ANSI:", output_ansi(), "\n")
cat("UTF8:", output_utf8(), "\n")

# switch to native locale
Sys.setlocale("LC_CTYPE", "")

tmp <- tempfile()
sink(tmp) # redirect output to a file
cat("ANSI:", output_ansi(), "\n")
cat("UTF8:", output_utf8(), "\n")
sink() # restore stdout

# inspect the output
readLines(tmp)
}
