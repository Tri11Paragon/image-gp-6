/*
 *  <Short Description>
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
#include <custom_transformer.h>
#include <blt/gp/program.h>
#include <images.h>
#include <image_operations.h>
#include <float_operations.h>

namespace blt::gp
{
    
    blt::expected<crossover_t::result_t, crossover_t::error_t> image_crossover_t::apply(gp_program& program, const tree_t& p1, const tree_t& p2)
    {
        return crossover_t::apply(program, p1, p2);
    }
    
    
    tree_t image_mutation_t::apply(gp_program& program, const tree_t& p)
    {
        // child tree
        tree_t c = p;
        
        auto& ops = c.get_operations();
        auto& vals = c.get_values();
        
        double node_mutation_chance = per_node_mutation_chance * (1.0 / static_cast<double>(ops.size()));
        
        for (blt::size_t c_node = 0; c_node < ops.size(); c_node++)
        {
            if (!program.get_random().choice(node_mutation_chance))
                continue;
            auto selected_point = static_cast<blt::i32>(mutation_operator::COPY);
            auto choice = program.get_random().get_double();
            
            for (const auto& [index, value] : blt::enumerate(mutation_operator_chances))
            {
                if (index == 0)
                {
                    if (choice <= value)
                    {
                        selected_point = static_cast<blt::i32>(index);
                        break;
                    }
                } else
                {
                    if (choice > mutation_operator_chances[index - 1] && choice <= value)
                    {
                        selected_point = static_cast<blt::i32>(index);
                        break;
                    }
                }
            }
            
            switch (static_cast<mutation_operator>(selected_point))
            {
                case mutation_operator::EXPRESSION:
                    c_node += mutate_point(program, c, c_node);
                    break;
                case mutation_operator::ADJUST:
                {
                    // this is going to be evil >:3
                    const auto& node = ops[c_node];
                    if (node.is_value)
                    {
                        blt::size_t bytes_from_head = 0;
                        for (auto it = ops.begin() + static_cast<blt::ptrdiff_t>(c_node) + 1; it != ops.end(); it++)
                            bytes_from_head += it->is_value ? stack_allocator::aligned_size(it->type_size) : 0;
                        // is a float
                        if (node.type_size == sizeof(float))
                        {
                            auto& val = vals.from<float>(bytes_from_head);
                            val += f_literal.get_function()();
                            val /= 2.0f;
                        } else if (node.type_size == sizeof(blt::u64)) // is u64
                        {
                            auto& val = vals.from<blt::u64>(bytes_from_head);
                            if (program.get_random().choice())
                                val = program.get_random().get_u64(val, u64_size_max);
                            else
                                val = program.get_random().get_u64(u64_size_min, val + 1);
                        } else // is an image
                        {
                            auto& val = vals.from<full_image_t>(bytes_from_head);
                            auto type = program.get_typesystem().get_type<full_image_t>();
                            auto& terminals = program.get_type_terminals(type.id());
                            
                            // Annoying. TODO: fix all of this.
                            operator_id id;
                            do
                            {
                                id = program.get_random().select(terminals);
                            } while (!program.is_static(id));
                            
                            stack_allocator stack;
                            
                            program.get_operator_info(id).function(nullptr, stack, stack);
                            
                            //auto adjustment = lit.get_function()();
                            auto& adjustment = stack.from<full_image_t>(0);
                            
                            for (const auto& [index, value] : blt::enumerate(val.rgb_data))
                            {
                                // add and normalize.
                                value += adjustment.rgb_data[index];
                                value /= 2.0f;
                            }
                        }
                    }
                }
                    break;
                case mutation_operator::FUNC:
                    break;
                case mutation_operator::SUB_FUNC:
                    break;
                case mutation_operator::JUMP_FUNC:
                    break;
                case mutation_operator::COPY:
                    break;
                case mutation_operator::END:
                default:
#if BLT_DEBUG_LEVEL > 1
                    BLT_ABORT("You shouldn't be able to get here!");
#else
                    BLT_UNREACHABLE;
#endif
            }
        }
        
        return c;
    }
    
}

