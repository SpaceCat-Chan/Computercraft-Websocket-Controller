#pragma once

#include <glm/ext.hpp>

struct PathingNode
{
	PathingNode *parent = nullptr;
	glm::ivec3 position;
	int g = 0, h = 0, f = 0;

	PathingNode(PathingNode *_parent, glm::ivec3 new_position, glm::ivec3 end_position)
	{
		new_parent(_parent);
		update_distance(new_position, end_position);
	}
	PathingNode(glm::ivec3 new_position, glm::ivec3 end_position)
	{
		update_distance(new_position, end_position);
	}
	void update_distance(glm::ivec3 new_position, glm::ivec3 end_position)
	{
		position = new_position;
		auto distance_diff = glm::abs(new_position - end_position);
		h = distance_diff.x + distance_diff.y + distance_diff.z;
		f = g + h;
	}
	void new_parent(PathingNode *_parent)
	{
		parent = _parent;
		g = parent->g+1;
		f = g + h;
	}
};
