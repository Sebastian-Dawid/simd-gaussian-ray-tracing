#include <cstdlib>
#include <fmt/core.h>
#include <future>
#include <thread>
#include <vk-renderer/vk-renderer.h>



int main()
{
    fmt::println("{}, World!\n", "Hello");

    renderer_t renderer;
    if (!renderer.init(512, 512, "Test")) return EXIT_FAILURE;
    std::promise<int> return_value;
    std::future<int> return_future = return_value.get_future();
    std::thread render_thread([&](std::promise<int> promise){
            while (!glfwWindowShouldClose(renderer.window.ptr))
            {
                glfwPollEvents();
                if (glfwGetKey(renderer.window.ptr, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(renderer.window.ptr, GLFW_TRUE);
                if (!renderer.update())
                {
                    promise.set_value(43);
                    return;
                }
            }
            promise.set_value(42);
        }, std::move(return_value));

    return EXIT_SUCCESS;
}
