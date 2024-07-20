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

#endif //IMAGE_GP_6_HELPER_H
