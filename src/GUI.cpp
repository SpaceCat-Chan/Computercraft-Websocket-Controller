#include "GUI.hpp"

#include <fstream>

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

void draw_main_ui(
    World &world,
    RenderWorld &render_world,
    server_manager &s,
    float &x_sensitivity,
    float &y_sensitivity,
    float &mouse_wheel_sensitivity,
    float &forwards_movement_speed,
    float &autosave_interval,
    std::variant<std::monostate, glm::ivec3, size_t> &currently_hovered,
    std::variant<std::monostate, glm::ivec3, size_t> &currently_selected)
{
	ImGui::Begin("Hi!");
	if (ImGui::TreeNode("Settings"))
	{
		ImGui::SliderFloat(
		    "x sensitivity",
		    &x_sensitivity,
		    0.0,
		    0.05,
		    "%f",
		    ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat(
		    "y sensitivity",
		    &y_sensitivity,
		    0.0,
		    0.05,
		    "%f",
		    ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat(
		    "mouse wheel sensitivity",
		    &mouse_wheel_sensitivity,
		    0.001,
		    0.25,
		    "%f",
		    ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat(
		    "forwards speed",
		    &forwards_movement_speed,
		    0.25,
		    25,
		    "%f",
		    ImGuiSliderFlags_Logarithmic);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("World"))
	{
		if (ImGui::BeginCombo(
		        "Selected Server",
		        render_world.selected_server().value_or("none").c_str()))
		{
			for (auto &server : world.m_blocks)
			{
				if (ImGui::Selectable(server.first.c_str()))
				{
					render_world.select_server(server.first);
					auto first_dimension = server.second.begin();
					if (first_dimension != server.second.end())
					{
						render_world.select_dimension(first_dimension->first);
					}
					else
					{
						render_world.select_dimension();
					}
					currently_selected = std::monostate{};
					currently_hovered = std::monostate{};
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::BeginCombo(
		        "Selected Dimension",
		        render_world.selected_dimension().value_or("none").c_str()))
		{
			if (render_world.selected_server())
			{
				for (auto &dimension :
				     world.m_blocks[*render_world.selected_server()])
				{
					if (ImGui::Selectable(dimension.first.c_str()))
					{
						render_world.select_dimension(dimension.first);
						currently_selected = std::monostate{};
						currently_hovered = std::monostate{};
					}
				}
			}
			ImGui::EndCombo();
		}

		static bool show_all_turtles = false;

		if (ImGui::BeginCombo(
		        "Selected Turtle",
		        render_world.selected_turtle_name(world)))
		{
			for (size_t i = 0; i < world.m_turtles.size(); i++)
			{
				auto &turtle = world.m_turtles[i];
				if (show_all_turtles
				    || (turtle.position.server == render_world.selected_server()
				        && turtle.position.dimension
				               == render_world.selected_dimension()))
				{

					if (ImGui::Selectable(turtle.name.c_str()))
					{
						render_world.select_turtle(i);
						render_world.select_server(turtle.position.server);
						render_world.select_dimension(
						    turtle.position.dimension);
					}
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Checkbox("show all turtles", &show_all_turtles);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("import/export"))
	{
		static std::string filename;
		ImGui::InputText("filename", &filename);
		if (ImGui::Button("Import"))
		{
			std::fstream file{filename, std::ios::in};
			if (file.is_open())
			{
				boost::archive::text_iarchive ar{file};
				s.send_stops();
				sleep(1);
				ar &world;
			}
			else
			{
				std::cout << "attempt to import from file: " << filename
				          << " failed";
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Export"))
		{
			std::fstream file{filename, std::ios::out | std::ios::trunc};
			boost::archive::text_oarchive ar{file};
			ar &world;
		}
		ImGui::SliderFloat(
		    "autosave interval",
		    &autosave_interval,
		    0.5,
		    10.0,
		    "%.3f minutes");
		ImGui::TreePop();
	}

	static bool demo_toggled = false;
	ImGui::Checkbox("toggle demo", &demo_toggled);
	if (demo_toggled)
	{
		ImGui::ShowDemoWindow();
	}
	ImGui::End();
}

void draw_turtle_inventory(Turtle &turtle, World &world)
{
	enum class MODE
	{
		full_stack,
		half_stack,
		amount
	};
	static MODE mode = MODE::full_stack;
	if (ImGui::RadioButton("Full Stack", mode == MODE::full_stack))
	{
		mode = MODE::full_stack;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Half Stack", mode == MODE::half_stack))
	{
		mode = MODE::half_stack;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Amount", mode == MODE::amount))
	{
		mode = MODE::amount;
	}
	ImGui::SameLine();
	static int amount = 0;
	ImGui::InputInt("a", &amount);
	for (size_t n = 0; n < 16; n++)
	{
		ImGui::PushID(n);
		if ((n % 4) != 0)
		{
			ImGui::SameLine();
		}
		auto item = turtle.inventory[n];
		std::string button_text;
		if (item)
		{
			button_text = std::to_string(item->amount);
		}
		ImGui::Button(button_text.c_str(), {50, 50});
		auto is_hovered = ImGui::IsItemHovered();

		if (item && turtle.current_inventory_get == std::nullopt
		    && !turtle.connection.expired())
		{
			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("INVENTORY_DRAG", &n, sizeof(n));
				switch (mode)
				{
				case MODE::full_stack:
					ImGui::Text(
					    "%s\nmoving %i out of %i",
					    item->name.c_str(),
					    item->amount,
					    item->amount);
					break;
				case MODE::half_stack:
					ImGui::Text(
					    "%s\nmoving %i out of %i",
					    item->name.c_str(),
					    item->amount / 2,
					    item->amount);
					break;
				case MODE::amount:
					ImGui::Text(
					    "%s\nmoving %i out of %i",
					    item->name.c_str(),
					    amount,
					    item->amount);
					break;
				}
				ImGui::EndDragDropSource();
			}
		}
		if (turtle.current_inventory_get == std::nullopt
		    && !turtle.connection.expired())
		{
			if (ImGui::BeginDragDropTarget())
			{
				if (auto payload
				    = ImGui::AcceptDragDropPayload("INVENTORY_DRAG");
				    payload != nullptr)
				{
					decltype(n) from
					    = *static_cast<decltype(n) *>(payload->Data);
					
					if (from != n)
					{
						std::optional<int> move_amount;
						switch (mode)
						{
						case MODE::full_stack:
							break;
						case MODE::half_stack:
							move_amount = item->amount / 2;
							break;
						case MODE::amount:
							move_amount = amount;
							break;
						}
						auto con = turtle.connection.lock();
						turtle.current_inventory_get
						    = con->inventory_move_future(from+1, n+1, move_amount)
						          .then([&turtle,
						                 &world](boost::future<nlohmann::json>
						                             result) {
							          if (!turtle.connection.expired())
							          {
								          auto con = turtle.connection.lock();
								          turtle.last_inventory_get = std::
								              chrono::steady_clock::now();
								          return con
								              ->execute_buffer_future(
								                  world.inventory_get_buffer)
								              .get();
							          }
							          // connection vanished, unable to get new
							          // inventory
							          return turtle.inventory;
						          });
					}
				}
				ImGui::EndDragDropTarget();
			}
		}

		if (item && is_hovered
		    && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			ImGui::BeginTooltip();
			ImGui::Text("%s\ndamage: %i", item->name.c_str(), item->damage);
			ImGui::EndTooltip();
		}
		ImGui::PopID();
	}
}

bool draw_turtle_ui(Turtle &turtle, World &world)
{
	static bool open = true;
	ImGui::Begin("Turtle Control", &open);
	static std::string eval_text;
	ImGui::Text(
	    "x: %i, y: %i, z: %i, o: %s, dimension: %s, server: %s",
	    turtle.position.position.x,
	    turtle.position.position.y,
	    turtle.position.position.z,
	    direction_to_string(turtle.position.direction),
	    turtle.position.dimension.c_str(),
	    turtle.position.server.c_str());
	if (!turtle.connection.expired())
	{
		auto temp = turtle.connection.lock();
		ImGui::InputTextMultiline("eval input", &eval_text);
		if (ImGui::Button("Submit Eval"))
		{
			temp->remote_eval(eval_text);
		}
		ImGui::SameLine();
		if (ImGui::Button("Submit Auth"))
		{
			temp->auth_message(eval_text);
		}
		if (ImGui::TreeNode("Pathing"))
		{
			static glm::ivec3 path_target;
			ImGui::InputInt3("path target", glm::value_ptr(path_target));
			if (ImGui::Button("Path to target"))
			{
				if (turtle.current_pathing)
				{
					turtle.current_pathing->finished = true;
					turtle.current_pathing->pather->stop = true;
					if (turtle.current_pathing->result.valid())
					{
						turtle.current_pathing->result.wait();
					}
				}
				turtle.current_pathing = Pathing{path_target, turtle, world};
			}
			if (turtle.current_pathing)
			{
				ImGui::Text(
				    "current pathing: going to %i, %i, %i",
				    turtle.current_pathing->target.x,
				    turtle.current_pathing->target.y,
				    turtle.current_pathing->target.z);
				if (turtle.current_pathing->finished)
				{
					ImGui::Text("pathing finished");
				}
				if (turtle.current_pathing->unable_to_path)
				{
					ImGui::Text("unable to path");
				}
				if (ImGui::Button("clear pathing"))
				{
					if (turtle.current_pathing)
					{
						turtle.current_pathing->finished = true;
						turtle.current_pathing->pather->stop = true;
						if (turtle.current_pathing->result.valid())
						{
							turtle.current_pathing->result.wait();
						}
						turtle.current_pathing = std::nullopt;
					}
				}
			}
			else
			{
				ImGui::Text("current pathing: none");
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Movement"))
		{
			ImGui::BeginGroup();
			if (ImGui::Button("Rotate Left"))
			{
				turtle.rotate_left();
			}
			if (ImGui::Button("Move Backwards"))
			{
				turtle.move_backwards();
			}
			ImGui::EndGroup();
			ImGui::SameLine();
			ImGui::BeginGroup();
			if (ImGui::Button("Move Up"))
			{
				turtle.move_up();
			}
			ImGui::Button("Turtle Image");
			if (ImGui::Button("Move Down"))
			{
				turtle.move_down();
			}
			ImGui::EndGroup();
			ImGui::SameLine();
			ImGui::BeginGroup();
			if (ImGui::Button("Rotate Right"))
			{
				turtle.rotate_right();
			}
			if (ImGui::Button("Move Forwards"))
			{
				turtle.move_forwards();
			}
			ImGui::EndGroup();
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Inventory"))
		{
			draw_turtle_inventory(turtle, world);
			ImGui::TreePop();
		}
	}
	else
	{
		ImGui::Text("turtle offline");
	}
	ImGui::End();

	return !open;
}

void draw_selected_ui(
    Block &block,
    World &world,
    RenderWorld &render_world,
    std::variant<std::monostate, glm::ivec3, size_t> &selected)
{
	ImGui::Text(
	    "name: %s\nmetadata: %i\nstate: %s\npos: {%i, %i, %i}",
	    block.name.c_str(),
	    block.metadata,
	    block.blockstate.dump().c_str(),
	    block.position.position.x,
	    block.position.position.y,
	    block.position.position.z);
	if (ImGui::Button("delete from world"))
	{
		erase_nested(
		    world.m_blocks,
		    *render_world.selected_server(),
		    *render_world.selected_dimension(),
		    block.position.position.x,
		    block.position.position.y,
		    block.position.position.z);
		selected = std::monostate{};
		render_world.dirty();
	}
}

bool draw_selected_ui(
    Turtle &turtle,
    World &world,
    RenderWorld &render_world,
    std::variant<std::monostate, glm::ivec3, size_t> &selected)
{
	render_world.select_location(turtle.position.position);
	ImGui::Text(
	    "pos: {%i, %i, %i}, o: %s\n dimension: %s\n "
	    "server: %s\nname: %s",
	    turtle.position.position.x,
	    turtle.position.position.y,
	    turtle.position.position.z,
	    direction_to_string(turtle.position.direction),
	    turtle.position.dimension.c_str(),
	    turtle.position.server.c_str(),
	    turtle.name.c_str());
	if (ImGui::Button("select"))
	{
		render_world.select_turtle(std::get<2>(selected));
		selected = std::monostate{};
	}
	if (ImGui::Button("delete from world"))
	{
		std::scoped_lock a{world.render_mutex};
		auto turtle_index = std::get<2>(selected);
		if (render_world.selected_turtle() == turtle_index)
		{
			render_world.select_turtle(std::nullopt);
		}
		if (!turtle.connection.expired())
		{
			auto con = turtle.connection.lock().get();
			con->send_stop();
		}
		selected = std::monostate{};
		return true;
	}
	return false;
}
