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

#include <blt/math/vectors.h>
#include <blt/gp/program.h>
#include <functional>
#include <helper.h>
#include <stb_perlin.h>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#ifndef IMAGE_GP_6_IMAGE_OPERATIONS_H
#define IMAGE_GP_6_IMAGE_OPERATIONS_H

inline blt::gp::operation_t add(make_double(std::plus()), "add");
inline blt::gp::operation_t sub(make_double(std::minus()), "sub");
inline blt::gp::operation_t mul(make_double(std::multiplies()), "mul");
inline blt::gp::operation_t pro_div([](const full_image_t& a, const full_image_t& b) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = b.rgb_data[i] == 0 ? 0 : (a.rgb_data[i] / b.rgb_data[i]);
    return img;
}, "div");
inline blt::gp::operation_t op_sin(make_single([](float a) {
    return (std::sin(a) + 1.0f) / 2.0f;
}), "sin");
inline blt::gp::operation_t op_cos(make_single([](float a) {
    return (std::cos(a) + 1.0f) / 2.0f;
}), "cos");
inline blt::gp::operation_t op_atan(make_single((float (*)(float)) &std::atan), "atan");
inline blt::gp::operation_t op_exp(make_single((float (*)(float)) &std::exp), "exp");
inline blt::gp::operation_t op_abs(make_single((float (*)(float)) &std::abs), "abs");
inline blt::gp::operation_t op_log(make_single((float (*)(float)) &std::log), "log");
inline blt::gp::operation_t op_round(make_single([](float f) { return std::round(f * 255.0f) / 255.0f; }), "round");
inline blt::gp::operation_t op_v_mod([](const full_image_t& a, const full_image_t& b) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = b.rgb_data[i] <= 0 ? 0 : static_cast<float>(blt::mem::type_cast<unsigned int>(a.rgb_data[i]) %
                                                                      blt::mem::type_cast<unsigned int>(b.rgb_data[i]));
    return img;
}, "v_mod");

inline blt::gp::operation_t bitwise_and([](const full_image_t& a, const full_image_t& b) {
    using blt::mem::type_cast;
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = static_cast<float>(type_cast<unsigned int>(a.rgb_data[i]) & type_cast<unsigned int>(b.rgb_data[i]));
    return img;
}, "and");

inline blt::gp::operation_t bitwise_or([](const full_image_t& a, const full_image_t& b) {
    using blt::mem::type_cast;
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = static_cast<float>(type_cast<unsigned int>(a.rgb_data[i]) | type_cast<unsigned int>(b.rgb_data[i]));
    return img;
}, "or");

inline blt::gp::operation_t bitwise_invert([](const full_image_t& a) {
    using blt::mem::type_cast;
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = static_cast<float>(~type_cast<unsigned int>(a.rgb_data[i]));
    return img;
}, "invert");

inline blt::gp::operation_t bitwise_xor([](const full_image_t& a, const full_image_t& b) {
    using blt::mem::type_cast;
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
    {
        auto in_a = type_cast<unsigned int>(a.rgb_data[i]);
        auto in_b = type_cast<unsigned int>(b.rgb_data[i]);
        img.rgb_data[i] = static_cast<float>(in_a ^ in_b);
    }
    return img;
}, "xor");

inline blt::gp::operation_t dissolve([](const full_image_t& a, const full_image_t& b) {
    using blt::mem::type_cast;
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
    {
        auto diff = (a.rgb_data[i] - b.rgb_data[i]) / 2.0f;
        img.rgb_data[i] = a.rgb_data[i] + diff;
    }
    return img;
}, "dissolve");

//inline blt::gp::operation_t band_pass([](const full_image_t& a, blt::u64 lp, blt::u64 hp) {
inline blt::gp::operation_t band_pass([](const full_image_t& a, float fa, float fb, blt::u64 size) {
    cv::Mat src(IMAGE_SIZE, IMAGE_SIZE, CV_32FC3, const_cast<float*>(a.rgb_data));
    full_image_t img{};
    std::memcpy(img.rgb_data, a.rgb_data, DATA_CHANNELS_SIZE * sizeof(float));
    
    cv::Mat dst{IMAGE_SIZE, IMAGE_SIZE, CV_32FC3, img.rgb_data};
    if (size % 2 == 0)
        size++;
    
    auto min = fa < fb ? fa : fb;
    auto max = fa > fb ? fa : fb;
    
    auto low = cv::getGaussianKernel(static_cast<int>(size), min * ((static_cast<int>(size) - 1) * 0.5 - 1) + 0.8, CV_32F);
    auto high = cv::getGaussianKernel(static_cast<int>(size), max * ((static_cast<int>(size) - 1) * 0.5 - 1) + 0.8, CV_32F);
    
    auto func = high - low;
    cv::Mat funcY;
    cv::transpose(func, funcY);
    cv::sepFilter2D(src, dst, 3, func, funcY);
    
    return img;
}, "band_pass");

inline blt::gp::operation_t high_pass([](const full_image_t& a, blt::u64 size) {
    full_image_t blur{};
    full_image_t base{};
    full_image_t ret{};
    std::memcpy(blur.rgb_data, a.rgb_data, DATA_CHANNELS_SIZE * sizeof(float));
    std::memcpy(base.rgb_data, a.rgb_data, DATA_CHANNELS_SIZE * sizeof(float));
    
    
    cv::Mat blur_mat{IMAGE_SIZE, IMAGE_SIZE, CV_32FC3, blur.rgb_data};
    cv::Mat base_mat{IMAGE_SIZE, IMAGE_SIZE, CV_32FC3, base.rgb_data};
    cv::Mat ret_mat{IMAGE_SIZE, IMAGE_SIZE, CV_32FC3, ret.rgb_data};
    if (size % 2 == 0)
        size++;
    
    for (blt::u64 i = 1; i < size; i += 2)
        cv::GaussianBlur(blur_mat, blur_mat, cv::Size(static_cast<int>(i), static_cast<int>(i)), 0, 0);
    
    const static cv::Mat half{IMAGE_SIZE, IMAGE_SIZE, CV_32FC3, 0.5f};
    
    cv::subtract(base_mat, blur_mat, ret_mat);
    cv::add(ret_mat, half, ret_mat);
    
    return ret;
}, "high_pass");

inline blt::gp::operation_t gaussian_blur([](const full_image_t& a, blt::u64 size) {
    full_image_t img{};
    std::memcpy(img.rgb_data, a.rgb_data, DATA_CHANNELS_SIZE * sizeof(float));
    
    cv::Mat dst{IMAGE_SIZE, IMAGE_SIZE, CV_32FC3, img.rgb_data};
    if (size % 2 == 0)
        size++;
    
    for (blt::u64 i = 1; i < size; i += 2)
        cv::GaussianBlur(dst, dst, cv::Size(static_cast<int>(i), static_cast<int>(i)), 0, 0);
    
    return img;
}, "gaussian_blur");

inline blt::gp::operation_t median_blur([](const full_image_t& a, blt::u64 size) {
    cv::Mat src(IMAGE_SIZE, IMAGE_SIZE, CV_32FC3, const_cast<float*>(a.rgb_data));
    full_image_t img{};
    cv::Mat dst{IMAGE_SIZE, IMAGE_SIZE, CV_32FC3, img.rgb_data};
    if (size % 2 == 0)
        size++;
    if (size > 5)
        size = 5;
    cv::medianBlur(src, dst, static_cast<int>(size));
    return img;
}, "median_blur");

inline blt::gp::operation_t bilateral_filter([](const full_image_t& a, blt::u64 size, float color, float space) {
    full_image_t img{};
    cv::Mat src(IMAGE_SIZE, IMAGE_SIZE, CV_32FC3, const_cast<float*>(a.rgb_data));
    cv::Mat dst{IMAGE_SIZE, IMAGE_SIZE, CV_32FC3, img.rgb_data};
    if (size % 2 == 0)
        size++;
    cv::bilateralFilter(src, dst, static_cast<int>(size), color * static_cast<double>(size) * 2.0, space * static_cast<double>(size) * 2.0);
    return img;
}, "bilateral_filter");

inline blt::gp::operation_t l_system([](const full_image_t& a) {
    return a;
}, "l_system");

inline blt::gp::operation_t hsv_to_rgb([](const full_image_t& a) {
    using blt::mem::type_cast;
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto h = static_cast<blt::i32>(a.rgb_data[i * CHANNELS + 0]) % 360;
        auto s = a.rgb_data[i * CHANNELS + 1];
        auto v = a.rgb_data[i * CHANNELS + 2];
        auto c = v * s;
        auto x = c * static_cast<float>(1 - std::abs(((h / 60) % 2) - 1));
        auto m = v - c;
        
        blt::vec3 rgb;
        if (h >= 0 && h < 60)
            rgb = {c, x, 0.0f};
        else if (h >= 60 && h < 120)
            rgb = {x, c, 0.0f};
        else if (h >= 120 && h < 180)
            rgb = {0.0f, c, x};
        else if (h >= 180 && h < 240)
            rgb = {0.0f, x, c};
        else if (h >= 240 && h < 300)
            rgb = {x, 0.0f, c};
        else if (h >= 300 && h < 360)
            rgb = {c, 0.0f, x};
        
        img.rgb_data[i * CHANNELS] = rgb.x() + m;
        img.rgb_data[i * CHANNELS + 1] = rgb.y() + m;
        img.rgb_data[i * CHANNELS + 2] = rgb.z() + m;
    }
    return img;
}, "hsv");

inline auto lit = blt::gp::operation_t([]() {
    full_image_t img{};
    auto bw = program.get_random().get_float(0.0f, 1.0f);
    for (auto& i : img.rgb_data)
        i = bw;
    return img;
}, "lit").set_ephemeral();
inline auto vec = blt::gp::operation_t([]() {
    full_image_t img{};
    auto r = program.get_random().get_float(0.0f, 1.0f);
    auto g = program.get_random().get_float(0.0f, 1.0f);
    auto b = program.get_random().get_float(0.0f, 1.0f);
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        img.rgb_data[i * CHANNELS] = r;
        img.rgb_data[i * CHANNELS + 1] = g;
        img.rgb_data[i * CHANNELS + 2] = b;
    }
    return img;
}, "vec").set_ephemeral();
inline blt::gp::operation_t random_val([]() {
    full_image_t img{};
    for (auto& i : img.rgb_data)
        i = program.get_random().get_float(0.0f, 1.0f);
    return img;
}, "color_noise");
inline blt::gp::operation_t perlin([](const full_image_t& x, const full_image_t& y, const full_image_t& z, const full_image_t& scale) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
    {
        auto s = scale.rgb_data[i];
        img.rgb_data[i] = perlin_noise(x.rgb_data[i] / s, y.rgb_data[i] / s, z.rgb_data[i] / s);
    }
    return img;
}, "perlin");
inline blt::gp::operation_t perlin_terminal([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
    {
        auto ctx = get_ctx(i);
        img.rgb_data[i] = perlin_noise(ctx.x / IMAGE_SIZE, ctx.y / IMAGE_SIZE, static_cast<float>(i % CHANNELS) / CHANNELS);
    }
    return img;
}, "perlin_term");
inline blt::gp::operation_t perlin_warped([](const full_image_t& u, const full_image_t& v) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
    {
        auto ctx = get_ctx(i);
        img.rgb_data[i] = perlin_noise((ctx.x + +u.rgb_data[i]) / IMAGE_SIZE, (ctx.y + v.rgb_data[i]) / IMAGE_SIZE,
                                       static_cast<float>(i % CHANNELS) / CHANNELS);
    }
    return img;
}, "perlin_warped");
inline blt::gp::operation_t op_img_size([]() {
    full_image_t img{};
    for (float& i : img.rgb_data)
    {
        i = IMAGE_SIZE;
    }
    return img;
}, "img_size");
inline blt::gp::operation_t op_x_r([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).x;
        img.rgb_data[i * CHANNELS] = ctx;
        img.rgb_data[i * CHANNELS + 1] = 0;
        img.rgb_data[i * CHANNELS + 2] = 0;
    }
    return img;
}, "x_r");
inline blt::gp::operation_t op_x_g([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).x;
        img.rgb_data[i * CHANNELS] = 0;
        img.rgb_data[i * CHANNELS + 1] = ctx;
        img.rgb_data[i * CHANNELS + 2] = 0;
    }
    return img;
}, "x_g");
inline blt::gp::operation_t op_x_b([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).x;
        img.rgb_data[i * CHANNELS] = 0;
        img.rgb_data[i * CHANNELS + 1] = 0;
        img.rgb_data[i * CHANNELS + 2] = ctx;
    }
    return img;
}, "x_b");
inline blt::gp::operation_t op_x_rgb([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).x;
        img.rgb_data[i * CHANNELS] = ctx;
        img.rgb_data[i * CHANNELS + 1] = ctx;
        img.rgb_data[i * CHANNELS + 2] = ctx;
    }
    return img;
}, "x_rgb");
inline blt::gp::operation_t op_y_r([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).y;
        img.rgb_data[i * CHANNELS] = ctx;
        img.rgb_data[i * CHANNELS + 1] = 0;
        img.rgb_data[i * CHANNELS + 2] = 0;
    }
    return img;
}, "y_r");
inline blt::gp::operation_t op_y_g([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).y;
        img.rgb_data[i * CHANNELS] = 0;
        img.rgb_data[i * CHANNELS + 1] = ctx;
        img.rgb_data[i * CHANNELS + 2] = 0;
    }
    return img;
}, "y_g");
inline blt::gp::operation_t op_y_b([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).y;
        img.rgb_data[i * CHANNELS] = 0;
        img.rgb_data[i * CHANNELS + 1] = 0;
        img.rgb_data[i * CHANNELS + 2] = ctx;
    }
    return img;
}, "y_b");
inline blt::gp::operation_t op_y_rgb([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).y;
        img.rgb_data[i * CHANNELS] = ctx;
        img.rgb_data[i * CHANNELS + 1] = ctx;
        img.rgb_data[i * CHANNELS + 2] = ctx;
    }
    return img;
}, "y_rgb");

template<typename context>
void create_image_operations(blt::gp::operator_builder<context>& builder)
{
    
    // idk when it got enabled but this works on 4.10

}

#endif //IMAGE_GP_6_IMAGE_OPERATIONS_H
