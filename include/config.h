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

#ifndef IMAGE_GP_6_CONFIG_H
#define IMAGE_GP_6_CONFIG_H

constexpr size_t log2(size_t n) // NOLINT
{
    return ((n < 2) ? 1 : 1 + log2(n / 2));
}

static const blt::u64 SEED = std::random_device()();
static constexpr blt::size_t IMAGE_SIZE = 128;
static constexpr blt::size_t IMAGE_PADDING = 16;
static constexpr blt::size_t POP_SIZE = 64;
static constexpr blt::size_t CHANNELS = 3;
static constexpr blt::size_t DATA_SIZE = IMAGE_SIZE * IMAGE_SIZE;
static constexpr blt::size_t DATA_CHANNELS_SIZE = DATA_SIZE * CHANNELS;
static constexpr blt::size_t BOX_COUNT = static_cast<blt::size_t>(log2(IMAGE_SIZE / 2));
static constexpr float THRESHOLD = 0.3;
static constexpr auto load_image = "../GSab4SWWcAA1TNR.png";

blt::gp::prog_config_t config = blt::gp::prog_config_t()
        .set_initial_min_tree_size(4)
        .set_initial_max_tree_size(8)
        .set_elite_count(2)
        .set_max_generations(50)
        .set_mutation_chance(1.0)
        .set_crossover_chance(1.0)
        .set_reproduction_chance(0.5)
        .set_pop_size(POP_SIZE)
        .set_thread_count(0);

#endif //IMAGE_GP_6_CONFIG_H
