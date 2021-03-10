# 'format' can handle long text in Unicode locale: UTF-8 is TRUE

    Code
      raw <- c(NA, "", "a", "ab", "foo", "food", "short text", "性", "性性", "性?")
      Encoding(raw) <- "UTF-8"
      utf8_format(raw, chars = 2, justify = "none", na.print = "NA")
    Output
       [1] "NA"  ""    "a"   "ab"  "fo…" "fo…" "sh…" "性"  "性…" "性…"
    Code
      utf8_format(raw, chars = 2, justify = "left", na.print = "NA")
    Output
       [1] "NA " "   " "a  " "ab " "fo…" "fo…" "sh…" "性 " "性…" "性…"
    Code
      utf8_format(raw, chars = 2, justify = "centre", na.print = "NA")
    Output
       [1] "NA " "   " " a " "ab " "fo…" "fo…" "sh…" "性 " "性…" "性…"
    Code
      utf8_format(raw, chars = 2, justify = "right", na.print = "NA")
    Output
       [1] " NA" "   " "  a" " ab" "…oo" "…od" "…xt" " 性" "…性" " …?"

