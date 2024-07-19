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
#include <blt/std/time.h>
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
#include "slr.h"

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
static constexpr auto load_image = "../silly.png";

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
    i /= CHANNELS;
    ctx.y = std::floor(static_cast<float>(i) / static_cast<float>(IMAGE_SIZE));
    ctx.x = static_cast<float>(i) - (ctx.y * IMAGE_SIZE);
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

inline blt::size_t get_index(blt::size_t x, blt::size_t y)
{
    return y * IMAGE_SIZE + x;
}

struct full_image_t
{
    float rgb_data[DATA_SIZE * CHANNELS]{};
    
    full_image_t()
    {
        for (auto& v : rgb_data)
            v = 0;
    }
    
    void load(const std::string& path)
    {
        int width, height, channels;
        auto data = stbi_loadf(path.c_str(), &width, &height, &channels, CHANNELS);
        
        stbir_resize_float_linear(data, width, height, 0, rgb_data, IMAGE_SIZE, IMAGE_SIZE, 0, static_cast<stbir_pixel_layout>(CHANNELS));
        
        stbi_image_free(data);
    }
    
    void save(const std::string&)
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
blt::size_t last_run = 0;
blt::i32 time_between_runs = 100;
bool is_running = false;

blt::gp::prog_config_t config = blt::gp::prog_config_t()
        .set_initial_min_tree_size(4)
        .set_initial_max_tree_size(8)
        .set_elite_count(2)
        .set_max_generations(50)
        .set_mutation_chance(1.0)
        .set_crossover_chance(1.0)
        .set_reproduction_chance(0.5)
        .set_pop_size(POP_SIZE)
        .set_thread_count(16);

blt::gp::type_provider type_system;
blt::gp::gp_program program{type_system, SEED, config};

template<typename SINGLE_FUNC>
constexpr static auto make_single(SINGLE_FUNC&& func)
{
    return [func](const full_image_t& a) {
        full_image_t img{};
        for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
            img.rgb_data[i] = func(a.rgb_data[i]);
        return img;
    };
}

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
        img.rgb_data[i] = b.rgb_data[i] == 0 ? 0 : (a.rgb_data[i] / b.rgb_data[i]);
    return img;
}, "div");
blt::gp::operation_t op_sin(make_single((float (*)(float)) &std::sin), "sin");
blt::gp::operation_t op_cos(make_single((float (*)(float)) &std::cos), "cos");
blt::gp::operation_t op_atan(make_single((float (*)(float)) &std::atan), "atan");
blt::gp::operation_t op_exp(make_single((float (*)(float)) &std::exp), "exp");
blt::gp::operation_t op_abs(make_single((float (*)(float)) &std::abs), "abs");
blt::gp::operation_t op_log(make_single((float (*)(float)) &std::log), "log");
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
    {
        auto in_a = type_cast<unsigned int>(a.rgb_data[i]);
        auto in_b = type_cast<unsigned int>(b.rgb_data[i]);
        img.rgb_data[i] = static_cast<float>(in_a ^ in_b);
    }
    return img;
}, "xor");

blt::gp::operation_t lit([]() {
    full_image_t img{};
    auto r = program.get_random().get_float(0.0f, 1.0f);
    auto g = program.get_random().get_float(0.0f, 1.0f);
    auto b = program.get_random().get_float(0.0f, 1.0f);
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        img.rgb_data[i * CHANNELS] = r;
        img.rgb_data[i * CHANNELS + 1] = g;
        img.rgb_data[i * CHANNELS + 2] = b;
    }
    return img;
}, "lit");
blt::gp::operation_t random_val([]() {
    full_image_t img{};
    for (auto& i : img.rgb_data)
        i = program.get_random().get_float(0.0f, 1.0f);
    return img;
}, "color_noise");
static blt::gp::operation_t perlin([](const full_image_t& x, const full_image_t& y, const full_image_t& z) {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
        img.rgb_data[i] = stb_perlin_noise3(x.rgb_data[i], y.rgb_data[i], z.rgb_data[i], 0, 0, 0);
    return img;
}, "perlin");
static blt::gp::operation_t perlin_terminal([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
    {
        auto ctx = get_ctx(i);
        img.rgb_data[i] = stb_perlin_noise3(ctx.x / IMAGE_SIZE, ctx.y / IMAGE_SIZE, static_cast<float>(i % CHANNELS) / CHANNELS, 0, 0, 0);
    }
    return img;
}, "perlin_term");
static blt::gp::operation_t op_img_size([]() {
    full_image_t img{};
    for (float& i : img.rgb_data)
    {
        i = IMAGE_SIZE;
    }
    return img;
}, "img_size");
static blt::gp::operation_t op_x_r([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).x;
        img.rgb_data[i * CHANNELS] = ctx;
        img.rgb_data[i * CHANNELS + 1] = 0;
        img.rgb_data[i * CHANNELS + 2] = 0;
    }
    return img;
}, "x_r");
static blt::gp::operation_t op_x_g([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).x;
        img.rgb_data[i * CHANNELS] = 0;
        img.rgb_data[i * CHANNELS + 1] = ctx;
        img.rgb_data[i * CHANNELS + 2] = 0;
    }
    return img;
}, "x_g");
static blt::gp::operation_t op_x_b([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).x;
        img.rgb_data[i * CHANNELS] = 0;
        img.rgb_data[i * CHANNELS + 1] = 0;
        img.rgb_data[i * CHANNELS + 2] = ctx;
    }
    return img;
}, "x_b");
static blt::gp::operation_t op_y_r([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).y;
        img.rgb_data[i * CHANNELS] = ctx;
        img.rgb_data[i * CHANNELS + 1] = 0;
        img.rgb_data[i * CHANNELS + 2] = 0;
    }
    return img;
}, "y_r");
static blt::gp::operation_t op_y_g([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).y;
        img.rgb_data[i * CHANNELS] = 0;
        img.rgb_data[i * CHANNELS + 1] = ctx;
        img.rgb_data[i * CHANNELS + 2] = 0;
    }
    return img;
}, "y_g");
static blt::gp::operation_t op_y_b([]() {
    full_image_t img{};
    for (blt::size_t i = 0; i < DATA_SIZE; i++)
    {
        auto ctx = get_ctx(i).y;
        img.rgb_data[i * CHANNELS] = 0;
        img.rgb_data[i * CHANNELS + 1] = 0;
        img.rgb_data[i * CHANNELS + 2] = ctx;
    }
    return img;
}, "y_b");

constexpr float compare_values(float a, float b)
{
    if (std::isnan(a) || std::isnan(b) || std::isinf(a) || std::isinf(b))
        return IMAGE_SIZE;
    auto dist = a - b;
    //BLT_TRACE(std::sqrt(dist * dist));
    return std::sqrt(dist * dist);
}

struct fractal_stats
{
    blt::f64 box_size, num_boxes, xy, x2, y2;
};

bool in_box(full_image_t& image, blt::size_t channel, blt::size_t box_size, blt::size_t i, blt::size_t j)
{
    // TODO: this could be made better by starting from the smallest boxes, moving upwards and using the last set of boxes
    //  instead of pixels, since they contain already computed information about if a box is in foam
    for (blt::size_t x = i; x < i + box_size; x++)
    {
        for (blt::size_t y = j; y < j + box_size; y++)
        {
            if (image.rgb_data[get_index(x, y) * CHANNELS + channel] > THRESHOLD)
                return true;
        }
    }
    return false;
}

blt::f64 get_fractal_value(full_image_t& image, blt::size_t channel)
{
    std::array<fractal_stats, BOX_COUNT> box_data{};
    std::array<double, BOX_COUNT> x_data{};
    std::array<double, BOX_COUNT> y_data{};
    for (blt::size_t box_size = IMAGE_SIZE / 2; box_size > 1; box_size /= 2)
    {
        blt::ptrdiff_t num_boxes = 0;
        for (blt::size_t i = 0; i < IMAGE_SIZE; i += box_size)
        {
            for (blt::size_t j = 0; j < IMAGE_SIZE; j += box_size)
            {
                if (in_box(image, channel, box_size, i, j))
                    num_boxes++;
            }
        }
        auto x = static_cast<blt::f64>(std::log2(box_size));
        auto y = static_cast<blt::f64>(num_boxes == 0 ? 0 : std::log2(num_boxes));
        //auto y = static_cast<blt::f64>(num_boxes);
        box_data[static_cast<blt::size_t>(std::log2(box_size)) - 1] = {x, y, x * y, x * x, y * y};
        x_data[static_cast<blt::size_t>(std::log2(box_size)) - 1] = x;
        y_data[static_cast<blt::size_t>(std::log2(box_size)) - 1] = y;
        //BLT_DEBUG("%lf vs %lf", x, y);
    }
//    fractal_stats total{};
//    for (const auto& b : box_data)
//    {
//        total.box_size += b.box_size;
//        total.num_boxes += b.num_boxes;
//        total.xy += b.xy;
//        total.x2 += b.x2;
//        total.y2 += b.y2;
//    }
//
//    auto n = static_cast<blt::f64>(BOX_COUNT);
//    auto b0 = ((total.num_boxes * total.x2) - (total.box_size * total.xy)) / ((n * total.x2) - (total.box_size * total.box_size));
//    auto b1 = ((n * total.xy) - (total.box_size * total.num_boxes)) / ((n * total.x2) - (total.box_size * total.box_size));
//
//    return b1;
    
    slr count{x_data, y_data};
    
    return count.beta;
}

constexpr auto create_fitness_function()
{
    return [](blt::gp::tree_t& current_tree, blt::gp::fitness_t& fitness, blt::size_t index) {
        auto& v = generation_images[index];
        v = current_tree.get_evaluation_value<full_image_t>(nullptr);
        
        fitness.raw_fitness = 0;
        for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
            fitness.raw_fitness += compare_values(v.rgb_data[i], base_image.rgb_data[i]);

        fitness.raw_fitness /= (IMAGE_SIZE * IMAGE_SIZE);
        
        for (blt::size_t channel = 0; channel < CHANNELS; channel++)
        {
            auto raw = -get_fractal_value(v, channel);
            auto fit = 1.0 - std::max(0.0, 1.0 - std::abs(1.35 - raw));
            BLT_DEBUG("Fitness %lf (raw: %lf) for channel %lu", fit, raw, channel);
            if (std::isnan(raw))
                fitness.raw_fitness += 400;
            else
                fitness.raw_fitness += raw;
        }
        
        //BLT_TRACE("Raw fitness: %lf for %ld", fitness.raw_fitness, index);
        fitness.standardized_fitness = fitness.raw_fitness;
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
    base_image.load(load_image);
    
    BLT_DEBUG("Setup Types and Operators");
    type_system.register_type<full_image_t>();
    
    blt::gp::operator_builder<context> builder{type_system};
    //builder.add_operator(perlin);
    builder.add_operator(perlin_terminal);
    
    builder.add_operator(add);
    builder.add_operator(sub);
    builder.add_operator(mul);
    builder.add_operator(pro_div);
    builder.add_operator(op_sin);
    builder.add_operator(op_cos);
    builder.add_operator(op_atan);
    builder.add_operator(op_exp);
    builder.add_operator(op_log);
    builder.add_operator(op_abs);
    builder.add_operator(op_v_mod);
    builder.add_operator(bitwise_and);
    builder.add_operator(bitwise_or);
    builder.add_operator(bitwise_xor);
    
    builder.add_operator(lit, true);
    builder.add_operator(random_val);
    builder.add_operator(op_x_r);
    builder.add_operator(op_x_g);
    builder.add_operator(op_x_b);
    builder.add_operator(op_y_r);
    builder.add_operator(op_y_g);
    builder.add_operator(op_y_b);
    
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
        if (ImGui::IsItemClicked() && !is_running)
        {
            execute_generation();
            print_stats();
        }
        ImGui::InputInt("Time Between Runs", &time_between_runs);
        ImGui::Checkbox("Run", &is_running);
        auto& stats = program.get_population_stats();
        ImGui::Text("Stats:");
        ImGui::Text("Average fitness: %lf", stats.average_fitness.load());
        ImGui::Text("Best fitness: %lf", stats.best_fitness.load());
        ImGui::Text("Worst fitness: %lf", stats.worst_fitness.load());
        ImGui::Text("Overall fitness: %lf", stats.overall_fitness.load());
        ImGui::End();
    }
    
    if (is_running && (blt::system::getCurrentTimeMilliseconds() - last_run) > static_cast<blt::size_t>(time_between_runs))
    {
        execute_generation();
        print_stats();
        
        last_run = blt::system::getCurrentTimeMilliseconds();
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
            
            if (blt::gfx::mousePressedLastFrame())
            {
                program.get_current_pop().get_individuals()[i].tree.print(program, std::cout, false);
            }

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
    for (blt::size_t i = 0; i < CHANNELS; i++)
    {
        auto v = -get_fractal_value(base_image, i);
        BLT_INFO("Base image values per channel: %lf", v);
    }
    
    BLT_PRINT_PROFILE("Image Test", blt::PRINT_CYCLES | blt::PRINT_THREAD | blt::PRINT_WALL);
    
    return 0;
}