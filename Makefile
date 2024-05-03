SHELL := sh
.DEFAULT_GOAL := help

.PHONY: clean build run help

BUILD_DIR = build

ifndef CONFIG
CONFIG=release
endif

clean: ## Remove build artifacts
	rm -rf $(BUILD_DIR)

build:	## Compile and link
	@premake5 gmake2;		\
	cd $(BUILD_DIR);		\
	make config=$(CONFIG);	\
	cd ..

run: build ## Build then run
	@build/bin/$(CONFIG)/volumetric-ray-tracer

help: ## Prints help for targets with comments
	@cat $(MAKEFILE_LIST) | grep -E '^[a-zA-Z_-]+:.*?## .*$$' | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
