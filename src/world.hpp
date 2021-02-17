#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "serialize_std_optional.hpp"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/unordered_set.hpp>
#include <boost/serialization/vector.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#include "nlohmann/json.hpp"

#include "AStar.hpp"

#include "Computer.hpp"
#include "Server.hpp"

using namespace std::literals;
using namespace std::literals::chrono_literals;

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
struct Item
{
	std::string name;
	int amount;
	int damage; // damage = max_durability - current_durability

	private:
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive &ar, unsigned int version)
	{
		ar &name;
		ar &amount;
		ar &damage;
	}
};

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
	boost::future<bool> result;
	glm::ivec3 target;
	std::vector<glm::ivec3> latest_results;
	int movement_index
	    = 0; // latest movement in latest_results that has been done
	std::optional<boost::future<nlohmann::json>> pending_movement;
	bool finished = false;
	bool unable_to_path = false;
};

struct TurtleValue
{
	enum jobs
	{
		NONE = 0b0,
		FARMER = 0b1,
		ALL = 0b1
	} job
	    = NONE;

	enum actions
	{
		place_block,
		destroy_block,
		picking_up_plant_drops,
		checking_block,
		harvest_plant //same as place_block/break_block and then devolves into picking_up_plant_drops
	};

	std::optional<actions> current_action;
	glm::ivec3 where;
	glm::ivec3 current_offset;
	std::variant<Direction, std::monostate, std::monostate> direction;
	int inventory_slot;

	private:
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive &ar, unsigned int version)
	{
		ar &job;
		ar &current_action;
		ar &where;
		ar &current_offset;
	}
};

struct Turtle
{
	WorldLocation position;
	std::string name;
	std::weak_ptr<ComputerInterface> connection;
	std::optional<Pathing> current_pathing;
	int fuel_level;
	std::array<std::optional<Item>, 16> inventory;
	std::chrono::steady_clock::time_point last_inventory_get
	    = std::chrono::steady_clock::now() - 24h;
	std::optional<boost::future<decltype(inventory)>> current_inventory_get;
	TurtleValue value;

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
				break;
			case 1:
				con->execute_buffer(forward_1);
				break;
			case 2:
				con->execute_buffer(forward_2);
				break;
			case 3:
				con->execute_buffer(forward_3);
				break;
			}
		}
	}
	boost::future<nlohmann::json> move_future(Direction direction)
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
		return boost::make_ready_future(nlohmann::json::object());
	}
	void move_forwards()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->move("forward");
		}
	}
	boost::future<nlohmann::json> move_forwards_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->move_future("forward");
		}
		boost::promise<nlohmann::json> a;
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
	boost::future<nlohmann::json> move_backwards_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->move_future("back");
		}
		boost::promise<nlohmann::json> a;
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
	boost::future<nlohmann::json> move_up_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->execute_buffer_future(up);
		}
		boost::promise<nlohmann::json> a;
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
	boost::future<nlohmann::json> move_down_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->execute_buffer_future(down);
		}
		boost::promise<nlohmann::json> a;
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
	boost::future<nlohmann::json> point_future(Direction direction)
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
		boost::promise<nlohmann::json> a;
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
	boost::future<nlohmann::json> rotate_right_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->rotate_future("right");
		}
		boost::promise<nlohmann::json> a;
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
	boost::future<nlohmann::json> rotate_left_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->rotate_future("left");
		}
		boost::promise<nlohmann::json> a;
		a.set_value(nlohmann::json::object());
		return a.get_future();
	}
	void break_block(Direction direction)
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
				con->execute_buffer(break_0);
				break;
			case 1:
				con->execute_buffer(break_1);
				break;
			case 2:
				con->execute_buffer(break_2);
				break;
			case 3:
				con->execute_buffer(break_3);
				break;
			}
		}
	}
	boost::future<nlohmann::json> break_block_future(Direction direction)
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
				return con->execute_buffer_future(break_0);
			case 1:
				return con->execute_buffer_future(break_1);
			case 2:
				return con->execute_buffer_future(break_2);
			case 3:
				return con->execute_buffer_future(break_3);
			}
		}
		return boost::make_ready_future(nlohmann::json::object());
	}
	void break_block_up()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->break_block("up");
		}
	}
	boost::future<nlohmann::json> break_block_up_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->break_block_future("up");
		}
		return boost::make_ready_future(nlohmann::json::object());
	}
	void break_block_down()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->break_block("down");
		}
	}
	boost::future<nlohmann::json> break_block_down_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->break_block_future("down");
		}
		return boost::make_ready_future(nlohmann::json::object());
	}
	void break_block_front()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->break_block("forward");
		}
	}
	boost::future<nlohmann::json> break_block_front_future()
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->break_block_future("forward");
		}
		return boost::make_ready_future(nlohmann::json::object());
	}
	void place_block(Direction direction, int slot)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			int offset = (static_cast<int>(direction)
			              - static_cast<int>(position.direction) + 4)
			             % 4;
			CommandBuffer place;
			switch (offset)
			{
			case 0:
				place.place_block("forward", slot);
				con->execute_buffer(place);
				break;
			case 1:
				place.buffer(rotate_1);
				place.place_block("forward", slot);
				con->execute_buffer(forward_1);
				break;
			case 2:
				place.buffer(rotate_2);
				place.place_block("forward", slot);
				con->execute_buffer(forward_2);
				break;
			case 3:
				place.buffer(rotate_3);
				place.place_block("forward", slot);
				con->execute_buffer(forward_3);
				break;
			}
		}
	}
	boost::future<nlohmann::json>
	place_block_future(Direction direction, int slot)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			int offset = (static_cast<int>(direction)
			              - static_cast<int>(position.direction) + 4)
			             % 4;
			CommandBuffer<nlohmann::json> place;
			switch (offset)
			{
			case 0:
				place.place_block("forward", slot);
				return con->execute_buffer_future(place);
			case 1:
				place.buffer(rotate_1);
				place.place_block("forward", slot);
				return con->execute_buffer_future(place);
			case 2:
				place.buffer(rotate_2);
				place.place_block("forward", slot);
				return con->execute_buffer_future(place);
			case 3:
				place.buffer(rotate_3);
				place.place_block("forward", slot);
				return con->execute_buffer_future(place);
			}
		}
		return boost::make_ready_future(nlohmann::json::object());
	}
	void place_block_up(int slot)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->place_block("up", slot);
		}
	}
	boost::future<nlohmann::json> place_block_up_future(int slot)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->place_block_future("up", slot);
		}
		return boost::make_ready_future(nlohmann::json::object());
	}
	void place_block_down(int slot)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->place_block("down", slot);
		}
	}
	boost::future<nlohmann::json> place_block_down_future(int slot)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->place_block_future("down", slot);
		}
		return boost::make_ready_future(nlohmann::json::object());
	}
	void place_block_front(int slot)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			con->place_block("forward", slot);
		}
	}
	boost::future<nlohmann::json> place_block_front_future(int slot)
	{
		if (!connection.expired())
		{
			auto con = connection.lock().get();
			return con->place_block_future("forward", slot);
		}
		return boost::make_ready_future(nlohmann::json::object());
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

			break_0.break_block("forward");
			break_0.SetDefaultParser();

			break_1.buffer(rotate_1);
			break_1.break_block("forward");
			break_1.SetDefaultParser();

			break_2.buffer(rotate_2);
			break_2.break_block("forward");
			break_2.SetDefaultParser();

			break_3.buffer(rotate_3);
			break_3.break_block("forward");
			break_3.SetDefaultParser();

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
	static CommandBuffer<nlohmann::json> break_0;
	static CommandBuffer<nlohmann::json> break_1;
	static CommandBuffer<nlohmann::json> break_2;
	static CommandBuffer<nlohmann::json> break_3;
	static CommandBuffer<nlohmann::json> up;
	static CommandBuffer<nlohmann::json> down;

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive &ar, const unsigned int version)
	{
		ar &position;
		ar &name;
		if (version >= 1)
		{
			ar &inventory;
		}
		if (version >= 2)
		{
			ar &value;
		}
	}
};

BOOST_CLASS_VERSION(Turtle, 2)

struct BlockValue
{
	enum uses
	{
		FarmingSoil = 1 << 0,
		FarmingStorage = 1 << 1,
		FarmingSeed = 1 << 2
	} use;

	bool is_being_checked = false;

	TurtleValue::jobs associated_jobs;

	std::string to_plant;
	std::vector<std::string> allowed_items;

	std::chrono::steady_clock::time_point last_check
	    = std::chrono::steady_clock::now() - 24h;
	std::chrono::duration<double> check_every;

	private:
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive &ar, unsigned int version)
	{
		ar &use;
		ar &to_plant;
		ar &allowed_items;
		double every = check_every.count();
		ar &every;
		check_every = std::chrono::duration<double>{every};
		ar &is_being_checked;
	}
};

struct Block
{
	WorldLocation position;
	glm::dvec3 color;
	std::string name;
	int metadata;
	nlohmann::json blockstate;
	std::optional<BlockValue> value;

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
		if (version >= 1)
		{
			ar &value;
		}
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
		if (version >= 1)
		{
			ar &value;
		}
	}
};

BOOST_CLASS_VERSION(Block, 1)

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
        std::unordered_map<typename first_in_pack_s<T...>::type, U>> &erase_from,
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

struct ServerSettings
{
	bool right_click_harvest = true; // available on most modded servers
	std::unordered_map<std::string, std::vector<std::string>>
	    plant_drops; // seed_name -> seed_drops ("minecraft:wheat_seeds" -> {"minecraft:wheat"})
	std::unordered_set<std::string>
	    blocks_to_not_be_ontop_of; //example: farmland,
	                               //if a turtle stands ontop of farmland,
	                               //the farmland becomes dirt
	std::unordered_map<std::string, int>
	    seed_maturity; //the growth level where a seed is mature

	private:
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive &ar, unsigned int version)
	{
		ar &right_click_harvest;
		ar &plant_drops;
		ar &blocks_to_not_be_ontop_of;
		ar &seed_maturity;
	}
};

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

		inventory_get_buffer.inventory();
		inventory_get_buffer.SupplyOutputParser(
		    [](nlohmann::json result) -> decltype(Turtle::inventory) {
			    decltype(Turtle::inventory) inventory;
			    auto items = result.at(0);
			    for (size_t i = 1; i < 16; i++)
			    {
				    if (items[i].is_null())
				    {
					    inventory[i] = std::nullopt;
				    }
				    else
				    {
					    inventory[i] = Item{};
					    inventory[i]->name = items[i].at("name");
					    inventory[i]->amount = items[i].at("count");
					    inventory[i]->damage = items[i].at("damage");
				    }
			    }
			    return inventory;
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
				parsed_block.first->position.server = parsed_block.second.server;
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
			if (turtle.current_inventory_get
			    && turtle.current_inventory_get->is_ready())
			{
				turtle.inventory = turtle.current_inventory_get->get();
				turtle.current_inventory_get = std::nullopt;
			}
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

				if (turtle.current_inventory_get == std::nullopt
				    && std::chrono::steady_clock::now()
				               - turtle.last_inventory_get
				           > 5s)
				{
					turtle.current_inventory_get
					    = turtle_connection->execute_buffer_future(
					        inventory_get_buffer);
					turtle.last_inventory_get = std::chrono::steady_clock::now();
				}
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
			if (turtle.second.is_ready())
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
					if (turtle.current_pathing->pending_movement->is_ready())
					{
						if (turtle_requires_repath(turtle))
						{
							turtle.current_pathing->pather
							    = std::make_unique<AStar>(
							        turtle.position.position,
							        turtle.current_pathing->target,
							        make_turtle_obstacle_function(turtle));
							turtle.current_pathing->result = boost::async(
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
					if (turtle.current_pathing->result.is_ready())
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
			turtle.current_pathing->pending_movement = turtle.move_down_future();
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

	std::optional<std::reference_wrapper<Block>> block_at(
	    const std::string &server_name,
	    const std::string &dimension_name,
	    glm::ivec3 location)
	{
		if (auto server = m_blocks.find(server_name); server != m_blocks.end())
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
							return y->second.at(z->first);
						}
					}
				}
			}
		}
		return std::nullopt;
	}

	std::function<bool(glm::ivec3)> make_turtle_obstacle_function(Turtle &turtle)
	{
		return [this,
		        name = turtle.name,
		        server_name = turtle.position.server,
		        dimension_name
		        = turtle.position.dimension](glm::ivec3 location) -> bool {
			auto block = block_at(server_name, dimension_name, location);
			if (block)
			{
				return true;
			}
			auto block_below = block_at(
			    server_name,
			    dimension_name,
			    location + glm::ivec3{0, -1, 0});
			if (block_below)
			{
				if (server_settings[server_name]
				        .blocks_to_not_be_ontop_of.contains(
				            block_below->get().name))
				{
					return true;
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
	std::function<bool(glm::ivec3)>
	make_turtle_allow_mining_obstacle_function(Turtle &turtle)
	{
		return [this,
		        name = turtle.name,
		        server = turtle.position.server,
		        dimension
		        = turtle.position.dimension](glm::ivec3 position) -> bool {
			auto block = block_at(server, dimension, position);
			if (block)
			{
				if (block->get().value)
				{
					return true;
				}
			}
			auto block_below = block_at(server, dimension, position);
			if (block_below)
			{
				if (server_settings[server].blocks_to_not_be_ontop_of.contains(
				        block_below->get().name))
				{
					return true;
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
	CommandBuffer<decltype(Turtle::inventory)> inventory_get_buffer;
	std::unordered_map<
	    std::string,
	    std::unordered_map<
	        std::string,
	        std::unordered_map<
	            int,
	            std::unordered_map<int, std::unordered_map<int, Block>>>>>
	    m_blocks;

	std::unordered_map<std::string, ServerSettings> server_settings;

	std::vector<Turtle> m_turtles;
	std::vector<std::pair<
	    std::shared_ptr<ComputerInterface>,
	    boost::future<std::optional<
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
		if (version >= 1)
		{
			ar &server_settings;
		}
	}
};

BOOST_CLASS_VERSION(World, 1)

void server_automation(World &world, const std::string server_name, bool &stop);
