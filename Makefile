SHELL := sh
.DEFAULT_GOAL := help

.PHONY: clean build run thesis help

BUILD_DIR = build

PV_AVAILABLE := $(shell command -v pv 2> /dev/null)

ifndef CONFIG
CONFIG=release
endif

clean: ## Remove build artifacts
	rm -rf $(BUILD_DIR); \
	pushd thesis > /dev/null; \
	$(MAKE) clean; \
	popd > /dev/null

build:	## Compile and link
	@premake5 $(ARGS) gmake2;		\
	pushd $(BUILD_DIR) > /dev/null;		\
	$(MAKE) config=$(CONFIG);	\
	popd > /dev/null

run: build ## Build then run
	@build/bin/$(CONFIG)/volumetric-ray-tracer $(ARGS)

hpc-run: ## Run for both clang and gcc through hpctoolkit
	@./run-hpc.sh; \
	make hpc-archive

hpc-archive: ## Archive the results of hpc-run in a *.tgz archive
ifdef PV_AVAILABLE
	@tar cf - runtime-log hpctoolkit | pv -s $$(du -sb ./hpctoolkit | awk '{print $$1}') | gzip > hpctoolkit-results.tgz
else
	@tar czf hpctoolkit-results.tgz runtime-log hpctoolkit
endif

thesis: ## Compile the thesis
	@pushd thesis > /dev/null; \
	$(MAKE) all; \
	popd > /dev/null

help: ## Prints help for targets
	@cat $(MAKEFILE_LIST) | grep -E '^[a-zA-Z_-]+:.*?## .*$$' | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
