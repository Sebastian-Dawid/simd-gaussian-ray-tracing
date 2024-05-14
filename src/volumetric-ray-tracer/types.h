#pragma once

#include <cmath>

#include <fmt/core.h>
#include <fmt/format.h>
#include <imgui.h>
#include <string>

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
};

struct gaussian_t
{
    vec4f_t albedo;
    vec4f_t mu;
    float sigma;
    float magnitude;
    float pdf(vec4f_t x) const
    {
        return this->magnitude * std::exp(-((x - this->mu).dot(x - this->mu))/(2 * std::pow(this->sigma, 2.f)));
    }
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
};
