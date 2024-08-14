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
        
        for (blt::size_t c_node = 0; c_node < ops.size(); c_node++)
        {
            double node_mutation_chance = per_node_mutation_chance / static_cast<double>(ops.size());
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
                        auto& current_func_info = program.get_operator_info(ops[c_node].id);
                        operator_id random_replacement = program.get_random().select(
                                program.get_type_non_terminals(current_func_info.return_type.id));
                        auto& replacement_func_info = program.get_operator_info(random_replacement);
                        
                        // cache memory used for offset data.
                        thread_local static std::vector<tree_t::child_t> children_data;
                        children_data.clear();
                        
                        c.find_child_extends(program, children_data, c_node, current_func_info.argument_types.size());
                        
                        for (const auto& [index, val] : blt::enumerate(replacement_func_info.argument_types))
                        {
                            // need to generate replacement.
                            if (index < current_func_info.argument_types.size() && val.id != current_func_info.argument_types[index].id)
                            {
                                // TODO: new config?
                                auto tree = config.generator.get().generate(
                                        {program, val.id, config.replacement_min_depth, config.replacement_max_depth});
                                
                                auto& child = children_data[children_data.size() - 1 - index];
                                blt::size_t total_bytes_for = c.total_value_bytes(child.start, child.end);
                                blt::size_t total_bytes_after = c.total_value_bytes(child.end);
                                
                                auto after_ptr = get_thread_pointer_for_size<struct mutation_func>(total_bytes_after);
                                vals.copy_to(after_ptr, total_bytes_after);
                                vals.pop_bytes(static_cast<blt::ptrdiff_t>(total_bytes_after + total_bytes_for));
                                
                                blt::size_t total_child_bytes = tree.total_value_bytes();
                                
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
                        
                        if (current_func_info.argc.argc > replacement_func_info.argc.argc)
                        {
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
                            // exactly enough args
                            // return types should have been replaced if needed. this part should do nothing?
                        } else
                        {
                            // not enough args
                            blt::size_t start_index = c_node + 1;
                            blt::size_t total_bytes_after = c.total_value_bytes(start_index);
                            auto* data = get_thread_pointer_for_size<struct mutation_func>(total_bytes_after);
                            vals.copy_to(data, total_bytes_after);
                            vals.pop_bytes(static_cast<blt::ptrdiff_t>(total_bytes_after));
                            
                            for (blt::ptrdiff_t i = static_cast<blt::ptrdiff_t>(replacement_func_info.argc.argc) - 1;
                                 i >= current_func_info.argc.argc; i--)
                            {
                                auto tree = config.generator.get().generate(
                                        {program, replacement_func_info.argument_types[i].id, config.replacement_min_depth,
                                         config.replacement_max_depth});
                                blt::size_t total_bytes_for = tree.total_value_bytes();
                                vals.copy_from(tree.get_values(), total_bytes_for);
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
                    auto new_argc = replacement_func_info.argc.argc;
                    // replacement function should be valid. let's make a copy of us.
                    auto current_end = c.find_endpoint(program, static_cast<blt::ptrdiff_t>(c_node));
                    blt::size_t for_bytes = c.total_value_bytes(c_node, current_end);
                    blt::size_t after_bytes = c.total_value_bytes(current_end);
                    auto size = current_end - c_node;
                    
                    auto combined_ptr = get_thread_pointer_for_size<struct SUB_FUNC_FOR>(for_bytes + after_bytes);
                    
                    vals.copy_to(combined_ptr, for_bytes + after_bytes);
                    vals.pop_bytes(static_cast<blt::ptrdiff_t>(for_bytes + after_bytes));
                    
                    blt::size_t start_index = c_node;
                    for (blt::ptrdiff_t i = new_argc - 1; i > static_cast<blt::ptrdiff_t>(arg_position); i--)
                    {
                        auto tree = config.generator.get().generate(
                                {program, replacement_func_info.argument_types[i].id, config.replacement_min_depth,
                                 config.replacement_max_depth});
                        blt::size_t total_bytes_for = tree.total_value_bytes();
                        vals.copy_from(tree.get_values(), total_bytes_for);
                        ops.insert(ops.begin() + static_cast<blt::ptrdiff_t>(start_index), tree.get_operations().begin(),
                                   tree.get_operations().end());
                        start_index += tree.get_operations().size();
                    }
                    start_index += size;
                    vals.copy_from(combined_ptr, for_bytes);
                    for (blt::ptrdiff_t i = static_cast<blt::ptrdiff_t>(arg_position) - 1; i >= 0; i--)
                    {
                        auto tree = config.generator.get().generate(
                                {program, replacement_func_info.argument_types[i].id, config.replacement_min_depth,
                                 config.replacement_max_depth});
                        blt::size_t total_bytes_for = tree.total_value_bytes();
                        vals.copy_from(tree.get_values(), total_bytes_for);
                        ops.insert(ops.begin() + static_cast<blt::ptrdiff_t>(start_index), tree.get_operations().begin(),
                                   tree.get_operations().end());
                        start_index += tree.get_operations().size();
                    }
                    vals.copy_from(combined_ptr + for_bytes, after_bytes);
                    
                    ops.insert(ops.begin() + static_cast<blt::ptrdiff_t>(c_node),
                               {replacement_func_info.function, program.get_typesystem().get_type(replacement_func_info.return_type).size(),
                                random_replacement, program.is_static(random_replacement)});

#if BLT_DEBUG_LEVEL >= 2
                    if (!c.check(program, nullptr))
                    {
                        std::cout << "Parent: " << std::endl;
                        p.print(program, std::cout, false, true);
                        std::cout << "Child:" << std::endl;
                        c.print(program, std::cout, false, true);
                        std::cout << "Child Values:" << std::endl;
                        c.print(program, std::cout, true, true);
                        std::cout << std::endl;
                        BLT_ABORT("Tree Check Failed.");
                    }
#endif
                }
                    break;
                case mutation_operator::JUMP_FUNC:
                {
                    auto& info = program.get_operator_info(ops[c_node].id);
                    blt::size_t argument_index = -1ul;
                    for (const auto& [index, v] : blt::enumerate(info.argument_types))
                    {
                        if (v.id == info.return_type.id)
                        {
                            argument_index = index;
                            break;
                        }
                    }
                    if (argument_index == -1ul)
                        continue;
                    
                    static thread_local std::vector<tree_t::child_t> child_data;
                    child_data.clear();
                    
                    c.find_child_extends(program, child_data, c_node, info.argument_types.size());
                    
                    auto child_index = child_data.size() - 1 - argument_index;
                    auto child = child_data[child_index];
                    auto for_bytes = c.total_value_bytes(child.start, child.end);
                    auto after_bytes = c.total_value_bytes(child_data.back().end);
                    
                    auto storage_ptr = get_thread_pointer_for_size<struct jump_func>(for_bytes + after_bytes);
                    vals.copy_to(storage_ptr + for_bytes, after_bytes);
                    vals.pop_bytes(static_cast<blt::ptrdiff_t>(after_bytes));
                    
                    for (auto i = static_cast<blt::ptrdiff_t>(child_data.size() - 1); i > static_cast<blt::ptrdiff_t>(child_index); i--)
                    {
                        auto& cc = child_data[i];
                        auto bytes = c.total_value_bytes(cc.start, cc.end);
                        vals.pop_bytes(static_cast<blt::ptrdiff_t>(bytes));
                        ops.erase(ops.begin() + cc.start, ops.begin() + cc.end);
                    }
                    vals.copy_to(storage_ptr, for_bytes);
                    vals.pop_bytes(static_cast<blt::ptrdiff_t>(for_bytes));
                    for (auto i = static_cast<blt::ptrdiff_t>(child_index - 1); i >= 0; i--)
                    {
                        auto& cc = child_data[i];
                        auto bytes = c.total_value_bytes(cc.start, cc.end);
                        vals.pop_bytes(static_cast<blt::ptrdiff_t>(bytes));
                        ops.erase(ops.begin() + cc.start, ops.begin() + cc.end);
                    }
                    ops.erase(ops.begin() + static_cast<blt::ptrdiff_t>(c_node));
                    vals.copy_from(storage_ptr, for_bytes + after_bytes);

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
                }
                    break;
                case mutation_operator::COPY:
                {
                    auto& info = program.get_operator_info(ops[c_node].id);
                    blt::size_t pt = -1ul;
                    blt::size_t pf = -1ul;
                    for (const auto& [index, v] : blt::enumerate(info.argument_types))
                    {
                        for (blt::size_t i = index + 1; i < info.argument_types.size(); i++)
                        {
                            auto& v1 = info.argument_types[i];
                            if (v == v1)
                            {
                                if (pt == -1ul)
                                    pt = index;
                                else
                                    pf = index;
                                break;
                            }
                        }
                        if (pt != -1ul && pf != -1ul)
                            break;
                    }
                    if (pt == -1ul || pf == -1ul)
                        continue;
                    
                    blt::size_t from = 0;
                    blt::size_t to = 0;
                    
                    if (program.get_random().choice())
                    {
                        from = pt;
                        to = pf;
                    } else
                    {
                        from = pf;
                        to = pt;
                    }
                    
                    static thread_local std::vector<tree_t::child_t> child_data;
                    child_data.clear();
                    
                    c.find_child_extends(program, child_data, c_node, info.argument_types.size());
                    
                    auto from_index = child_data.size() - 1 - from;
                    auto to_index = child_data.size() - 1 - to;
                    auto& from_child = child_data[from_index];
                    auto& to_child = child_data[to_index];
                    blt::size_t from_bytes = c.total_value_bytes(from_child.start, from_child.end);
                    blt::size_t after_from_bytes = c.total_value_bytes(from_child.end);
                    blt::size_t to_bytes = c.total_value_bytes(to_child.start, to_child.end);
                    blt::size_t after_to_bytes = c.total_value_bytes(to_child.end);
                    
                    auto after_bytes = std::max(after_from_bytes, after_to_bytes);
                    
                    auto from_ptr = get_thread_pointer_for_size<struct copy>(from_bytes);
                    auto after_ptr = get_thread_pointer_for_size<struct copy_after>(after_bytes);
                    
                    vals.copy_to(after_ptr, after_from_bytes);
                    vals.pop_bytes(static_cast<blt::ptrdiff_t>(after_from_bytes));
                    vals.copy_to(from_ptr, from_bytes);
                    vals.copy_from(after_ptr, after_from_bytes);
                    
                    vals.copy_to(after_ptr, after_to_bytes);
                    vals.pop_bytes(static_cast<blt::ptrdiff_t>(after_to_bytes + to_bytes));
                    
                    vals.copy_from(from_ptr, from_bytes);
                    vals.copy_from(after_ptr, after_to_bytes);
                    
                    static std::vector<op_container_t> op_copy;
                    op_copy.clear();
                    op_copy.insert(op_copy.begin(), ops.begin() + from_child.start, ops.begin() + from_child.end);
                    
                    ops.erase(ops.begin() + to_child.start, ops.begin() + to_child.end);
                    ops.insert(ops.begin() + to_child.start, op_copy.begin(), op_copy.end());
                }
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
        
        return c;
    }
    
}

