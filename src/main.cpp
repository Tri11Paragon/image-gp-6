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
#include <blt/gp/program.h>
#include <blt/profiling/profiler_v2.h>
#include <blt/gp/tree.h>
#include <blt/std/logging.h>
#include <blt/std/memory_util.h>
#include <blt/std/hashmap.h>
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <stb_image_write.h>
#include <stb_perlin.h>
#include <blt/gfx/window.h>
#include "blt/gfx/renderer/resource_manager.h"
#include "blt/gfx/renderer/batch_2d_renderer.h"
#include "blt/gfx/renderer/camera.h"
#include "blt/gfx/raycast.h"
#include <imgui.h>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <random>

static const blt::u64 SEED = std::random_device()();
static constexpr long IMAGE_SIZE = 128;
static constexpr long IMAGE_PADDING = 16;
static constexpr long POP_SIZE = 50;
static constexpr blt::size_t CHANNELS = 3;
static constexpr blt::size_t DATA_SIZE = IMAGE_SIZE * IMAGE_SIZE;
static constexpr blt::size_t DATA_CHANNELS_SIZE = DATA_SIZE * CHANNELS;

blt::gfx::matrix_state_manager global_matrices;
blt::gfx::resource_manager resources;
blt::gfx::batch_renderer_2d renderer_2d(resources, global_matrices);
blt::gfx::first_person_camera_2d camera;

struct context
{
    float x, y;
};

inline context get_ctx(blt::size_t i)
{
    context ctx{};
    ctx.y = std::floor(static_cast<float>(i) / static_cast<float>(IMAGE_SIZE * CHANNELS));
    ctx.x = static_cast<float>(i) - (ctx.y * IMAGE_SIZE * CHANNELS);
    return ctx;
}

inline context get_pop_ctx(blt::size_t i)
{
    auto const sq = static_cast<float>(std::sqrt(POP_SIZE));
    context ctx{};
    ctx.y = std::floor(static_cast<float>(i) / static_cast<float>(sq));
    ctx.x = static_cast<float>(i) - (ctx.y * sq);
    return ctx;
}

struct full_image_t
{
    float rgb_data[DATA_SIZE * CHANNELS];
    
    full_image_t() = default;
    
    void load(const std::string& path)
    {
        int width, height, channels;
        auto data = stbi_loadf(path.c_str(), &width, &height, &channels, CHANNELS);
        
        stbir_resize_float_linear(data, width, height, 0, rgb_data, IMAGE_SIZE, IMAGE_SIZE, 0, static_cast<stbir_pixel_layout>(CHANNELS));
        
        stbi_image_free(data);
    }
    
    void save(const std::string& str)
    {
        //stbi_write_png(str.c_str(), IMAGE_SIZE, IMAGE_SIZE, CHANNELS, rgb_data, 0);
    }
    
    friend std::ostream& operator<<(std::ostream& str, const full_image_t&)
    {
        return str;
    }
};

std::array<full_image_t, POP_SIZE> generation_images;

full_image_t base_image;

blt::gp::prog_config_t config = blt::gp::prog_config_t()
        .set_initial_min_tree_size(2)
        .set_initial_max_tree_size(6)
        .set_elite_count(1)
        .set_max_generations(50)
        .set_mutation_chance(0.8)
        .set_crossover_chance(1.0)
        .set_reproduction_chance(0)
        .set_pop_size(POP_SIZE)
        .set_thread_count(0);

blt::gp::type_provider type_system;
blt::gp::gp_program program{type_system, SEED, config};

blt::gp::operation_t add([](const full_image_t& a, const full_image_t& b) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = a.rgb_data[i] + b.rgb_data[i];
    return img;
}, "add");
blt::gp::operation_t sub([](const full_image_t& a, const full_image_t& b) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = a.rgb_data[i] - b.rgb_data[i];
    return img;
}, "sub");
blt::gp::operation_t mul([](const full_image_t& a, const full_image_t& b) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = a.rgb_data[i] * b.rgb_data[i];
    return img;
}, "mul");
blt::gp::operation_t pro_div([](const full_image_t& a, const full_image_t& b) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = a.rgb_data[i] / (b.rgb_data[i] == 0 ? 1 : b.rgb_data[i]);
    return img;
}, "div");
blt::gp::operation_t op_sin([](const full_image_t& a) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = std::sin(a.rgb_data[i]);
    return img;
}, "sin");
blt::gp::operation_t op_cos([](const full_image_t& a) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = std::cos(a.rgb_data[i]);
    return img;
}, "cos");
blt::gp::operation_t op_exp([](const full_image_t& a) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = std::exp(a.rgb_data[i]);
    return img;
}, "exp");
blt::gp::operation_t op_log([](const full_image_t& a) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = a.rgb_data[i] == 0 ? 0 : std::log(a.rgb_data[i]);
    return img;
}, "log");
blt::gp::operation_t op_v_mod([](const full_image_t& a, const full_image_t& b) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = b.rgb_data[i] <= 0 ? 0 : static_cast<float>(blt::mem::type_cast<unsigned int>(a.rgb_data[i]) %
                                                                      blt::mem::type_cast<unsigned int>(b.rgb_data[i]));
    return img;
}, "v_mod");

blt::gp::operation_t bitwise_and([](const full_image_t& a, const full_image_t& b) {
    using blt::mem::type_cast;
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = static_cast<float>(type_cast<unsigned int>(a.rgb_data[i]) & type_cast<unsigned int>(b.rgb_data[i]));
    return img;
}, "and");

blt::gp::operation_t bitwise_or([](const full_image_t& a, const full_image_t& b) {
    using blt::mem::type_cast;
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = static_cast<float>(type_cast<unsigned int>(a.rgb_data[i]) | type_cast<unsigned int>(b.rgb_data[i]));
    return img;
}, "or");

blt::gp::operation_t bitwise_xor([](const full_image_t& a, const full_image_t& b) {
    using blt::mem::type_cast;
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = static_cast<float>(type_cast<unsigned int>(a.rgb_data[i]) ^ type_cast<unsigned int>(b.rgb_data[i]));
    return img;
}, "xor");

blt::gp::operation_t lit([]() {
    full_image_t img{};
    for (auto& i : img.rgb_data)
        i = program.get_random().get_float(0.0f, 1.0f);
    return img;
}, "lit");
blt::gp::operation_t random_val([]() {
    full_image_t img{};
    for (auto& i : img.rgb_data)
        i = program.get_random().get_float(0.0f, 1.0f);
    return img;
}, "random");
static blt::gp::operation_t perlin_terminal([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i);
        img.rgb_data[i * CHANNELS] = img.rgb_data[i * CHANNELS + 1] = img.rgb_data[i * CHANNELS + 2] = stb_perlin_noise3(ctx.x / IMAGE_SIZE,
                                                                                                                         ctx.y / IMAGE_SIZE, 0.532, 0,
                                                                                                                         0, 0);
    }
    return img;
}, "perlin_term");
static blt::gp::operation_t op_x([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).x;
        img.rgb_data[i * CHANNELS] = ctx;
        img.rgb_data[i * CHANNELS + 1] = ctx;
        img.rgb_data[i * CHANNELS + 2] = ctx;
    }
    return img;
}, "x");
static blt::gp::operation_t op_y([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).y;
        img.rgb_data[i * CHANNELS] = ctx;
        img.rgb_data[i * CHANNELS + 1] = ctx;
        img.rgb_data[i * CHANNELS + 2] = ctx;
    }
    return img;
}, "y");

constexpr auto create_fitness_function()
{
    return [](blt::gp::tree_t& current_tree, blt::gp::fitness_t& fitness, blt::size_t index) {
        auto& v = generation_images[index];
        v = current_tree.get_evaluation_value<full_image_t>(nullptr);
        
        fitness.raw_fitness = 0;
        for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        {
            auto base = base_image.rgb_data[i];
            auto dist = v.rgb_data[i] - base;
            fitness.raw_fitness += std::sqrt(dist * dist);
        }
        
        //BLT_TRACE("Raw fitness: %lf for %ld", fitness.raw_fitness, index);
        fitness.standardized_fitness = fitness.raw_fitness / IMAGE_SIZE;
        fitness.adjusted_fitness = (1.0 / (1.0 + fitness.standardized_fitness));
    };
}

void execute_generation()
{
    BLT_TRACE("------------{Begin Generation %ld}------------", program.get_current_generation());
    BLT_TRACE("Evaluate Fitness");
    BLT_START_INTERVAL("Image Test", "Fitness");
    program.evaluate_fitness();
    BLT_END_INTERVAL("Image Test", "Fitness");
    BLT_START_INTERVAL("Image Test", "Gen");
    auto sel = blt::gp::select_tournament_t{};
    program.create_next_generation(sel, sel, sel);
    BLT_END_INTERVAL("Image Test", "Gen");
    BLT_TRACE("Move to next generation");
    program.next_generation();
    BLT_TRACE("----------------------------------------------");
    std::cout << std::endl;
}

void print_stats()
{
    auto& stats = program.get_population_stats();
    BLT_INFO("Stats:");
    BLT_INFO("Average fitness: %lf", stats.average_fitness.load());
    BLT_INFO("Best fitness: %lf", stats.best_fitness.load());
    BLT_INFO("Worst fitness: %lf", stats.worst_fitness.load());
    BLT_INFO("Overall fitness: %lf", stats.overall_fitness.load());
}

void init(const blt::gfx::window_data&)
{
    using namespace blt::gfx;
    
    for (blt::size_t i = 0; i < config.population_size; i++)
        resources.set(std::to_string(i), new texture_gl2D(IMAGE_SIZE, IMAGE_SIZE, GL_RGB8));
    
    BLT_INFO("Starting BLT-GP Image Test");
    BLT_INFO("Using Seed: %ld", SEED);
    BLT_START_INTERVAL("Image Test", "Main");
    BLT_DEBUG("Setup Base Image");
    base_image.load("../my_pride_flag.png");
    
    BLT_DEBUG("Setup Types and Operators");
    type_system.register_type<full_image_t>();
    
    blt::gp::operator_builder<context> builder{type_system};
    builder.add_operator(perlin_terminal);
    
    builder.add_operator(add);
    builder.add_operator(sub);
    builder.add_operator(mul);
    builder.add_operator(pro_div);
    builder.add_operator(op_sin);
    builder.add_operator(op_cos);
    builder.add_operator(op_exp);
    builder.add_operator(op_log);
    builder.add_operator(op_v_mod);
    builder.add_operator(bitwise_and);
    builder.add_operator(bitwise_or);
    builder.add_operator(bitwise_xor);
    
    builder.add_operator(lit, true);
    builder.add_operator(random_val);
    builder.add_operator(op_x);
    builder.add_operator(op_y);
    
    program.set_operations(builder.build());
    
    BLT_DEBUG("Generate Initial Population");
    static constexpr auto fitness_func = create_fitness_function();
    program.generate_population(type_system.get_type<full_image_t>().id(), fitness_func, true);
    
    global_matrices.create_internals();
    resources.load_resources();
    renderer_2d.create();
}

void update(const blt::gfx::window_data& data)
{
    global_matrices.update_perspectives(data.width, data.height, 90, 0.1, 2000);
    
    for (blt::size_t i = 0; i < config.population_size; i++)
        resources.get(std::to_string(i)).value()->upload(generation_images[i].rgb_data, IMAGE_SIZE, IMAGE_SIZE, GL_RGB, GL_FLOAT);
    
    ImGui::SetNextWindowSize(ImVec2(350, 512), ImGuiCond_Once);
    if (ImGui::Begin("Program Control"))
    {
        ImGui::Button("Run Generation");
        if (ImGui::IsItemClicked())
        {
            execute_generation();
        }
        ImGui::End();
    }
    
    const auto mouse_pos = blt::make_vec2(blt::gfx::calculateRay2D(data.width, data.height, global_matrices.getScale2D(), global_matrices.getView2D(),
                                                                   global_matrices.getOrtho()));
    
    for (blt::size_t i = 0; i < config.population_size; i++)
    {
        auto ctx = get_pop_ctx(i);
        float x = ctx.x * IMAGE_SIZE + ctx.x * IMAGE_PADDING;
        float y = ctx.y * IMAGE_SIZE + ctx.y * IMAGE_PADDING;
        
        auto pos = blt::vec2(x, y);
        auto dist = pos - mouse_pos;
        auto p_dist = blt::vec2(std::abs(dist.x()), std::abs(dist.y()));
        
        constexpr auto HALF_SIZE = IMAGE_SIZE / 2.0f;
        if (p_dist.x() < HALF_SIZE && p_dist.y() < HALF_SIZE)
        {
            renderer_2d.drawRectangleInternal(blt::make_color(0.9, 0.9, 0.3),
                                              {x, y, IMAGE_SIZE + IMAGE_PADDING / 2.0f, IMAGE_SIZE + IMAGE_PADDING / 2.0f},
                                              10.0f);
            
            auto& io = ImGui::GetIO();
            
            if (io.WantCaptureMouse)
                continue;

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
        }
        
        renderer_2d.drawRectangleInternal(blt::make_vec4(blt::vec3(program.get_current_pop().get_individuals()[i].fitness.adjusted_fitness), 1.0),
                                          {x, y, IMAGE_SIZE + IMAGE_PADDING / 2.0f, IMAGE_SIZE + IMAGE_PADDING / 2.0f},
                                          5.0f);
        
        renderer_2d.drawRectangleInternal(blt::gfx::render_info_t::make_info(std::to_string(i)),
                                          {x, y, IMAGE_SIZE,
                                           IMAGE_SIZE}, 15.0f);
    }
    
    //BLT_TRACE("%f, %f", mouse_pos.x(), mouse_pos.y());
    
    camera.update();
    camera.update_view(global_matrices);
    global_matrices.update();
    
    renderer_2d.render(data.width, data.height);
}

int main()
{
    blt::gfx::init(blt::gfx::window_data{"My Sexy Window", init, update, 1440, 720}.setSyncInterval(1));
    global_matrices.cleanup();
    resources.cleanup();
    renderer_2d.cleanup();
    blt::gfx::cleanup();
    
    BLT_END_INTERVAL("Image Test", "Main");
    
    base_image.save("input.png");
    
    BLT_PRINT_PROFILE("Image Test", blt::PRINT_CYCLES | blt::PRINT_THREAD | blt::PRINT_WALL);
    
    return 0;
}