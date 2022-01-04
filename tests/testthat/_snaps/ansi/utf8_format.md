# 'format' can handle long text in Unicode locale

    Code
      raw <- c(NA, "", "a", "ab", "foo", "food", "short text", "<U+6027>",
        "<U+6027><U+6027>", "<U+6027>?")
      Encoding(raw) <- "UTF-8"
      body(mapper)
    Output
      lapply(x, charToRaw)
    Code
      mapper(utf8_format(raw, chars = 2, justify = "none", na.print = "NA"))
    Output
      [[1]]
      [1] 4e 41
      
      [[2]]
      raw(0)
      
      [[3]]
      [1] 61
      
      [[4]]
      [1] 61 62
      
      [[5]]
      [1] 66 6f e2 80 a6
      
      [[6]]
      [1] 66 6f e2 80 a6
      
      [[7]]
      [1] 73 68 e2 80 a6
      
      [[8]]
      [1] 3c 55 e2 80 a6
      
      [[9]]
      [1] 3c 55 e2 80 a6
      
      [[10]]
      [1] 3c 55 e2 80 a6
      
    Code
      mapper(utf8_format(raw, chars = 2, justify = "left", na.print = "NA"))
    Output
      [[1]]
      [1] 4e 41 20
      
      [[2]]
      [1] 20 20 20
      
      [[3]]
      [1] 61 20 20
      
      [[4]]
      [1] 61 62 20
      
      [[5]]
      [1] 66 6f e2 80 a6
      
      [[6]]
      [1] 66 6f e2 80 a6
      
      [[7]]
      [1] 73 68 e2 80 a6
      
      [[8]]
      [1] 3c 55 e2 80 a6
      
      [[9]]
      [1] 3c 55 e2 80 a6
      
      [[10]]
      [1] 3c 55 e2 80 a6
      
    Code
      mapper(utf8_format(raw, chars = 2, justify = "centre", na.print = "NA"))
    Output
      [[1]]
      [1] 4e 41 20
      
      [[2]]
      [1] 20 20 20
      
      [[3]]
      [1] 20 61 20
      
      [[4]]
      [1] 61 62 20
      
      [[5]]
      [1] 66 6f e2 80 a6
      
      [[6]]
      [1] 66 6f e2 80 a6
      
      [[7]]
      [1] 73 68 e2 80 a6
      
      [[8]]
      [1] 3c 55 e2 80 a6
      
      [[9]]
      [1] 3c 55 e2 80 a6
      
      [[10]]
      [1] 3c 55 e2 80 a6
      
    Code
      mapper(utf8_format(raw, chars = 2, justify = "right", na.print = "NA"))
    Output
      [[1]]
      [1] 20 4e 41
      
      [[2]]
      [1] 20 20 20
      
      [[3]]
      [1] 20 20 61
      
      [[4]]
      [1] 20 61 62
      
      [[5]]
      [1] e2 80 a6 6f 6f
      
      [[6]]
      [1] e2 80 a6 6f 64
      
      [[7]]
      [1] e2 80 a6 78 74
      
      [[8]]
      [1] e2 80 a6 37 3e
      
      [[9]]
      [1] e2 80 a6 37 3e
      
      [[10]]
      [1] e2 80 a6 3e 3f
      

