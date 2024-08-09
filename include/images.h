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
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <stb_image_write.h>

struct stb_image_t
{
    public:
        stb_image_t() = default;
        
        stb_image_t(const stb_image_t& copy) noexcept: width(copy.width), height(copy.height), channels(copy.channels)
        {
            data = static_cast<float*>(std::malloc(width * height * channels));
            std::memcpy(data, copy.data, width * height * channels);
        }
        
        stb_image_t(stb_image_t&& move) noexcept:
                width(move.width), height(move.height), channels(move.channels), data(std::exchange(move.data, nullptr))
        {}
        
        stb_image_t& operator=(const stb_image_t& copy)
        {
            if (&copy == this)
                return *this;
            if (width != copy.width || height != copy.height || channels != copy.channels)
            {
                width = copy.width;
                height = copy.height;
                channels = copy.channels;
                stbi_image_free(data);
                data = static_cast<float*>(std::malloc(width * height * channels * sizeof(float)));
            }
            std::memcpy(data, copy.data, width * height * channels);
            
            return *this;
        }
        
        stb_image_t& operator=(stb_image_t&& move) noexcept
        {
            width = move.width;
            height = move.height;
            channels = move.channels;
            data = std::exchange(move.data, data);
            return *this;
        }
        
        stb_image_t& load(const std::string& path)
        {
            data = stbi_loadf(path.c_str(), &width, &height, &channels, CHANNELS);
            channels = CHANNELS;
            return *this;
        }
        
        stb_image_t& save(const std::string& str)
        {
            if (data == nullptr)
                return *this;
            auto uc_data = std::unique_ptr<unsigned char[]>(new unsigned char[width * height * channels]);
            for (int i = 0; i < width * height * channels; i++)
                uc_data[i] = static_cast<unsigned char>(data[i] * std::numeric_limits<unsigned char>::max());
            stbi_write_png(str.c_str(), width, height, channels, uc_data.get(), 0);
            return *this;
        }
        
        stb_image_t& resize(int new_width, int new_height)
        {
            auto new_data = static_cast<float*>(malloc(new_width * new_height * channels * sizeof(float)));
            stbir_resize_float_linear(data, width, height, 0, new_data, new_width, new_height, 0,
                                      static_cast<stbir_pixel_layout>(CHANNELS));
            stbi_image_free(data);
            data = new_data;
            width = new_width;
            height = new_height;
            return *this;
        }
        
        [[nodiscard]] int get_width() const
        {
            return width;
        }
        
        [[nodiscard]] int get_height() const
        {
            return height;
        }
        
        float* get_data()
        {
            return data;
        }
        
        [[nodiscard]] const float* get_data() const
        {
            return data;
        }
        
        ~stb_image_t()
        {
            stbi_image_free(data);
            data = nullptr;
        }
    
    private:
        int width{}, height{}, channels{};
        
        float* data = nullptr;
};

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
    
    void load(const stb_image_t& image)
    {
        stbir_resize_float_linear(image.get_data(), image.get_width(), image.get_height(), 0, rgb_data, IMAGE_SIZE, IMAGE_SIZE, 0,
                                  static_cast<stbir_pixel_layout>(CHANNELS));
    }
    
    void save(const std::string& str)
    {
        unsigned char data[DATA_SIZE * CHANNELS]{};
        for (const auto& [index, value] : blt::enumerate(rgb_data))
            data[index] = static_cast<unsigned char>(value * std::numeric_limits<unsigned char>::max());
        stbi_write_png(str.c_str(), IMAGE_SIZE, IMAGE_SIZE, CHANNELS, data, 0);
    }
    
    friend std::ostream& operator<<(std::ostream& str, const full_image_t&)
    {
        return str;
    }
};

#endif //IMAGE_GP_6_IMAGES_H
