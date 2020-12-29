#pragma once

#include "Mesh/Mesh.hpp"
#include "Texture/Texture.hpp"
#include "Shader/Shader.hpp"

class Camera;

/**
 * \brief a class that keeps track of the textures used by a mesh and the mesh itself
 * 
 * this class also enforces a few rules about shaders
 * specifically any shader used must have the sampler2D uniforms:
 * u_Texture - ambient and diffuse texture
 * u_Specular - Specular texture
 * u_Bump - Bump map/Normal Map
 * u_Disp - displacement Map
 * 
 * and the following bool uniforms
 * u_UseBumpMap - wether or not to use a bump map
 * u_UseDispMap - wether or not to use a displacement map
 */
class TexturedMesh {
	Mesh m_Mesh;
	std::vector<Texture> m_Diffuse;
	std::vector<Texture> m_Specular;

	std::vector<Texture> m_Bump;
	std::vector<bool> m_UsesBump;

	std::vector<Texture> m_Disp;
	std::vector<bool> m_UsesDisp;

	friend void Render(TexturedMesh &Mesh, Shader &ShaderProgram, Camera &View, bool OutsideLight, bool OutsideMesh);

	public:
	const std::vector<glm::dvec3> &Triangles() { return m_Mesh.Triangles(); }

	TexturedMesh() = default;
	/**
	 * \brief loads Mesh from file
	 */
	TexturedMesh(std::string Filename, bool SaveToCPU=false);

	TexturedMesh(const TexturedMesh &Copy) = delete;
	TexturedMesh(TexturedMesh &&Move);

	const TexturedMesh &operator=(const TexturedMesh &Copy) = delete;
	const TexturedMesh &operator=(TexturedMesh &&Move);

	void Load(std::string Filename, bool SaveToCPU=false);

	/**
	 * \brief binds the given mesh index, does not bind textures
	 * 
	 * \param Index the mesh index to bind
	 */
	void Bind(size_t Index=0);

	/**
	 * \brief binds the given mesh index and corresponding textures
	 * 
	 * \param Index the mesh index to bind
	 * \param ShaderProgram the shader to bind the textures to
	 */
	void Bind(size_t Index, Shader &ShaderProgram);
	/**
	 * \brief same as Bind(0, ShaderProgram)
	 * 
	 * \param ShaderProgram look at \ref Bind(size_t, Shader&)
	 * 
	 * \sa Bind(size_t, Shader&)
	 */
	void Bind(Shader &ShaderProgram) { Bind(0, ShaderProgram); }

	/**
	 * \brief gets the amount of vertexes at specified mesh Index
	 * 
	 * \param Index the mesh index to check
	 */
	size_t GetIndexCount(size_t Index) { return m_Mesh.GetIndexCount(Index); }

	size_t GetMeshCount() { return m_Mesh.GetMeshCount(); }

	glm::vec3 GetMostExtremeVertex(Mesh::Side VertexSide) { return m_Mesh.GetMostExtremeVertex(VertexSide); }
};