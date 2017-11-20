utf8 1.1.0
==========

### NEW FEATURES

  * Added `output_ansi()` and `output_utf8()` functions to test for
    output capabilities.

### MINOR IMPROVEMENTS

  * Add `utf8` argument to `utf8_encode()`, `utf8_format()`, `utf8_print()`,
    and `utf8_width()` for precise control over assumed output capabilities;
    defaults to the result of `output_utf8()`.

  * Add ability to style backslash escapes with the `escapes` arguments
    to `utf8_encode()` and `utf8_print()`. Switch from "faint" styling
    to no styling by default.

  * Slightly reword error messages for `as_utf8()`.

  * Fix (spurious) `rchk` warnings.

### BUG FIXES

  * Fix bug in `utf8_width()` determining width of non-ASCII strings
    when `LC_CTYPE=C`.

### DEPRECATED AND DEFUNCT

  * No longer export the C version of `as_utf8()` (the R version is still
    present).


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
