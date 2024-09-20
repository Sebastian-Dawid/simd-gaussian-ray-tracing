SHELL := sh

CWD = $(shell pwd)
SRC_DIR = src

IMGUI_SRCS=$(shell find $(SRC_DIR)/external/imgui -name '*.cpp')
IMGUI_OBJS=$(IMGUI_SRCS:$(SRC_DIR)/external/%=build/obj/%.o)
VK_RENDERER_SRCS=$(shell find $(SRC_DIR)/vk-renderer -name '*.cpp')
VK_RENDERER_OBJS=$(VK_RENDERER_SRCS:$(SRC_DIR)/%=build/obj/%.o)
VRT_SRCS=$(shell find $(SRC_DIR)/vrt -name '*.cpp')
VRT_OBJS=$(VRT_SRCS:$(SRC_DIR)/%=build/obj/%.o)

INC_DIRS = -I$(SRC_DIR)/external/imgui -I$(SRC_DIR)
CFLAGS = -O3 -g $(WARN) -march=native -save-temps=obj -fverbose-asm -ffast-math -std=c++20 $(INC_DIRS)

.PHONY: all clean

all: imgui vk-renderer vrt volumetric-ray-tracer

clean:
	@rm -rf build

build/obj/imgui/%.cpp.o: $(SRC_DIR)/external/imgui/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

imgui: $(IMGUI_OBJS)
	@mkdir -p build/lib
	$(AR) rcs build/lib/libimgui.a $(IMGUI_OBJS)

build/obj/vk-renderer/%.cpp.o: $(SRC_DIR)/vk-renderer/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -DVULKAN_HPP_NO_EXCEPTIONS -c $< -o $@

vk-renderer: $(VK_RENDERER_OBJS)
	@mkdir -p build/lib
	$(AR) rcs build/lib/libvk-renderer.a $(VK_RENDERER_OBJS)

build/obj/vrt/%.cpp.o: $(SRC_DIR)/vrt/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -fPIC $(CFLAGS) -DINCLUDE_IMGUI -c $< -o $@

vrt: $(VRT_OBJS)
	@mkdir -p build/lib
	$(CXX) -shared $(VRT_OBJS) -o build/lib/libvrt.so

volumetric-ray-tracer: vk-renderer vrt imgui
	@mkdir -p build/bin
	$(CXX) $(CFLAGS) -DINCLUDE_IMGUI $(SRC_DIR)/volumetric-ray-tracer/main.cpp -o build/bin/volumetric-ray-tracer -Llib  -lfmt -lvulkan -lglfw -lvk-renderer -limgui -Wl,-rpath -Wl,$(CWD)/build/lib -lvrt
