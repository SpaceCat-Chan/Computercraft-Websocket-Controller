#include <fstream>

#include <GL/glew.h>
#include <SDL.h>
#include <glm/ext.hpp>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "misc/cpp/imgui_stdlib.h"

#include "SDL-Helper-Libraries/KeyTracker/KeyTracker.hpp"

#include "Window/Window.hpp"

#include "AStar.hpp"
#include "Computer.hpp"
#include "Server.hpp"

#include "GUI.hpp"
#include "SelectBlock.hpp"
#include "render_world.hpp"
#include "world.hpp"

void static GLAPIENTRY MessageCallback(
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

	constexpr long window_w = 1600, window_h = 900;
	window.Load("Websocket Control Panel", window_w, window_h);
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

	glLineWidth(10.f);
	glEnable(GL_LINE_SMOOTH);

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

	KeyTracker keyboard;

	bool stop = false;
	SDL_Event event;
	float x_sensitivity = 0.01;
	float y_sensitivity = 0.01;
	float mouse_wheel_sensitivity = 0.1;
	auto now = std::chrono::steady_clock::now();
	float autosave_interval = 2.5;
	std::variant<std::monostate, glm::ivec3, size_t> currently_hovered{
	    std::monostate{}};
	std::variant<std::monostate, glm::ivec3, size_t> currently_selected{
	    std::monostate{}};
	std::chrono::steady_clock::time_point mouse_down_time;
	bool in_freecam = false;
	auto frame_end_time = std::chrono::steady_clock::now();
	float forwards_movement_speed = 7.5;
	while (!stop)
	{
		auto frame_start_time = std::chrono::steady_clock::now();
		auto dt = frame_start_time - frame_end_time;
		keyboard.Update(
		    std::chrono::duration_cast<std::chrono::milliseconds>(dt).count());
		while (SDL_PollEvent(&event))
		{
			keyboard.UpdateKey(&event);
			ImGui_ImplSDL2_ProcessEvent(&event);
			switch (event.type)
			{
			case SDL_QUIT:
				stop = true;
				break;
			case SDL_MOUSEMOTION:
				if (!io.WantCaptureMouse && ImGui::IsMouseDragging(0))
				{
					if (!in_freecam)
					{
						render_world.camera.RotateAround(
						    static_cast<double>(event.motion.xrel)
						        * -x_sensitivity,
						    static_cast<double>(event.motion.yrel)
						        * -y_sensitivity);
					}
					else
					{
						render_world.camera.Rotate(
						    static_cast<double>(event.motion.xrel)
						        * -x_sensitivity,
						    static_cast<double>(event.motion.yrel)
						        * -y_sensitivity);
					}
				}
				break;
			case SDL_MOUSEWHEEL:
				if (!io.WantCaptureMouse && !in_freecam)
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
			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == SDL_BUTTON_LEFT)
				{
					mouse_down_time = std::chrono::steady_clock::now();
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (event.button.button == SDL_BUTTON_LEFT)
				{
					if (std::chrono::steady_clock::now() - mouse_down_time
					        < 200ms
					    && !io.WantCaptureMouse)
					{
						currently_selected = currently_hovered;
					}
				}
				break;
			}
		}
		if (!io.WantCaptureMouse && render_world.selected_server()
		    && render_world.selected_dimension())
		{
			int x, y;
			SDL_GetMouseState(&x, &y);
			currently_hovered = find_selected(
			    RayInfo{
			        glm::dvec2{
			            (static_cast<double>(x) / window_w) * 2 - 1,
			            ((static_cast<double>(y) / window_h) * 2 - 1) * -1},
			        render_world},
			    world,
			    *render_world.selected_server(),
			    *render_world.selected_dimension());
			if (currently_hovered.index() == 0)
			{
				render_world.select_location(std::nullopt);
			}
			else if (currently_hovered.index() == 1)
			{
				auto selected_value = std::get<1>(currently_hovered);
				auto &Block = world.m_blocks[*render_world.selected_server()]
				                            [*render_world.selected_dimension()]
				                            [selected_value.x][selected_value.y]
				                            [selected_value.z];
				render_world.select_location(Block.position.position);
			}
			else if (currently_hovered.index() == 2)
			{
				auto selected_turtle = std::get<2>(currently_hovered);
				auto &turtle = world.m_turtles[selected_turtle];
				render_world.select_location(turtle.position.position);
			}
		}
		auto ddt = std::chrono::duration_cast<std::chrono::duration<double>>(dt)
		               .count();
		if (keyboard[SDL_SCANCODE_W].Active && in_freecam)
		{
			auto forward = render_world.camera.GetViewVector()
			               * (ddt * forwards_movement_speed);
			render_world.camera.Move(forward);
		}
		if (keyboard[SDL_SCANCODE_S].Active && in_freecam)
		{
			auto forward = -1.0 * render_world.camera.GetViewVector()
			               * (ddt * forwards_movement_speed);
			render_world.camera.Move(forward);
		}
		if (keyboard[SDL_SCANCODE_SPACE].Clicked)
		{
			in_freecam = !in_freecam;
			if (!in_freecam)
			{
				auto view_dir = render_world.camera.GetViewVector();
				if (render_world.selected_turtle())
				{
					auto &turtle
					    = world.m_turtles[*render_world.selected_turtle()];
					auto turtle_location = glm::dvec3{turtle.position.position}
					                       + glm::dvec3{0.5, 0.5, 0.5};
					render_world.camera.LookAt(turtle_location);
					render_world.camera.MoveTo(
					    turtle_location - (glm::sqrt(12.0) * view_dir));
				}
				else
				{
					render_world.camera.LookAt(
					    view_dir * glm::sqrt(12.0)
					    + render_world.camera.GetPosition());
				}
			}
			else
			{
				render_world.camera.LookIn(
				    render_world.camera.GetViewVector()
				    - render_world.camera.GetPosition());
			}
		}

		world.add_new_turtles();
		world.update_pathings();
		render_world.copy_into_buffers(world, in_freecam);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		render_world.render();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		draw_main_ui(
		    world,
		    render_world,
		    s,
		    x_sensitivity,
		    y_sensitivity,
		    mouse_wheel_sensitivity,
		    forwards_movement_speed,
		    autosave_interval,
		    currently_hovered,
		    currently_selected);

		if (render_world.selected_turtle())
		{
			bool close = draw_turtle_ui(
			    world.m_turtles[*render_world.selected_turtle()],
			    world);
			if (close)
			{
				render_world.select_turtle(std::nullopt);
			}
		}

		if (currently_selected.index() != 0)
		{
			static bool open = true;
			if (ImGui::Begin("selection", &open))
			{
				if (currently_selected.index() == 1)
				{
					// block
					auto selected_value = std::get<1>(currently_selected);
					auto &Block
					    = world.m_blocks[*render_world.selected_server()]
					                    [*render_world.selected_dimension()]
					                    [selected_value.x][selected_value.y]
					                    [selected_value.z];
					draw_selected_ui(
					    Block,
					    world,
					    render_world,
					    currently_selected);
				}
				else if (currently_selected.index() == 2)
				{
					// turtle
					auto selected_turtle = std::get<2>(currently_selected);
					auto &turtle = world.m_turtles[selected_turtle];
					bool remove = draw_selected_ui(
					    turtle,
					    world,
					    render_world,
					    currently_selected);
					if (remove)
					{
						world.m_turtles.erase(
						    world.m_turtles.begin() + selected_turtle);
						render_world.dirty();
					}
				}
			}
			ImGui::End();
			if (open == false)
			{
				open = true;
				currently_selected = std::monostate{};
			}
		}
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);

		auto newer_now = std::chrono::steady_clock::now();
		auto time_since_last_save = std::chrono::duration_cast<
		    std::chrono::duration<float, std::ratio<60>>>(newer_now - now);
		if (time_since_last_save.count() > autosave_interval)
		{
			std::fstream default_save{
			    "world_default.save",
			    std::ios::out | std::ios::trunc};
			boost::archive::text_oarchive ar{default_save};
			ar << world;
			now = newer_now;
			std::cout << "autosave\n";
		}
		frame_end_time = frame_start_time;
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
