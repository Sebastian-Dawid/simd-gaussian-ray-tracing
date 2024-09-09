#include "gaussians-from-file.h"
#include <include/error_fmt.h>
#include <include/definitions.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <include/tiny_obj_loader.h>

std::vector<vrt::gaussian_t> read_from_obj(const char *const filename)
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
    std::vector<vrt::gaussian_t> gaussians;
    f32 sig = [len = u64{attrib.vertices.size()/3}] () -> f32 {
        if (len < 300) return 0.3f;
        else if (len < 1000) return 0.15f;
        return 0.05f;
    }();
    for (u64 i = 0; i < attrib.vertices.size(); i+=3)
    {
        vrt::vec4f_t pt{ .x = attrib.vertices[i], .y = attrib.vertices[i+1], .z = attrib.vertices[i+2], .w = 0.0f };
        vrt::vec4f_t c = pt;
        c.normalize();
        gaussians.emplace_back(vrt::gaussian_t{
                .albedo = c * 0.5f + vrt::vec4f_t{ 0.5f, 0.5f, 0.5f, 1.0f },
                .mu = pt,
                .sigma = sig,
                .magnitude = 1.0f
                });
    }
    return gaussians;
}
