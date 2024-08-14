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

#ifndef IMAGE_GP_6_CUSTOM_TRANSFORMER_H
#define IMAGE_GP_6_CUSTOM_TRANSFORMER_H

#include <blt/gp/transformers.h>

namespace blt::gp
{
    template<typename T>
    inline static constexpr double sum(const T& array)
    {
        double init = 0.0;
        for (double i : array)
            init += i;
        return init;
    }
    
    template<blt::size_t size, typename... Args>
    static constexpr std::array<double, size> aggregate_array(Args... list)
    {
        std::array<double, size> data {list...};
        auto total_prob = sum(data);
        double sum_of_prob = 0;
        for (auto& d : data) {
            auto prob = d / total_prob;
            d = prob + sum_of_prob;
            sum_of_prob += prob;
        }
        return data;
    }
    
    class image_crossover_t : public crossover_t
    {
        public:
            image_crossover_t() = default;
            
            explicit image_crossover_t(const config_t& config): crossover_t(config)
            {}
            
            blt::expected<result_t, error_t> apply(gp_program& program, const tree_t& p1, const tree_t& p2) final;
    };
    
    class image_mutation_t : public mutation_t
    {
        public:
            enum class mutation_operator : blt::i32
            {
                EXPRESSION,     // Generate a new random expression
                ADJUST,         // adjust the value of the type. (if it is a function it will mutate it to a different one)
                SUB_FUNC,       // subexpression becomes argument to new random function. Other args are generated.
                JUMP_FUNC,      // subexpression becomes this new node. Other arguments discarded.
                COPY,           // node can become copy of another subexpression.
                END,            // helper
            };
            
            image_mutation_t() = default;
            
            explicit image_mutation_t(const config_t& config): mutation_t(config)
            {}
            
            tree_t apply(gp_program& program, const tree_t& p) final;
        
        private:
            static constexpr auto operators_size = static_cast<blt::i32>(mutation_operator::END);
        private:
            // this value is adjusted inversely to the size of the tree.
            double per_node_mutation_chance = 1.0;
            
            static constexpr std::array<double, operators_size> mutation_operator_chances = aggregate_array<operators_size>(
                    0.05,       // EXPRESSION
                    0.1,        // ADJUST
                    0.01,       // SUB_FUNC
                    0.15,       // JUMP_FUNC
                    0.12        // COPY
            );
    };
}

#endif //IMAGE_GP_6_CUSTOM_TRANSFORMER_H
