#include "vk-renderer.h"

#include <vulkan/vulkan_to_string.hpp>
#include <vk-renderer/VkBootstrap/VkBootstrap.h>
#include <include/error_fmt.h>

#include <imgui.h>
#include <backend/imgui_impl_glfw.h>
#include <backend/imgui_impl_vulkan.h>

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

void deletion_queue_t::push_function(std::function<void()> &&function)
{
    this->deletors.push_back(function);
}

void deletion_queue_t::flush()
{
    for (auto it = deletors.rbegin(); it != deletors.rend(); ++it)
    {
        (*it)();
    }
    this->deletors.clear();
}

void transition_image(vk::CommandBuffer cmd, vk::Image image, vk::ImageLayout layout, vk::ImageLayout target)
{
    vk::ImageMemoryBarrier2 image_barrier(vk::PipelineStageFlagBits2::eAllCommands,
            vk::AccessFlagBits2::eMemoryWrite, vk::PipelineStageFlagBits2::eAllCommands,
            vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
            layout, target, {}, {}, image, vk::ImageSubresourceRange(
            (target == vk::ImageLayout::eDepthAttachmentOptimal) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor,
            0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS));
    vk::DependencyInfo dependency_info({}, {}, {}, {}, {}, 1, &image_barrier);
    cmd.pipelineBarrier2(dependency_info);
}

std::optional<renderer_t::allocated_buffer_t> renderer_t::create_buffer(const size_t allocation_size, const vk::BufferUsageFlags usage, const VmaMemoryUsage memory_usage)
{
    vk::BufferCreateInfo buffer_info({}, allocation_size, usage);
    VmaAllocationCreateInfo vma_alloc_info{ .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT, .usage = memory_usage };
    allocated_buffer_t buffer;
    VkResult err;
    if ((err = vmaCreateBuffer(this->allocator, (VkBufferCreateInfo*)&buffer_info, &vma_alloc_info, (VkBuffer*)&buffer.buffer, &buffer.allocation, &buffer.info)) != VK_SUCCESS)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create buffer with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(vk::Result(err)));
        return std::nullopt;
    }
    return buffer;
}

void renderer_t::destroy_buffer(const allocated_buffer_t &buffer)
{
    vmaDestroyBuffer(this->allocator, (VkBuffer)buffer.buffer, buffer.allocation);
}

std::optional<renderer_t::allocated_image_t> renderer_t::create_image(const vk::Extent3D extent, const vk::Format format,  const vk::ImageUsageFlags usage)
{
    allocated_image_t new_image {
        .extent = extent,
        .format = format,
    };
    vk::ImageCreateInfo image_info({}, vk::ImageType::e2D, format, extent, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage);
    VmaAllocationCreateInfo allocation_info{ .usage = VMA_MEMORY_USAGE_GPU_ONLY, .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    vk::Result err;
    if ((err = (vk::Result)vmaCreateImage(this->allocator, (VkImageCreateInfo*)&image_info, &allocation_info, (VkImage*)&new_image.image, &new_image.allocation, nullptr)) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create image with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(err));
        return std::nullopt;
    }

    vk::ImageAspectFlags aspect_flags = vk::ImageAspectFlagBits::eColor;
    if (format == vk::Format::eD32Sfloat) aspect_flags = vk::ImageAspectFlagBits::eDepth;

    vk::ImageViewCreateInfo view_info({}, new_image.image, vk::ImageViewType::e2D, format, {}, vk::ImageSubresourceRange(aspect_flags, 0, 1, 0, 1));
    std::tie(err, new_image.view) = this->device.device.createImageView(view_info);
    if (err != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create image with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(err));
        return std::nullopt;
    }
    return new_image;
}

std::optional<renderer_t::allocated_image_t> renderer_t::upload_image(const void* data, const vk::Extent3D extent, const vk::Format format,  const vk::ImageUsageFlags usage)
{
    size_t data_size = extent.depth * extent.width * extent.height * 4;
    auto upload_buffer = create_buffer(data_size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
    if (!upload_buffer.has_value()) return std::nullopt;
    std::memcpy(upload_buffer.value().info.pMappedData, data, data_size);
    auto new_image = this->create_image(extent, format, usage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc);
    if (!new_image.has_value()) return std::nullopt;

    this->immediate_submit([&](vk::CommandBuffer cmd) {
            transition_image(cmd, new_image.value().image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            vk::BufferImageCopy copy_region(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0 ,1), {}, extent);
            cmd.copyBufferToImage(upload_buffer.value().buffer, new_image.value().image, vk::ImageLayout::eTransferDstOptimal, copy_region);
            transition_image(cmd, new_image.value().image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal);
            });
    this->destroy_buffer(upload_buffer.value());
    return new_image.value();
}

void renderer_t::destroy_image(const allocated_image_t &image)
{
    this->device.device.destroyImageView(image.view);
    vmaDestroyImage(this->allocator, image.image, image.allocation);
}

bool renderer_t::immediate_submit(std::function<void(vk::CommandBuffer cmd)> &&function)
{
    vk::Result result = this->device.device.resetFences(this->immediate_sync.fence);
    if (result != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to reset fence with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }
    if ((result = this->immediate_sync.cmd.reset()) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to reset command buffer with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }
    vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    if ((result = this->immediate_sync.cmd.begin(begin_info)) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to begin recording command buffer with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }
    function(this->immediate_sync.cmd);
    if ((result = this->immediate_sync.cmd.end()) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to end recording command buffer with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }

    vk::CommandBufferSubmitInfo cmd_info(this->immediate_sync.cmd);
    vk::SubmitInfo2 submit_info({}, {}, cmd_info);

    if ((result = this->device.graphics.queue.submit2(submit_info, this->immediate_sync.fence)) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to submit command buffer with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }
    if ((result = this->device.device.waitForFences(this->immediate_sync.fence, true, 9999999999)) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to wait for fences with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }

    return true;
}

// BUG: Can cause the buffer to be "destroyed" while in use this could be mitigated using a fence but this function is not called from the render thread
bool renderer_t::stage_image(const void *data, const u32 width, const u32 height, const u32 elem_size)
{
    std::lock_guard<std::mutex> guard(this->staging_buffers.mutex);
    if (this->staging_buffers.extent.width != width || this->staging_buffers.extent.height != height)
    {
        auto staging_buffer = this->create_buffer(width * height * elem_size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
        if (!staging_buffer.has_value())
        {
            fmt::print(stderr, "[ {} ]\tFailed to recreate staging buffer!\n", ERROR_FMT("ERROR"));
            return false;
        }
        this->destroy_buffer(this->staging_buffers.buffer);
        this->staging_buffers.buffer = staging_buffer.value();
        this->staging_buffers.extent = vk::Extent3D(width, height, 1);

    }
    std::memcpy(this->staging_buffers.buffer.info.pMappedData, data, width * height * elem_size);
    return true;
}

bool renderer_t::init_imgui()
{
    vk::DescriptorPoolSize pool_sizes[] = {
        { vk::DescriptorType::eSampler, 1000 },
        { vk::DescriptorType::eCombinedImageSampler, 1000 },
        { vk::DescriptorType::eSampledImage, 1000 },
        { vk::DescriptorType::eStorageImage, 1000 },
        { vk::DescriptorType::eUniformTexelBuffer, 1000 },
        { vk::DescriptorType::eStorageTexelBuffer, 1000 },
        { vk::DescriptorType::eUniformBuffer, 1000 },
        { vk::DescriptorType::eStorageBuffer, 1000 },
        { vk::DescriptorType::eUniformBufferDynamic, 1000 },
        { vk::DescriptorType::eStorageBufferDynamic, 1000 },
        { vk::DescriptorType::eInputAttachment, 1000 },
    };

    vk::DescriptorPoolCreateInfo pool_info(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, pool_sizes);
    auto [result, imgui_pool] = this->device.device.createDescriptorPool(pool_info);
    if (result != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create imgui descriptor pool!\n", ERROR_FMT("ERROR"));
        return false;
    }

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(this->window.ptr, true);
    ImGui_ImplVulkan_InitInfo init_info = {0};
    init_info.Instance = this->instance;
    init_info.PhysicalDevice = this->physical_device;
    init_info.Device = this->device.device;
    init_info.Queue = this->device.graphics.queue;
    init_info.DescriptorPool = imgui_pool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;
    init_info.ColorAttachmentFormat = (VkFormat)this->swapchain.format;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    if (!this->immediate_submit([&](vk::CommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(); })) return false;
#pragma GCC diagnostic pop

    ImGui_ImplVulkan_DestroyFontsTexture();

    this->deletion_queue.push_function([=, this]() {
            ImGui_ImplVulkan_Shutdown();
            this->device.device.destroyDescriptorPool(imgui_pool);
            });

    return true;
}

void renderer_t::draw_imgui(vk::CommandBuffer cmd, vk::ImageView target_image_view)
{
    vk::RenderingAttachmentInfo color_attachment(target_image_view, vk::ImageLayout::eGeneral);
    vk::RenderingInfo render_info({}, { vk::Offset2D(0, 0), this->swapchain.extent }, 1, {}, color_attachment);
    cmd.beginRendering(render_info);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    cmd.endRendering();
}

bool renderer_t::update()
{
    size_t current_frame = this->frame_count % FRAME_OVERLAP;
    vk::Result result = this->device.device.waitForFences(this->frames[current_frame].render_fence, true, 1000000000);
    if (result != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to wait on render fence with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }
 
    u32 swapchain_image_index;
    std::tie(result, swapchain_image_index) = this->device.device.acquireNextImageKHR(this->swapchain.swapchain, 1000000000, this->frames[current_frame].swapchain_semaphore, nullptr);
    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        this->window.resize_requested = true;
        return true;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        fmt::print(stderr, "[ {} ]\tFailed to acquire image with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }

    if ((result = this->device.device.resetFences(this->frames[current_frame].render_fence)) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to reset render fence with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }

    vk::CommandBuffer cmd = this->frames[current_frame].buffer;
    if ((result = cmd.reset()) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to reset command buffer with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }
    
    vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    if ((result = cmd.begin(&begin_info)) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to begin recording command buffer with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }

    transition_image(cmd, this->swapchain.images[swapchain_image_index], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    const vk::ClearColorValue color(std::array<f32, 4>{0.f, 0.f, 0.f, 1.f});
    cmd.clearColorImage(this->swapchain.images[swapchain_image_index], vk::ImageLayout::eTransferDstOptimal, color, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

    std::lock_guard<std::mutex> guard(this->staging_buffers.mutex);
    vk::BufferImageCopy copy_region(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0 ,1), {}, this->staging_buffers.extent);
    cmd.copyBufferToImage(this->staging_buffers.buffer.buffer, this->swapchain.images[swapchain_image_index], vk::ImageLayout::eTransferDstOptimal, copy_region);
    //transition_image(cmd, this->swapchain.images[swapchain_image_index], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);

    // TODO: add parameter for this
    if (true)
    {
        transition_image(cmd, this->swapchain.images[swapchain_image_index], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
        this->draw_imgui(cmd, this->swapchain.views[swapchain_image_index]);
        transition_image(cmd, this->swapchain.images[swapchain_image_index], vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);
    }
    else
    {
        transition_image(cmd, this->swapchain.images[swapchain_image_index], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);
    }

    if ((result = cmd.end()) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to end recording command buffer with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }

    vk::CommandBufferSubmitInfo cmd_info(cmd);
    vk::SemaphoreSubmitInfo wait_info(this->frames[current_frame].swapchain_semaphore, 1, vk::PipelineStageFlagBits2::eColorAttachmentOutput, 0);
    vk::SemaphoreSubmitInfo signal_info(this->frames[current_frame].render_semaphore, 1, vk::PipelineStageFlagBits2::eAllGraphics, 0);
    vk::SubmitInfo2 submit_info({}, wait_info, cmd_info, signal_info);
    if ((result = this->device.graphics.queue.submit2(submit_info, this->frames[current_frame].render_fence)) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to submit to graphics queue with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }

    vk::PresentInfoKHR present_info(this->frames[current_frame].render_semaphore, this->swapchain.swapchain, swapchain_image_index);
    result = this->device.present.queue.presentKHR(&present_info);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        this->window.resize_requested = true;
        return true;
    }
    else if (result != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to present with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }

    this->frame_count++;
    return true;
}

void renderer_t::run(bool &running)
{
    while (!glfwWindowShouldClose(this->window.ptr))
    {
        glfwPollEvents();
        if (glfwGetKey(this->window.ptr, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(this->window.ptr, GLFW_TRUE);

        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            /// TODO: Record and display Rendering Stats when enabled
            this->custom_imgui();

            ImGui::Render();
        }

        if (!this->update())
        {
            return;
        }
    }
    running = false;
}

bool renderer_t::create_swapchain(u32 width, u32 height)
{
    vkb::SwapchainBuilder swapchain_builder{ this->physical_device, this->device.device, this->window.surface };
    this->swapchain.format = vk::Format::eB8G8R8A8Unorm;

    const vkb::Result<vkb::Swapchain> swapchain_or_error = swapchain_builder.set_desired_format((VkSurfaceFormatKHR)vk::SurfaceFormatKHR(this->swapchain.format, vk::ColorSpaceKHR::eSrgbNonlinear))
        .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build();
    if (!swapchain_or_error)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create swapchain with error: {}\n", ERROR_FMT("ERROR"), swapchain_or_error.error().message());
        return false;
    }
    vkb::Swapchain vkb_swapchain = swapchain_or_error.value();

    this->swapchain.swapchain = vkb_swapchain.swapchain;
    this->swapchain.extent = vkb_swapchain.extent;
    auto images = vkb_swapchain.get_images().value();
    auto views = vkb_swapchain.get_image_views().value();
    for (u32 i = 0; i < images.size(); ++i)
    {
        this->swapchain.images.push_back(images[i]);
        this->swapchain.views.push_back(views[i]);
    }
    return true;
}

void renderer_t::destroy_swapchain()
{
    for (vk::ImageView view : this->swapchain.views)
        this->device.device.destroyImageView(view);
    this->device.device.destroySwapchainKHR(this->swapchain.swapchain);
    this->swapchain.images.clear();
    this->swapchain.views.clear();
}

bool renderer_t::resize_swapchain()
{
    vk::Result err;
    if ((err = this->device.device.waitIdle()) != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to wait idle with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(err));
        return false;
    }
    this->destroy_swapchain();
    i32 w, h;
    glfwGetWindowSize(this->window.ptr, &w, &h);
    this->window.width = w;
    this->window.height = h;
    if (!this->create_swapchain(w, h)) return false;
    this->window.resize_requested = false;
    return true;
}

static renderer_t *active_renderer = nullptr;


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void renderer_t::framebuffer_size_callback(GLFWwindow *window, i32 width, i32 height)
{
    active_renderer->resize_swapchain();
}
#pragma GCC diagnostic pop

bool renderer_t::init(u32 width, u32 height, std::string name)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if ((this->window.ptr = glfwCreateWindow(width, height, name.c_str(), NULL, NULL)) == nullptr)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create GLFW window!\n", ERROR_FMT("ERROR"));
        return false;
    }
    this->window.width = width;
    this->window.height = height;
    active_renderer = this;
    glfwSetFramebufferSizeCallback(this->window.ptr, renderer_t::framebuffer_size_callback);

#ifdef DEBUG
    const auto debug_callback = [](VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
            const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data) -> VkBool32
    {
        auto severity = vkb::to_string_message_severity(message_severity);
        auto type = vkb::to_string_message_type(message_type);
        FILE *fd = stdout;
        if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            fd = (fd == stdout) ? stderr : fd;
            fmt::print(fd, "[ {} ]\t", ERROR_FMT(severity));
        }
        else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            fmt::print(fd, "[ {} ]\t", WARN_FMT(severity));
        else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
            fmt::print(fd, "[ {} ]\t", VERBOSE_FMT(severity));
        else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            fmt::print(fd, "[ {} ]\t", INFO_FMT(severity));
        
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) fmt::print(fd, "[ {} ]\t", GENERAL_FMT(type));
        else if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) fmt::print(fd, "[ {} ]\t", PERFORMANCE_FMT(type));
        else if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) fmt::print(fd, "[ {} ]\t", VALIDATION_FMT(type));

        fmt::print(fd, "{}\n", callback_data->pMessage);
        fmt::print(fd, "user data: {}\n", user_data);
        return VK_FALSE;
    };
#endif

    vkb::InstanceBuilder instance_builder;
    const vkb::Result<vkb::Instance> instance_or_err = instance_builder.set_app_name(name.c_str())
#ifdef DEBUG
        .set_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
#ifdef INFO_LAYERS
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
#endif
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                )
        .set_debug_messenger_type(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        .set_debug_callback(debug_callback)
        .request_validation_layers()
#endif
        .require_api_version(1,3,0)
        .build();

    if (!instance_or_err)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create vulkan instance with error: {}\n", ERROR_FMT("ERROR"), instance_or_err.error().message());
        return false;
    }
    const vkb::Instance vkb_instance = instance_or_err.value();
    this->deletion_queue.push_function([vkb_instance](){ vkb::destroy_instance(vkb_instance); });
    this->instance = vkb_instance.instance;
    this->messenger = vkb_instance.debug_messenger;

    VkSurfaceKHR surface;
    VkResult err;
    if ((err = glfwCreateWindowSurface(this->instance, this->window.ptr, nullptr, &surface)) != VK_SUCCESS)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create vulkan surface with error: {}", ERROR_FMT("ERROR"), vk::to_string(vk::Result(err)));
        return false;
    }
    this->window.surface = vk::SurfaceKHR(surface);
    this->deletion_queue.push_function([this](){ this->instance.destroySurfaceKHR(this->window.surface); });

    vkb::PhysicalDeviceSelector selector{ vkb_instance };
    const vkb::Result<vkb::PhysicalDevice> physical_device_or_error = selector.set_surface(surface)
        .set_minimum_version(1, 3)
        .set_required_features_13(VkPhysicalDeviceVulkan13Features{
                .synchronization2 = true,
                .dynamicRendering = true})
        .set_required_features_12(VkPhysicalDeviceVulkan12Features{
                .descriptorIndexing = true,
                .bufferDeviceAddress = true})
        .select();

    if (!physical_device_or_error)
    {
        fmt::print(stderr, "[ {} ]\tFailed to select physical device with error: {}\n", ERROR_FMT("ERROR"), physical_device_or_error.error().message());
        return false;
    }

    this->physical_device = vk::PhysicalDevice(physical_device_or_error.value());
    const vkb::DeviceBuilder device_builder{ physical_device_or_error.value() };
    const vkb::Result<vkb::Device> device_or_error = device_builder.build();
    if (!device_or_error)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create device with error: {}\n", ERROR_FMT("ERROR"), device_or_error.error().message());
        return false;
    }

    vkb::Device vkb_device = device_or_error.value();
    this->deletion_queue.push_function([vkb_device](){ vkb::destroy_device(vkb_device); });
    this->device.device = vk::Device(vkb_device.device);

    const vkb::Result<VkQueue> present_queue_or_error = vkb_device.get_queue(vkb::QueueType::present);
    if (!present_queue_or_error)
    {
        fmt::print(stderr, "[ {} ]\tFailed to get present queue with error: {}\n", ERROR_FMT("ERROR"), present_queue_or_error.error().message());
        return false;
    }
    const vkb::Result<u32> present_queue_index_or_error = vkb_device.get_queue_index(vkb::QueueType::present);
    if (!present_queue_index_or_error)
    {
        fmt::print(stderr, "[ {} ]\tFailed to get present queue index with error: {}\n", ERROR_FMT("ERROR"), present_queue_index_or_error.error().message());
        return false;
    }
    this->device.present.queue = vk::Queue(present_queue_or_error.value());
    this->device.present.family_index = present_queue_index_or_error.value();
    
    const vkb::Result<VkQueue> graphics_queue_or_error = vkb_device.get_queue(vkb::QueueType::graphics);
    if (!graphics_queue_or_error)
    {
        fmt::print(stderr, "[ {} ]\tFailed to get graphics queue with error: {}\n", ERROR_FMT("ERROR"), graphics_queue_or_error.error().message());
        return false;
    }
    const vkb::Result<u32> graphics_queue_index_or_error = vkb_device.get_queue_index(vkb::QueueType::graphics);
    if (!graphics_queue_index_or_error)
    {
        fmt::print(stderr, "[ {} ]\tFailed to get graphics queue index with error: {}\n", ERROR_FMT("ERROR"), graphics_queue_index_or_error.error().message());
        return false;
    }
    this->device.graphics.queue = vk::Queue(graphics_queue_or_error.value());
    this->device.graphics.family_index = graphics_queue_index_or_error.value();

    VmaAllocatorCreateInfo allocator_info = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = this->physical_device,
        .device = this->device.device,
        .instance = this->instance
    };
    vmaCreateAllocator(&allocator_info, &this->allocator);
    this->deletion_queue.push_function([this](){ vmaDestroyAllocator(this->allocator); });

    if (!this->create_swapchain(width, height)) return false;
    this->deletion_queue.push_function([this](){ this->destroy_swapchain(); });

    const vk::CommandPoolCreateInfo pool_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, this->device.graphics.family_index);
    vk::Result result;
    std::vector<vk::CommandBuffer> command_buffer;

    vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits::eSignaled);
    vk::SemaphoreCreateInfo semaphore_info;

    for (u8 i = 0; i < FRAME_OVERLAP; ++i)
    {
        std::tie(result, this->frames[i].pool) = this->device.device.createCommandPool(pool_info);
        if (result != vk::Result::eSuccess)
        {
            fmt::print(stderr, "[ {} ]\tFailed to create command pool with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
            return false;
        }
        this->deletion_queue.push_function([this, i](){ this->device.device.destroyCommandPool(this->frames[i].pool); });

        vk::CommandBufferAllocateInfo allocate_info(this->frames[i].pool, vk::CommandBufferLevel::ePrimary, 1);
        std::tie(result, command_buffer) = this->device.device.allocateCommandBuffers(allocate_info);
        if (result != vk::Result::eSuccess)
        {
            fmt::print(stderr, "[ {} ]\tFailed to create command buffer with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
            return false;
        }
        this->frames[i].buffer = command_buffer[0];

        std::tie(result, this->frames[i].render_fence) = this->device.device.createFence(fence_info);
        if (result != vk::Result::eSuccess)
        {
            fmt::print(stderr, "[ {} ]\tFailed to create render fence with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
            return false;
        }
        this->deletion_queue.push_function([this, i](){ this->device.device.destroyFence(this->frames[i].render_fence); });

        std::tie(result, this->frames[i].render_semaphore) = this->device.device.createSemaphore(semaphore_info);
        if (result != vk::Result::eSuccess)
        {
            fmt::print(stderr, "[ {} ]\tFailed to create render semaphore with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
            return false;
        }
        this->deletion_queue.push_function([this, i](){ this->device.device.destroySemaphore(this->frames[i].render_semaphore); });

        std::tie(result, this->frames[i].swapchain_semaphore) = this->device.device.createSemaphore(semaphore_info);
        if (result != vk::Result::eSuccess)
        {
            fmt::print(stderr, "[ {} ]\tFailed to create swapchain semaphore with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
            return false;
        }
        this->deletion_queue.push_function([this, i](){ this->device.device.destroySemaphore(this->frames[i].swapchain_semaphore); });
    }

    std::tie(result, this->immediate_sync.fence) = this->device.device.createFence(fence_info);
    if (result != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create fence with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(result));
        return false;
    }
    this->deletion_queue.push_function([this](){ this->device.device.destroyFence(this->immediate_sync.fence); });

    std::tie(result, this->immediate_sync.pool) = this->device.device.createCommandPool(pool_info);
    if (result != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create command pool!\n", ERROR_FMT("ERROR"));
        return false;
    }
    vk::CommandBufferAllocateInfo cmd_alloc_info(this->immediate_sync.pool, vk::CommandBufferLevel::ePrimary, 1);
    std::tie(result, command_buffer) = this->device.device.allocateCommandBuffers(cmd_alloc_info);
    if (result != vk::Result::eSuccess)
    {
        fmt::print(stderr, "[ {} ]\tFailed to create command buffer!\n", ERROR_FMT("ERROR"));
        return false;
    }
    this->immediate_sync.cmd = command_buffer[0];
    this->deletion_queue.push_function([this](){
                this->device.device.destroyCommandPool(this->immediate_sync.pool);
            });

    if (!this->init_imgui()) return false;

    auto staging_buffer = this->create_buffer(width * height * 4, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
    if (staging_buffer.has_value())
    {
        this->staging_buffers.buffer = staging_buffer.value();
        this->deletion_queue.push_function([this](){ this->destroy_buffer(this->staging_buffers.buffer); });
        this->staging_buffers.extent = vk::Extent3D(width, height, 1);
        std::vector<u32> pixels(width * height);
        bool w = false, h = false;
        for (size_t x = 0; x < width; ++x)
        {
            if (!(x % 16)) w = !w;
            for (size_t y = 0; y < height; ++y)
            {
                if (!(y % 16)) h = !h;
                pixels[y * width + x] = (w ^ h) ? 0xFFAAAAAA : 0xFF555555;
            }
        }
        std::memcpy(this->staging_buffers.buffer.info.pMappedData, pixels.data(), width * height * 4);
    }

    return true;
}

renderer_t::~renderer_t()
{
    vk::Result err;
    if ((err = this->device.device.waitIdle()) != vk::Result::eSuccess) // wait for current command to finish
    {
        fmt::print(stderr, "[ {} ]\tFailed to wait idle with error: {}\n", ERROR_FMT("ERROR"), vk::to_string(err));
    }
    this->deletion_queue.flush();
}
