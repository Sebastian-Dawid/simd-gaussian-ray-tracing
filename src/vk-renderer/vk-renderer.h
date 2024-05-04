#pragma once

#include <functional>
#include "vk-mem-alloc/vk_mem_alloc.h"
#include <include/definitions.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

constexpr u32 FRAME_OVERLAP = 2;

struct renderer_t
{
        vk::Instance instance;
        vk::PhysicalDevice physical_device;
        vk::DebugUtilsMessengerEXT messenger;
        VmaAllocator allocator;
        struct
        {
            GLFWwindow *ptr;
            u32 width;
            u32 height;
            vk::SurfaceKHR surface;
            bool resize_requested;
        } window;
        struct {
            vk::Device device;
            struct {
                vk::Queue queue;
                u32 family_index;
            } present, graphics;
        } device;
        struct {
            vk::SwapchainKHR swapchain;
            vk::Format format;
            vk::Extent2D extent;
            std::vector<vk::Image> images;
            std::vector<vk::ImageView> views;
        } swapchain;

        struct
        {
            vk::CommandPool pool;
            vk::CommandBuffer buffer;
            vk::Semaphore swapchain_semaphore, render_semaphore;
            vk::Fence render_fence;
        } frames[FRAME_OVERLAP];

        struct
        {
            vk::Fence fence;
            vk::CommandBuffer cmd;
            vk::CommandPool pool;
        } immediate_sync;
        u64 frame_count = 0;

        struct allocated_buffer_t
        {
            vk::Buffer buffer;
            VmaAllocation allocation;
            VmaAllocationInfo info;
        };
        std::optional<allocated_buffer_t> create_buffer(const size_t allocation_size, const vk::BufferUsageFlags usage, const VmaMemoryUsage memory_usage);
        void destroy_buffer(const allocated_buffer_t &buffer);
        struct allocated_image_t
        {
            vk::Image image;
            vk::ImageView view;
            VmaAllocation allocation;
            vk::Extent3D extent;
            vk::Format format;
        };
        std::optional<allocated_image_t> create_image(const vk::Extent3D size, const vk::Format format, const vk::ImageUsageFlags usage);
        std::optional<allocated_image_t> upload_image(const void *data, const vk::Extent3D extent, const vk::Format format, const vk::ImageUsageFlags usage);
        void destroy_image(const allocated_image_t &image);
        
        struct
        {
            vk::Extent3D extent;
            allocated_buffer_t buffer;
        } staging_buffer;

        bool immediate_submit(std::function<void(vk::CommandBuffer cmd)> &&function);

        /// Stages image to be uploaded to the instances present buffer.
        /// # Parameters:
        /// * `void *data` image data to upload
        /// * `u32 height` height of the image
        /// * `u32 width` width of the image
        /// # Returns
        /// * `0` Staging successful
        /// * `!= 0` Staging failed
        /// TODO: Add more meaningful error codes when implementing this function
        i32 stage_img(void *data, u32 width, u32 height);
        bool update();

        /// Initializes window and vulkan.
        /// # Parameters:
        /// # Returns:
        /// * `false` Initialization failed.
        /// * `true` Initialization successful
        bool init(u32 width, u32 height, std::string name);
        ~renderer_t();
};
