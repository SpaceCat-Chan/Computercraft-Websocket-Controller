#include "world.hpp"

CommandBuffer<nlohmann::json> Turtle::rotate_1{};
CommandBuffer<nlohmann::json> Turtle::rotate_2{};
CommandBuffer<nlohmann::json> Turtle::rotate_3{};
CommandBuffer<nlohmann::json> Turtle::forward_0{};
CommandBuffer<nlohmann::json> Turtle::forward_1{};
CommandBuffer<nlohmann::json> Turtle::forward_2{};
CommandBuffer<nlohmann::json> Turtle::forward_3{};
CommandBuffer<nlohmann::json> Turtle::up{};
CommandBuffer<nlohmann::json> Turtle::down{};
Turtle::static_init_t Turtle::static_init{};

Pathing::Pathing(glm::ivec3 _target, Turtle &turtle, World &world)
{
	target = _target;
	pather = std::make_unique<AStar>(
	    turtle.position.position,
	    _target,
	    world.make_turtle_obstacle_function(turtle));
	result = std::async(&AStar::run, pather.get());
}
Pathing::Pathing(
    glm::ivec3 _target,
    Turtle &turtle,
    std::function<bool(glm::ivec3)> obstacle)
{
	target = _target;
	pather
	    = std::make_unique<AStar>(turtle.position.position, _target, obstacle);
	result = std::async(&AStar::run, pather.get());
}

Direction operator+(Direction a, int i)
{
	return static_cast<Direction>((static_cast<int>(a) + i) % 4);
}
Direction operator-(Direction a, int i)
{
	return static_cast<Direction>(((static_cast<int>(a) - i) + 4) % 4);
}
Direction operator-(Direction a)
{
	return static_cast<Direction>((static_cast<int>(a) + 2) % 4);
}
