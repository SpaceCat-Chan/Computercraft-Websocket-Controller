#include "world.hpp"

CommandBuffer<nlohmann::json> Turtle::rotate_1{};
CommandBuffer<nlohmann::json> Turtle::rotate_2{};
CommandBuffer<nlohmann::json> Turtle::rotate_3{};
CommandBuffer<nlohmann::json> Turtle::forward_0{};
CommandBuffer<nlohmann::json> Turtle::forward_1{};
CommandBuffer<nlohmann::json> Turtle::forward_2{};
CommandBuffer<nlohmann::json> Turtle::forward_3{};
CommandBuffer<nlohmann::json> Turtle::break_0{};
CommandBuffer<nlohmann::json> Turtle::break_1{};
CommandBuffer<nlohmann::json> Turtle::break_2{};
CommandBuffer<nlohmann::json> Turtle::break_3{};
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
	result = boost::async(&AStar::run, pather.get());
}
Pathing::Pathing(
    glm::ivec3 _target,
    Turtle &turtle,
    std::function<bool(glm::ivec3)> obstacle)
{
	target = _target;
	pather
	    = std::make_unique<AStar>(turtle.position.position, _target, obstacle);
	result = boost::async(&AStar::run, pather.get());
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

std::optional<std::reference_wrapper<Turtle>> find_closest_turtle(
    glm::ivec3 position,
    TurtleValue::jobs jobs,
    World &world,
    std::string server_name,
    std::string dimension)
{
	std::multimap<double, std::reference_wrapper<Turtle>> turtles;
	for (auto &turtle : world.m_turtles)
	{
		if (turtle.position.server == server_name
		    && turtle.position.dimension == dimension)
		{
			if ((turtle.value.job & jobs) != 0
			    && turtle.value.current_action == std::nullopt)
			{
				turtles.emplace(
				    glm::distance(
				        glm::dvec3{turtle.position.position},
				        glm::dvec3{position}),
				    turtle);
			}
		}
	}
	auto turtle_iter = turtles.begin();
	if (turtle_iter == turtles.end())
	{
		return std::nullopt;
	}
	else
	{
		return turtle_iter->second;
	}
}

std::optional<std::reference_wrapper<Turtle>> find_closest_turtle(
    WorldLocation &position,
    TurtleValue::jobs jobs,
    World &world)
{
	return find_closest_turtle(
	    position.position,
	    jobs,
	    world,
	    position.server,
	    position.dimension);
}

std::optional<int> find_item_slot(Turtle &turtle, std::string name)
{
	for (size_t i = 0; i < turtle.inventory.size(); i++)
	{
		auto &item = turtle.inventory[i];
		if (item)
		{
			if (item->name == name)
			{
				return i;
			}
		}
	}
	return std::nullopt;
}

void server_automation(World &world, const std::string server_name, bool &stop)
{
	while (!stop)
	{
		if (world.m_blocks.find(server_name) == world.m_blocks.end())
		{
			//this server no longer exists
			return;
		}
		//first, check all turtles and make sure they are doing thier current actions
		for (auto &turtle : world.m_turtles)
		{
			if (turtle.position.server != server_name
			    || !turtle.value.current_action)
			{
				continue;
			}
			if (turtle.connection.expired())
			{
				continue;
			}
			if (turtle.value.current_action == TurtleValue::place_block
			    || turtle.value.current_action == TurtleValue::destroy_block
			    || turtle.value.current_action == TurtleValue::harvest_plant)
			{
				if (!turtle.current_pathing)
				{
					turtle.current_pathing = Pathing{
					    turtle.value.where,
					    turtle,
					    world.make_turtle_allow_mining_obstacle_function(
					        turtle)};
				}
				else if (turtle.current_pathing->target != turtle.value.where)
				{
					turtle.current_pathing = Pathing{
					    turtle.value.where,
					    turtle,
					    world.make_turtle_allow_mining_obstacle_function(
					        turtle)};
				}
				else
				{
					if (turtle.current_pathing->finished)
					{
						if (turtle.current_pathing->unable_to_path)
						{
							//fuck, ask user for help or something
							continue;
						}
						else
						{
							switch (turtle.value.direction.index())
							{
							case 0:
								if (turtle.value.current_action
								    == TurtleValue::destroy_block)
								{
									turtle.break_block(
									    std::get<0>(turtle.value.direction));
								}
								else if (
								    turtle.value.current_action
								    == TurtleValue::place_block)
								{
									turtle.place_block(
									    std::get<0>(turtle.value.direction),
									    turtle.value.inventory_slot);
								}
								else if (
								    turtle.value.current_action
								    == TurtleValue::harvest_plant)
								{
									if (world.server_settings[server_name]
									        .right_click_harvest)
									{
										auto slot = find_item_slot(
										    turtle,
										    "minecraft:stick");
										if (slot)
										{
											turtle.place_block(
											    std::get<0>(
											        turtle.value.direction),
											    *slot);
										}
										else
										{
											turtle.break_block(std::get<0>(
											    turtle.value.direction));
										}
									}
									else
									{
										turtle.break_block(
										    std::get<0>(turtle.value.direction));
									}
								}

								break;
							case 1:
								if (turtle.value.current_action
								    == TurtleValue::destroy_block)
								{
									turtle.break_block_up();
								}
								else if (
								    turtle.value.current_action
								    == TurtleValue::place_block)
								{
									turtle.place_block_up(
									    turtle.value.inventory_slot);
								}
								else if (
								    turtle.value.current_action
								    == TurtleValue::harvest_plant)
								{
									if (world.server_settings[server_name]
									        .right_click_harvest)
									{
										auto slot = find_item_slot(
										    turtle,
										    "minecraft:stick");
										if (slot)
										{
											turtle.place_block_up(*slot);
										}
										else
										{
											turtle.break_block_up();
										}
									}
									else
									{
										turtle.break_block_up();
									}
								}
								break;
							case 2:
								if (turtle.value.current_action
								    == TurtleValue::destroy_block)
								{
									turtle.break_block_down();
								}
								else if (
								    turtle.value.current_action
								    == TurtleValue::place_block)
								{
									turtle.place_block_down(
									    turtle.value.inventory_slot);
								}
								else if (
								    turtle.value.current_action
								    == TurtleValue::harvest_plant)
								{
									if (world.server_settings[server_name]
									        .right_click_harvest)
									{
										auto slot = find_item_slot(
										    turtle,
										    "minecraft:stick");
										if (slot)
										{
											turtle.place_block_down(*slot);
										}
										else
										{
											turtle.break_block_down();
										}
									}
									else
									{
										turtle.break_block_down();
									}
								}
								break;
							}
							turtle.current_pathing = std::nullopt;
							if (turtle.value.current_action
							    == TurtleValue::harvest_plant)
							{
								turtle.value.where += glm::ivec3{0, 1, 0};
								switch (turtle.value.direction.index())
								{
								case 0:
									turtle.value.where
									    += -direction_to_orientation(
									        std::get<0>(turtle.value.direction));
									break;
								case 1:
									turtle.value.where += glm::ivec3{0,1,0};
									break;
								case 2:
									turtle.value.where += glm::ivec3{0,-1,0};
									break;
								}
								turtle.value.current_offset
								    = glm::ivec3{-1, 0, -1};
								turtle.value.current_action
								    = TurtleValue::picking_up_plant_drops;
							}
							else
							{
								turtle.value.current_action = std::nullopt;
							}
						}
					}
				}
			}
			else if (
			    turtle.value.current_action
			    == TurtleValue::picking_up_plant_drops)
			{
				if (turtle.current_pathing == std::nullopt)
				{
					turtle.current_pathing = Pathing{
					    turtle.value.where + turtle.value.current_offset,
					    turtle,
					    world.make_turtle_allow_mining_obstacle_function(
					        turtle)};
				}
				else if (turtle.current_pathing->finished)
				{
					if (turtle.current_pathing->unable_to_path)
					{
						continue;
					}
					else
					{
						auto con = turtle.connection.lock();

						con->pickup_item("down");

						turtle.value.current_offset.x++;
						if (turtle.value.current_offset.x >= 2)
						{
							turtle.value.current_offset.x = -1;
							turtle.value.current_offset.z++;
						}
						if (turtle.value.current_offset.z >= 2)
						{
							turtle.value.current_action = std::nullopt;
						}
						else
						{
							turtle.current_pathing = Pathing{
							    turtle.value.where + turtle.value.current_offset,
							    turtle,
							    world.make_turtle_allow_mining_obstacle_function(
							        turtle)};
						}
					}
				}
			}
			else if (turtle.value.current_action == TurtleValue::checking_block)
			{
				if (turtle.current_pathing == std::nullopt)
				{
					turtle.current_pathing = Pathing{
					    turtle.value.where + turtle.value.current_offset,
					    turtle,
					    world.make_turtle_allow_mining_obstacle_function(
					        turtle)};
				}
				else if (turtle.current_pathing->finished)
				{
					if (turtle.current_pathing->unable_to_path)
					{
						if ((turtle.value.current_offset.x
						     + turtle.value.current_offset.y
						     + turtle.value.current_offset.z)
						    == -1)
						{
							turtle.value.current_offset *= -1;
							turtle.current_pathing = Pathing{
							    turtle.value.where + turtle.value.current_offset,
							    turtle,
							    world.make_turtle_allow_mining_obstacle_function(
							        turtle)};
						}
						else
						{
							if (turtle.value.current_offset.y)
							{
								turtle.value.current_offset
								    = glm::ivec3{-1, 0, 0};
								turtle.current_pathing = Pathing{
								    turtle.value.where
								        + turtle.value.current_offset,
								    turtle,
								    world
								        .make_turtle_allow_mining_obstacle_function(
								            turtle)};
							}
							else if (turtle.value.current_offset.x)
							{
								turtle.value.current_offset
								    = glm::ivec3{0, 0, -1};
								turtle.current_pathing = Pathing{
								    turtle.value.where
								        + turtle.value.current_offset,
								    turtle,
								    world
								        .make_turtle_allow_mining_obstacle_function(
								            turtle)};
							}
							else
							{
								//unable to get to block
								continue;
							}
						}
					}
					else
					{
						if (turtle.value.current_offset.y == 0)
						{
							turtle.point(orientation_to_direction(
							    -turtle.value.current_offset));
						}
						turtle.value.current_action = std::nullopt;
						auto &block = world.m_blocks.at(
						    server_name)[turtle.position.dimension]
						                [turtle.position.position.x]
						                [turtle.position.position.y]
						                [turtle.position.position.z];
						if (block.value)
						{
							block.value->is_being_checked = true;
							block.value->last_check
							    = std::chrono::steady_clock::now();
						}
					}
				}
			}
		}
		//find blocks in world that need to be rechecked
		for (auto &dimension : world.m_blocks.at(server_name))
		{
			for (auto &x : dimension.second)
			{
				for (auto &y : x.second)
				{
					for (auto &z : y.second)
					{
						if (z.second.value)
						{
							if (z.second.value->last_check
							            - std::chrono::steady_clock::now()
							        > z.second.value->check_every
							    && !z.second.value->is_being_checked)
							{
								//oh boi it's recheck time
								auto turtle_opt = find_closest_turtle(
								    z.second.position.position,
								    z.second.value->associated_jobs,
								    world,
								    server_name,
								    dimension.first);
								if (turtle_opt)
								{
									auto &turtle = turtle_opt->get();
									turtle.value.current_action
									    = TurtleValue::checking_block;
									turtle.value.where
									    = glm::ivec3{x.first, y.first, z.first};
									turtle.value.current_offset
									    = glm::ivec3{0, -1, 0};
									z.second.value->is_being_checked = true;
								}
							}
							if ((z.second.value->use & BlockValue::FarmingSeed)
							    && world.server_settings[server_name]
							           .seed_maturity.contains(z.second.name))
							{
								if (z.second.blockstate["age"]
								    >= world.server_settings[server_name]
								           .seed_maturity[z.second.name])
								{
									auto turtle_opt = find_closest_turtle(
									    z.second.position,
									    TurtleValue::FARMER,
									    world);
									if (turtle_opt)
									{
										auto &turtle = turtle_opt->get();
										turtle.value.current_action
										    = TurtleValue::harvest_plant;
										turtle.value.where
										    = z.second.position.position;
										turtle.value.direction = std::variant<
										    Direction,
										    std::monostate,
										    std::monostate>{
										    std::in_place_index<2>};
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
