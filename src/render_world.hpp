#pragma once

#include <GL/glew.h>
#include <glm/ext.hpp>

#include "Camera/Camera.hpp"
#include "Shader/Shader.hpp"
#include "TexturedMesh/TexturedMesh.hpp"
#include "ssbo/ssbo.hpp"

#include "world.hpp"

class RenderWorld
{
	public:
	RenderWorld()
	{
		shader.AddShaderFile("res/shader.vert", GL_VERTEX_SHADER);
		shader.AddShaderFile("res/shader.frag", GL_FRAGMENT_SHADER);
		shader.Link();

		block_mesh.Load("res/block.obj");
		turtle_mesh.Load("res/turtle.obj");

		camera.MoveTo({2, 2, 2});
		camera.LookAt({0, 0, 0});
		camera.CreateProjectionX(45.0, 16.0 / 9.0, 0.1, 100.0);
	}

	void copy_into_buffers(World &world)
	{
		if (is_data_dirty)
		{
			std::scoped_lock<std::mutex> a{world.render_mutex};
			std::vector<glm::ivec4> new_block_positions;
			std::vector<glm::vec4> new_block_colors;
			std::vector<glm::ivec4> new_turtle_positions;
			std::vector<glm::vec4> new_turtle_colors;
			for (auto &x : world.m_blocks)
			{
				for (auto &y : x.second)
				{
					for (auto &z : y.second)
					{
						new_block_positions
						    .emplace_back(x.first, y.first, z.first, 0);
						new_block_colors.emplace_back(z.second.color, 1);
					}
				}
			}

			new_turtle_positions.reserve(world.m_turtles.size());
			new_turtle_colors.reserve(world.m_turtles.size());
			for (auto &turtle : world.m_turtles)
			{
				new_turtle_positions.emplace_back(
				    turtle.position,
				    turtle.direction);
				new_turtle_colors.emplace_back(1, 1, 1, 1);
			}
			block_positions.LoadData(new_block_positions, GL_STREAM_DRAW);
			block_colors.LoadData(new_block_colors, GL_STREAM_DRAW);
			turtle_positions.LoadData(new_turtle_positions, GL_STREAM_DRAW);
			turtle_colors.LoadData(new_turtle_colors, GL_STREAM_DRAW);
			is_data_dirty = false;
		}
	}

	void render()
	{
		shader.Bind();
		shader.SetUniform("u_view_proj", camera.GetMVP());
		shader.SetUniform("u_model", glm::dmat4{1});

		block_positions.Bind(0);
		block_colors.Bind(1);

		block_mesh.Bind(shader);
		glDrawElementsInstanced(
		    GL_TRIANGLES,
		    block_mesh.GetIndexCount(0),
		    GL_UNSIGNED_INT,
		    0,
		    block_positions.size());

		shader.SetUniform(
		    "u_view_proj",
		    camera.GetMVP()
		        * glm::translate(glm::dmat4{1}, glm::dvec3{1.5, 0.5, -0.5}));
		shader.SetUniform(
		    "u_model",
		    glm::rotate(
		        glm::translate(glm::dmat4{1}, glm::dvec3{-0.5, -0.5, -0.5}),
		        glm::radians(180.0),
		        glm::dvec3{0, 1, 0}));
		turtle_positions.Bind(0);
		turtle_colors.Bind(1);

		turtle_mesh.Bind(shader);
		glDrawElementsInstanced(
		    GL_TRIANGLES,
		    turtle_mesh.GetIndexCount(0),
		    GL_UNSIGNED_INT,
		    0,
		    turtle_positions.size());
	}
	void dirty() { is_data_dirty = true; }

	Camera camera;

	private:
	ssbo<glm::ivec4> block_positions;
	ssbo<glm::vec4> block_colors;
	ssbo<glm::ivec4> turtle_positions;
	ssbo<glm::vec4> turtle_colors;

	TexturedMesh block_mesh;
	TexturedMesh turtle_mesh;

	Shader shader;

	bool is_data_dirty = false;
};
