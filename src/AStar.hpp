#pragma once

#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <iostream>

#include <glm/ext.hpp>
#include <glm/gtx/hash.hpp>

#include "PathingNode.hpp"

class AStar
{
	public:
	AStar(
	    glm::ivec3 start,
	    glm::ivec3 end,
	    std::function<bool(glm::ivec3)> obstacle)
	{
		std::cout << this << "\n";
		m_end = end;
		m_obstacle = obstacle;
		auto start_candidate = std::make_shared<PathingNode>(start, end);
		m_f_open_nodes.insert({start_candidate->f, start_candidate});
		m_pos_open_nodes.insert({start_candidate->position, start_candidate});
		if ((m_obstacle(start) || m_obstacle(end)) && !(start == end))
		{
			guaranteed_impossible = true;
		}
	}

	bool run()
	{
		std::cout << this << "\n";
		if (guaranteed_impossible)
		{
			return false;
		}

		while (!stop)
		{
			if (m_closed_nodes.find(m_end) != m_closed_nodes.end())
			{
				return true;
			}
			if (m_f_open_nodes.empty())
			{
				return false;
			}
			single_iteration();
		}
		return false;
	}

	bool obstacle(glm::ivec3 pos)
	{
		return m_obstacle(pos);
	}

	std::vector<glm::ivec3> path_result()
	{
		std::vector<glm::ivec3> result;
		auto end = m_closed_nodes.find(m_end)->second.get();
		result.resize(end->g + 1);
		while (end != nullptr)
		{
			result[end->g] = end->position;
			end = end->parent;
		}
		return result;
	}

	bool stop = false;

	private:
	bool guaranteed_impossible = false;
	void single_iteration()
	{
		auto candidate = m_f_open_nodes.begin();
		std::array<glm::ivec3, 6> new_candidates{
		    candidate->second->position + glm::ivec3{-1, 0, 0},
		    candidate->second->position + glm::ivec3{1, 0, 0},
		    candidate->second->position + glm::ivec3{0, -1, 0},
		    candidate->second->position + glm::ivec3{0, 1, 0},
		    candidate->second->position + glm::ivec3{0, 0, -1},
		    candidate->second->position + glm::ivec3{0, 0, 1}};
		for (auto &new_candidate : new_candidates)
		{
			if (m_closed_nodes.find(new_candidate) != m_closed_nodes.end())
			{
				continue;
			}
			if (m_obstacle(new_candidate))
			{
				continue;
			}
			if (auto already = m_pos_open_nodes.find(new_candidate);
			    already != m_pos_open_nodes.end())
			{
				if (already->second->g > (candidate->second->g + 1))
				{
					already->second->new_parent(candidate->second.get());
				}
			}
			else
			{
				auto constructed_candidate = std::make_shared<PathingNode>(
				    candidate->second.get(),
				    new_candidate,
				    m_end);
				m_f_open_nodes.insert(
				    {constructed_candidate->f, constructed_candidate});
				m_pos_open_nodes.insert(
				    {constructed_candidate->position, constructed_candidate});
			}
		}
		auto orig_candidate_pos = candidate->second->position;
		m_closed_nodes.insert({orig_candidate_pos, candidate->second});
		m_f_open_nodes.erase(candidate);
		m_pos_open_nodes.erase(orig_candidate_pos);
	}

	std::multimap<int, std::shared_ptr<PathingNode>> m_f_open_nodes;
	std::unordered_map<glm::ivec3, std::shared_ptr<PathingNode>>
	    m_pos_open_nodes;
	std::unordered_map<glm::ivec3, std::shared_ptr<PathingNode>> m_closed_nodes;
	std::function<bool(glm::ivec3)> m_obstacle;

	glm::ivec3 m_end;
};
