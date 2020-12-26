#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <glm/ext.hpp>

#include "SDL-Helper-Libraries/sfstream/sfstream.hpp"

#define TINYOBJLOADER_USE_DOUBLE
#include "tiny_obj_loader.h"
#undef TINYOBJLOADER_USE_DOUBLE

/**
 * \brief Class for loading and handeling meshes
 *
 * meshes are loaded from obj files
 */
class Mesh
{
	static constexpr size_t BufferAmount = 9;
	GLuint m_VertexArray = 0;
	GLuint m_VertexBuffer[BufferAmount];
	std::vector<GLuint> m_IndexBuffer;
	std::vector<size_t> m_IndexAmount;

	public:
	enum class Side
	{
		NegY,
		PosY,
		NegZ,
		PosZ,
		NegX,
		PosX
	};

	private:
	std::map<Side, glm::dvec3> MostExtremeVertexes;
	std::vector<glm::dvec3> m_Triangles;

	public:
	const std::vector<glm::dvec3> &Triangles() { return m_Triangles; }

	Mesh()
	{
		glGenBuffers(BufferAmount, m_VertexBuffer);
		glGenVertexArrays(1, &m_VertexArray);
	}
	~Mesh()
	{
		glDeleteBuffers(BufferAmount, m_VertexBuffer);
		glDeleteVertexArrays(1, &m_VertexArray);

		for (size_t i = 0; i < m_IndexBuffer.size(); i++)
		{
			glDeleteBuffers(1, &m_IndexBuffer[i]);
		}
	}

	Mesh(const Mesh &) = delete;
	Mesh(Mesh &&Move)
	{
		m_VertexArray = Move.m_VertexArray;
		Move.m_VertexArray = 0;
		for (size_t i = 0; i < BufferAmount; ++i)
		{
			m_VertexBuffer[i] = Move.m_VertexBuffer[i];
			Move.m_VertexBuffer[i] = 0;
		}
		m_IndexBuffer = std::move(Move.m_IndexBuffer);
		m_IndexAmount = std::move(Move.m_IndexAmount);

		MostExtremeVertexes = std::move(Move.MostExtremeVertexes);
		m_Triangles = std::move(Move.m_Triangles);
	}
	Mesh &operator=(const Mesh &) = delete;
	Mesh &operator=(Mesh &&Move)
	{
		m_VertexArray = Move.m_VertexArray;
		Move.m_VertexArray = 0;
		for (size_t i = 0; i < BufferAmount; ++i)
		{
			m_VertexBuffer[i] = Move.m_VertexBuffer[i];
			Move.m_VertexBuffer[i] = 0;
		}
		m_IndexBuffer = std::move(Move.m_IndexBuffer);
		m_IndexAmount = std::move(Move.m_IndexAmount);

		MostExtremeVertexes = std::move(Move.MostExtremeVertexes);
		m_Triangles = std::move(Move.m_Triangles);

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
	void LoadMesh(
	    std::string Filename,
	    std::vector<std::string> &DiffuseFiles,
	    std::vector<std::string> &SpecularFiles,
	    std::vector<std::string> &BumpFiles,
	    std::vector<std::string> &DispFiles,
	    bool SaveToCPU = false);

	/**
	 * \brief binds the selected Mesh
	 *
	 * \param MeshIndex the index of the mesh to bind
	 */
	void Bind(size_t MeshIndex)
	{
		glBindVertexArray(m_VertexArray);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer[MeshIndex]);
	}

	/**
	 * \brief return the amount of vertexes that a mesh has
	 *
	 * \param MeshIndex the index of the mesh to count
	 *
	 * \return the amount of vertexes
	 */
	size_t GetIndexCount(size_t MeshIndex) { return m_IndexAmount[MeshIndex]; }

	size_t GetMeshCount() { return m_IndexBuffer.size(); }

	/**
	 * \brief gets the vertex most in the direction specified
	 *
	 * \param VertexSide the side of the vertex to check for
	 *
	 * \return the vertex that is most in the direction specified, different
	 * sides may have the same vertex
	 */
	glm::vec3 GetMostExtremeVertex(Side VertexSide);
};