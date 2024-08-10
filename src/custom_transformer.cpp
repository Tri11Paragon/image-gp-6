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
    
    template<typename>
    static blt::u8* get_thread_pointer_for_size(blt::size_t bytes)
    {
        static thread_local blt::expanding_buffer<blt::u8> buffer;
        if (bytes > buffer.size())
            buffer.resize(bytes);
        return buffer.data();
    }
    
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
            
            // select an operator to apply
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
                    if (!node.is_value)
                    {
                        BLT_TRACE("Running adjust on function");
                        auto& current_func_info = program.get_operator_info(ops[c_node].id);
                        operator_id random_replacement = program.get_random().select(
                                program.get_type_non_terminals(current_func_info.return_type.id));
                        auto& replacement_func_info = program.get_operator_info(random_replacement);
                        
                        struct child_t
                        {
                            blt::ptrdiff_t start;
                            // one past the end
                            blt::ptrdiff_t end;
                        };
                        
                        // cache memory used for offset data.
                        thread_local static std::vector<child_t> children_data;
                        children_data.clear();
                        
                        while (children_data.size() < current_func_info.argument_types.size())
                        {
                            auto current_point = children_data.size();
                            child_t prev{};
                            if (current_point == 0)
                            {
                                // first child.
                                prev = {static_cast<blt::ptrdiff_t>(c_node + 1),
                                        c.find_endpoint(program, static_cast<blt::ptrdiff_t>(c_node + 1))};
                                children_data.push_back(prev);
                                continue;
                            } else
                                prev = children_data[current_point - 1];
                            child_t next = {prev.end, c.find_endpoint(program, prev.end)};
                            children_data.push_back(next);
                        }
                        
                        BLT_TRACE("%ld vs %ld, replacement will be size %ld", children_data.size(), current_func_info.argument_types.size(),
                                  replacement_func_info.argument_types.size());
                        
                        for (const auto& [index, val] : blt::enumerate(replacement_func_info.argument_types))
                        {
                            // need to generate replacement.
                            if (index < current_func_info.argument_types.size() && val.id != current_func_info.argument_types[index].id)
                            {
                                BLT_TRACE_STREAM << "Replacing tree argument from type "
                                                 << program.get_typesystem().get_type(current_func_info.argument_types[index]).name() << " to type "
                                                 << program.get_typesystem().get_type(val).name() << "\n";
                                // TODO: new config?
                                auto tree = config.generator.get().generate(
                                        {program, val.id, config.replacement_min_depth, config.replacement_max_depth});
                                blt::size_t total_bytes_after = 0;
                                blt::size_t total_bytes_for = 0;
                                
                                auto& child = children_data[index];
                                for (blt::ptrdiff_t i = child.start; i < child.end; i++)
                                {
                                    if (ops[i].is_value)
                                        total_bytes_for += stack_allocator::aligned_size(ops[i].type_size);
                                }
                                for (blt::size_t i = child.end; i < ops.size(); i++)
                                {
                                    if (ops[i].is_value)
                                        total_bytes_after += stack_allocator::aligned_size(ops[i].type_size);
                                }
                                BLT_TRACE("Size for %ld size after: %ld", total_bytes_for, total_bytes_after);
                                
                                auto after_ptr = get_thread_pointer_for_size<struct mutation_func>(total_bytes_after);
                                vals.copy_to(after_ptr, total_bytes_after);
                                vals.pop_bytes(static_cast<blt::ptrdiff_t>(total_bytes_after + total_bytes_for));
                                
                                blt::size_t total_child_bytes = 0;
                                for (const auto& v : tree.get_operations())
                                {
                                    if (v.is_value)
                                        total_child_bytes += stack_allocator::aligned_size(v.type_size);
                                }
                                
                                BLT_TRACE("Copying %ld bytes back into stack", total_child_bytes);
                                
                                vals.copy_from(tree.get_values(), total_child_bytes);
                                vals.copy_from(after_ptr, total_bytes_after);
                                
                                ops.erase(ops.begin() + child.start, ops.begin() + child.end);
                                ops.insert(ops.begin() + child.start, tree.get_operations().begin(), tree.get_operations().end());
                                
                                // shift over everybody after.
                                for (auto& new_child : blt::iterate(children_data.begin() + static_cast<blt::ptrdiff_t>(index), children_data.end()))
                                {
                                    // remove the old tree size, then add the new tree size to get the correct positions.
                                    new_child.start =
                                            new_child.start - (child.end - child.start) + static_cast<blt::ptrdiff_t>(tree.get_operations().size());
                                    new_child.end =
                                            new_child.end - (child.end - child.start) + static_cast<blt::ptrdiff_t>(tree.get_operations().size());
                                }
                                child.end = static_cast<blt::ptrdiff_t>(child.start + tree.get_operations().size());

#if BLT_DEBUG_LEVEL >= 2
                                blt::size_t found_bytes = vals.size().total_used_bytes;
                                blt::size_t expected_bytes = std::accumulate(ops.begin(),
                                                                             ops.end(), 0ul,
                                                                             [](const auto& v1, const auto& v2) {
                                                                                 if (v2.is_value)
                                                                                     return v1 + stack_allocator::aligned_size(v2.type_size);
                                                                                 return v1;
                                                                             });
                                if (found_bytes != expected_bytes)
                                {
                                    BLT_WARN("Found bytes %ld vs Expected Bytes %ld", found_bytes, expected_bytes);
                                    BLT_ABORT("Amount of bytes in stack doesn't match the number of bytes expected for the operations");
                                }
#endif
                            }
                        }
                        
                        if (current_func_info.argc.argc > replacement_func_info.argc.argc)
                        {
                            BLT_TRACE("TOO MANY ARGS");
                            // too many args
                            blt::size_t end_index = children_data.back().end;
                            blt::size_t start_index = children_data[replacement_func_info.argc.argc].start;
                            blt::size_t total_bytes_for = 0;
                            blt::size_t total_bytes_after = 0;
                            for (blt::size_t i = start_index; i < end_index; i++)
                            {
                                if (ops[i].is_value)
                                    total_bytes_for += stack_allocator::aligned_size(ops[i].type_size);
                            }
                            for (blt::size_t i = end_index; i < ops.size(); i++)
                            {
                                if (ops[i].is_value)
                                    total_bytes_after += stack_allocator::aligned_size(ops[i].type_size);
                            }
                            auto* data = get_thread_pointer_for_size<struct mutation_func>(total_bytes_after);
                            vals.copy_to(data, total_bytes_after);
                            vals.pop_bytes(static_cast<blt::ptrdiff_t>(total_bytes_after + total_bytes_for));
                            vals.copy_from(data, total_bytes_after);
                            ops.erase(ops.begin() + static_cast<blt::ptrdiff_t>(start_index), ops.begin() + static_cast<blt::ptrdiff_t>(end_index));
                        } else if (current_func_info.argc.argc == replacement_func_info.argc.argc)
                        {
                            BLT_TRACE("JUST ENOUGH ARGS");
                            // exactly enough args
                            // return types should have been replaced if needed. this part should do nothing?
                        } else
                        {
                            BLT_TRACE("NOT ENOUGH ARGS");
                            // not enough args
                            blt::size_t total_bytes_after = 0;
                            blt::size_t start_index = c_node + 1;
                            if (current_func_info.argc.argc != 0)
                                start_index = children_data.back().end;
                            for (blt::size_t i = start_index; i < ops.size(); i++)
                            {
                                if (ops[i].is_value)
                                    total_bytes_after += stack_allocator::aligned_size(ops[i].type_size);
                            }
                            auto* data = get_thread_pointer_for_size<struct mutation_func>(total_bytes_after);
                            vals.copy_to(data, total_bytes_after);
                            vals.pop_bytes(static_cast<blt::ptrdiff_t>(total_bytes_after));
                            
                            for (blt::size_t i = current_func_info.argc.argc; i < replacement_func_info.argc.argc; i++)
                            {
                                BLT_TRACE("Generating argument %ld", i);
                                auto tree = config.generator.get().generate(
                                        {program, replacement_func_info.argument_types[i].id, config.replacement_min_depth,
                                         config.replacement_max_depth});
                                blt::size_t total_bytes_for = 0;
                                for (const auto& op : tree.get_operations())
                                {
                                    if (op.is_value)
                                        total_bytes_for += stack_allocator::aligned_size(op.type_size);
                                }
                                vals.copy_from(tree.get_values(), total_bytes_for);
                                ops.insert(ops.begin() + static_cast<blt::ptrdiff_t>(start_index), tree.get_operations().begin(),
                                           tree.get_operations().end());
                                start_index += tree.get_operations().size();
                            }
                            vals.copy_from(data, total_bytes_after);
                        }
                        // now finally update the type.
                        ops[c_node] = {replacement_func_info.function, program.get_typesystem().get_type(replacement_func_info.return_type).size(),
                                       random_replacement, false};
                    } else
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
                        } else if (node.type_size == sizeof(full_image_t))// is an image
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
                        } else
                        {
                            BLT_ABORT("This type size doesn't exist!");
                        }
                    }
                }
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

#if BLT_DEBUG_LEVEL >= 2
        blt::size_t bytes_expected = 0;
        auto bytes_size = vals.size().total_used_bytes;
        
        for (const auto& op : c.get_operations())
        {
            if (op.is_value)
                bytes_expected += stack_allocator::aligned_size(op.type_size);
        }
        
        if (bytes_expected != bytes_size)
        {
            BLT_WARN_STREAM << "Stack state: " << vals.size() << "\n";
            BLT_WARN("Child tree bytes %ld vs expected %ld, difference: %ld", bytes_size, bytes_expected,
                     static_cast<blt::ptrdiff_t>(bytes_expected) - static_cast<blt::ptrdiff_t>(bytes_size));
            BLT_ABORT("Amount of bytes in stack doesn't match the number of bytes expected for the operations");
        }
        auto copy = c;
        try
        {
            auto result = copy.evaluate(nullptr);
            blt::black_box(result);
        } catch (const std::exception& e)
        {
            std::cout << "Parent: " << std::endl;
            p.print(program, std::cout, false, true);
            std::cout << "Child:" << std::endl;
            c.print(program, std::cout, false, true);
            std::cout << std::endl;
            c.print(program, std::cout, true, true);
            std::cout << std::endl;
            BLT_WARN(e.what());
            throw e;
        }
#endif
        
        BLT_TRACE("- - - - - - - - - Passed and finished - - - - - - - - -");
        
        return c;
    }
    
}

