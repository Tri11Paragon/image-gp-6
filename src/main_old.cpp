///*
// *  <Short Description>
// *  Copyright (C) 2024  Brett Terpstra
// *
// *  This program is free software: you can redistribute it and/or modify
// *  it under the terms of the GNU General Public License as published by
// *  the Free Software Foundation, either version 3 of the License, or
// *  (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// *  GNU General Public License for more details.
// *
// *  You should have received a copy of the GNU General Public License
// *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
// */
//#include <blt/gp/program.h>
//#include <blt/profiling/profiler_v2.h>
//#include <blt/gp/tree.h>
//#include <blt/std/logging.h>
//#include <blt/std/memory_util.h>
//#include <blt/std/hashmap.h>
//#include <stb_image.h>
//#include <stb_image_resize2.h>
//#include <stb_image_write.h>
//#include <stb_perlin.h>
//#include <blt/gfx/window.h>
//#include "blt/gfx/renderer/resource_manager.h"
//#include "blt/gfx/renderer/batch_2d_renderer.h"
//#include "blt/gfx/renderer/camera.h"
//#include "blt/gfx/raycast.h"
//#include <imgui.h>
//#include "opencv2/imgcodecs.hpp"
//#include "opencv2/imgproc.hpp"
//#include <random>
//
//static const blt::u64 SEED = std::random_device()();
//static constexpr long IMAGE_SIZE = 128;
//static constexpr long IMAGE_PADDING = 16;
//static constexpr long POP_SIZE = 9;
//static constexpr blt::size_t CHANNELS = 3;
//static constexpr blt::size_t DATA_SIZE = IMAGE_SIZE * IMAGE_SIZE;
//
//blt::gfx::matrix_state_manager global_matrices;
//blt::gfx::resource_manager resources;
//blt::gfx::batch_renderer_2d renderer_2d(resources, global_matrices);
//blt::gfx::first_person_camera_2d camera;
//
//struct context
//{
//    float x, y;
//};
//
//struct full_image_t
//{
//    std::array<blt::u8, DATA_SIZE * CHANNELS> rgb_data;
//
//    void load(const std::string& path)
//    {
//        int width, height, channels;
//        auto data = stbi_load(path.c_str(), &width, &height, &channels, CHANNELS);
//
//        stbir_resize_uint8_linear(data, width, height, 0, rgb_data.data(), IMAGE_SIZE, IMAGE_SIZE, 0, static_cast<stbir_pixel_layout>(CHANNELS));
//
//        stbi_image_free(data);
//    }
//
//    void save(const std::string& str)
//    {
//        stbi_write_png(str.c_str(), IMAGE_SIZE, IMAGE_SIZE, CHANNELS, rgb_data.data(), 0);
//    }
//};
//
//std::array<full_image_t, POP_SIZE> generation_images;
//
//full_image_t base_data;
//full_image_t found_data;
//
//cv::Mat base_image_hsv;
//
//int h_bins = 50, s_bins = 60;
//int histSize[] = {h_bins, s_bins};
//
//// hue varies from 0 to 179, saturation from 0 to 255
//float h_ranges[] = {0, 180};
//float s_ranges[] = {0, 256};
//
//const float* ranges[] = {h_ranges, s_ranges};
//
//// Use the 0-th and 1-st channels
//int channels[] = {0, 1, 2};
//
//cv::Mat base_image_hist;
//
//blt::size_t selected_image = 0;
//blt::size_t last_fitness = 0;
//
//blt::gp::prog_config_t config = blt::gp::prog_config_t()
//        .set_initial_min_tree_size(2)
//        .set_initial_max_tree_size(6)
//        .set_elite_count(1)
//        .set_max_generations(50)
//        .set_mutation_chance(0.8)
//        .set_crossover_chance(1.0)
//        .set_reproduction_chance(0)
//        .set_pop_size(POP_SIZE)
//        .set_thread_count(0);
//
//blt::gp::type_provider type_system;
//blt::gp::gp_program program_red{type_system, SEED, config};
//blt::gp::gp_program program_green{type_system, SEED, config};
//blt::gp::gp_program program_blue{type_system, SEED, config};
//
//template<typename>
//void create_program(blt::gp::gp_program& program)
//{
//    static blt::gp::operation_t add([](float a, float b) { return a + b; }, "add");
//    static blt::gp::operation_t sub([](float a, float b) { return a - b; }, "sub");
//    static blt::gp::operation_t mul([](float a, float b) { return a * b; }, "mul");
//    static blt::gp::operation_t pro_div([](float a, float b) { return b == 0.0f ? 1.0f : a / b; }, "div");
//    static blt::gp::operation_t op_sin([](float a) { return std::sin(a); }, "sin");
//    static blt::gp::operation_t op_cos([](float a) { return std::cos(a); }, "cos");
//    static blt::gp::operation_t op_exp([](float a) { return std::exp(a); }, "exp");
//    static blt::gp::operation_t op_log([](float a) { return a == 0.0f ? 0.0f : std::log(a); }, "log");
//    static blt::gp::operation_t op_mod(
//            [](float a, float b) { return static_cast<int>(b) <= 0 ? 0.0f : static_cast<float>(static_cast<int>(a) % static_cast<int>(b)); }, "mod");
//    static blt::gp::operation_t op_b_mod(
//            [](float a, float b) {
//                return blt::mem::type_cast<int>(b) <= 0 ? 0.0f : blt::mem::type_cast<float>(
//                        blt::mem::type_cast<int>(a) % blt::mem::type_cast<int>(b));
//            }, "b_mod");
//    static blt::gp::operation_t op_v_mod(
//            [](float a, float b) {
//                return blt::mem::type_cast<int>(b) <= 0 ? 0.0f : static_cast<float>(blt::mem::type_cast<int>(a) % blt::mem::type_cast<int>(b));
//            },
//            "v_mod");
//    static blt::gp::operation_t bitwise_and([](float a, float b) {
//        return blt::mem::type_cast<float>(blt::mem::type_cast<int>(a) & blt::mem::type_cast<int>(b));
//    }, "b_and");
//    static blt::gp::operation_t bitwise_or([](float a, float b) {
//        return blt::mem::type_cast<float>(blt::mem::type_cast<int>(a) | blt::mem::type_cast<int>(b));
//    }, "b_or");
//    static blt::gp::operation_t bitwise_xor([](float a, float b) {
//        return blt::mem::type_cast<float>(blt::mem::type_cast<int>(a) ^ blt::mem::type_cast<int>(b));
//    }, "b_xor");
//
//    static blt::gp::operation_t bw_raw_and([](float a, float b) {
//        return static_cast<float>(blt::mem::type_cast<int>(a) & blt::mem::type_cast<int>(b));
//    }, "raw_and");
//    static blt::gp::operation_t bw_raw_or([](float a, float b) {
//        return static_cast<float>(blt::mem::type_cast<int>(a) | blt::mem::type_cast<int>(b));
//    }, "raw_or");
//    static blt::gp::operation_t bw_raw_xor([](float a, float b) {
//        return static_cast<float>(blt::mem::type_cast<int>(a) ^ blt::mem::type_cast<int>(b));
//    }, "raw_xor");
//
//    static blt::gp::operation_t value_and([](float a, float b) {
//        return static_cast<int>(a) & static_cast<int>(b);
//    }, "v_and");
//    static blt::gp::operation_t value_or([](float a, float b) {
//        return static_cast<int>(a) | static_cast<int>(b);
//    }, "v_or");
//    static blt::gp::operation_t value_xor([](float a, float b) {
//        return static_cast<int>(a) ^ static_cast<int>(b);
//    }, "v_xor");
//
//    static blt::gp::operation_t lit([&program]() {
//        return program.get_random().get_float(0.0f, 1.0f);
//    }, "lit");
//    static blt::gp::operation_t random([&program]() {
//        return program.get_random().get_float(0.0f, 1.0f);
//    }, "random");
//    static blt::gp::operation_t perlin([](float x, float y, float z, float scale) {
//        if (scale == 0)
//            scale = 1;
//        return stb_perlin_noise3(x / scale, y / scale, z / scale, 0, 0, 0);
//    }, "perlin");
//    static blt::gp::operation_t perlin_terminal([](const context& context) {
//        return stb_perlin_noise3(context.x / IMAGE_SIZE, context.y / IMAGE_SIZE, 0.23423, 0, 0, 0);
//    }, "perlin_term");
//    static blt::gp::operation_t perlin_bumpy([](float x, float y, float z) {
//        return stb_perlin_noise3(x / 128.0f, y / 128.0f, z / 128.0f, 0, 0, 0);
//    }, "perlin_bump");
//    static blt::gp::operation_t op_x([](const context& context) {
//        return context.x;
//    }, "x");
//    static blt::gp::operation_t op_y([](const context& context) {
//        return context.y;
//    }, "y");
//
//    blt::gp::operator_builder<context> builder{type_system};
//    builder.add_operator(perlin);
//    builder.add_operator(perlin_bumpy);
//    builder.add_operator(perlin_terminal);
//
//    builder.add_operator(add);
//    builder.add_operator(sub);
//    builder.add_operator(mul);
//    builder.add_operator(pro_div);
//    builder.add_operator(op_sin);
//    builder.add_operator(op_cos);
//    builder.add_operator(op_exp);
//    builder.add_operator(op_log);
////    builder.add_operator(op_mod);
////    builder.add_operator(op_b_mod);
//    builder.add_operator(op_v_mod);
////    builder.add_operator(bitwise_and);
////    builder.add_operator(bitwise_or);
////    builder.add_operator(bitwise_xor);
////    builder.add_operator(value_and);
////    builder.add_operator(value_or);
////    builder.add_operator(value_xor);
//    builder.add_operator(bw_raw_and);
//    builder.add_operator(bw_raw_or);
//    builder.add_operator(bw_raw_xor);
//
//    builder.add_operator(lit, true);
//    builder.add_operator(random);
//    builder.add_operator(op_x);
//    builder.add_operator(op_y);
//
//    program.set_operations(builder.build());
//}
//
//inline context get_ctx(blt::size_t i)
//{
//    context ctx{};
//    ctx.y = std::floor(static_cast<float>(i) / static_cast<float>(IMAGE_SIZE));
//    ctx.x = static_cast<float>(i) - (ctx.y * IMAGE_SIZE);
////    ctx.x = static_cast<float>(i / IMAGE_SIZE);
////    ctx.y = static_cast<float>(i % IMAGE_SIZE);
////    std::cout << ctx.x << " " << ctx.y << std::endl;
//    return ctx;
//}
//
//inline context get_pop_ctx(blt::size_t i)
//{
//    auto const sq = static_cast<float>(std::sqrt(POP_SIZE));
//    context ctx{};
//    ctx.y = std::floor(static_cast<float>(i) / static_cast<float>(sq));
//    ctx.x = static_cast<float>(i) - (ctx.y * sq);
//    return ctx;
//}
//
//constexpr auto create_fitness_function(blt::size_t channel)
//{
//    return [channel](blt::gp::tree_t& current_tree, blt::gp::fitness_t& fitness, blt::size_t index) {
//        auto& v = generation_images[index];
//        fitness.raw_fitness = 0;
//        for (blt::size_t i = 0; i < DATA_SIZE; i++)
//        {
//            context ctx = get_ctx(i);
//            auto base = base_data.rgb_data[i * CHANNELS + channel];
//            auto set = static_cast<blt::u8>(current_tree.get_evaluation_value<float>(&ctx) * 255);
//            v.rgb_data[i * CHANNELS + channel] = set;
//            auto diff = static_cast<float>(set - base);
//            fitness.raw_fitness += std::sqrt(diff * diff);
//        }
//
//        //BLT_TRACE("Raw fitness: %lf for %ld", fitness.raw_fitness, index);
//        fitness.standardized_fitness = fitness.raw_fitness / IMAGE_SIZE;
//        fitness.adjusted_fitness = (1.0 / (1.0 + fitness.standardized_fitness));
////        auto& v = fitness_data[in];
////        for (blt::size_t i = 0; i < DATA_SIZE; i++)
////        {
////            context ctx = get_ctx(i);
////            v.gray_data[i] = static_cast<blt::u8>(current_tree.get_evaluation_value<float>(&ctx) * 255);
////
////            auto dist = static_cast<float>(v.gray_data[i]) - static_cast<float>(base_data.rgb_data[i * CHANNELS + channel]);
////
////            fitness.raw_fitness += std::sqrt(dist * dist);
////        }
////        cv::Mat img(IMAGE_SIZE, IMAGE_SIZE, CV_8UC1, v.gray_data.data());
////        cv::Mat img_rgb;
////        cv::Mat img_hsv;
////        cv::cvtColor(img, img_rgb, cv::COLOR_GRAY2RGB);
////        cv::cvtColor(img_rgb, img_hsv, cv::COLOR_RGB2HSV);
////        cv::Mat hist;
////        cv::calcHist(&img_hsv, 1, channels, cv::Mat(), hist, 2, histSize, ranges, true, false);
////        cv::normalize(hist, hist, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
////
////        auto comp = 1.0 - cv::compareHist(base_image_hist, hist, cv::HISTCMP_CHISQR);
////
////        fitness.raw_fitness *= comp;
//
////        fitness.standardized_fitness = fitness.raw_fitness / IMAGE_SIZE;
////        fitness.adjusted_fitness = (1.0 / (1.0 + fitness.standardized_fitness));
//    };
//}
//
//constexpr auto fitness_function_red = create_fitness_function(0);
//
//constexpr auto fitness_function_green = create_fitness_function(1);
//
//constexpr auto fitness_function_blue = create_fitness_function(2);
//
//void execute_generation(blt::gp::gp_program& program)
//{
//    BLT_TRACE("------------{Begin Generation %ld}------------", program.get_current_generation());
//    BLT_TRACE("Evaluate Fitness");
//    BLT_START_INTERVAL("Image Test", "Fitness");
//    program.evaluate_fitness();
//    BLT_END_INTERVAL("Image Test", "Fitness");
//    BLT_START_INTERVAL("Image Test", "Gen");
//    auto sel = blt::gp::select_tournament_t{};
//    program.create_next_generation(sel, sel, sel);
//    BLT_END_INTERVAL("Image Test", "Gen");
//    BLT_TRACE("Move to next generation");
//    program.next_generation();
//    BLT_TRACE("----------------------------------------------");
//    std::cout << std::endl;
//}
//
//void print_stats(blt::gp::gp_program& program)
//{
//    auto& stats = program.get_population_stats();
//    BLT_INFO("Stats:");
//    BLT_INFO("Average fitness: %lf", stats.average_fitness.load());
//    BLT_INFO("Best fitness: %lf", stats.best_fitness.load());
//    BLT_INFO("Worst fitness: %lf", stats.worst_fitness.load());
//    BLT_INFO("Overall fitness: %lf", stats.overall_fitness.load());
//}
//
//void write_tree_large(int image_size, blt::size_t index, blt::size_t best_red, blt::size_t best_blue, blt::size_t best_green)
//{
//    auto value = std::unique_ptr<blt::u8>(new blt::u8[image_size * image_size * CHANNELS]);
//
//    BLT_TRACE("Writing large image of index %ld", index);
//    auto& red = program_red.get_current_pop().get_individuals()[best_red].tree;
//    auto& green = program_green.get_current_pop().get_individuals()[best_green].tree;
//    auto& blue = program_blue.get_current_pop().get_individuals()[best_blue].tree;
//
//    for (blt::size_t i = 0; i < static_cast<blt::size_t>(image_size) * image_size; i++)
//    {
//        auto ctx = get_ctx(i);
//        value.get()[i * CHANNELS] = static_cast<blt::u8>(red.get_evaluation_value<float>(&ctx) * 255);
//        value.get()[i * CHANNELS + 1] = static_cast<blt::u8>(green.get_evaluation_value<float>(&ctx) * 255);
//        value.get()[i * CHANNELS + 2] = static_cast<blt::u8>(blue.get_evaluation_value<float>(&ctx) * 255);
//    }
//
//    stbi_write_png(("best_image_large_" + std::to_string(index) + ".png").c_str(), image_size, image_size, CHANNELS, value.get(), 0);
//}
//
//void write_tree(blt::size_t index, blt::size_t best_red, blt::size_t best_blue, blt::size_t best_green)
//{
//    BLT_TRACE("Writing tree of index %ld", index);
//    std::cout << "Red: ";
//    program_red.get_current_pop().get_individuals()[best_red].tree.print(program_red, std::cout);
//    std::cout << "Green: ";
//    program_green.get_current_pop().get_individuals()[best_green].tree.print(program_green, std::cout);
//    std::cout << "Blue: ";
//    program_blue.get_current_pop().get_individuals()[best_blue].tree.print(program_blue, std::cout);
//
//    for (blt::size_t i = 0; i < DATA_SIZE; i++)
//    {
//        found_data.rgb_data[i * CHANNELS] = generation_images[best_red].rgb_data[i * CHANNELS];
//        found_data.rgb_data[i * CHANNELS + 1] = generation_images[best_green].rgb_data[i * CHANNELS + 1];
//        found_data.rgb_data[i * CHANNELS + 2] = generation_images[best_blue].rgb_data[i * CHANNELS + 2];
//    }
//
//    found_data.save("best_image_" + std::to_string(index) + ".png");
//}
//
//void write_results()
//{
//    constexpr static blt::size_t best_count = 5;
//    auto best_red = program_red.get_best_indexes<best_count>();
//    auto best_green = program_green.get_best_indexes<best_count>();
//    auto best_blue = program_blue.get_best_indexes<best_count>();
//
//    for (blt::size_t i = 0; i < best_count; i++)
//    {
//        write_tree(i, best_red[i], best_green[i], best_blue[i]);
//        write_tree_large(512, i, best_red[i], best_green[i], best_blue[i]);
//    }
//
//    print_stats(program_red);
//    print_stats(program_green);
//    print_stats(program_blue);
//}
//
//void init(const blt::gfx::window_data&)
//{
//    using namespace blt::gfx;
//
//    for (blt::size_t i = 0; i < config.population_size; i++)
//        resources.set(std::to_string(i), new texture_gl2D(IMAGE_SIZE, IMAGE_SIZE, GL_RGB8));
//
//    BLT_INFO("Starting BLT-GP Image Test");
//    BLT_INFO("Using Seed: %ld", SEED);
//    BLT_START_INTERVAL("Image Test", "Main");
//    BLT_DEBUG("Setup Base Image");
//    base_data.load("../Rolex_De_Grande_-_Joo.png");
//
//    cv::Mat base_image_mat{IMAGE_SIZE, IMAGE_SIZE, CV_8UC3, base_data.rgb_data.data()};
//    cv::cvtColor(base_image_mat, base_image_hsv, cv::COLOR_RGB2HSV);
//
//    cv::calcHist(&base_image_hsv, 1, channels, cv::Mat(), base_image_hist, 2, histSize, ranges, true, false);
//    cv::normalize(base_image_hist, base_image_hist, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
//
//    BLT_DEBUG("Setup Types and Operators");
//    type_system.register_type<float>();
//
//    create_program<struct red>(program_red);
//    create_program<struct green>(program_green);
//    create_program<struct blue>(program_blue);
//
//    BLT_DEBUG("Generate Initial Population");
//    program_red.generate_population(type_system.get_type<float>().id(), fitness_function_red, true);
//    program_green.generate_population(type_system.get_type<float>().id(), fitness_function_green, true);
//    program_blue.generate_population(type_system.get_type<float>().id(), fitness_function_blue, true);
//
////    for (auto& ind : program_red.get_current_pop().get_individuals())
////        ind.fitness.raw_fitness = POP_SIZE + 1;
////    for (auto& ind : program_green.get_current_pop().get_individuals())
////        ind.fitness.raw_fitness = POP_SIZE + 1;
////    for (auto& ind : program_blue.get_current_pop().get_individuals())
////        ind.fitness.raw_fitness = POP_SIZE + 1;
//
////    evaluate_program(program_red);
////    evaluate_program(program_green);
////    evaluate_program(program_blue);
//
//    global_matrices.create_internals();
//    resources.load_resources();
//    renderer_2d.create();
//}
//
//void update(const blt::gfx::window_data& data)
//{
//    global_matrices.update_perspectives(data.width, data.height, 90, 0.1, 2000);
//
//    for (blt::size_t i = 0; i < config.population_size; i++)
//        resources.get(std::to_string(i)).value()->upload(generation_images[i].rgb_data.data(), IMAGE_SIZE, IMAGE_SIZE, GL_RGB);
//
//    ImGui::SetNextWindowSize(ImVec2(350, 512), ImGuiCond_Once);
//    if (ImGui::Begin("Program Control"))
//    {
//        ImGui::Button("Run Generation");
//        if (ImGui::IsItemClicked())
//        {
//            execute_generation(program_red);
//            execute_generation(program_green);
//            execute_generation(program_blue);
//
////            for (auto& ind : program_red.get_current_pop().get_individuals())
////                ind.fitness.raw_fitness = POP_SIZE + 1;
////            for (auto& ind : program_green.get_current_pop().get_individuals())
////                ind.fitness.raw_fitness = POP_SIZE + 1;
////            for (auto& ind : program_blue.get_current_pop().get_individuals())
////                ind.fitness.raw_fitness = POP_SIZE + 1;
//
//            last_fitness = 0;
//        }
//        ImGui::Button("Write Best");
//        if (ImGui::IsItemClicked())
//            write_results();
//        ImGui::Text("Selected Image: %ld", selected_image);
//        ImGui::InputDouble("Fitness Value Red", &program_red.get_current_pop().get_individuals()[selected_image].fitness.raw_fitness, 1.0, 10);
//        ImGui::InputDouble("Fitness Value Green", &program_green.get_current_pop().get_individuals()[selected_image].fitness.raw_fitness, 1.0, 10);
//        ImGui::InputDouble("Fitness Value Blue", &program_blue.get_current_pop().get_individuals()[selected_image].fitness.raw_fitness, 1.0, 10);
//        ImGui::End();
//    }
//
//    const auto mouse_pos = blt::make_vec2(blt::gfx::calculateRay2D(data.width, data.height, global_matrices.getScale2D(), global_matrices.getView2D(),
//                                                                   global_matrices.getOrtho()));
//
//    double max_fitness_r = 0;
//    double max_fitness_g = 0;
//    double max_fitness_b = 0;
//    for (blt::size_t i = 0; i < config.population_size; i++)
//    {
//        if (program_red.get_current_pop().get_individuals()[i].fitness.raw_fitness > max_fitness_r)
//            max_fitness_r = program_red.get_current_pop().get_individuals()[i].fitness.raw_fitness;
//        if (program_red.get_current_pop().get_individuals()[i].fitness.raw_fitness > max_fitness_g)
//            max_fitness_g = program_green.get_current_pop().get_individuals()[i].fitness.raw_fitness;
//        if (program_red.get_current_pop().get_individuals()[i].fitness.raw_fitness > max_fitness_b)
//            max_fitness_b = program_blue.get_current_pop().get_individuals()[i].fitness.raw_fitness;
//    }
//
//    for (blt::size_t i = 0; i < config.population_size; i++)
//    {
//        auto ctx = get_pop_ctx(i);
//        float x = ctx.x * IMAGE_SIZE + ctx.x * IMAGE_PADDING;
//        float y = ctx.y * IMAGE_SIZE + ctx.y * IMAGE_PADDING;
//
//        auto pos = blt::vec2(x, y);
//        auto dist = pos - mouse_pos;
//        auto p_dist = blt::vec2(std::abs(dist.x()), std::abs(dist.y()));
//
//        constexpr auto HALF_SIZE = IMAGE_SIZE / 2.0f;
//        if (p_dist.x() < HALF_SIZE && p_dist.y() < HALF_SIZE)
//        {
//            renderer_2d.drawRectangleInternal(blt::make_color(0.9, 0.9, 0.3),
//                                              {x, y, IMAGE_SIZE + IMAGE_PADDING / 2.0f, IMAGE_SIZE + IMAGE_PADDING / 2.0f},
//                                              10.0f);
//
//            auto& io = ImGui::GetIO();
//
//            if (io.WantCaptureMouse)
//                continue;
//
//            if (blt::gfx::mousePressedLastFrame())
//            {
//                if (blt::gfx::isKeyPressed(GLFW_KEY_LEFT_SHIFT))
//                {
//                    program_red.get_current_pop().get_individuals()[i].fitness.raw_fitness = static_cast<double>(last_fitness);
//                    program_green.get_current_pop().get_individuals()[i].fitness.raw_fitness = static_cast<double>(last_fitness);
//                    program_blue.get_current_pop().get_individuals()[i].fitness.raw_fitness = static_cast<double>(last_fitness);
//                    if (blt::gfx::mousePressedLastFrame())
//                        last_fitness++;
//                } else
//                    selected_image = i;
//            }
//        }
//
//        auto red = program_red.get_current_pop().get_individuals()[i].fitness.raw_fitness / max_fitness_r;
//        auto green = program_green.get_current_pop().get_individuals()[i].fitness.raw_fitness / max_fitness_g;
//        auto blue = program_blue.get_current_pop().get_individuals()[i].fitness.raw_fitness / max_fitness_b;
//        renderer_2d.drawRectangleInternal(blt::make_vec4(blt::vec3(red, green, blue), 1.0),
//                                          {x, y, IMAGE_SIZE + IMAGE_PADDING / 2.0f, IMAGE_SIZE + IMAGE_PADDING / 2.0f},
//                                          5.0f);
//
//        renderer_2d.drawRectangleInternal(blt::gfx::render_info_t::make_info(std::to_string(i)),
//                                          {x, y, IMAGE_SIZE,
//                                           IMAGE_SIZE}, 15.0f);
//    }
//
//    //BLT_TRACE("%f, %f", mouse_pos.x(), mouse_pos.y());
//
//    camera.update();
//    camera.update_view(global_matrices);
//    global_matrices.update();
//
//    renderer_2d.render(data.width, data.height);
//}
//
//int old_main()
//{
//    blt::gfx::init(blt::gfx::window_data{"My Sexy Window", init, update, 1440, 720}.setSyncInterval(1));
//    global_matrices.cleanup();
//    resources.cleanup();
//    renderer_2d.cleanup();
//    blt::gfx::cleanup();
//
//    BLT_END_INTERVAL("Image Test", "Main");
//
//    base_data.save("input.png");
//
//    BLT_PRINT_PROFILE("Image Test", blt::PRINT_CYCLES | blt::PRINT_THREAD | blt::PRINT_WALL);
//
//    return 0;
//}