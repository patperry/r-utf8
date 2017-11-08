utf8 1.0.0.9000
===============

### MINOR IMPROVEMENTS

  * Fix (spurious) `rchk` warnings.


utf8 1.0.0 (2017-11-06)
=======================

### NEW FEATURES

  * Split off functions `as_utf8()`, `utf8_valid()`, `utf8_normalize()`,
    `utf8_encode()`, `utf8_format()`, `utf8_print()`, and `utf8_width()`
    from [corpus][corpus] package.

  * Added special handling for Unicode grapheme clusters in formatting
    and width measurement functions.

  * Added ANSI styling to escape sequences.

  * Added ability to style row and column names in `utf8_print()`.


[corpus]: http://corpustext.com/ "corpus: Text Corpus Analysis"
