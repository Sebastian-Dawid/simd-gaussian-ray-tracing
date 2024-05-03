# source: https://scaron.info/blog/makefiles-for-latex.html

.DEFAULT_GOAL := help
.PHONY: help

PAPER=main.tex
NAME=sdawid_ba
LATEX=rubber --shell-escape --pdf --into target

FIGURES=

all: $(FIGURES) ## Build full thesis
	mkdir -p target
	$(LATEX) $(PAPER) --jobname $(NAME)

clean:	## Clean LaTeX output
	rubber --clean --into target $(PAPER) --jobname $(NAME)

watch:	## Recompile when source changes
	@while [ 1 ]; do;				\
		inotifywait $(PAPER);		\
		sleep 0.01;					\
		make all;					\
		echo "\n================\n" \
		done

help:
	@grep -P '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'
