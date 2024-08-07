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
#include "float_operations.h"
#include <images.h>
#include <helper.h>
#include <image_operations.h>

blt::gfx::matrix_state_manager global_matrices;
blt::gfx::resource_manager resources;
blt::gfx::batch_renderer_2d renderer_2d(resources, global_matrices);
blt::gfx::first_person_camera_2d camera;

static constexpr blt::size_t TYPE_COUNT = 2;

std::array<double, POP_SIZE> fitness_values{};
double last_fitness = 0;
double hovered_fitness = 0;
double hovered_fitness_value = 0;
bool evaluate = true;

std::array<bool, TYPE_COUNT> has_literal_converter = {
        true,
        true
};

std::array<std::function<void(blt::gp::gp_program& program, void*, void*, void*, void*)>, TYPE_COUNT> literal_crossover_funcs = {
        [](blt::gp::gp_program&, void* p1_in_ptr, void* p2_in_ptr, void* c1_out_ptr, void* c2_out_ptr) {
            auto& p1_in = *static_cast<full_image_t*>(p1_in_ptr);
            auto& p2_in = *static_cast<full_image_t*>(p2_in_ptr);
            auto& c1_out = *static_cast<full_image_t*>(c1_out_ptr);
            auto& c2_out = *static_cast<full_image_t*>(c2_out_ptr);
            
            for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
            {
                auto diff = p1_in.rgb_data[i] - p2_in.rgb_data[i];
                c1_out.rgb_data[i] = p1_in.rgb_data[i] - diff;
                c2_out.rgb_data[i] = p2_in.rgb_data[i] + diff;
            }
        },
        [](blt::gp::gp_program&, void* p1_in_ptr, void* p2_in_ptr, void* c1_out_ptr, void* c2_out_ptr) {
            auto& p1_in = *static_cast<float*>(p1_in_ptr);
            auto& p2_in = *static_cast<float*>(p2_in_ptr);
            auto& c1_out = *static_cast<float*>(c1_out_ptr);
            auto& c2_out = *static_cast<float*>(c2_out_ptr);
            
            auto diff = p1_in - p2_in;
            c1_out = p1_in - diff;
            c2_out = p2_in + diff;
        }
};

std::array<std::function<void(blt::gp::gp_program& program, void*, void*)>, TYPE_COUNT> literal_mutation_funcs = {
        [](blt::gp::gp_program& program, void* p1_in_ptr, void* c1_out_ptr) {
            auto& p1_in = *static_cast<full_image_t*>(p1_in_ptr);
            auto& c1_out = *static_cast<full_image_t*>(c1_out_ptr);
            
            for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
                c1_out.rgb_data[i] = p1_in.rgb_data[i] + program.get_random().get_float(-1.0f, 1.0f);
        },
        [](blt::gp::gp_program& program, void* p1_in_ptr, void* c1_out_ptr) {
            auto& p1_in = *static_cast<float*>(p1_in_ptr);
            auto& c1_out = *static_cast<float*>(c1_out_ptr);
            
            c1_out = p1_in + program.get_random().get_float(-1.0f, 1.0f);
        }
};

class image_crossover_t : public blt::gp::crossover_t
{
    public:
        blt::expected<result_t, error_t> apply(blt::gp::gp_program& prog, const blt::gp::tree_t& p1, const blt::gp::tree_t& p2) final
        {
            auto sel = prog.get_random().choice();
            if (sel)
                return blt::gp::crossover_t::apply(prog, p1, p2);
            std::abort();
        }
};

std::array<full_image_t, POP_SIZE> generation_images;

full_image_t base_image;
blt::size_t last_run = 0;
blt::i32 time_between_runs = 16;
bool is_running = false;

std::unique_ptr<std::thread> gp_thread = nullptr;

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
    blt::f64 r, g, b, total, combined;
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

fractal_stats get_fractal_value(full_image_t& image)
{
    fractal_stats stats{};
    std::array<double, BOX_COUNT> x_data{};
    std::array<double, BOX_COUNT> boxes_r{};
    std::array<double, BOX_COUNT> boxes_g{};
    std::array<double, BOX_COUNT> boxes_b{};
    std::array<double, BOX_COUNT> boxes_total{};
    std::array<double, BOX_COUNT> boxes_combined{};
    for (blt::size_t box_size = IMAGE_SIZE / 2; box_size > 1; box_size /= 2)
    {
        blt::ptrdiff_t num_boxes_r = 0;
        blt::ptrdiff_t num_boxes_g = 0;
        blt::ptrdiff_t num_boxes_b = 0;
        blt::ptrdiff_t num_boxes_total = 0;
        blt::ptrdiff_t num_boxes_combined = 0;
        for (blt::size_t i = 0; i < IMAGE_SIZE; i += box_size)
        {
            for (blt::size_t j = 0; j < IMAGE_SIZE; j += box_size)
            {
                auto r = in_box(image, 0, box_size, i, j);
                auto g = in_box(image, 1, box_size, i, j);
                auto b = in_box(image, 2, box_size, i, j);
                
                if (r)
                    num_boxes_r++;
                if (g)
                    num_boxes_g++;
                if (b)
                    num_boxes_b++;
                if (r && g && b)
                    num_boxes_combined++;
                if (r || g || b)
                    num_boxes_total++;
            }
        }
        auto x = static_cast<blt::f64>(std::log2(box_size));
        
        x_data[static_cast<blt::size_t>(std::log2(box_size)) - 1] = x;
        boxes_r[static_cast<blt::size_t>(std::log2(box_size)) - 1] = static_cast<blt::f64>(num_boxes_r == 0 ? 0 : std::log2(num_boxes_r));
        boxes_g[static_cast<blt::size_t>(std::log2(box_size)) - 1] = static_cast<blt::f64>(num_boxes_g == 0 ? 0 : std::log2(num_boxes_g));
        boxes_b[static_cast<blt::size_t>(std::log2(box_size)) - 1] = static_cast<blt::f64>(num_boxes_b == 0 ? 0 : std::log2(num_boxes_b));
        boxes_total[static_cast<blt::size_t>(std::log2(box_size)) - 1] = static_cast<blt::f64>(num_boxes_combined == 0 ? 0 : std::log2(
                num_boxes_combined));
        boxes_combined[static_cast<blt::size_t>(std::log2(box_size)) - 1] = static_cast<blt::f64>(num_boxes_total == 0 ? 0 : std::log2(
                num_boxes_total));
    }
    
    slr count_r{x_data, boxes_r};
    slr count_g{x_data, boxes_g};
    slr count_b{x_data, boxes_b};
    slr count_total{x_data, boxes_total};
    slr count_combined{x_data, boxes_combined};

#define FUNC(f) (-f)
    stats.r = FUNC(count_r.beta);
    stats.g = FUNC(count_g.beta);
    stats.b = FUNC(count_b.beta);
    stats.total = FUNC(count_total.beta);
    stats.combined = FUNC(count_combined.beta);
#undef FUNC
    
    return stats;
}

constexpr auto create_fitness_function()
{
    return [](blt::gp::tree_t& current_tree, blt::gp::fitness_t& fitness, blt::size_t index) {
        auto& v = generation_images[index];
        if (evaluate)
            v = current_tree.get_evaluation_value<full_image_t>(nullptr);
        
        if (fitness_values[index] < 0)
        {
            fitness.raw_fitness = 0;
            for (blt::size_t i = 0; i < DATA_CHANNELS_SIZE; i++)
                fitness.raw_fitness += compare_values(v.rgb_data[i], base_image.rgb_data[i]);
            
            fitness.raw_fitness /= (IMAGE_SIZE * IMAGE_SIZE);
            auto raw = get_fractal_value(v);
            auto fit = std::max(0.0, 1.0 - std::abs(1.35 - raw.combined));
            auto fit2 = std::max(0.0, 1.0 - std::abs(1.35 - raw.total));
            //BLT_DEBUG("Fitness %lf %lf %lf || %lf => %lf (fit: %lf)", raw.r, raw.g, raw.b, raw.total, raw.combined, fit);
            if (std::isnan(raw.total) || std::isnan(raw.combined))
                fitness.raw_fitness += 400;
            else
                fitness.raw_fitness += raw.total + raw.combined + 1.0;
            
            fitness.raw_fitness += last_fitness;
        } else
            fitness.raw_fitness = fitness_values[index];
        fitness.standardized_fitness = fitness.raw_fitness;
        fitness.adjusted_fitness = (1.0 / (1.0 + fitness.standardized_fitness));
    };
}

void execute_generation()
{
    BLT_TRACE("------------{Begin Generation %ld}------------", program.get_current_generation());
    BLT_TRACE("Evaluate Fitness");
    BLT_START_INTERVAL("Image Test", "Fitness");
    evaluate = false;
    program.evaluate_fitness();
    BLT_END_INTERVAL("Image Test", "Fitness");
    BLT_START_INTERVAL("Image Test", "Gen");
    auto sel = blt::gp::select_tournament_t{};
//    auto sel = blt::gp::select_fitness_proportionate_t{};
    program.create_next_generation(sel, sel, sel);
    BLT_END_INTERVAL("Image Test", "Gen");
    BLT_TRACE("Move to next generation");
    program.next_generation();
    BLT_TRACE("Evaluate Image");
    BLT_START_INTERVAL("Image Test", "Image Eval");
    evaluate = true;
    program.evaluate_fitness();
    BLT_END_INTERVAL("Image Test", "Image Eval");
    BLT_TRACE("----------------------------------------------");
    std::cout << std::endl;
    // reset all fitness values.
    for (auto& v : fitness_values)
        v = -1;
    last_fitness = 0;
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

std::atomic_bool run_generation = false;

void run_gp()
{
    BLT_DEBUG("Generate Initial Population");
    static constexpr auto fitness_func = create_fitness_function();
    program.generate_population(type_system.get_type<full_image_t>().id(), fitness_func, true);
    
    while (!program.should_thread_terminate())
    {
        if ((run_generation || is_running) && (blt::system::getCurrentTimeMilliseconds() - last_run) > static_cast<blt::size_t>(time_between_runs))
        {
            execute_generation();
            print_stats();
            run_generation = false;
            last_run = blt::system::getCurrentTimeMilliseconds();
        }
    }
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
    type_system.register_type<float>();
    
    blt::gp::operator_builder<context> builder{type_system};
    create_image_operations(builder);
    create_float_operations(builder);
    
    program.set_operations(builder.build());
    
    global_matrices.create_internals();
    resources.load_resources();
    renderer_2d.create();
    
    gp_thread = std::make_unique<std::thread>(run_gp);
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
        if ((ImGui::IsItemClicked() || (blt::gfx::isKeyPressed(GLFW_KEY_R) && blt::gfx::keyPressedLastFrame())) && !is_running)
        {
            run_generation = true;
        }
        ImGui::InputInt("Time Between Runs", &time_between_runs, 16);
        ImGui::Checkbox("Run", &is_running);
        auto& stats = program.get_population_stats();
        ImGui::Text("Stats:");
        ImGui::Text("Average fitness: %lf", stats.average_fitness.load());
        ImGui::Text("Best fitness: %lf", stats.best_fitness.load());
        ImGui::Text("Worst fitness: %lf", stats.worst_fitness.load());
        ImGui::Text("Overall fitness: %lf", stats.overall_fitness.load());
        ImGui::Separator();
        ImGui::Text("Hovered Fitness: %lf", hovered_fitness);
        ImGui::Text("Hovered Fitness Value: %lf", hovered_fitness_value);
        ImGui::End();
    }
    
    const auto mouse_pos = blt::make_vec2(blt::gfx::calculateRay2D(data.width, data.height, global_matrices.getScale2D(), global_matrices.getView2D(),
                                                                   global_matrices.getOrtho()));
    
    for (blt::size_t i = 0; i < program.get_current_pop().get_individuals().size(); i++)
    {
        auto& ind = program.get_current_pop().get_individuals()[i];
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
            
            hovered_fitness = ind.fitness.adjusted_fitness;
            
            if (blt::gfx::isKeyPressed(GLFW_KEY_LEFT_SHIFT))
            {
                if (blt::gfx::mousePressedLastFrame())
                {
                    std::cout << "Fitness: " << ind.fitness.adjusted_fitness << " " << ind.fitness.raw_fitness << " ";
                    ind.tree.print(program, std::cout, false);
                    std::cout << std::endl;
                }
            } else
            {
                if (blt::gfx::mousePressedLastFrame() || (blt::gfx::isKeyPressed(GLFW_KEY_F) && blt::gfx::keyPressedLastFrame()))
                {
                    fitness_values[i] = last_fitness;
                    last_fitness += 1;
                }
            }
            
            hovered_fitness_value = fitness_values[i];
        }
        
        auto val = static_cast<float>(ind.fitness.adjusted_fitness);
        renderer_2d.drawRectangleInternal(
                blt::make_color(val, val, val),
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
    // reset all fitness values.
    for (auto& v : fitness_values)
        v = -1;
    blt::gfx::init(blt::gfx::window_data{"My Sexy Window", init, update, 1440, 720}.setSyncInterval(1));
    global_matrices.cleanup();
    resources.cleanup();
    renderer_2d.cleanup();
    blt::gfx::cleanup();
    
    BLT_END_INTERVAL("Image Test", "Main");
    
    base_image.save("input.png");
    
    auto v = get_fractal_value(base_image);
    BLT_INFO("Base image values per channel: %lf", v.total);
    
    BLT_PRINT_PROFILE("Image Test", blt::PRINT_CYCLES | blt::PRINT_THREAD | blt::PRINT_WALL);
    BLT_PRINT_PROFILE("Mutation", blt::PRINT_CYCLES | blt::PRINT_THREAD | blt::PRINT_WALL | blt::AVERAGE_HISTORY);
    
    is_running = false;
    program.kill();
    if (gp_thread->joinable())
        gp_thread->join();
    
    return 0;
}