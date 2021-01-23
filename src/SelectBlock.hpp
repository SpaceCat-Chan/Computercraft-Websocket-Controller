#pragma once

#include <variant>

#include <glm/ext.hpp>

#include "render_world.hpp"

class RayInfo
{
	public:
	glm::dvec3 ray_origin;
	glm::dvec3 ray_direction;
	glm::dvec3 m;
	glm::dvec3 k;

	constexpr static inline glm::dvec3 box_size = {0.5, 0.5, 0.5};

	RayInfo(glm::dvec2 normalized_mouse, RenderWorld &Camera)
	{
		ray_origin = Camera.camera.GetPosition();
		ray_direction = glm::unProject(
		    glm::dvec3{normalized_mouse, 1},
		    Camera.camera.GetView(),
		    Camera.camera.GetProjection(),
		    glm::vec4{-1, -1, 2, 2}) - ray_origin;
		ray_direction = glm::normalize(ray_direction);

		m = 1.0 / ray_direction;
		k = glm::abs(m) * box_size;
	}
};

glm::vec2 box_intersection(RayInfo &info, glm::dvec3 box_position)
{
	auto ro = info.ray_origin - box_position;
	glm::dvec3 n
	    = info.m * ro; // can precompute if traversing a set of aligned boxes
	glm::dvec3 t1 = -n - info.k;
	glm::dvec3 t2 = -n + info.k;
	float tN = glm::max(glm::max(t1.x, t1.y), t1.z);
	float tF = glm::min(glm::min(t2.x, t2.y), t2.z);
	if (tN > tF || tF < 0.f)
		return glm::vec2(-1.f); // no intersection
	return glm::vec2(tN, tF);
}

std::variant<std::monostate, glm::ivec3, size_t> find_selected(
    RayInfo ray,
    World &world,
    std::string server,
    std::string dimension)
{
	auto make_valid_function
	    = [](double ro, double rd) -> std::function<bool(int)> {
		switch (static_cast<int>(glm::sign(rd)))
		{
		case -1:
			return std::bind(
			    std::less_equal<int>{},
			    std::placeholders::_1,
			    static_cast<int>(std::floor(ro)));
		case 0:
			return std::bind(
			    std::equal_to<int>{},
			    std::placeholders::_1,
			    static_cast<int>(std::floor(ro)));
		case 1:
			return std::bind(
			    std::greater_equal<int>{},
			    std::placeholders::_1,
			    static_cast<int>(std::floor(ro)));
		default:
			return [](int) { return false; };
		}
	};
	std::function<bool(int)> x_valid
	    = make_valid_function(ray.ray_origin.x, ray.ray_direction.x);
	std::function<bool(int)> y_valid
	    = make_valid_function(ray.ray_origin.y, ray.ray_direction.y);
	std::function<bool(int)> z_valid
	    = make_valid_function(ray.ray_origin.z, ray.ray_direction.z);
	double current_min_distance = std::numeric_limits<double>::max();
	std::variant<std::monostate, glm::ivec3, size_t> current_selected{
	    std::monostate{}};
	for (auto &x : world.m_blocks[server][dimension])
	{
		if (!x_valid(x.first))
		{
			continue;
		}
		for (auto &y : x.second)
		{
			if (!y_valid(y.first))
			{
				continue;
			}
			for (auto &z : y.second)
			{
				if (!z_valid(z.first))
				{
					continue;
				}
				auto distances = box_intersection(
				    ray,
				    glm::dvec3{z.second.position.position}
				        + glm::dvec3{0.5, 0.5, 0.5});
				if (distances[0] != -1 && distances[0] < current_min_distance)
				{
					current_min_distance = distances[0];
					current_selected = glm::ivec3{x.first, y.first, z.first};
				}
			}
		}
	}
	for (size_t i = 0; i < world.m_turtles.size(); i++)
	{
		auto &turtle = world.m_turtles[i];
		if (turtle.position.server == server
		    && turtle.position.dimension == dimension
		    && (turtle.position.position.x)
		    && y_valid(turtle.position.position.y)
		    && z_valid(turtle.position.position.z))
		{
			auto distances = box_intersection(
			    ray,
			    glm::dvec3{turtle.position.position}
			        + glm::dvec3{0.5, 0.5, 0.5});
			if (distances[0] != -1 && distances[0] < current_min_distance)
			{
				current_min_distance = distances[0];
				current_selected = i;
			}
		}
	}
	return current_selected;
}
