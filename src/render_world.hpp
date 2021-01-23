#pragma once

#include <GL/glew.h>
#include <glm/ext.hpp>

#include "Camera/Camera.hpp"
#include "MeshLine/MeshLine.hpp"
#include "Shader/Shader.hpp"
#include "TexturedMesh/TexturedMesh.hpp"
#include "ssbo/ssbo.hpp"

#include "world.hpp"

class RenderWorld
{
	public:
	RenderWorld()
	{
		m_shader.AddShaderFile("res/shader.vert", GL_VERTEX_SHADER);
		m_shader.AddShaderFile("res/shader.frag", GL_FRAGMENT_SHADER);
		m_shader.Link();

		m_line_shader.AddShaderFile("res/line_shader.vert", GL_VERTEX_SHADER);
		m_line_shader.AddShaderFile("res/line_shader.frag", GL_FRAGMENT_SHADER);
		m_line_shader.Link();

		m_basic_shader.AddShaderFile("res/basic_shader.vert", GL_VERTEX_SHADER);
		m_basic_shader.AddShaderFile("res/shader.frag", GL_FRAGMENT_SHADER);
		m_basic_shader.Link();

		m_block_mesh.Load("res/block.obj");
		m_turtle_mesh.Load("res/turtle.obj");
		m_selected_mesh.Load("res/selection.obj");

		camera.MoveTo({2, 2, 2});
		camera.LookAt({0, 0, 0});
		camera.CreateProjectionX(45.0, 16.0 / 9.0, 0.1, 10000.0);
	}

	void copy_into_buffers(World &world, bool in_freecam)
	{
		if (m_is_data_dirty)
		{
			std::scoped_lock<std::mutex> a{world.render_mutex};
			if (m_selected_turtle && !in_freecam)
			{
				auto &turtle = world.m_turtles[*m_selected_turtle];
				auto old_camera_look_at = camera.GetViewVector();
				auto old_camera_position = camera.GetPosition();
				auto look_to_pos = old_camera_position - old_camera_look_at;
				camera.LookAt(
				    glm::dvec3{turtle.position.position}
				    + glm::dvec3{0.5, 0.5, 0.5});
				camera.MoveTo(
				    glm::dvec3{turtle.position.position}
				    + glm::dvec3{0.5, 0.5, 0.5} + look_to_pos);
			}
			std::vector<glm::ivec4> new_block_positions;
			std::vector<glm::vec4> new_block_colors;
			std::vector<glm::ivec4> new_turtle_positions;
			std::vector<glm::vec4> new_turtle_colors;

			if (m_selected_server)
			{
				auto server = world.m_blocks.find(*m_selected_server);
				if (m_selected_dimension && server != world.m_blocks.end())
				{
					auto dimension = server->second.find(*m_selected_dimension);
					if (dimension != server->second.end())
					{
						for (auto &x : dimension->second)
						{
							for (auto &y : x.second)
							{
								for (auto &z : y.second)
								{
									new_block_positions.emplace_back(
									    x.first,
									    y.first,
									    z.first,
									    0);
									new_block_colors.emplace_back(
									    z.second.color,
									    1);
								}
							}
						}
					}
				}
			}

			new_turtle_positions.reserve(world.m_turtles.size());
			new_turtle_colors.reserve(world.m_turtles.size());
			for (auto &turtle : world.m_turtles)
			{
				if (m_selected_server
				    && *m_selected_server == turtle.position.server
				    && m_selected_dimension
				    && *m_selected_dimension == turtle.position.dimension)
				{
					new_turtle_positions.emplace_back(
					    turtle.position.position,
					    turtle.position.direction);
					new_turtle_colors.emplace_back(1, 1, 1, 1);
				}
			}
			m_block_positions.LoadData(new_block_positions, GL_STREAM_DRAW);
			m_block_colors.LoadData(new_block_colors, GL_STREAM_DRAW);
			m_turtle_positions.LoadData(new_turtle_positions, GL_STREAM_DRAW);
			m_turtle_colors.LoadData(new_turtle_colors, GL_STREAM_DRAW);
			m_is_data_dirty = false;
		}
		if (m_are_pathes_dirty)
		{
			m_paths.clear();
			std::scoped_lock<std::mutex> a{world.render_mutex};
			for (auto &turtle : world.m_turtles)
			{
				if (turtle.current_pathing
				    && !(turtle.current_pathing->latest_results.size() >= 2))
				{
					m_paths.push_back(MeshLine{});
					m_paths.back().LoadMesh(
					    turtle.current_pathing->latest_results);
				}
			}
			m_are_pathes_dirty = true;
		}
	}

	void render()
	{
		m_shader.Bind();
		m_shader.SetUniform("u_view_proj", camera.GetMVP());
		m_shader.SetUniform("u_model", glm::dmat4{1});

		m_block_positions.Bind(0);
		m_block_colors.Bind(1);

		m_block_mesh.Bind(m_shader);
		glDrawElementsInstanced(
		    GL_TRIANGLES,
		    m_block_mesh.GetIndexCount(0),
		    GL_UNSIGNED_INT,
		    0,
		    m_block_positions.size());
		if (selected_location)
		{
			m_basic_shader.Bind();
			m_basic_shader.SetUniform("u_view_proj", camera.GetMVP());
			m_basic_shader.SetUniform(
			    "u_model",
			    glm::translate(glm::mat4{1}, *selected_location));

			m_selected_mesh.Bind(m_basic_shader);
			glDrawElements(
			    GL_TRIANGLES,
			    m_selected_mesh.GetIndexCount(0),
			    GL_UNSIGNED_INT,
			    nullptr);
		}

		m_shader.SetUniform(
		    "u_view_proj",
		    camera.GetMVP()
		        * glm::translate(glm::dmat4{1}, glm::dvec3{0.5, -0.5, 0.5}));
		m_shader.SetUniform(
		    "u_model",
		    glm::rotate(
		        glm::translate(glm::dmat4{1}, glm::dvec3{0.5, 0.5, 0.5}),
		        glm::radians(180.0),
		        glm::dvec3{0, 1, 0}));
		m_turtle_positions.Bind(0);
		m_turtle_colors.Bind(1);

		m_turtle_mesh.Bind(m_shader);
		glDrawElementsInstanced(
		    GL_TRIANGLES,
		    m_turtle_mesh.GetIndexCount(0),
		    GL_UNSIGNED_INT,
		    0,
		    m_turtle_positions.size());

		m_line_shader.Bind();
		m_line_shader.SetUniform(
		    "u_view_proj",
		    camera.GetMVP()
		        * glm::translate(glm::dmat4{1}, glm::dvec3{0.5, 0.5, 0.5}));
		m_line_shader.SetUniform("u_model", glm::dmat4{1});

		for (size_t i = 0; i < m_paths.size(); i++)
		{
			m_paths[i].Bind();
			glDrawArrays(GL_LINE_STRIP, 0, m_paths[i].GetIndexCount());
		}
	}
	void dirty() { m_is_data_dirty = true; }
	void select_server(std::optional<std::string> i = {})
	{
		dirty();
		m_selected_server = i;
	}
	void select_dimension(std::optional<std::string> i = {})
	{
		dirty();
		m_selected_dimension = i;
	}
	void select_turtle(std::optional<size_t> i = {})
	{
		dirty();
		m_selected_turtle = i;
	}
	void dirty_paths() { m_are_pathes_dirty = true; }
	const auto &selected_server() const { return m_selected_server; }
	const auto &selected_dimension() const { return m_selected_dimension; }
	const auto &selected_turtle() const { return m_selected_turtle; }
	const char *selected_turtle_name(const World &world) const
	{
		if (m_selected_turtle)
		{
			return world.m_turtles[*m_selected_turtle].name.c_str();
		}
		else
		{
			return "none";
		}
	}
	void select_location(std::optional<glm::vec3> location)
	{
		selected_location = location;
	}

	Camera camera;

	private:
	std::optional<std::string> m_selected_server;
	std::optional<std::string> m_selected_dimension;
	std::optional<size_t> m_selected_turtle;
	std::optional<glm::vec3> selected_location;
	ssbo<glm::ivec4> m_block_positions;
	ssbo<glm::vec4> m_block_colors;
	ssbo<glm::ivec4> m_turtle_positions;
	ssbo<glm::vec4> m_turtle_colors;

	std::vector<MeshLine> m_paths;

	TexturedMesh m_block_mesh;
	TexturedMesh m_turtle_mesh;
	TexturedMesh m_selected_mesh;

	Shader m_shader;
	Shader m_line_shader;
	Shader m_basic_shader;

	bool m_is_data_dirty = false;
	bool m_are_pathes_dirty = false;
};
