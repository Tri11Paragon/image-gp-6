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

#if BLT_DEBUG_LEVEL >= 2
            tree_t c_copy = c;
#endif
            
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
//                        BLT_TRACE("Running adjust on function");
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
//                                BLT_TRACE("Child %ld: %s", current_point, std::string(*program.get_name(ops[c_node + 1].id)).c_str());
                                // first child.
                                prev = {static_cast<blt::ptrdiff_t>(c_node + 1),
                                        c.find_endpoint(program, static_cast<blt::ptrdiff_t>(c_node + 1))};
                                children_data.push_back(prev);
                                continue;
                            } else
                                prev = children_data[current_point - 1];
//                            BLT_TRACE("Child %ld: %s", current_point, std::string(*program.get_name(ops[prev.end].id)).c_str());
                            child_t next = {prev.end, c.find_endpoint(program, prev.end)};
                            children_data.push_back(next);
                        }

//                        BLT_TRACE("%ld vs %ld, replacement will be size %ld", children_data.size(), current_func_info.argument_types.size(),
//                                  replacement_func_info.argument_types.size());
                        
                        for (const auto& [index, val] : blt::enumerate(replacement_func_info.argument_types))
                        {
                            // need to generate replacement.
                            if (index < current_func_info.argument_types.size() && val.id != current_func_info.argument_types[index].id)
                            {
//                                BLT_TRACE_STREAM << "Replacing tree argument (index: " << index << " proper: " << (children_data.size() - 1 - index)
//                                                 << ") from type "
//                                                 << program.get_typesystem().get_type(current_func_info.argument_types[index]).name() << " to type "
//                                                 << program.get_typesystem().get_type(val).name() << "\n";
                                // TODO: new config?
                                auto tree = config.generator.get().generate(
                                        {program, val.id, config.replacement_min_depth, config.replacement_max_depth});
                                
                                auto& child = children_data[children_data.size() - 1 - index];
                                blt::size_t total_bytes_for = c.total_value_bytes(child.start, child.end);
                                blt::size_t total_bytes_after = c.total_value_bytes(child.end);
//                                BLT_TRACE("Removing bytes %ld, bytes after that must stay: %ld", total_bytes_for, total_bytes_after);
                                
                                auto after_ptr = get_thread_pointer_for_size<struct mutation_func>(total_bytes_after);
                                vals.copy_to(after_ptr, total_bytes_after);
                                vals.pop_bytes(static_cast<blt::ptrdiff_t>(total_bytes_after + total_bytes_for));
                                
                                blt::size_t total_child_bytes = tree.total_value_bytes();
                                
//                                BLT_TRACE("Copying %ld bytes back into stack", total_child_bytes);
                                
                                vals.copy_from(tree.get_values(), total_child_bytes);
                                vals.copy_from(after_ptr, total_bytes_after);
                                
                                ops.erase(ops.begin() + child.start, ops.begin() + child.end);
                                ops.insert(ops.begin() + child.start, tree.get_operations().begin(), tree.get_operations().end());
                                
                                // shift over everybody after.
                                if (index > 0)
                                {
                                    // don't need to update if the index is the last
                                    for (auto& new_child : blt::iterate(children_data.end() - static_cast<blt::ptrdiff_t>(index),
                                                                        children_data.end()))
                                    {
                                        // remove the old tree size, then add the new tree size to get the correct positions.
                                        new_child.start =
                                                new_child.start - (child.end - child.start) +
                                                static_cast<blt::ptrdiff_t>(tree.get_operations().size());
                                        new_child.end =
                                                new_child.end - (child.end - child.start) + static_cast<blt::ptrdiff_t>(tree.get_operations().size());
                                    }
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

//                        BLT_DEBUG("Current:");
//                        for (const auto& [index, val] : blt::enumerate(current_func_info.argument_types))
//                            BLT_DEBUG("%ld: %s", index, std::string(program.get_typesystem().get_type(val).name()).c_str());
//
//                        BLT_DEBUG("Replacement:");
//                        for (const auto& [index, val] : blt::enumerate(replacement_func_info.argument_types))
//                            BLT_DEBUG("%ld: %s", index, std::string(program.get_typesystem().get_type(val).name()).c_str());
                        
                        if (current_func_info.argc.argc > replacement_func_info.argc.argc)
                        {
//                            BLT_TRACE("TOO MANY ARGS");
                            // too many args
//                            BLT_TRACE("Removing %s starting index 0 to %ld",
//                                      std::string(*program.get_name(ops[children_data.front().start].id)).c_str(), (current_func_info.argc.argc -
//                                                                                                                    replacement_func_info.argc.argc) - 1);
                            blt::size_t end_index = children_data[(current_func_info.argc.argc - replacement_func_info.argc.argc) - 1].end;
                            blt::size_t start_index = children_data.begin()->start;
                            blt::size_t total_bytes_for = c.total_value_bytes(start_index, end_index);
                            blt::size_t total_bytes_after = c.total_value_bytes(end_index);
                            auto* data = get_thread_pointer_for_size<struct mutation_func>(total_bytes_after);
                            vals.copy_to(data, total_bytes_after);
                            vals.pop_bytes(static_cast<blt::ptrdiff_t>(total_bytes_after + total_bytes_for));
                            vals.copy_from(data, total_bytes_after);
                            ops.erase(ops.begin() + static_cast<blt::ptrdiff_t>(start_index), ops.begin() + static_cast<blt::ptrdiff_t>(end_index));
                        } else if (current_func_info.argc.argc == replacement_func_info.argc.argc)
                        {
//                            BLT_TRACE("JUST ENOUGH ARGS");
                            // exactly enough args
                            // return types should have been replaced if needed. this part should do nothing?
                        } else
                        {
//                            BLT_TRACE("NOT ENOUGH ARGS");
                            // not enough args
                            blt::size_t start_index = c_node + 1;
                            blt::size_t total_bytes_after = c.total_value_bytes(start_index);
                            //if (current_func_info.argc.argc != 0)
                            //    start_index = children_data.back().end;
//                            BLT_TRACE("Total bytes after: %ld", total_bytes_after);
                            auto* data = get_thread_pointer_for_size<struct mutation_func>(total_bytes_after);
                            vals.copy_to(data, total_bytes_after);
                            vals.pop_bytes(static_cast<blt::ptrdiff_t>(total_bytes_after));
                            
                            for (blt::ptrdiff_t i = static_cast<blt::ptrdiff_t>(replacement_func_info.argc.argc) - 1;
                                 i >= current_func_info.argc.argc; i--)
                            {
//                                BLT_TRACE("Generating argument %ld of type %s", i,
//                                          std::string(program.get_typesystem().get_type(replacement_func_info.argument_types[i].id).name()).c_str());
                                auto tree = config.generator.get().generate(
                                        {program, replacement_func_info.argument_types[i].id, config.replacement_min_depth,
                                         config.replacement_max_depth});
                                blt::size_t total_bytes_for = tree.total_value_bytes();
                                vals.copy_from(tree.get_values(), total_bytes_for);
//                                BLT_TRACE("%ld vs %ld", start_index, ops.size());
//                                if (start_index < ops.size())
//                                    BLT_TRACE("Argument %ld insert before this: %s", i, std::string(*program.get_name(ops[start_index].id)).c_str());
                                ops.insert(ops.begin() + static_cast<blt::ptrdiff_t>(start_index), tree.get_operations().begin(),
                                           tree.get_operations().end());
                                start_index += tree.get_operations().size();
                            }
                            vals.copy_from(data, total_bytes_after);
                        }
                        // now finally update the type.
                        ops[c_node] = {replacement_func_info.function, program.get_typesystem().get_type(replacement_func_info.return_type).size(),
                                       random_replacement, program.is_static(random_replacement)};
                    } else
                    {
                        blt::size_t bytes_from_head = c.total_value_bytes(c_node + 1);
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
                            
                            program.get_operator_info(id).function(nullptr, stack, stack, nullptr);
                            
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
#if BLT_DEBUG_LEVEL >= 2
                    if (!c.check(program, nullptr))
                    {
                        std::cout << "Parent: " << std::endl;
                        c_copy.print(program, std::cout, false, true);
                        std::cout << "Child Values:" << std::endl;
                        c.print(program, std::cout, true, true);
                        std::cout << std::endl;
                        BLT_ABORT("Tree Check Failed.");
                    }
#endif
                }
                    break;
                case mutation_operator::SUB_FUNC:
                {
                    BLT_TRACE(ops[c_node].id);
                    BLT_TRACE(std::string(*program.get_name(ops[c_node].id)).c_str());
                    auto& current_func_info = program.get_operator_info(ops[c_node].id);

                    // need to:
                    // mutate the current function.
                    // current function is moved to one of the arguments.
                    // other arguments are generated.

                    // get a replacement which returns the same type.
                    auto& non_terminals = program.get_type_non_terminals(current_func_info.return_type.id);
                    if (non_terminals.empty())
                        continue;
                    operator_id random_replacement = program.get_random().select(non_terminals);
                    blt::size_t arg_position = 0;
                    do
                    {
                        auto& replacement_func_info = program.get_operator_info(random_replacement);
                        for (const auto& [index, v] : blt::enumerate(replacement_func_info.argument_types))
                        {
                            if (v.id == current_func_info.return_type.id)
                            {
                                arg_position = index;
                                goto exit;
                            }
                        }
                        random_replacement = program.get_random().select(program.get_type_non_terminals(current_func_info.return_type.id));
                    } while (true);
                exit:
                    auto& replacement_func_info = program.get_operator_info(random_replacement);
                    // replacement function should be valid. let's make a copy of us.
                    auto current_end = c.find_endpoint(program, static_cast<blt::ptrdiff_t>(c_node));
                    blt::size_t for_bytes = c.total_value_bytes(c_node, current_end);
                    blt::size_t after_bytes = c.total_value_bytes(current_end);
                    
                    auto after_data =
                }
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
        if (!c.check(program, nullptr))
        {
            std::cout << "Parent: " << std::endl;
            p.print(program, std::cout, false, true);
            std::cout << "Child Values:" << std::endl;
            c.print(program, std::cout, true, true);
            std::cout << std::endl;
            BLT_ABORT("Tree Check Failed.");
        }
#endif
        
        BLT_TRACE("- - - - - - - - - Passed and finished - - - - - - - - -");
        
        return c;
    }
    
}

