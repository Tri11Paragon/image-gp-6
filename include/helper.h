#pragma once
/*
 *  Copyright (C) 2024  Brett Terpstra
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef IMAGE_GP_6_HELPER_H
#define IMAGE_GP_6_HELPER_H

#include <images.h>

template<typename SINGLE_FUNC>
constexpr static auto make_single(SINGLE_FUNC&& func)
{
    return [func](const full_image_t& a) {
        full_image_t img{};
        for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
            img.rgb_data[i] = func(a.rgb_data[i]);
        return img;
    };
}

template<typename DOUBLE_FUNC>
constexpr static auto make_double(DOUBLE_FUNC&& func)
{
    return [func](const full_image_t& a, const full_image_t& b) {
        full_image_t img{};
        for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
            img.rgb_data[i] = func(a.rgb_data[i], b.rgb_data[i]);
        return img;
    };
}

struct context
{
    float x, y;
};

inline context get_ctx(blt::size_t i)
{
    context ctx{};
    i /= CHANNELS;
    ctx.y = std::floor(static_cast<float>(i) / static_cast<float>(IMAGE_SIZE));
    ctx.x = static_cast<float>(i) - (ctx.y * IMAGE_SIZE);
    return ctx;
}

inline context get_pop_ctx(blt::size_t i)
{
    auto const sq = static_cast<float>(std::sqrt(POP_SIZE));
    context ctx{};
    ctx.y = std::floor(static_cast<float>(i) / static_cast<float>(sq));
    ctx.x = static_cast<float>(i) - (ctx.y * sq);
    return ctx;
}

inline blt::size_t get_index(blt::size_t x, blt::size_t y)
{
    return y * IMAGE_SIZE + x;
}

inline float perlin_noise(float x, float y, float z)
{
    return (stb_perlin_noise3(x, y, z, 0, 0, 0) + 1.0f) / 2.0f;
}

#endif //IMAGE_GP_6_HELPER_H
