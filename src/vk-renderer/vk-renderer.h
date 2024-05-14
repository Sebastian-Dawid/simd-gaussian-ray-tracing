#pragma once

#include <functional>
#include "vk-mem-alloc/vk_mem_alloc.h"
#include <include/definitions.h>
#include <mutex>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

constexpr u32 FRAME_OVERLAP = 2;

struct deletion_queue_t
{
    std::vector<std::function<void()>> deletors;
    void push_function(std::function<void()> &&function);
    void flush();
};

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
        u64 buffer_count = 0;

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
            std::mutex mutex;
        } staging_buffers;

        deletion_queue_t deletion_queue;

        /// Performs an immediate submit of the commands recorded in the given function.
        /// Uses the immediate sync command buffer
        bool immediate_submit(std::function<void(vk::CommandBuffer cmd)> &&function);

        /// Stages image to be uploaded to the instances present buffer.
        /// # Parameters:
        /// * `void *data` image data to upload
        /// * `u32 height` height of the image
        /// * `u32 width` width of the image
        /// * `u32 elem_size` size of a single pixel in bytes
        /// # Returns
        /// * `true` Staging successful
        /// * `false` failed to recreate buffer
        bool stage_image(const void *data, const u32 width, const u32 height, const u32 elem_size = 4);
        /// Initialize structures necessary for ImGui (descriptors, command pool, etc.)
        bool init_imgui();
        /// Draws ImGui Windows to the given image view
        void draw_imgui(vk::CommandBuffer cmd, vk::ImageView target_image_view);
        /// User defined Windows to draw
        std::function<void()> custom_imgui = [](){};
        /// Renders and presents a new frame. Deals with associated sync structures.
        bool update();
        /// Main loop (IO, etc)
        void run(bool &running);

        /// Creates new swapchain (init and resize)
        bool create_swapchain(u32 width, u32 height);
        /// Destroys the current swapchain
        void destroy_swapchain();
        /// Destroys then creates new swapchain with updated size
        bool resize_swapchain();

        /// Callback to deal with resizing the window at runtime
        static void framebuffer_size_callback(GLFWwindow *window, i32 width, i32 height);

        /// Initializes window and vulkan.
        /// # Parameters:
        /// # Returns:
        /// * `false` Initialization failed.
        /// * `true` Initialization successful
        bool init(u32 width, u32 height, std::string name);
        ~renderer_t();
};
