utf8
====

[![Build Status (Linux)][travis-badge]][travis]
[![Build Status (Windows)][appveyor-badge]][appveyor]
[![Coverage Status][codecov-badge]][codecov]
[![CRAN Status][cran-badge]][cran]
[![License][apache-badge]][apache]
[![CRAN RStudio Mirror Downloads][cranlogs-badge]][cran]


*utf8* is an R package for manipulating and printing UTF-8 text that fixes
[multiple][windows-enc2utf8] [bugs][emoji-print] in R's UTF-8 handling.


Installation
------------

### Stable version

*utf8* is [available on CRAN][cran]. To install the latest released version,
run the following command in R:

    ### install.packages("utf8") # not available yet, actually


### Development version

To install the latest development version, run the following:

    tmp <- tempfile()
    system2("git", c("clone", "--recursive",
                     shQuote("https://github.com/patperry/r-utf8.git"), shQuote(tmp)))
    devtools::install(tmp)

Note that *utf8* uses a git submodule, so you cannot use
`devtools::install_github`.


Usage
-----



Citation
--------

Cite *utf8* with the following BibTeX entry:

    @Manual{,
        title = {utf8: UTF-8 Text Processing},
        author = {Patrick O. Perry},
        year = {2017},
        note = {R package version 0.1.0},
    }


Contributing
------------

The project maintainer welcomes contributions in the form of feature requests,
bug reports, comments, unit tests, vignettes, or other code.  If you'd like to
contribute, either

 + fork the repository and submit a pull request (note the nonstandard
   instructions for [building from source][building]);

 + [file an issue][issues];

 + or contact the maintainer via e-mail.

This project is released with a [Contributor Code of Conduct][conduct],
and if you choose to contribute, you must adhere to its terms.


[apache]: https://www.apache.org/licenses/LICENSE-2.0.html "Apache License, Version 2.0"
[apache-badge]: https://img.shields.io/badge/License-Apache%202.0-blue.svg "Apache License, Version 2.0"
[appveyor]: https://ci.appveyor.com/project/patperry/r-utf8/branch/master "Continuous Integration (Windows)"
[appveyor-badge]: https://ci.appveyor.com/api/projects/status/github/patperry/r-utf8?branch=master&svg=true "Continuous Inegration (Windows)"
[bench-term-matrix]: https://github.com/patperry/bench-term-matrix#readme "Term Matrix Benchmark"
[bench-ndjson]: https://github.com/jeroen/ndjson-benchmark#readme "NDJSON Benchmark"
[building]: #development-version "Building from Source"
[casefold]: https://www.w3.org/International/wiki/Case_folding "Case Folding"
[cc]: https://en.wikipedia.org/wiki/C0_and_C1_control_codes "C0 and C1 Control Codes"
[codecov]: https://codecov.io/github/patperry/r-utf8?branch=master "Code Coverage"
[codecov-badge]: https://codecov.io/github/patperry/r-utf8/coverage.svg?branch=master "Code Coverage"
[conduct]: https://github.com/patperry/r-utf8/blob/master/CONDUCT.md "Contributor Code of Conduct"
[utf8]: https://github.com/patperry/utf8 "utf8lite C Library"
[cran]: https://cran.r-project.org/package=utf8 "CRAN Page"
[cran-badge]: http://www.r-pkg.org/badges/version/utf8 "CRAN Page"
[cranlogs-badge]: http://cranlogs.r-pkg.org/badges/utf8 "CRAN Downloads"
[emoji-print]: https://twitter.com/ptrckprry/status/887732831161425920 "MacOS Emoji Printing"
[issues]: https://github.com/patperry/r-utf8/issues "Issues"
[ndjson]: http://ndjson.org/ "Newline-Delimited JSON"
[nfc]: http://unicode.org/reports/tr15/ "Unicode Normalization Forms"
[quanteda]: http://quanteda.io/ "Quanteda"
[sentbreak]: http://unicode.org/reports/tr29/#Sentence_Boundaries "Unicode Text Segmentation, Sentence Boundaries"
[stringr]: http://stringr.tidyverse.org/ "Stringr"
[tidytext]: http://juliasilge.github.io/tidytext/ "Tidytext"
[travis]: https://travis-ci.org/patperry/r-utf8 "Continuous Integration (Linux)"
[travis-badge]: https://api.travis-ci.org/patperry/r-utf8.svg?branch=master "Continuous Integration (Linux)"
[windows-enc2utf8]: https://twitter.com/ptrckprry/status/901494853758054401 "Windows enc2utf8 Bug"
