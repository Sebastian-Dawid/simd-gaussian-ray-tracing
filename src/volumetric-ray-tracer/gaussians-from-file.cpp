#include "gaussians-from-file.h"
#include <include/error_fmt.h>
#include <include/definitions.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <include/tiny_obj_loader.h>

std::vector<gaussian_t> read_from_obj(const char *const filename)
{
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filename))
    {
        if (!reader.Error().empty())
        {
            fmt::println(stderr, "[ {} ]\tTinyObjReader: {}", ERROR_FMT("ERROR"), reader.Error());
        }
        exit(EXIT_FAILURE);
    }

    if (!reader.Warning().empty())
    {
        fmt::println("[ {} ]\tTinyObjReader: {}", WARN_FMT("WARNING"), reader.Warning());
    }

    const tinyobj::attrib_t &attrib = reader.GetAttrib();
    std::vector<gaussian_t> gaussians;
    for (u64 i = 0; i < attrib.vertices.size(); i+=3)
    {
        i32 c = rand() % 3;
        f32 depth = attrib.vertices[i + 2];
        gaussians.emplace_back(gaussian_t{
                .albedo = vec4f_t{ (f32)(c == 0)/depth, (f32)(c == 1)/depth, (f32)(c == 2)/depth, 1.0f },
                .mu = { .x = attrib.vertices[i], .y = attrib.vertices[i+1], .z = attrib.vertices[i+2], .w = 0.0f },
                .sigma = 0.2f,
                .magnitude = 1.0f
                });
    }
    return gaussians;
}
