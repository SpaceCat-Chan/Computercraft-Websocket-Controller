#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <glm/ext.hpp>

/**
 * \brief Class for loading and handeling meshes
 *
 * meshes are loaded from obj files
 */
class MeshLine
{
	GLuint m_VertexArray = 0;
	GLuint m_VertexBuffer = 0;
	size_t m_vertex_count = 0;

	public:
	MeshLine()
	{
		glGenBuffers(1, &m_VertexBuffer);
		glGenVertexArrays(1, &m_VertexArray);
	}
	~MeshLine()
	{
		glDeleteBuffers(1, &m_VertexBuffer);
		glDeleteVertexArrays(1, &m_VertexArray);
	}

	MeshLine(const MeshLine &) = delete;
	MeshLine(MeshLine &&Move)
	{
		m_VertexArray = Move.m_VertexArray;
		Move.m_VertexArray = 0;
		m_VertexBuffer = Move.m_VertexBuffer;
		Move.m_VertexBuffer = 0;
	}
	MeshLine &operator=(const MeshLine &) = delete;
	MeshLine &operator=(MeshLine &&Move)
	{
		m_VertexArray = Move.m_VertexArray;
		Move.m_VertexArray = 0;
		m_VertexBuffer = Move.m_VertexBuffer;
		Move.m_VertexBuffer = 0;

		return *this;
	}

	/**
	 * \brief a function that loads a mesh
	 *
	 * \param Filename the name of the file to load from
	 * \param DiffuseFiles a referance to a vector of strings, will be filled
	 * with the filenames of all needed diffuse maps \param SpecularFiles same
	 * as DiffuseFile but for Specular map instead \param BumpFiles same thing
	 * but for normals \param DispFiles Displacement maps
	 *
	 * there is currently no way to access the stored mesh data without major
	 * slowdown or changes to this class
	 */
	void LoadMesh(std::vector<glm::ivec3> &points)
	{
		glBindVertexArray(m_VertexArray);

		glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
		glBufferData(
		    GL_ARRAY_BUFFER,
		    points.size() * sizeof(GLint),
		    points.data(),
		    GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);
		glEnableVertexAttribArray(0);
	}

	/**
	 * \brief binds the selected Mesh
	 *
	 * \param MeshIndex the index of the mesh to bind
	 */
	void Bind() { glBindVertexArray(m_VertexArray); }

	/**
	 * \brief return the amount of vertexes that a mesh has
	 *
	 * \param MeshIndex the index of the mesh to count
	 *
	 * \return the amount of vertexes
	 */
	size_t GetIndexCount() { return m_vertex_count; }
};