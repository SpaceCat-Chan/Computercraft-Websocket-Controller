#include <GL/glew.h>
#include <SDL.h>
#include <glm/ext.hpp>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "misc/cpp/imgui_stdlib.h"

#include "Window/Window.hpp"

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

	RenderWorld render_world;
	world.dirty_renderer = std::bind(&RenderWorld::dirty, &render_world);

	auto run_result = std::async(std::bind(&server_manager::run, &s));

	bool stop = false;
	SDL_Event event;
	float x_sensitivity = 0.01;
	float y_sensitivity = 0.01;
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
			}
		}

		world.add_new_turtles();
		render_world.copy_into_buffers(world);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		render_world.render();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		ImGui::Begin("Hi!");
		ImGui::Text("i exist");
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

		if (ImGui::BeginCombo(
		        "Selected Turtle",
		        render_world.selected_turtle()
		            ? world.m_turtles[*render_world.selected_turtle()]
		                  .name.c_str()
		            : "none"))
		{
			for (size_t i = 0; i < world.m_turtles.size(); i++)
			{
				auto &turtle = world.m_turtles[i];
				if (ImGui::Selectable(turtle.name.c_str()))
				{
					render_world.select_turtle(i);
				}
			}
			ImGui::EndCombo();
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
			if (!turtle.connection.expired())
			{
				auto temp = turtle.connection.lock();
				ImGui::Text("x: %i, y: %i, z: %i, o: ", turtle.position.x, turtle.position.y, turtle.position.z);
				ImGui::InputTextMultiline("eval input", &eval_text);
				if (ImGui::Button("Submit Eval"))
				{
					temp->remote_eval(eval_text);
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

	window.Destroy();
	s.send_stops();
	sleep(1);
	s.stop();
}
