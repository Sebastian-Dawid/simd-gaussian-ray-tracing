SHELL := sh
# CC := clang
# CXX := clang++
# AR := llvm-ar

CWD ?= $(shell pwd)
SRC_DIR ?= src

IMGUI_SRCS=$(shell find $(SRC_DIR)/external/imgui -name '*.cpp')
IMGUI_OBJS=$(IMGUI_SRCS:$(SRC_DIR)/external/%=obj/%.o)
VK_RENDERER_SRCS=$(shell find $(SRC_DIR)/vk-renderer -name '*.cpp')
VK_RENDERER_OBJS=$(VK_RENDERER_SRCS:$(SRC_DIR)/%=obj/%.o)
VRT_SRCS=$(shell find $(SRC_DIR)/vrt -name '*.cpp')
VRT_OBJS=$(VRT_SRCS:$(SRC_DIR)/%=obj/%.o)

INC_DIRS = -I$(SRC_DIR)/external/imgui -I$(SRC_DIR)
CFLAGS = -O3 -g $(WARN) -march=native -save-temps=obj -fverbose-asm -ffast-math -std=c++20 $(INC_DIRS)

all:
	@mkdir -p build; \
	cd build; \
	make -f ../fallback.mak volumetric-ray-tracer SRC_DIR=../src CWD=$(CWD); \
	cd ..

clean:
	@rm -rf build

obj/imgui/%.cpp.o: $(SRC_DIR)/external/imgui/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

imgui: $(IMGUI_OBJS)
	@mkdir -p lib
	@echo $(IMGUI_OBJS)
	$(AR) rcs lib/libimgui.a $(IMGUI_OBJS)

obj/vk-renderer/%.cpp.o: $(SRC_DIR)/vk-renderer/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -DVULKAN_HPP_NO_EXCEPTIONS -c $< -o $@

vk-renderer: $(VK_RENDERER_OBJS)
	@mkdir -p lib
	$(AR) rcs lib/libvk-renderer.a $(VK_RENDERER_OBJS)

obj/vrt/%.cpp.o: $(SRC_DIR)/vrt/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -fPIC $(CFLAGS) -DINCLUDE_IMGUI -c $< -o $@

vrt: $(VRT_OBJS)
	@mkdir -p lib
	$(CXX) -shared $(VRT_OBJS) -o lib/libvrt.so

volumetric-ray-tracer: vk-renderer vrt imgui
	@mkdir -p bin
	$(CXX) $(CFLAGS) -DINCLUDE_IMGUI $(SRC_DIR)/volumetric-ray-tracer/main.cpp -o bin/volumetric-ray-tracer -Llib  -lfmt -lvulkan -lglfw -lvk-renderer -limgui -Wl,-rpath -Wl,$(CWD)/build/lib -lvrt
