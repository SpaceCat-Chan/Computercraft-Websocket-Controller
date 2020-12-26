#pragma once

#include <string>
#include <unordered_map>

#include <glm/ext.hpp>

#include "nlohmann/json.hpp"

#include "Computer.hpp"
#include "Server.hpp"

struct Block
{
	glm::ivec3 position;
	glm::dvec3 color;
	std::string name;
	int metadata;
	nlohmann::json blockstate;
};

struct Turtle
{
	glm::ivec3 position;
	std::string name;
	std::weak_ptr<ComputerInterface> connection;
	enum Direction : int
	{
		north = 0,
		east = 1,
		south = 2,
		west = 3
	} direction;

	static glm::ivec3 orientation_to_direction(Direction d)
	{
		switch (d)
		{
		case north:
			return {0, 0, -1};
		case east:
			return {1, 0, 0};
		case south:
			return {0, 0, 1};
		case west:
			return {-1, 0, 0};
		}
	}
};

class World
{
	public:
	World()
	{
		surrounding_blocks_buffer.eval(R"(
			return position.get()
		)");
		surrounding_blocks_buffer.eval(R"(
			return turtle.inspect()
		)");
		surrounding_blocks_buffer.eval(R"(
			return turtle.inspectUp()
		)");
		surrounding_blocks_buffer.eval(R"(
			return turtle.inspectDown()
		)");
		surrounding_blocks_buffer.SupplyOutputParser([](nlohmann::json result) {
			glm::ivec3 turtle_position;
			turtle_position.x = result.at(0).at("returns").at(1).get<int>();
			turtle_position.y = result.at(0).at("returns").at(2).get<int>();
			turtle_position.z = result.at(0).at("returns").at(3).get<int>();
			Turtle::Direction direction{
			    result.at(0).at("returns").at(4).get<int>()};

			std::array<std::pair<std::optional<Block>, glm::ivec3>, 3> blocks;
			for (size_t i = 0; i < 3; i++)
			{
				glm::ivec3 block_direction;
				switch (i)
				{
				case 0:
					block_direction
					    = Turtle::orientation_to_direction(direction);
					break;
				case 1:
					block_direction = {0, 1, 0};
					break;
				case 2:
					block_direction = {0, -1, 0};
					break;
				}
				if (result.at(i + 1).at("returns").at(1) == true)
				{
					blocks[i].first = Block{};
					blocks[i].first->name = result.at(i + 1)
					                            .at("returns")
					                            .at(2)
					                            .at("name")
					                            .get<std::string>();
					blocks[i].first->metadata = result.at(i + 1)
					                                .at("returns")
					                                .at(2)
					                                .at("metadata")
					                                .get<int>();
					blocks[i].first->blockstate
					    = result.at(i + 1).at("returns").at(2).at("state");
					blocks[i].first->position
					    = turtle_position + block_direction;
				}
				blocks[i].second = turtle_position + block_direction;
			}

			return blocks;
		});
	}

	void update_data_gets()
	{
		for (size_t i = m_in_progress_data_gets.size(); i != 0; i--)
		{
			auto get = i - 1;
			if (m_in_progress_data_gets[get].valid())
			{
				auto blocks = m_in_progress_data_gets[get].get();
				m_in_progress_data_gets.erase(
				    m_in_progress_data_gets.begin() + get);
				for (auto &block : blocks)
				{
					if (block.first)
					{
						m_blocks[block.second.x][block.second.y][block.second.z]
						    = *(block.first);
					}
					else
					{
						auto first_level = m_blocks.find(block.second.x);
						if (first_level != m_blocks.end())
						{
							auto second_level
							    = first_level->second.find(block.second.y);
							if (second_level != first_level->second.end())
							{
								auto third_level
								    = second_level->second.find(block.second.z);
								if (third_level != second_level->second.end())
								{
									second_level->second.erase(third_level);
								}
								if (second_level->second.empty())
								{
									first_level->second.erase(second_level);
								}
							}
							if (first_level->second.empty())
							{
								m_blocks.erase(first_level);
							}
						}
					}
				}
			}
		}
	}

	void get_data_from_turtles()
	{
		for (auto turtle : m_turtles)
		{
			if (!turtle.connection.expired())
			{
				auto turtle_connection
				    = static_cast<std::shared_ptr<ComputerInterface>>(
				        turtle.connection);
				turtle_connection->execute_buffer(surrounding_blocks_buffer);
			}
		}
	}

	void new_turtle(std::shared_ptr<ComputerInterface> turtle)
	{
		m_turtles_in_progress.push_back({turtle, turtle->remote_eval(R"(
			return os.getComputerLabel()
		)")});
		std::cout << "new turtle\n";
	}

	void add_new_turtles()
	{
		for (auto &turtle : m_turtles_in_progress)
		{
			turtle.second.wait();
			auto info = turtle.second.get();
			auto label = info.at("returns").at(1).get<std::string>();
			std::cout << "adding computer with label " << label << " to world\n";
			bool found = false;
			for (auto &check_turtle : m_turtles)
			{
				if (check_turtle.name == label)
				{
					std::cout << "found turtle already in world\n";
					check_turtle.connection = turtle.first;
					size_t name = hash(turtle.first);
					check_turtle.name = name;
					turtle.first->remote_eval(
					    "os.setComputerLabel(\"" + std::to_string(name)
					    + "\")");
					found = true;
					break;
				}
			}
			if (!found)
			{
				std::cout << "creating new turtle in world\n";
				Turtle new_turtle;
				new_turtle.connection = turtle.first;
				size_t name = hash(turtle.first);
				new_turtle.name = name;
				turtle.first->remote_eval(
				    "os.setComputerLabel(\"" + std::to_string(name) + "\")");
				m_turtles.push_back(new_turtle);
			}
		}
		m_turtles_in_progress.clear();
	}

	CommandBuffer<std::array<std::pair<std::optional<Block>, glm::ivec3>, 3>>
	    surrounding_blocks_buffer;

	std::vector<
	    std::future<std::array<std::pair<std::optional<Block>, glm::ivec3>, 3>>>
	    m_in_progress_data_gets;
	std::unordered_map<
	    int,
	    std::unordered_map<int, std::unordered_map<int, Block>>>
	    m_blocks;
	std::vector<Turtle> m_turtles;
	std::vector<std::pair<
	    std::shared_ptr<ComputerInterface>,
	    std::future<nlohmann::json>>>
	    m_turtles_in_progress;
};
