if (require(testthat)) {
  library(utf8)
  test_check("utf8")
} else {
  message("testthat not available.")
}
