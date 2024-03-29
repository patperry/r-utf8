RSCRIPT= Rscript --vanilla
UTF8_LIB= src/utf8.so
BUILT_VIGNETTES= \
	vignettes/utf8.Rmd

all: $(UTF8_LIB) README.md $(BUILT_VIGNETTES)

$(UTF8_LIB):
	$(RSCRIPT) -e 'devtools::compile_dll()'

NEWS: NEWS.md
	sed -e 's/^### //g; s/`//g' $< > $@

README.md: README.Rmd
	$(RSCRIPT) -e 'knitr::knit("README.Rmd")'

vignettes/%.Rmd: vignettes/%.Rmd.in
	$(RSCRIPT) -e 'devtools::load_all(); setwd("vignettes"); knitr::knit(basename("$<"), basename("$@"))'

check: $(UTF8_LIB)
	$(RSCRIPT) -e 'devtools::test(".")'

clean:
	$(RSCRIPT) -e 'devtools::clean_dll(".")' && rm -f README.md

cov:
	$(RSCRIPT) -e 'covr::package_coverage(line_exclusions = c("R/deprecated.R", list.files("src/utf8lite", recursive = TRUE, full.names = TRUE)))'

dist: $(BUILT_VIGNETTES) NEWS README.md
	mkdir -p dist && cd dist && R CMD build ..

distclean: clean
	rm -rf $(BUILT_VIGNETTES)

doc: $(BUILT_VIGNETTES) NEWS README.md

install: $(UTF8_LIB)
	$(RSCRIPT) -e 'devtools::install(".")'

site: $(BUILT_VIGNETTES)
	$(RSCRIPT) -e 'pkgdown::build_site(".")'

utf8lite:
	git subtree pull git@github.com:patperry/utf8lite HEAD --prefix src/utf8lite

.PHONY: all check clean con dist distclean doc install site utf8lite
