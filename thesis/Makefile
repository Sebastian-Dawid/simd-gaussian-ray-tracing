# source: https://scaron.info/blog/makefiles-for-latex.html

SHELL := zsh
.DEFAULT_GOAL := help
.PHONY: help

PAPER=main.tex
NAME=sdawid_ba
LATEX=latexmk -pdf -jobname=./target/$(NAME)

FIGURES=$(shell find plots -name '*.tex')
FILES=main.tex main.bib glossary.tex $(FIGURES)

TODO_AVAILABLE = $(shell command -v todo-manager 2> /dev/null)

all: $(FIGURES) ## Build full thesis
ifdef TODO_AVAILABLE
	todo-manager -o TODOs.md -i main.tex
endif
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
