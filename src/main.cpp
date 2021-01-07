#include <fstream>

#include <GL/glew.h>
#include <SDL.h>
#include <glm/ext.hpp>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "misc/cpp/imgui_stdlib.h"

#include "Window/Window.hpp"

#include "AStar.hpp"
#include "Computer.hpp"
#include "Server.hpp"

#include "render_world.hpp"
#include "world.hpp"

void GLAPIENTRY MessageCallback(
    GLenum source, // NOLINT
    GLenum type,
    GLuint id, // NOLINT
    GLenum severity,
    GLsizei length, // NOLINT
    const GLchar *message,
    const void *userParam) // NOLINT
{
	// SDL_SetRelativeMouseMode(SDL_TRUE);
	if (type == 0x8251 || type == 0x8250)
	{
		return;
	}
	std::cerr << "GL CALLBACK: "
	          << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "")
	          << " type = " << std::hex << type << ", severity = " << severity
	          << ", message = " << message << '\n';
}

int main()
{
	Window window;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	{
		std::cerr << "failed to init SDL: " << SDL_GetError() << '\n';
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(
	    SDL_GL_CONTEXT_PROFILE_MASK,
	    SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetSwapInterval(1);

	window.Load("Websocket Control Panel", 1600, 900);
	window.Bind();

	if (auto error = glewInit(); error != GLEW_OK)
	{
		std::cout << "GLEW failed to initialize: " << glewGetErrorString(error)
		          << '\n';
		return 1;
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, nullptr);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(window);
	ImGui_ImplOpenGL3_Init();

	server_manager s;

	World world;
	s.register_new_handler(
	    std::bind(&World::new_turtle, &world, std::placeholders::_1));

	{
		std::fstream default_save{"world_default.save", std::ios::in};
		if (default_save.is_open())
		{
			boost::archive::text_iarchive ar{default_save};
			ar >> world;
		}
	}
	RenderWorld render_world;
	world.dirty_renderer = std::bind(&RenderWorld::dirty, &render_world);
	world.dirty_renderer_pathes
	    = std::bind(&RenderWorld::dirty_paths, &render_world);

	auto run_result = std::async(&server_manager::run, &s);
	auto scheduler = std::async(&server_manager::scheduler, &s);

	bool stop = false;
	SDL_Event event;
	float x_sensitivity = 0.01;
	float y_sensitivity = 0.01;
	float mouse_wheel_sensitivity = 0.1;
	bool demo_toggled = false;
	while (!stop)
	{
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			switch (event.type)
			{
			case SDL_QUIT:
				stop = true;
				break;
			case SDL_MOUSEMOTION:
				if (!io.WantCaptureMouse && ImGui::IsMouseDragging(0))
				{
					glm::dvec3 up = glm::dvec3{0, 3, 0};
					glm::dvec3 camera_position
					    = render_world.camera.GetPosition();
					glm::dvec3 camera_looking_at
					    = render_world.camera.GetViewVector();
					camera_position
					    = glm::translate(
					          glm::rotate(
					              glm::translate(
					                  glm::dmat4{1},
					                  camera_looking_at),
					              static_cast<double>(event.motion.xrel)
					                  * -x_sensitivity,
					              up),
					          -camera_looking_at)
					      * glm::dvec4{camera_position, 1};

					glm::dvec3 to_camera = camera_position - camera_looking_at;
					glm::dvec3 pitch_axis = glm::cross(up, to_camera);
					glm::dvec3 new_camera_position
					    = glm::translate(
					          glm::rotate(
					              glm::translate(
					                  glm::dmat4{1},
					                  camera_looking_at),
					              static_cast<double>(event.motion.yrel)
					                  * -y_sensitivity,
					              pitch_axis),
					          -camera_looking_at)
					      * glm::dvec4{camera_position, 1};
					if (std::abs(glm::dot(
					        glm::normalize(
					            new_camera_position - camera_looking_at),
					        glm::normalize(up)))
					    < glm::cos(glm::radians(1.0)))
					{
						camera_position = new_camera_position;
					}
					render_world.camera.MoveTo(camera_position);
				}
				break;
			case SDL_MOUSEWHEEL:
				if (!io.WantCaptureMouse)
				{
					glm::dvec3 camera_position
					    = render_world.camera.GetPosition();
					glm::dvec3 camera_looking_at
					    = render_world.camera.GetViewVector();
					glm::dvec3 to_camera = camera_position - camera_looking_at;
					double multiplier = static_cast<double>(event.wheel.y)
					                    * mouse_wheel_sensitivity;
					if (event.wheel.direction == SDL_MOUSEWHEEL_NORMAL)
					{
						multiplier *= -1;
					}
					to_camera = to_camera * (multiplier + 1);
					render_world.camera.MoveTo(camera_looking_at + to_camera);
				}
				break;
			}
		}

		world.add_new_turtles();
		world.update_pathings();
		render_world.copy_into_buffers(world);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		render_world.render();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

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
							render_world.select_dimension(
							    first_dimension->first);
						}
						else
						{
							render_world.select_dimension();
						}
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
					    || turtle.position.server
					               == render_world.selected_server()
					           && turtle.position.dimension
					                  == render_world.selected_dimension())
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
			ImGui::TreePop();
		}

		ImGui::Checkbox("toggle demo", &demo_toggled);
		if (demo_toggled)
		{
			ImGui::ShowDemoWindow();
		}
		ImGui::End();

		if (render_world.selected_turtle())
		{
			auto &turtle = world.m_turtles[*render_world.selected_turtle()];
			ImGui::Begin("Turtle Control");
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
					turtle.current_pathing
					    = Pathing{path_target, turtle, world};
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
			}
			else
			{
				ImGui::Text("turtle offline");
			}

			ImGui::End();
		}
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	{
		std::fstream default_save{
		    "world_default.save",
		    std::ios::out | std::ios::trunc};
		boost::archive::text_oarchive ar{default_save};
		ar << world;
	}

	window.Destroy();
	s.scheduler_stop = true;
	scheduler.wait();
	s.send_stops();
	sleep(1);
	s.stop();
}
