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
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_PERLIN_IMPLEMENTATION

#include <blt/gp/program.h>
#include <blt/profiling/profiler_v2.h>
#include <blt/gp/tree.h>
#include <blt/std/logging.h>
#include <blt/std/memory_util.h>
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <stb_image_write.h>
#include <stb_perlin.h>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <random>

static const blt::u64 SEED = std::random_device()();
static constexpr long IMAGE_SIZE = 128;
static constexpr blt::size_t CHANNELS = 3;
static constexpr blt::size_t DATA_SIZE = IMAGE_SIZE * IMAGE_SIZE;

struct context
{
    float x, y;
};

struct image_t
{
    std::array<blt::u8, DATA_SIZE> image_data;
};

struct full_image_t
{
    std::array<blt::u8, DATA_SIZE * CHANNELS> image_data;
    
    void load(const std::string& path)
    {
        int width, height, channels;
        auto data = stbi_load(path.c_str(), &width, &height, &channels, CHANNELS);
        
        stbir_resize_uint8_linear(data, width, height, 0, image_data.data(), IMAGE_SIZE, IMAGE_SIZE, 0, static_cast<stbir_pixel_layout>(CHANNELS));
        
        stbi_image_free(data);
    }
    
    void save(const std::string& str)
    {
        stbi_write_png(str.c_str(), IMAGE_SIZE, IMAGE_SIZE, CHANNELS, image_data.data(), 0);
    }
};

using fitness_data_t = std::array<image_t, 50>;

fitness_data_t fitness_red;
fitness_data_t fitness_green;
fitness_data_t fitness_blue;
full_image_t base_data;
full_image_t found_data;

cv::Mat base_image_hsv;

int h_bins = 50, s_bins = 60;
int histSize[] = { h_bins, s_bins };

// hue varies from 0 to 179, saturation from 0 to 255
float h_ranges[] = { 0, 180 };
float s_ranges[] = { 0, 256 };

const float* ranges[] = { h_ranges, s_ranges };

// Use the 0-th and 1-st channels
int channels[] = { 0, 1, 2 };

cv::Mat base_image_hist;

blt::gp::prog_config_t config = blt::gp::prog_config_t()
        .set_initial_min_tree_size(2)
        .set_initial_max_tree_size(6)
        .set_elite_count(1)
        .set_max_generations(50)
        .set_mutation_chance(0.4)
        .set_crossover_chance(0.9)
        .set_pop_size(50)
        .set_thread_count(0);

blt::gp::type_provider type_system;
blt::gp::gp_program program_red{type_system, SEED, config};
blt::gp::gp_program program_green{type_system, SEED, config};
blt::gp::gp_program program_blue{type_system, SEED, config};

template<typename>
void create_program(blt::gp::gp_program& program)
{
    static blt::gp::operation_t add([](float a, float b) { return a + b; }, "add");
    static blt::gp::operation_t sub([](float a, float b) { return a - b; }, "sub");
    static blt::gp::operation_t mul([](float a, float b) { return a * b; }, "mul");
    static blt::gp::operation_t pro_div([](float a, float b) { return b == 0.0f ? 1.0f : a / b; }, "div");
    static blt::gp::operation_t op_sin([](float a) { return std::sin(a); }, "sin");
    static blt::gp::operation_t op_cos([](float a) { return std::cos(a); }, "cos");
    static blt::gp::operation_t op_exp([](float a) { return std::exp(a); }, "exp");
    static blt::gp::operation_t op_log([](float a) { return a == 0.0f ? 0.0f : std::log(a); }, "log");
    static blt::gp::operation_t op_mod(
            [](float a, float b) { return static_cast<int>(b) <= 0 ? 0.0f : static_cast<float>(static_cast<int>(a) % static_cast<int>(b)); }, "mod");
    static blt::gp::operation_t op_b_mod(
            [](float a, float b) {
                return blt::mem::type_cast<int>(b) <= 0 ? 0.0f : blt::mem::type_cast<float>(
                        blt::mem::type_cast<int>(a) % blt::mem::type_cast<int>(b));
            }, "b_mod");
    static blt::gp::operation_t op_v_mod(
            [](float a, float b) {
                return blt::mem::type_cast<int>(b) <= 0 ? 0.0f : static_cast<float>(blt::mem::type_cast<int>(a) % blt::mem::type_cast<int>(b));
            },
            "v_mod");
    static blt::gp::operation_t bitwise_and([](float a, float b) {
        return blt::mem::type_cast<float>(blt::mem::type_cast<int>(a) & blt::mem::type_cast<int>(b));
    }, "b_and");
    static blt::gp::operation_t bitwise_or([](float a, float b) {
        return blt::mem::type_cast<float>(blt::mem::type_cast<int>(a) | blt::mem::type_cast<int>(b));
    }, "b_or");
    static blt::gp::operation_t bitwise_xor([](float a, float b) {
        return blt::mem::type_cast<float>(blt::mem::type_cast<int>(a) ^ blt::mem::type_cast<int>(b));
    }, "b_xor");
    
    static blt::gp::operation_t bw_raw_and([](float a, float b) {
        return static_cast<float>(blt::mem::type_cast<int>(a) & blt::mem::type_cast<int>(b));
    }, "raw_and");
    static blt::gp::operation_t bw_raw_or([](float a, float b) {
        return static_cast<float>(blt::mem::type_cast<int>(a) | blt::mem::type_cast<int>(b));
    }, "raw_or");
    static blt::gp::operation_t bw_raw_xor([](float a, float b) {
        return static_cast<float>(blt::mem::type_cast<int>(a) ^ blt::mem::type_cast<int>(b));
    }, "raw_xor");
    
    static blt::gp::operation_t value_and([](float a, float b) {
        return static_cast<int>(a) & static_cast<int>(b);
    }, "v_and");
    static blt::gp::operation_t value_or([](float a, float b) {
        return static_cast<int>(a) | static_cast<int>(b);
    }, "v_or");
    static blt::gp::operation_t value_xor([](float a, float b) {
        return static_cast<int>(a) ^ static_cast<int>(b);
    }, "v_xor");
    
    static blt::gp::operation_t lit([&program]() {
        return program.get_random().get_float(0.0f, 1.0f);
    }, "lit");
    static blt::gp::operation_t random([&program]() {
        return program.get_random().get_float(0.0f, 1.0f);
    }, "random");
    static blt::gp::operation_t perlin([](float x, float y, float z, float scale) {
        if (scale == 0)
            scale = 1;
        return stb_perlin_noise3(x / scale, y / scale, z / scale, 0, 0, 0);
    }, "perlin");
    static blt::gp::operation_t perlin_terminal([](const context& context) {
        return stb_perlin_noise3(context.x / IMAGE_SIZE, context.y / IMAGE_SIZE, 0.23423, 0, 0, 0);
    }, "perlin_term");
    static blt::gp::operation_t perlin_bumpy([](float x, float y, float z) {
        return stb_perlin_noise3(x / 128.0f, y / 128.0f, z / 128.0f, 0, 0, 0);
    }, "perlin_bump");
    static blt::gp::operation_t op_x([](const context& context) {
        return context.x;
    }, "x");
    static blt::gp::operation_t op_y([](const context& context) {
        return context.y;
    }, "y");
    
    blt::gp::operator_builder<context> builder{type_system};
    builder.add_operator(perlin);
    builder.add_operator(perlin_bumpy);
    builder.add_operator(perlin_terminal);
    
    builder.add_operator(add);
    builder.add_operator(sub);
    builder.add_operator(mul);
    builder.add_operator(pro_div);
    builder.add_operator(op_sin);
    builder.add_operator(op_cos);
    builder.add_operator(op_exp);
    builder.add_operator(op_log);
//    builder.add_operator(op_mod);
//    builder.add_operator(op_b_mod);
    builder.add_operator(op_v_mod);
//    builder.add_operator(bitwise_and);
//    builder.add_operator(bitwise_or);
//    builder.add_operator(bitwise_xor);
//    builder.add_operator(value_and);
//    builder.add_operator(value_or);
//    builder.add_operator(value_xor);
    builder.add_operator(bw_raw_and);
    builder.add_operator(bw_raw_or);
    builder.add_operator(bw_raw_xor);
    
    builder.add_operator(lit, true);
    builder.add_operator(random);
    builder.add_operator(op_x);
    builder.add_operator(op_y);
    
    program.set_operations(builder.build());
}

inline context get_ctx(blt::size_t i)
{
    context ctx{};
    ctx.y = std::floor(static_cast<float>(i) / static_cast<float>(IMAGE_SIZE));
    ctx.x = static_cast<float>(i) - (ctx.y * IMAGE_SIZE);
//    ctx.x = static_cast<float>(i / IMAGE_SIZE);
//    ctx.y = static_cast<float>(i % IMAGE_SIZE);
//    std::cout << ctx.x << " " << ctx.y << std::endl;
    return ctx;
}

constexpr auto create_fitness_function(fitness_data_t& fitness_data, blt::size_t channel)
{
    return [&fitness_data, channel](blt::gp::tree_t& current_tree, blt::gp::fitness_t& fitness, blt::size_t in) {
        auto& v = fitness_data[in];
        for (blt::size_t i = 0; i < DATA_SIZE; i++)
        {
            context ctx = get_ctx(i);
            v.image_data[i] = static_cast<blt::u8>(current_tree.get_evaluation_value<float>(&ctx) * 255);
            
            auto dist = static_cast<float>(v.image_data[i]) - static_cast<float>(base_data.image_data[i * CHANNELS + channel]);
            
            fitness.raw_fitness += std::sqrt(dist * dist);
        }
        BLT_TRACE("Hello1");
        cv::Mat img(IMAGE_SIZE, IMAGE_SIZE, CV_8UC3, v.image_data.data());
        BLT_TRACE("Hello2");
        cv::Mat img_hsv;
        BLT_TRACE("Hello3");
        cv::cvtColor(img, img_hsv, cv::COLOR_RGB2HSV);
        BLT_TRACE("Hello4");
        cv::Mat hist;
        BLT_TRACE("Hello5");
        cv::calcHist(&img_hsv, 1, channels, cv::Mat(), hist, 2, histSize, ranges, true, false);
        BLT_TRACE("Hello6");
        cv::normalize(hist, hist, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
        BLT_TRACE("Hello7");
        
        auto comp = cv::compareHist(base_image_hist, hist, cv::HISTCMP_CORREL);
        
        fitness.standardized_fitness = fitness.raw_fitness / IMAGE_SIZE;
        fitness.adjusted_fitness = (1.0 / (1.0 + fitness.standardized_fitness)) * comp;
    };
}

constexpr auto fitness_function_red = create_fitness_function(fitness_red, 0);

constexpr auto fitness_function_green = create_fitness_function(fitness_green, 1);

constexpr auto fitness_function_blue = create_fitness_function(fitness_blue, 2);

void evaluate_program(blt::gp::gp_program& program)
{
    BLT_DEBUG("Begin Generation Loop");
    while (!program.should_terminate())
    {
        BLT_TRACE("------------{Begin Generation %ld}------------", program.get_current_generation());
        BLT_START_INTERVAL("Image Test", "Gen");
        program.create_next_generation(blt::gp::select_tournament_t{}, blt::gp::select_tournament_t{}, blt::gp::select_tournament_t{});
        BLT_END_INTERVAL("Image Test", "Gen");
        BLT_TRACE("Move to next generation");
        BLT_START_INTERVAL("Image Test", "Fitness");
        program.next_generation();
        BLT_TRACE("Evaluate Fitness");
        program.evaluate_fitness();
        BLT_END_INTERVAL("Image Test", "Fitness");
        BLT_TRACE("----------------------------------------------");
        std::cout << std::endl;
    }
}

void print_stats(blt::gp::gp_program& program)
{
    auto& stats = program.get_population_stats();
    BLT_INFO("Stats:");
    BLT_INFO("Average fitness: %lf", stats.average_fitness.load());
    BLT_INFO("Best fitness: %lf", stats.best_fitness.load());
    BLT_INFO("Worst fitness: %lf", stats.worst_fitness.load());
    BLT_INFO("Overall fitness: %lf", stats.overall_fitness.load());
}

void write_tree_large(int image_size, blt::size_t index, blt::size_t best_red, blt::size_t best_blue, blt::size_t best_green)
{
    auto value = std::unique_ptr<blt::u8>(new blt::u8[image_size * image_size * CHANNELS]);
    
    BLT_TRACE("Writing large image of index %ld", index);
    auto& red = program_red.get_current_pop().get_individuals()[best_red].tree;
    auto& green = program_green.get_current_pop().get_individuals()[best_green].tree;
    auto& blue = program_blue.get_current_pop().get_individuals()[best_blue].tree;
    
    for (blt::size_t i = 0; i < static_cast<blt::size_t>(image_size) * image_size; i++)
    {
        auto ctx = get_ctx(i);
        value.get()[i * CHANNELS] = static_cast<blt::u8>(red.get_evaluation_value<float>(&ctx) * 255);
        value.get()[i * CHANNELS + 1] = static_cast<blt::u8>(green.get_evaluation_value<float>(&ctx) * 255);
        value.get()[i * CHANNELS + 2] = static_cast<blt::u8>(blue.get_evaluation_value<float>(&ctx) * 255);
    }
    
    stbi_write_png(("best_image_large_" + std::to_string(index) + ".png").c_str(), image_size, image_size, CHANNELS, value.get(), 0);
}

void write_tree(blt::size_t index, blt::size_t best_red, blt::size_t best_blue, blt::size_t best_green)
{
    BLT_TRACE("Writing tree of index %ld", index);
    std::cout << "Red: ";
    program_red.get_current_pop().get_individuals()[best_red].tree.print(program_red, std::cout);
    std::cout << "Green: ";
    program_green.get_current_pop().get_individuals()[best_green].tree.print(program_green, std::cout);
    std::cout << "Blue: ";
    program_blue.get_current_pop().get_individuals()[best_blue].tree.print(program_blue, std::cout);
    
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        found_data.image_data[i * CHANNELS] = fitness_red[best_red].image_data[i];
        found_data.image_data[i * CHANNELS + 1] = fitness_green[best_green].image_data[i];
        found_data.image_data[i * CHANNELS + 2] = fitness_blue[best_blue].image_data[i];
    }
    
    found_data.save("best_image_" + std::to_string(index) + ".png");
}

void write_results()
{
    constexpr static blt::size_t best_count = 5;
    auto best_red = program_red.get_best_indexes<best_count>();
    auto best_green = program_green.get_best_indexes<best_count>();
    auto best_blue = program_blue.get_best_indexes<best_count>();
    
    for (blt::size_t i = 0; i < best_count; i++)
    {
        write_tree(i, best_red[i], best_green[i], best_blue[i]);
        write_tree_large(512, i, best_red[i], best_green[i], best_blue[i]);
    }
    
    print_stats(program_red);
    print_stats(program_green);
    print_stats(program_blue);
}

template<typename Arg>
auto convert_args(context& ctx, Arg&& arg)
{
    if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<Arg>>, context>)
    {
        return ctx;
    } else
    {
        return std::forward<Arg>(arg);
    }
}

template<typename... Args, typename T>
void make_operator_image(T op, Args... args)
{
    auto value = std::unique_ptr<blt::u8>(new blt::u8[IMAGE_SIZE * IMAGE_SIZE * CHANNELS]);
    
    for (blt::size_t i = 0; i < IMAGE_SIZE * IMAGE_SIZE; i++)
    {
        auto ctx = get_ctx(i);
        value.get()[i * CHANNELS] = static_cast<blt::u8>(op(convert_args(ctx, args)...) * 255);
        value.get()[i * CHANNELS + 1] = static_cast<blt::u8>(op(convert_args(ctx, args)...) * 255);
        value.get()[i * CHANNELS + 2] = static_cast<blt::u8>(op(convert_args(ctx, args)...) * 255);
    }
    
    stbi_write_png((blt::type_string<T> + ".png").c_str(), IMAGE_SIZE, IMAGE_SIZE, CHANNELS, value.get(), 0);
}

int main()
{
    BLT_INFO("Starting BLT-GP Image Test");
    BLT_INFO("Using Seed: %ld", SEED);
    BLT_START_INTERVAL("Image Test", "Main");
    BLT_DEBUG("Setup Base Image");
    base_data.load("../Rolex_De_Grande_-_Joo.png");
    
    cv::Mat base_image_mat{IMAGE_SIZE, IMAGE_SIZE, CV_8UC3, base_data.image_data.data()};
    cv::cvtColor(base_image_mat, base_image_hsv, cv::COLOR_RGB2HSV);
    
    cv::calcHist( &base_image_hsv, 1, channels, cv::Mat(), base_image_hist, 2, histSize, ranges, true, false );
    cv::normalize( base_image_hist, base_image_hist, 0, 1, cv::NORM_MINMAX, -1, cv::Mat() );
    
    BLT_DEBUG("Setup Types and Operators");
    type_system.register_type<float>();
    
    create_program<struct red>(program_red);
    create_program<struct green>(program_green);
    create_program<struct blue>(program_blue);
    
    BLT_DEBUG("Generate Initial Population");
    program_red.generate_population(type_system.get_type<float>().id(), fitness_function_red);
    program_green.generate_population(type_system.get_type<float>().id(), fitness_function_green);
    program_blue.generate_population(type_system.get_type<float>().id(), fitness_function_blue);
    
    evaluate_program(program_red);
    evaluate_program(program_green);
    evaluate_program(program_blue);
    
    BLT_END_INTERVAL("Image Test", "Main");
    
    write_results();
    base_data.save("input.png");
    
    // TODO: make stats helper
    
    BLT_PRINT_PROFILE("Image Test", blt::PRINT_CYCLES | blt::PRINT_THREAD | blt::PRINT_WALL);
    
    return 0;
}