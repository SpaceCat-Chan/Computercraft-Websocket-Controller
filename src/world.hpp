#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#include "nlohmann/json.hpp"

#include "AStar.hpp"

#include "Computer.hpp"
#include "Server.hpp"

using namespace std::literals;

enum Direction : int
{
	north = 0,
	east = 1,
	south = 2,
	west = 3
};

Direction operator+(Direction a, int i);
Direction operator-(Direction a, int i);
Direction operator-(Direction a);

constexpr static glm::ivec3 direction_to_orientation(Direction d)
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
constexpr static Direction orientation_to_direction(glm::ivec3 o)
{
	if (o == glm::ivec3{0, 0, -1})
	{
		return north;
	}
	else if (o == glm::ivec3{1, 0, 0})
	{
		return east;
	}
	else if (o == glm::ivec3{0, 0, 1})
	{
		return south;
	}
	else if (o == glm::ivec3{-1, 0, 0})
	{
		return west;
	}
	else
	{
		if (std::is_constant_evaluated())
		{
			return static_cast<Direction>(-1);
		}
		else
		{
			std::cout << "attempted to make " << glm::to_string(o)
			          << " into a direction\n";
			throw std::invalid_argument{"fuck"};
		}
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
	void serialize(Archive &ar, const unsigned int version)
	{
		ar &server;
		ar &dimension;
		ar &position;
		ar &direction;
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
	BOOST_SERIALIZATION_SPLIT_MEMBER()
	template <typename Archive>
	void save(Archive &ar, const unsigned int version) const
	{
		ar &position;
		ar &color;
		ar &name;
		ar &metadata;
		ar &blockstate.dump();
	}
	template <typename Archive>
	void load(Archive &ar, const unsigned int version)
	{
		ar &position;
		ar &color;
		ar &name;
		ar &metadata;
		std::string blockstate_string;
		ar &blockstate_string;
		blockstate = nlohmann::json::parse(blockstate_string);
	}
};

namespace boost
{
namespace serialization
{
template <typename Archive>
void serialize(Archive &ar, glm::dvec3 &vec, const unsigned int version)
{
	ar &vec[0];
	ar &vec[1];
	ar &vec[2];
}
template <typename Archive>
void serialize(Archive &ar, glm::ivec3 &vec, const unsigned int version)
{
	ar &vec[0];
	ar &vec[1];
	ar &vec[2];
}
} // namespace serialization
} // namespace boost

struct Turtle;
class World;

struct Pathing
{
	Pathing(glm::ivec3 _target, Turtle &turtle, World &world);
	Pathing(
	    glm::ivec3 _target,
	    Turtle &turtle,
	    std::function<bool(glm::ivec3)> obstacle);
	std::unique_ptr<AStar> pather;
	std::future<bool> result;
	glm::ivec3 target;
	std::vector<glm::ivec3> latest_results;
	int movement_index
	    = 0; // lates movement in latest_results that has been done
	std::optional<std::future<nlohmann::json>> pending_movement;
	bool finished = false;
	bool unable_to_path = false;
};

struct Turtle
{
	WorldLocation position;
	std::string name;
	std::weak_ptr<ComputerInterface> connection;
	std::optional<Pathing> current_pathing;
	int fuel_level;

	void move(Direction direction)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			int offset = (static_cast<int>(direction)
			              - static_cast<int>(position.direction) + 4)
			             % 4;
			switch (offset)
			{
			case 0:
				con->execute_buffer(forward_0);
			case 1:
				con->execute_buffer(forward_1);
			case 2:
				con->execute_buffer(forward_2);
			case 3:
				con->execute_buffer(forward_3);
			}
		}
	}
	std::future<nlohmann::json> move_future(Direction direction)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			int offset = (static_cast<int>(direction)
			              - static_cast<int>(position.direction) + 4)
			             % 4;
			switch (offset)
			{
			case 0:
				return con->execute_buffer_future(forward_0);
			case 1:
				return con->execute_buffer_future(forward_1);
			case 2:
				return con->execute_buffer_future(forward_2);
			case 3:
				return con->execute_buffer_future(forward_3);
			}
		}
		std::promise<nlohmann::json> a;
		a.set_value(nlohmann::json::object());
		return a.get_future();
	}
	void move_forwards()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->move("forward");
		}
	}
	std::future<nlohmann::json> move_forwards_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->move_future("forward");
		}
		std::promise<nlohmann::json> a;
		a.set_value(nlohmann::json::object());
		return a.get_future();
	}
	void move_backwards()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->move("back");
		}
	}
	std::future<nlohmann::json> move_backwards_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->move_future("back");
		}
		std::promise<nlohmann::json> a;
		a.set_value(nlohmann::json::object());
		return a.get_future();
	}
	void move_up()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->execute_buffer(up);
		}
	}
	std::future<nlohmann::json> move_up_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->execute_buffer_future(up);
		}
		std::promise<nlohmann::json> a;
		a.set_value(nlohmann::json::object());
		return a.get_future();
	}
	void move_down()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->execute_buffer(down);
		}
	}
	std::future<nlohmann::json> move_down_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->execute_buffer_future(down);
		}
		std::promise<nlohmann::json> a;
		a.set_value(nlohmann::json::object());
		return a.get_future();
	}
	void point(Direction direction)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			int offset = ((static_cast<int>(direction)
			               - static_cast<int>(position.direction))
			              + 4)
			             % 4;
			switch (offset)
			{
			case 0:
				break;
			case 1:
				con->execute_buffer(rotate_1);
			case 2:
				con->execute_buffer(rotate_2);
			case 3:
				con->execute_buffer(rotate_3);
			}
		}
	}
	std::future<nlohmann::json> point_future(Direction direction)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			int offset = ((static_cast<int>(direction)
			               - static_cast<int>(position.direction))
			              + 4)
			             % 4;
			switch (offset)
			{
			case 0:
				break;
			case 1:
				return con->execute_buffer_future(rotate_1);
			case 2:
				return con->execute_buffer_future(rotate_2);
			case 3:
				return con->execute_buffer_future(rotate_3);
			}
		}
		std::promise<nlohmann::json> a;
		a.set_value(nlohmann::json::object());
		return a.get_future();
	}
	void rotate_right()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->rotate("right");
		}
	}
	std::future<nlohmann::json> rotate_right_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->rotate_future("right");
		}
		std::promise<nlohmann::json> a;
		a.set_value(nlohmann::json::object());
		return a.get_future();
	}
	void rotate_left()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->rotate("left");
		}
	}
	std::future<nlohmann::json> rotate_left_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->rotate_future("left");
		}
		std::promise<nlohmann::json> a;
		a.set_value(nlohmann::json::object());
		return a.get_future();
	}

	private:
	class static_init_t
	{
		public:
		static_init_t()
		{
			rotate_1.rotate("right");
			rotate_1.SetDefaultParser();

			rotate_2.rotate("right");
			rotate_2.rotate("right");
			rotate_2.SetDefaultParser();

			rotate_3.rotate("left");
			rotate_3.SetDefaultParser();

			forward_0.move("forward");
			forward_0.SetDefaultParser();

			forward_1.buffer(rotate_1);
			forward_1.move("forward");
			forward_1.SetDefaultParser();

			forward_2.buffer(rotate_2);
			forward_2.move("forward");
			forward_2.SetDefaultParser();

			forward_3.buffer(rotate_3);
			forward_3.move("forward");
			forward_3.SetDefaultParser();

			up.move("up");
			up.SetDefaultParser();
			down.move("down");
			down.SetDefaultParser();
		}
	};
	friend class static_init_t;
	static static_init_t static_init;

	static CommandBuffer<nlohmann::json> rotate_1;
	static CommandBuffer<nlohmann::json> rotate_2;
	static CommandBuffer<nlohmann::json> rotate_3;
	static CommandBuffer<nlohmann::json> forward_0;
	static CommandBuffer<nlohmann::json> forward_1;
	static CommandBuffer<nlohmann::json> forward_2;
	static CommandBuffer<nlohmann::json> forward_3;
	static CommandBuffer<nlohmann::json> up;
	static CommandBuffer<nlohmann::json> down;

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive &ar, const unsigned int version)
	{
		ar &position;
		ar &name;
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
		auto position_parse = [](nlohmann::json result) {
			auto returns = result.at("returns");
			WorldLocation parsed;
			parsed.position.x = returns.at(1).get<int>();
			parsed.position.y = returns.at(2).get<int>();
			parsed.position.z = returns.at(3).get<int>();
			parsed.direction = Direction{returns.at(4).get<int>()};
			parsed.dimension = returns.at(5).get<std::string>();
			parsed.server = returns.at(6).get<std::string>();
			return parsed;
		};
		position_and_name.eval(R"(
			return position.get()
		)");
		position_and_name.eval(R"(return os.getComputerLabel())");
		position_and_name.SupplyOutputParser(
		    [position_parse](nlohmann::json result)
		        -> std::optional<
		            std::pair<WorldLocation, std::optional<nlohmann::json>>> {
			    if (result.contains("error"))
			    {
				    return std::nullopt;
			    }
			    auto position = position_parse(result.at(0));
			    if (result.at(1).at("returns").size() == 2)
			    {
				    return std::pair{
				        position,
				        std::optional{result.at(1).at("returns").at(1)}};
			    }
			    else
			    {
				    return std::pair{position, std::optional<nlohmann::json>{}};
			    }
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
			if (block.first->name == "computercraft:turtle_expanded"
			    || block.first->name == "computercraft:turtle_advanced")
			{
				return;
			}
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
			// fuel and inventory stuff goes here
		}
	}

	void get_data_from_turtles()
	{
		std::scoped_lock a{render_mutex};
		for (auto &turtle : m_turtles)
		{
			// TODO: add a timer so turtles aren't spammed, maybe make a timer
			// per data point
			if (!turtle.connection.expired())
			{
				auto turtle_connection
				    = static_cast<std::shared_ptr<ComputerInterface>>(
				        turtle.connection);
				// fuel and inventory things would go here
			}
		}
	}

	void new_turtle(std::shared_ptr<ComputerInterface> turtle)
	{
		std::scoped_lock a{render_mutex};
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
		m_turtles_in_progress.push_back(
		    {turtle, turtle->execute_buffer_future(position_and_name)});
		turtle->auth_message("welcome");
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
			if (turtle.second.wait_for(std::chrono::seconds{0})
			    == std::future_status::ready)
			{
				auto position_and_name_opt = turtle.second.get();
				if (!position_and_name_opt.has_value())
				{
					m_turtles_in_progress.erase(
					    m_turtles_in_progress.begin() + i - 1);
					continue;
				}
				auto position_and_name = *position_and_name_opt;
				auto position = position_and_name.first;
				auto info = position_and_name.second;
				std::string label;
				if (info)
				{
					label = info->get<std::string>();
				}
				else
				{
					label = hash(turtle.first);
				}
				std::cout << "adding computer with label " << label
				          << " to world\n";
				bool found = false;
				for (auto &check_turtle : m_turtles)
				{
					if (check_turtle.name == label)
					{
						std::cout << "found turtle already in world\n";
						check_turtle.connection = turtle.first;
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

	void update_pathings()
	{
		std::scoped_lock<std::mutex> a{render_mutex};
		for (auto &turtle : m_turtles)
		{
			if (turtle.current_pathing && !turtle.current_pathing->finished)
			{
				if (turtle.current_pathing->pending_movement)
				{
					if (turtle.current_pathing->pending_movement->wait_for(0s)
					    == std::future_status::ready)
					{
						if (turtle_requires_repath(turtle))
						{
							turtle.current_pathing->pather
							    = std::make_unique<AStar>(
							        turtle.position.position,
							        turtle.current_pathing->target,
							        make_turtle_obstacle_function(turtle));
							turtle.current_pathing->result = std::async(
							    &AStar::run,
							    turtle.current_pathing->pather.get());
							turtle.current_pathing->pending_movement
							    = std::nullopt;
							turtle.current_pathing->latest_results.clear();
							dirty_renderer_pathes();
							turtle.current_pathing->movement_index = 0;
						}
						else
						{
							if (turtle.position.position
							    == turtle.current_pathing->target)
							{
								turtle.current_pathing->finished = true;
							}
							else
							{
								start_next_pathing_move(turtle);
							}
						}
					}
				}
				else
				{
					if (turtle.current_pathing->result.wait_for(0s)
					    == std::future_status::ready)
					{
						auto result = turtle.current_pathing->result.get();
						if (result)
						{
							if (turtle.position.position
							    == turtle.current_pathing->target)
							{
								turtle.current_pathing->finished = true;
							}
							else
							{
								turtle.current_pathing->latest_results
								    = turtle.current_pathing->pather
								          ->path_result();
								dirty_renderer_pathes();
								start_next_pathing_move(turtle);
							}
						}
						else
						{
							turtle.current_pathing->finished = true;
							turtle.current_pathing->unable_to_path = true;
						}
					}
				}
			}
		}
	}

	void start_next_pathing_move(Turtle &turtle)
	{
		auto &mi = turtle.current_pathing->movement_index;
		auto move_diff = turtle.current_pathing->latest_results[mi + 1]
		                 - turtle.current_pathing->latest_results[mi];
		std::cout << "from: "
		          << glm::to_string(turtle.current_pathing->latest_results[mi])
		          << "\nto: "
		          << glm::to_string(
		                 turtle.current_pathing->latest_results[mi + 1])
		          << "\ndelta: " << glm::to_string(move_diff) << '\n';
		if (move_diff == glm::ivec3{0, 1, 0})
		{
			turtle.current_pathing->pending_movement = turtle.move_up_future();
		}
		else if (move_diff == glm::ivec3{0, -1, 0})
		{
			turtle.current_pathing->pending_movement
			    = turtle.move_down_future();
		}
		else
		{
			turtle.current_pathing->pending_movement
			    = turtle.move_future(orientation_to_direction(move_diff));
		}
		mi++;
	}

	bool turtle_requires_repath(Turtle &turtle)
	{
		return turtle.current_pathing->pather->obstacle(
		           turtle.current_pathing->latest_results
		               [turtle.current_pathing->movement_index + 1])
		       || turtle.position.position
		              != turtle.current_pathing->latest_results
		                     [turtle.current_pathing->movement_index];
	}

	std::function<bool(glm::ivec3)>
	make_turtle_obstacle_function(Turtle &turtle)
	{
		return [this,
		        name = turtle.name,
		        server_name = turtle.position.server,
		        dimension_name
		        = turtle.position.dimension](glm::ivec3 location) -> bool {
			if (auto server = m_blocks.find(server_name);
			    server != m_blocks.end())
			{
				if (auto dimension = server->second.find(dimension_name);
				    dimension != server->second.end())
				{
					if (auto x = dimension->second.find(location.x);
					    x != dimension->second.end())
					{
						if (auto y = x->second.find(location.y);
						    y != x->second.end())
						{
							if (auto z = y->second.find(location.z);
							    z != y->second.end())
							{
								return true;
							}
						}
					}
				}
			}
			for (auto &turtle : m_turtles)
			{
				if (turtle.position.position == location && turtle.name != name)
				{
					return true;
				}
			}
			return false;
		};
	}

	std::mutex render_mutex;

	CommandBuffer<
	    std::optional<std::pair<WorldLocation, std::optional<nlohmann::json>>>>
	    position_and_name;
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
	    std::future<std::optional<
	        std::pair<WorldLocation, std::optional<nlohmann::json>>>>>>
	    m_turtles_in_progress;

	std::function<void(void)> dirty_renderer;
	std::function<void(void)> dirty_renderer_pathes;

	private:
	template <typename Archive>
	void serialize(Archive &ar, const unsigned int version)
	{
		ar &m_blocks;
		ar &m_turtles;
	}
};
