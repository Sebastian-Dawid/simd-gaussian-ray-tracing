#include <cstdlib>
#include <fmt/core.h>
#include <thread>
#include <vk-renderer/vk-renderer.h>



int main()
{
    fmt::println("{}, World!\n", "Hello");

    renderer_t renderer;
    if (!renderer.init(512, 512, "Test")) return EXIT_FAILURE;
    std::thread render_thread([&](){
            while (!glfwWindowShouldClose(renderer.window.ptr))
            {
                glfwPollEvents();
                if (glfwGetKey(renderer.window.ptr, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(renderer.window.ptr, GLFW_TRUE);
                if (!renderer.update())
                {
                    return;
                }
            }
        });

    render_thread.join();

    return EXIT_SUCCESS;
}
