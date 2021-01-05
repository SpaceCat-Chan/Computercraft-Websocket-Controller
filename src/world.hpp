#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include <glm/ext.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "nlohmann/json.hpp"

#include "Computer.hpp"
#include "Server.hpp"

enum Direction : int
{
	north = 0,
	east = 1,
	south = 2,
	west = 3
};
constexpr static glm::ivec3 orientation_to_direction(Direction d)
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
constexpr static const char *direction_to_string(Direction d)
{
	switch (d)
	{
	case north:
		return "north";
	case east:
		return "east";
	case south:
		return "south";
	case west:
		return "west";
	}
}

struct WorldLocation
{
	std::string server;
	std::string dimension;
	glm::ivec3 position;
	Direction direction;

	private:
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & server;
		ar & dimension;
		ar & position;
		ar & direction;
	}
};

struct Block
{
	WorldLocation position;
	glm::dvec3 color;
	std::string name;
	int metadata;
	nlohmann::json blockstate;

	private:
	friend class boost::serialization::access;
	BOOST_SERIALIZATION_SPLIT_MEMBER();
	template<typename Archive>
	void save (Archive& ar, const unsigned int version) const
	{
		ar & position;
		ar & color;
		ar & name;
		ar & metadata;
		ar & blockstate.dump();
	}
	template<typename Archive>
	void load(Archive& ar, const unsigned int version)
	{
		ar & position;
		ar & color;
		ar & name;
		ar & metadata;
		std::string blockstate_string;
		ar & blockstate_string;
		blockstate = blockstate_string;
	}
};

namespace boost {
	namespace serialization {
		template <typename Archive>
		void serialize(Archive &ar, glm::dvec3 &vec, const unsigned int version)
		{
			ar & vec[0];
			ar & vec[1];
			ar & vec[2];
		}
		template <typename Archive>
		void serialize(Archive &ar, glm::ivec3 &vec, const unsigned int version)
		{
			ar & vec[0];
			ar & vec[1];
			ar & vec[2];
		}
	}
}

struct Turtle
{
	WorldLocation position;
	std::string name;
	std::weak_ptr<ComputerInterface> connection;
	std::optional<std::future<WorldLocation>> position_update;
	std::optional<std::future<
	    std::array<std::pair<std::optional<Block>, WorldLocation>, 3>>>
	    block_update;
	
	private:
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & position;
		ar & name;
	}
};

template <typename T, typename... U>
const T &first_in_pack(const T &first, const U &... others)
{
	return first;
}

template <typename T, typename... U>
struct first_in_pack_s
{
	using type = T;
};

template <typename U, typename V, typename... T>
void erase_nested(
    std::unordered_map<
        V,
        std::unordered_map<typename first_in_pack_s<T...>::type, U>>
        &erase_from,
    const V &to_erase,
    const T &... next_layers)
{
	auto found = erase_from.find(to_erase);
	if (found != erase_from.end())
	{
		erase_nested(found->second, next_layers...);
		if (found->second.empty())
		{
			erase_from.erase(found);
		}
	}
}

template <typename T, typename V>
void erase_nested(std::unordered_map<T, V> &erase_from, const T &to_erase)
{
	auto found = erase_from.find(to_erase);
	if (found != erase_from.end())
	{
		erase_from.erase(found);
	}
}

class World
{
	friend class boost::serialization::access;
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
			WorldLocation turtle;
			turtle.position.x = result.at(0).at("returns").at(1).get<int>();
			turtle.position.y = result.at(0).at("returns").at(2).get<int>();
			turtle.position.z = result.at(0).at("returns").at(3).get<int>();
			turtle.direction
			    = Direction{result.at(0).at("returns").at(4).get<int>()};
			turtle.dimension
			    = result.at(0).at("returns").at(5).get<std::string>();
			turtle.server = result.at(0).at("returns").at(6).get<std::string>();

			std::array<std::pair<std::optional<Block>, WorldLocation>, 3>
			    blocks;
			for (size_t i = 0; i < 3; i++)
			{
				glm::ivec3 block_direction;
				switch (i)
				{
				case 0:
					block_direction
					    = orientation_to_direction(turtle.direction);
					break;
				case 1:
					block_direction = {0, 1, 0};
					break;
				case 2:
					block_direction = {0, -1, 0};
					break;
				}
				blocks[i].second.position = turtle.position + block_direction;
				blocks[i].second.dimension = turtle.dimension;
				blocks[i].second.server = turtle.server;

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
					blocks[i].first->position.position
					    = blocks[i].second.position;
					blocks[i].first->position.dimension
					    = blocks[i].second.dimension;
					blocks[i].first->position.server = blocks[i].second.server;
				}
			}

			return blocks;
		});

		position_buffer.eval(R"(
			return position.get()
		)");
		auto position_parse = [](nlohmann::json result) {
			WorldLocation parsed;
			auto returns = result.at(0).at("returns");
			parsed.position.x = returns.at(1).get<int>();
			parsed.position.y = returns.at(2).get<int>();
			parsed.position.z = returns.at(3).get<int>();
			parsed.direction = Direction{returns.at(4).get<int>()};
			parsed.dimension = returns.at(5).get<std::string>();
			parsed.server = returns.at(6).get<std::string>();
			return parsed;
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
			std::pair<std::optional<Block>, WorldLocation> parsed_block;
			parsed_block.second.position = glm::ivec3{
			    block.at("position").at(0),
			    block.at("position").at(1),
			    block.at("position").at(2)};
			parsed_block.second.dimension
			    = block.at("dimension").get<std::string>();
			parsed_block.second.server = block.at("server").get<std::string>();
			if (block.at("found_block").get<bool>())
			{
				parsed_block.first = Block{};
				parsed_block.first->position.position
				    = parsed_block.second.position;
				parsed_block.first->position.dimension
				    = parsed_block.second.dimension;
				parsed_block.first->position.server
				    = parsed_block.second.server;
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
				turtle.position.position.x = position.at("x").get<int>();
				turtle.position.position.y = position.at("y").get<int>();
				turtle.position.position.z = position.at("z").get<int>();
				turtle.position.direction = position.at("o").get<Direction>();
				turtle.position.dimension
				    = position.at("dimension").get<std::string>();
				turtle.position.server
				    = position.at("server").get<std::string>();
			}
		}
		dirty_renderer();
	}

	void update_block(std::pair<std::optional<Block>, WorldLocation> block)
	{
		if (block.first)
		{
			m_blocks[block.second.server][block.second.dimension]
			        [block.second.position.x][block.second.position.y]
			        [block.second.position.z]
			    = *(block.first);
		}
		else
		{
			erase_nested(
			    m_blocks,
			    block.second.server,
			    block.second.dimension,
			    block.second.position.x,
			    block.second.position.y,
			    block.second.position.z);
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
				turtle.position.position = data.position;
				turtle.position.direction = data.direction;
				turtle.position.server
				    = data.server; // technically shouldn't need to copy server
				                   // and dimension stuff here since it should
				                   // stay the same, but it can't hurt to be
				                   // safe
				turtle.position.dimension = data.dimension;
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
						auto name = std::to_string(hash(turtle.first));
						check_turtle.name = name;
						turtle.first->remote_eval(
						    "os.setComputerLabel(\"" + name + "\")");
						check_turtle.position.position = position.position;
						check_turtle.position.direction = position.direction;
						check_turtle.position.server = position.server;
						check_turtle.position.dimension = position.dimension;
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
					new_turtle.position.position = position.position;
					new_turtle.position.direction = position.direction;
					new_turtle.position.server = position.server;
					new_turtle.position.dimension = position.dimension;
					m_turtles.push_back(std::move(new_turtle));
				}
				m_turtles_in_progress.erase(
				    m_turtles_in_progress.begin() + i - 1);
			}
		}
		if (turtles_added && dirty_renderer)
		{
			dirty_renderer();
		}
	}

	std::mutex render_mutex;

	CommandBuffer<std::array<std::pair<std::optional<Block>, WorldLocation>, 3>>
	    surrounding_blocks_buffer;
	CommandBuffer<WorldLocation> position_buffer;
	CommandBuffer<std::pair<WorldLocation, nlohmann::json>> position_and_name;
	std::unordered_map<
	    std::string,
	    std::unordered_map<
	        std::string,
	        std::unordered_map<
	            int,
	            std::unordered_map<int, std::unordered_map<int, Block>>>>>
	    m_blocks;

	std::vector<Turtle> m_turtles;
	std::vector<std::pair<
	    std::shared_ptr<ComputerInterface>,
	    std::future<std::pair<WorldLocation, nlohmann::json>>>>
	    m_turtles_in_progress;

	std::function<void(void)> dirty_renderer;

	private:
	template<typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & m_blocks;
		ar & m_turtles;
	}
};
