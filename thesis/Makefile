# source: https://scaron.info/blog/makefiles-for-latex.html

SHELL := zsh
.DEFAULT_GOAL := help
.PHONY: help

FILES=main.tex plots/** main.bib glossary.tex
PAPER=main.tex
NAME=sdawid_ba
LATEX=latexmk -pdf -jobname=./target/$(NAME)

FIGURES=

all: $(FIGURES) ## Build full thesis
	mkdir -p target
	$(LATEX) $(PAPER)

clean:	## Clean LaTeX output
	rm -rf target

watch:	## Recompile when source changes
	@while [ 1 ]; do;				 \
		inotifywait $(FILES);		 \
		sleep 0.01;					 \
		make all;					 \
		echo "\n================\n"; \
		done

help:
	@grep -P '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'
