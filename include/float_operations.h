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

#include <blt/gp/program.h>
#include <functional>
#include <helper.h>

#ifndef IMAGE_GP_6_FLOAT_OPERATIONS_H
#define IMAGE_GP_6_FLOAT_OPERATIONS_H

inline blt::gp::operation_t f_add([](float a, float b) {
    return a + b;
}, "f_add");
inline blt::gp::operation_t f_sub([](float a, float b) {
    return a - b;
}, "f_sub");
inline blt::gp::operation_t f_mul([](float a, float b) {
    return a * b;
}, "f_mul");
inline blt::gp::operation_t f_pro_div([](float a, float b) {
    return b == 0.0f ? 0.0f : (a / b);
}, "f_div");
inline blt::gp::operation_t f_literal([]() {
    return program.get_random().get_float(0.0, 1.0);
}, "float_lit");

template<typename context>
void create_float_operations(blt::gp::operator_builder<context>& builder)
{
//    builder.add_operator(f_add);
//    builder.add_operator(f_sub);
//    builder.add_operator(f_mul);
//    builder.add_operator(f_pro_div);
    builder.add_operator(f_literal, true);
}

#endif //IMAGE_GP_6_FLOAT_OPERATIONS_H
