#include <SDL.h>
#include <GL/glew.h>
#include <glm/ext.hpp>

#include "Window/Window.hpp"

#include "Server.hpp"
#include "Computer.hpp"

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

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cerr << "failed to init SDL: " << SDL_GetError() << '\n';
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetSwapInterval(1);

	window.Load("Websocket Control Panel", 1600, 900);
	window.Bind();

	if(auto error = glewInit(); error != GLEW_OK)
	{
		std::cout << "GLEW failed to initialize: " << glewGetErrorString(error) << '\n';
		return 1;
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, nullptr);

	server_manager s;

	World world;
	s.register_new_handler(std::bind(&World::new_turtle, &world, std::placeholders::_1));

	auto run_result = std::async(std::bind(&server_manager::run, &s));

	bool stop = false;
	while (!stop)
	{
		world.add_new_turtles();
		std::string line;
		std::getline(std::cin, line);
		if (line == "stop")
		{
			stop = true;
		}
		else
		{
			s.m_computers.begin()->second->remote_eval(line);
		}
	}
	s.send_stops();
	sleep(1);
	s.stop();
}
