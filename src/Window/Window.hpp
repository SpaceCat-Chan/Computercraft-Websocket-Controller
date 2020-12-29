/**
 * \file Window.hpp
 */

#pragma once

#include <iostream>
#include <memory>
#include <functional>

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

/**
 * \class Window
 * \brief a class that handles one window
 */

class Window
{
	SDL_Window *m_Window = nullptr;
	static SDL_GLContext m_Context;

	friend void SDL_GL_SwapWindow(Window &);
	friend void ImGui_ImplSDL2_InitForOpenGL(Window& a)
	{
		ImGui_ImplSDL2_InitForOpenGL(a.m_Window, m_Context);
	}
	friend void ImGui_ImplSDL2_NewFrame(Window& a)
	{
		ImGui_ImplSDL2_NewFrame(a.m_Window);
	}

public:
	Window() = default;
	/**
	 * \brief construct a window with basic info
	 * 
	 * \param WindowName the name of the window
	 * \param W the Width of the window
	 * \param H the Height of the window
	 */
	Window(std::string WindowName, long W, long H);

	Window(const Window &Copy) = delete;
	/**
	 * \brief construct a window from moving another window
	 * 
	 * \param Move the window to move from
	 * 
	 * notice: the other window will be cleared
	 */
	Window(Window &&Move);

	const Window &operator=(const Window &Copy) = delete;
	/**
	 * \brief move another window
	 * 
	 * \param Move the window to move
	 * \return a refernce to this window
	 * 
	 * notice: the other window will be cleared
	 */
	const Window &operator=(Window &&Move);

	/**
	 * Destroys the window
	 */
	void Destroy();
	/**
	 * \brief Creates a new info with basic info
	 * 
	 * \param WindowName the name of the window to create
	 * \param W the Width of the window
	 * \param H the Height of the window
	 */
	void Load(std::string WindowName, long W, long H);

	/**
	 * \brief binds this window to be drawn to
	 */
	void Bind();
};

inline void SDL_GL_SwapWindow(Window &window)
{
	SDL_GL_SwapWindow(window.m_Window);
}
