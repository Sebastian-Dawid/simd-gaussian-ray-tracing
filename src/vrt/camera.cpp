#include "camera.h"
#include <glm/ext/matrix_transform.hpp>
#include <include/tsimd_sh.H>

namespace vrt
{
    void camera_t::turn(const f32 yaw, const f32 pitch, const bool constrain)
    {
        f32 _pitch = pitch;
        if (constrain)
        {
            _pitch = (_pitch >  89.f) ?  89.f : _pitch;
            _pitch = (_pitch < -89.f) ? -89.f : _pitch;
        }
        this->front = glm::normalize(glm::vec3(
                    std::cos(glm::radians(yaw)) * std::cos(glm::radians(_pitch)),
                    std::sin(glm::radians(_pitch)),
                    std::sin(glm::radians(yaw)) * std::cos(glm::radians(_pitch))
                    ));
        this->right = glm::normalize(glm::cross(this->front, this->world_up));
        this->up    = glm::normalize(glm::cross(this->right, this->front));
        this->update();
    }

    camera_t::camera_t(const glm::vec3 position, const glm::vec3 up, const glm::vec3 front,
            const f32 yaw, const f32 pitch, const u64 width, const u64 height, const f32 focal_length)
    {
        this->position = position;
        this->up = up;
        this->world_up = up;
        this->front = front;
        this->focal_length = focal_length;
        this->w = width;
        this->h = height;
        this->turn(yaw, pitch);
    }

    camera_t::camera_t(const camera_create_info_t &ci)
    {
        this->position = ci.position;
        this->up = ci.up;
        this->world_up = ci.up;
        this->front = ci.front;
        this->focal_length = ci.focal_length;
        this->w = ci.width;
        this->h = ci.height;
        this->turn(ci.yaw, ci.pitch);
    }

    void camera_t::update()
    {
        this->view_matrix = glm::translate(glm::lookAt(this->position, this->position + this->front, this->up), this->focal_length * this->front);
        //PRINT_MAT(this->view_matrix);
        //if (this->projection_plane.xs != nullptr) simd_aligned_free(this->projection_plane.xs);
        //if (this->projection_plane.ys != nullptr) simd_aligned_free(this->projection_plane.ys);
        //if (this->projection_plane.zs != nullptr) simd_aligned_free(this->projection_plane.zs);
        this->projection_plane.xs = (f32 *)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * this->w * this->h);
        this->projection_plane.ys = (f32 *)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * this->w * this->h);
        this->projection_plane.zs = (f32 *)simd_aligned_malloc(SIMD_BYTES, sizeof(f32) * this->w * this->h);
        for (u64 i = 0; i < this->h; ++i)
        {
            for (u64 j = 0; j < this->w; ++j)
            {
                glm::vec3 pt = glm::inverse(this->view_matrix)
                    * glm::vec4(-1.f + j / (this->w / 2.f), -1.f + i / (this->h / 2.f), 0.f, 1.f);
                this->projection_plane.xs[i * this->w + j] = pt.x;
                this->projection_plane.ys[i * this->w + j] = pt.y;
                this->projection_plane.zs[i * this->w + j] = pt.z;
            }
        }
    }

    camera_t::~camera_t()
    {
        if (this->projection_plane.xs != nullptr) simd_aligned_free(this->projection_plane.xs);
        if (this->projection_plane.ys != nullptr) simd_aligned_free(this->projection_plane.ys);
        if (this->projection_plane.zs != nullptr) simd_aligned_free(this->projection_plane.zs);
    }
};
