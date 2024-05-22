#pragma once

#include <include/tsimd_sh.H>
#include <cmath>

#include <fmt/core.h>
#include <fmt/format.h>
#ifdef INCLUDE_IMGUI
#include <imgui.h>
#endif
#include <string>

#include "approx.h"

struct vec4f_t
{
    float x, y, z, w = 0.f;
    vec4f_t operator+(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x + other.x, .y = this->y + other.y, .z = this->z + other.z, .w = this->w + other.w };
    }
    vec4f_t operator-(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x - other.x, .y = this->y - other.y, .z = this->z - other.z, .w = this->w - other.w };
    }
    vec4f_t operator*(const float &lambda) const
    {
        return vec4f_t{ .x = this->x * lambda, .y = this->y * lambda, .z = this->z * lambda, .w = this->w * lambda };
    }
    vec4f_t operator/(const vec4f_t &other) const
    {
        return vec4f_t{ .x = this->x / other.x, .y = this->y / other.y, .z = this->z / other.z, .w = this->w / other.w };
    }
    float dot(const vec4f_t &other) const
    {
        return this->x * other.x + this->y * other.y + this->z * other.z + this->w * other.w;
    }
    vec4f_t max(const vec4f_t &other) const
    {
        return vec4f_t{ .x = std::max(this->x, other.x), .y = std::max(this->y, other.y), .z = std::max(this->z, other.z), .w = std::max(this->w, other.w) };
    }
    void normalize()
    {
        float norm = std::sqrt(this->dot(*this));
        this->x /= norm;
        this->y /= norm;
        this->z /= norm;
        this->w /= norm;
    }
#ifdef INCLUDE_IMGUI
    void imgui_controls(std::string label = "x, y, z, w", float min = -10.f, float max = 10.f, bool color = false)
    {
        ImGui::PushID(this);
        if (color)
            ImGui::ColorEdit4(label.c_str(), &this->x);
        else
            ImGui::SliderFloat4(label.c_str(), &this->x, min, max);
        ImGui::PopID();
    }
    void imgui_window(std::string name = "Vec4f")
    {
        ImGui::Begin(name.c_str());
        this->imgui_controls();
        ImGui::End();
    }
#endif
};

struct simd_vec4f_t
{
    simd::Vec<simd::Float> x, y, z, w = simd::set1(0.f);
    simd_vec4f_t operator+(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ .x = this->x + other.x, .y = this->y + other.y, .z = this->z + other.z, .w = this->w + other.w };
    }
    simd_vec4f_t operator-(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ .x = this->x - other.x, .y = this->y - other.y, .z = this->z - other.z, .w = this->w - other.w };
    }
    simd_vec4f_t operator*(const simd::Vec<simd::Float> &lambda) const
    {
        return simd_vec4f_t{ .x = this->x * lambda, .y = this->y * lambda, .z = this->z * lambda, .w = this->w * lambda };
    }
    simd_vec4f_t operator/(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ .x = this->x / other.x, .y = this->y / other.y, .z = this->z / other.z, .w = this->w / other.w };
    }
    simd::Vec<simd::Float> dot(const simd_vec4f_t &other) const
    {
        return this->x * other.x + this->y * other.y + this->z * other.z + this->w * other.w;
    }
    simd_vec4f_t max(const simd_vec4f_t &other) const
    {
        return simd_vec4f_t{ .x = simd::max(this->x, other.x), .y = simd::max(this->y, other.y), .z = simd::max(this->z, other.z), .w = simd::max(this->w, other.w) };
    }
    void normalize()
    {
        simd::Vec<simd::Float> norm = simd::sqrt(this->dot(*this));
        this->x /= norm;
        this->y /= norm;
        this->z /= norm;
        this->w /= norm;
    }
};

struct gaussian_t
{
    vec4f_t albedo;
    vec4f_t mu;
    float sigma;
    float magnitude;
    float pdf(vec4f_t x) const
    {
        return this->magnitude * _exp(-((x - this->mu).dot(x - this->mu))/(2 * this->sigma * this->sigma));
    }
#ifdef INCLUDE_IMGUI
    void imgui_controls()
    {
        ImGui::PushID(this);
        this->albedo.imgui_controls("albedo", 0.f, 0.f, true);
        this->mu.imgui_controls("mu");
        ImGui::SliderFloat("sigma", &this->sigma, 0.f, 1.f);
        ImGui::SliderFloat("magnitude", &this->magnitude, 0.f, 10.f);
        ImGui::PopID();
    }
    void imgui_window(std::string name = "Gaussian")
    {
        ImGui::Begin(name.c_str());
        this->imgui_controls();
        ImGui::End();
    }
#endif
};

struct simd_gaussian_t
{
    simd_vec4f_t albedo;
    simd_vec4f_t mu;
    simd::Vec<simd::Float> sigma;
    simd::Vec<simd::Float> magnitude;
    simd::Vec<simd::Float> pdf(const simd_vec4f_t x) const
    {
        return this->magnitude * _simd_exp(-((x - this->mu).dot(x - this->mu))/(simd::set1(2.f) * this->sigma * this->sigma));
    }
};
