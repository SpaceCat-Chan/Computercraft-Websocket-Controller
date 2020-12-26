#include "Window.hpp"

SDL_GLContext Window::m_Context;

Window::Window(Window &&Move)
{
	m_Window = Move.m_Window;

	Move.m_Window = nullptr;
	Move.m_Context = nullptr;
}

const Window &Window::operator=(Window &&Move)
{
	Destroy();

	m_Window = Move.m_Window;

	Move.m_Window = nullptr;

	return *this;
}

void Window::Destroy()
{
	if (m_Window)
	{
		SDL_DestroyWindow(m_Window);
		m_Window = nullptr;
	}
}
void Window::Load(std::string WindowName, long W, long H)
{
	Destroy();
	m_Window = SDL_CreateWindow(WindowName.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H, SDL_WINDOW_OPENGL);
	if (m_Window == nullptr)
	{
		std::cerr << "unable to create window: " << WindowName << " with {W, H}: {" << W << ", " << H << "}\nSDL_Error: " << SDL_GetError() << '\n';
		throw;
	}
}

void Window::Bind()
{
	if (m_Context == nullptr)
	{
		m_Context = SDL_GL_CreateContext(m_Window);
		if (m_Context == nullptr)
		{
			std::cerr << "Failed to create Context\nSDL_Error: " << SDL_GetError() << '\n';
			throw;
		}
	}
	if (SDL_GL_MakeCurrent(m_Window, m_Context) < 0)
	{
		std::cerr << "unable to bind Context\nSDL_Error: " << SDL_GetError() << '\n';
	}
}