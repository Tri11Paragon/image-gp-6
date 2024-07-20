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

#ifndef IMAGE_GP_6_IMAGES_H
#define IMAGE_GP_6_IMAGES_H

#include <config.h>

struct full_image_t
{
    float rgb_data[DATA_SIZE * CHANNELS]{};
    
    full_image_t()
    {
        for (auto& v : rgb_data)
            v = 0;
    }
    
    void load(const std::string& path)
    {
        int width, height, channels;
        auto data = stbi_loadf(path.c_str(), &width, &height, &channels, CHANNELS);
        
        stbir_resize_float_linear(data, width, height, 0, rgb_data, IMAGE_SIZE, IMAGE_SIZE, 0, static_cast<stbir_pixel_layout>(CHANNELS));
        
        stbi_image_free(data);
    }
    
    void save(const std::string&)
    {
        //stbi_write_png(str.c_str(), IMAGE_SIZE, IMAGE_SIZE, CHANNELS, rgb_data, 0);
    }
    
    friend std::ostream& operator<<(std::ostream& str, const full_image_t&)
    {
        return str;
    }
};

#endif //IMAGE_GP_6_IMAGES_H
