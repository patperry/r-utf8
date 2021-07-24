local_utf8 <- function(enable = TRUE, .local_envir = parent.frame()) {
  withr::local_options(
    list(cli.unicode = enable),
    .local_envir = .local_envir
  )
}
