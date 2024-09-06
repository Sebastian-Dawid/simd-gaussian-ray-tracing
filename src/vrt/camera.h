#pragma once

#include <glm/glm.hpp>
#include <include/definitions.h>

namespace vrt
{
    struct camera_create_info_t
    {
        glm::vec3 position     = glm::vec3(0.f, 0.f, 0.f);
        glm::vec3 up           = glm::vec3(0.f, 1.f, 0.f);
        glm::vec3 front        = glm::vec3(0.f, 0.f, 1.f);
        f32       yaw          = -90.f;
        f32       pitch        = 0.f;
        u64       width        = 256;
        u64       height       = 256;
        f32       focal_length = 1.f;
    };

    struct camera_t
    {
        glm::vec3 position;
        glm::vec3 front;
        glm::vec3 up, world_up;
        glm::vec3 right;
        glm::mat4 view_matrix;
        f32 focal_length;
        u64 w, h;

        struct
        {
            f32 *xs = nullptr;
            f32 *ys = nullptr;
            f32 *zs = nullptr;
        } projection_plane;

        void update();
        void turn(const f32 yaw, const f32 pitch, const bool constrain = true);
        camera_t(const glm::vec3 position, const glm::vec3 up = glm::vec3(0.f, 1.f, 0.f), const glm::vec3 front = glm::vec3(0.f, 0.f, 1.f),
                const f32 yaw = -90.f, const f32 pitch = 0.f, const u64 width = 256, const u64 height = 256,
                const f32 focal_length = 1.f);
        camera_t(const camera_create_info_t &ci);
        ~camera_t();
    };
};
