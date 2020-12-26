

#pragma once

#include <vector>
#include <iostream>
#include <sstream>

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

#include <glm/ext.hpp>

#include "SDL-Helper-Libraries/sfstream/sfstream.hpp"

template <typename T, typename U>
inline constexpr bool is_same_v = std::is_same<T, U>::value;

/**
 * \class Shader
 * \brief a class to handle shaders
 * 
 * it is not permitted to copy a Shader object
 * 
 * all contained Shaders will be deleted when the class is deleted
 */
class Shader
{
	std::vector<GLint> m_Shaders;
	GLint m_ShaderProgram;
	bool m_Linked = false;

public:
	Shader();
	~Shader();

	Shader(const Shader &Copy) = delete;
	Shader(Shader &&Move);

	const Shader &operator=(const Shader &Copy) = delete;
	const Shader &operator=(Shader &&Move);

	/**
	 * \brief Compiles and Adds a shader from a file
	 * 
	 * \param Name the name of the file to load the shader from
	 * \param type the type of shader to add
	 * 
	 * \return wether or not adding the shader was succesful
	 */
	bool AddShaderFile(std::string Name, GLenum type);
	/**
	 * \brief Compiles and Adds a shader from source code
	 * 
	 * \param Name the source code of the shader
	 * \param type the type of shader to add
	 * 
	 * \return wether or not adding the shader was succesful
	 */
	bool AddShader(std::string Source, GLenum type);
	/**
	 * \brief links the Shader program
	 */
	void Link();
	/**
	 * \brief binds the shader
	 * 
	 * will also link the shader if needed
	 */
	void Bind();

	GLint GetShader(size_t Index);
	
	/**
	 * \name SetUniform
	 * 
	 * \param Name The name of the uniform
	 * \param Other values to pass to the Uniform
	 */
	///@{
	void SetUniform(std::string Name, GLfloat);
	void SetUniform(std::string Name, GLfloat, GLfloat);
	void SetUniform(std::string Name, GLfloat, GLfloat, GLfloat);
	void SetUniform(std::string Name, GLfloat, GLfloat, GLfloat, GLfloat);
	void SetUniform(std::string Name, GLint);
	void SetUniform(std::string Name, GLint, GLint);
	void SetUniform(std::string Name, GLint, GLint, GLint);
	void SetUniform(std::string Name, GLint, GLint, GLint, GLint);
	void SetUniform(std::string Name, GLuint);
	void SetUniform(std::string Name, GLuint, GLuint);
	void SetUniform(std::string Name, GLuint, GLuint, GLuint);
	void SetUniform(std::string Name, GLuint, GLuint, GLuint, GLuint);
	void SetUniform(std::string Name, glm::dvec1);
	void SetUniform(std::string Name, glm::dvec2);
	void SetUniform(std::string Name, glm::dvec3);
	void SetUniform(std::string Name, glm::dvec4);
	void SetUniform(std::string Name, glm::dmat2x2);
	void SetUniform(std::string Name, glm::dmat3x2);
	void SetUniform(std::string Name, glm::dmat2x3);
	void SetUniform(std::string Name, glm::dmat4x2);
	void SetUniform(std::string Name, glm::dmat2x4);
	void SetUniform(std::string Name, glm::dmat3x3);
	void SetUniform(std::string Name, glm::dmat4x3);
	void SetUniform(std::string Name, glm::dmat3x4);
	void SetUniform(std::string Name, glm::dmat4x4);

	void SetUniform(std::string Name, std::vector<GLfloat>);
	void SetUniform(std::string Name, std::vector<std::conditional_t<!is_same_v<GLfloat, float>, float, double>>);
	void SetUniform(std::string Name, std::vector<GLint>);
	void SetUniform(std::string Name, std::vector<GLuint>);
	
	void SetUniform(std::string Name, std::vector<glm::vec2>);
	void SetUniform(std::string Name, std::vector<glm::dvec2>);
	void SetUniform(std::string Name, std::vector<glm::ivec2>);
	void SetUniform(std::string Name, std::vector<glm::uvec2>);
	
	void SetUniform(std::string Name, std::vector<glm::vec3>);
	void SetUniform(std::string Name, std::vector<glm::dvec3>);
	void SetUniform(std::string Name, std::vector<glm::ivec3>);
	void SetUniform(std::string Name, std::vector<glm::uvec3>);
	
	void SetUniform(std::string Name, std::vector<glm::vec4>);
	void SetUniform(std::string Name, std::vector<glm::dvec4>);
	void SetUniform(std::string Name, std::vector<glm::ivec4>);
	void SetUniform(std::string Name, std::vector<glm::uvec4>);
	///@}
};