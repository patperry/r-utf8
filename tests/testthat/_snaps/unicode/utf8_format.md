# 'format' can handle long text in Unicode locale

    Code
      raw <- c(NA, "", "a", "ab", "foo", "food", "short text", "性", "性性", "性?")
      Encoding(raw) <- "UTF-8"
      body(mapper)
    Output
      x
    Code
      mapper(utf8_format(raw, chars = 2, justify = "none", na.print = "NA"))
    Output
       [1] "NA"  ""    "a"   "ab"  "fo…" "fo…" "sh…" "性"  "性…" "性…"
    Code
      mapper(utf8_format(raw, chars = 2, justify = "left", na.print = "NA"))
    Output
       [1] "NA " "   " "a  " "ab " "fo…" "fo…" "sh…" "性 " "性…" "性…"
    Code
      mapper(utf8_format(raw, chars = 2, justify = "centre", na.print = "NA"))
    Output
       [1] "NA " "   " " a " "ab " "fo…" "fo…" "sh…" "性 " "性…" "性…"
    Code
      mapper(utf8_format(raw, chars = 2, justify = "right", na.print = "NA"))
    Output
       [1] " NA" "   " "  a" " ab" "…oo" "…od" "…xt" " 性" "…性" " …?"

