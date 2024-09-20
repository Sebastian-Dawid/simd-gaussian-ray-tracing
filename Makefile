SHELL := sh
.DEFAULT_GOAL := help

.PHONY: clean build run thesis help

BUILD_DIR = build

PV_AVAILABLE := $(shell command -v pv 2> /dev/null)
PREMAKE_AVAILABLE := $(shell command -v premake5 2> /dev/null)
HPC_AVAILABLE := $(shell command -v hpcrun 2> /dev/null)

THESIS_FILES := ./thesis/main.tex ./thesis/main.bib ./thesis/glossary.tex ./thesis/Makefile ./thesis/latexmkrc ./thesis/images ./thesis/plots ./thesis/smart-thesis ./thesis/README.md
SOURCE_FILES := $(THESIS_FILES) src test-objects Makefile fallback.mak premake5.lua julia README.md

ifndef CONFIG
CONFIG=release
endif

clean: ## Remove build artifacts
	rm -rf $(BUILD_DIR)

all: ## Compile and link example, libraries and tests
ifdef PREMAKE_AVAILABLE
	@premake5 $(ARGS) gmake2;		\
	pushd $(BUILD_DIR) > /dev/null;		\
	$(MAKE) config=$(CONFIG);	\
	popd > /dev/null
else
	$(MAKE) -f fallback.mak
endif

libs: ## Compile libraries
ifdef PREMAKE_AVAILABLE
	@premake5 $(ARGS) gmake2;		\
	pushd $(BUILD_DIR) > /dev/null;		\
	$(MAKE) config=$(CONFIG) imgui vrt vk-renderer;	\
	popd > /dev/null
else
	$(MAKE) -f fallback.mak imgui vrt vk-renderer
endif

build: libs	## Compile and link example application
ifdef PREMAKE_AVAILABLE
	@premake5 $(ARGS) gmake2;		\
	pushd $(BUILD_DIR) > /dev/null;		\
	$(MAKE) config=$(CONFIG) volumetric-ray-tracer;	\
	popd > /dev/null
else
	$(MAKE) -f fallback.mak volumetric-ray-tracer
endif

run: build ## Build then run
	@build/bin/$(CONFIG)/volumetric-ray-tracer $(ARGS)

hpc-run: ## Run for both clang and gcc through hpctoolkit
ifdef HPC_AVAILABLE
	@./run-hpc.sh; \
	make hpc-archive
else
	@echo "ERROR: HPCToolkit is not available!"
endif

hpc-archive: ## Archive the results of hpc-run in a *.tgz archive
ifdef PV_AVAILABLE
	@tar cf - hpctoolkit | pv -s $$(du -sb hpctoolkit | awk '{print $$1}') | gzip > hpctoolkit-results.tgz
else
	@tar czf hpctoolkit-results.tgz hpctoolkit
endif

gather-runtimes: ## Average Runtimes of SIMD execution modes
	@./runtimes.sh

pkg-code: ## Package source code into a *.tgz archive without the thesis source
ifdef PV_AVAILABLE
	@tar cf - $(SOURCE_FILES) | pv -s $$(du -csb $(SOURCE_FILES) | grep total | awk '{print $$1}') | gzip > code-$$(git rev-parse --short HEAD).tgz
else
	@tar czf code-$$(git rev-parse --short HEAD).tgz $(SOURCE_FILES)
endif

thesis: ## Compile the thesis
	@pushd thesis > /dev/null; \
	$(MAKE) all; \
	popd > /dev/null

help: ## Prints help for targets
	@cat $(MAKEFILE_LIST) | grep -E '^[a-zA-Z_-]+:.*?## .*$$' | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
