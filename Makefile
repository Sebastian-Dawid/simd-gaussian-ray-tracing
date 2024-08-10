SHELL := sh
.DEFAULT_GOAL := help

.PHONY: clean build run thesis help

BUILD_DIR = build

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

thesis: ## 
	@pushd thesis > /dev/null; \
	$(MAKE) all; \
	popd > /dev/null

help: ## Prints help for targets with comments
	@cat $(MAKEFILE_LIST) | grep -E '^[a-zA-Z_-]+:.*?## .*$$' | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
