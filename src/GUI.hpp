#pragma once

#include "render_world.hpp"
#include <variant>

void draw_main_ui(
    World &world,
    RenderWorld &render_world,
    server_manager &s,
    float &x_sensitivity,
    float &y_sensitivity,
    float &mouse_wheel_sensitivity,
    float &forwards_movement_speed,
    float &autosave_interval,
    std::variant<std::monostate, glm::ivec3, size_t> &currently_hovered,
    std::variant<std::monostate, glm::ivec3, size_t> &currently_selected);
bool draw_turtle_ui(Turtle &turtle, World &world);
void draw_selected_ui(
    Block &selected,
    World &world,
    RenderWorld &render_world,
    std::variant<std::monostate, glm::ivec3, size_t> &currently_selected);
bool draw_selected_ui(
    Turtle &selected,
    World &world,
    RenderWorld &render_world,
    std::variant<std::monostate, glm::ivec3, size_t> &currently_selected);
