#pragma once

#include <mutex>
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
	std::optional<std::future<std::pair<glm::ivec3, Direction>>>
	    position_update;
	std::optional<
	    std::future<std::array<std::pair<std::optional<Block>, glm::ivec3>, 3>>>
	    block_update;

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

					std::hash<std::string> hasher;
					auto name_hash = hasher(blocks[i].first->name);
					glm::vec4 color;
					color.r = (name_hash & 0xff) / 256.0;
					color.g = ((name_hash >> 8) & 0xff) / 256.0;
					color.b = ((name_hash >> 16) & 0xff) / 256.0;

					blocks[i].first->color = color;

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

		position_buffer.eval(R"(
			return position.get()
		)");
		auto position_parse = [](nlohmann::json result) {
			glm::ivec3 position;
			position.x = result.at(0).at("returns").at(1).get<int>();
			position.y = result.at(0).at("returns").at(2).get<int>();
			position.z = result.at(0).at("returns").at(3).get<int>();
			Turtle::Direction direction{
			    result.at(0).at("returns").at(4).get<int>()};
			return std::pair{position, direction};
		};
		position_buffer.SupplyOutputParser(position_parse);

		position_and_name.buffer(position_buffer);
		position_and_name.eval(R"(return os.getComputerLabel())");
		position_and_name.SupplyOutputParser(
		    [position_parse](nlohmann::json result) {
			    auto position = position_parse(result.at(0));
			    return std::pair{position, result.at(1).at("returns").at(1)};
		    });
	}

	void update_block_from_JSON(ComputerInterface &a, nlohmann::json blocks)
	{
		std::scoped_lock lock{render_mutex};
		for (auto &block : blocks)
		{
			std::pair<std::optional<Block>, glm::ivec3> parsed_block;
			parsed_block.second = glm::ivec3{
			    block.at("position").at(0),
			    block.at("position").at(1),
			    block.at("position").at(2)};
			if (block.at("found_block").get<bool>())
			{
				parsed_block.first = Block{};
				parsed_block.first->position = parsed_block.second;
				parsed_block.first->metadata
				    = block.at("block").at("metadata").get<int>();
				parsed_block.first->blockstate = block.at("block").at("state");
				parsed_block.first->name = block.at("block").at("name");
				std::hash<std::string> hasher;
				auto name_hash = hasher(parsed_block.first->name);
				glm::vec4 color;
				color.r = (name_hash & 0xff) / 256.0;
				color.g = ((name_hash >> 8) & 0xff) / 256.0;
				color.b = ((name_hash >> 16) & 0xff) / 256.0;

				parsed_block.first->color = color;
			}
			update_block(parsed_block);
		}
	}
	void update_turtle_from_JSON(
	    ComputerInterface &turtle_connection,
	    nlohmann::json position)
	{
		for (auto &turtle : m_turtles)
		{
			if (turtle.connection.lock().get() == &turtle_connection)
			{
				turtle.position.x = position.at("x").get<int>();
				turtle.position.y = position.at("y").get<int>();
				turtle.position.z = position.at("z").get<int>();
				turtle.direction = position.at("o").get<Turtle::Direction>();
			}
		}
		dirty_renderer();
	}

	void update_block(std::pair<std::optional<Block>, glm::ivec3> block)
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
				auto second_level = first_level->second.find(block.second.y);
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
		if (dirty_renderer)
		{
			dirty_renderer();
		}
	}

	void update_data_gets()
	{
		std::scoped_lock<std::mutex> a{render_mutex};
		for (auto &turtle : m_turtles)
		{
			bool updated_data = false;
			if (turtle.block_update.has_value() && turtle.block_update->valid())
			{
				updated_data = true;
				auto blocks = turtle.block_update->get();
				for (auto &block : blocks)
				{
					update_block(block);
				}
				turtle.block_update = std::nullopt;
			}
			if (turtle.position_update.has_value()
			    && turtle.position_update->valid())
			{
				updated_data = true;
				auto data = turtle.position_update->get();
				turtle.position = data.first;
				turtle.direction = data.second;
				turtle.position_update = std::nullopt;
			}
			if (updated_data && dirty_renderer)
			{
				dirty_renderer();
			}
		}
	}

	void get_data_from_turtles()
	{
		std::scoped_lock a{render_mutex};
		for (auto &turtle : m_turtles)
		{
			if (!turtle.connection.expired())
			{
				auto turtle_connection
				    = static_cast<std::shared_ptr<ComputerInterface>>(
				        turtle.connection);
				if (turtle.block_update == std::nullopt)
				{
					turtle.block_update = turtle_connection->execute_buffer(
					    surrounding_blocks_buffer);
				}
				if (turtle.position_update == std::nullopt)
				{
					turtle.position_update
					    = turtle_connection->execute_buffer(position_buffer);
				}
			}
		}
	}

	void new_turtle(std::shared_ptr<ComputerInterface> turtle)
	{
		std::scoped_lock a{render_mutex};
		sleep(1);
		m_turtles_in_progress.push_back(
		    {turtle, turtle->execute_buffer(position_and_name)});
		turtle->set_unexpected_message_handler(
		    std::bind(
		        &World::update_block_from_JSON,
		        this,
		        std::placeholders::_1,
		        std::placeholders::_2),
		    -1);
		turtle->set_unexpected_message_handler(
		    std::bind(
		        &World::update_turtle_from_JSON,
		        this,
		        std::placeholders::_1,
		        std::placeholders::_2),
		    -2);
		std::cout << "new turtle\n";
	}

	void add_new_turtles()
	{
		std::scoped_lock a{render_mutex};
		bool turtles_added = false;
		for (size_t i = m_turtles_in_progress.size(); i != 0; i--)
		{
			auto &turtle = m_turtles_in_progress[i - 1];
			turtles_added = true;
			if (turtle.second.valid())
			{
				auto position_and_name = turtle.second.get();
				auto position = position_and_name.first;
				auto info = position_and_name.second;
				auto label = info.get<std::string>();
				std::cout << "adding computer with label " << label
				          << " to world\n";
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
						check_turtle.position = position.first;
						check_turtle.direction = position.second;
						found = true;
						break;
					}
				}
				if (!found)
				{
					std::cout << "creating new turtle in world\n";
					Turtle new_turtle;
					new_turtle.connection = turtle.first;
					std::string name = std::to_string(hash(turtle.first));
					new_turtle.name = name;
					turtle.first->remote_eval(
					    "os.setComputerLabel(\"" + name + "\")");
					new_turtle.position = position.first;
					new_turtle.direction = position.second;
					m_turtles.push_back(std::move(new_turtle));
				}
				m_turtles_in_progress.erase(m_turtles_in_progress.begin() + i - 1);
			}
		}
		if (turtles_added && dirty_renderer)
		{
			dirty_renderer();
		}
	}

	std::mutex render_mutex;

	CommandBuffer<std::array<std::pair<std::optional<Block>, glm::ivec3>, 3>>
	    surrounding_blocks_buffer;
	CommandBuffer<std::pair<glm::ivec3, Turtle::Direction>> position_buffer;
	CommandBuffer<
	    std::pair<std::pair<glm::ivec3, Turtle::Direction>, nlohmann::json>>
	    position_and_name;

	std::unordered_map<
	    int,
	    std::unordered_map<int, std::unordered_map<int, Block>>>
	    m_blocks;
	std::vector<Turtle> m_turtles;
	std::vector<std::pair<
	    std::shared_ptr<ComputerInterface>,
	    std::future<std::pair<
	        std::pair<glm::ivec3, Turtle::Direction>,
	        nlohmann::json>>>>
	    m_turtles_in_progress;

	std::function<void(void)> dirty_renderer;
};
