#pragma once

#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <glm/ext.hpp>

#include "Detail.hpp"
namespace TextureArray_Detail
{
template <typename T>
struct ConvertTo
{
	using type = T;
};

template <>
struct ConvertTo<double>
{
	using type = GLfloat;
};

template <>
struct ConvertTo<glm::dvec1>
{
	using type = glm::vec1;
};

template <>
struct ConvertTo<glm::dvec2>
{
	using type = glm::vec2;
};

template <>
struct ConvertTo<glm::dvec3>
{
	using type = glm::vec3;
};

template <>
struct ConvertTo<glm::dvec4>
{
	using type = glm::vec4;
};

template <typename T>
using ConvertTo_t = typename ConvertTo<T>::type;
}; // namespace TextureArray_Detail

class TextureArray
{
	GLuint m_TextureID = 0;
	int m_W = 0;

	template <typename T, typename A, typename B>
	using or_other = std::conditional_t<std::is_same_v<T, A>, B, A>;

	public:
	TextureArray() = default;
	~TextureArray() { Destroy(); }
	TextureArray(const std::vector<GLfloat> &);
	TextureArray(const std::vector<or_other<GLfloat, float, double>> &);
	TextureArray(const std::vector<GLint> &);
	TextureArray(const std::vector<GLuint> &);
	TextureArray(const std::vector<glm::vec1> &);
	TextureArray(const std::vector<glm::ivec1> &);
	TextureArray(const std::vector<glm::uvec1> &);
	TextureArray(const std::vector<glm::dvec1> &);
	TextureArray(const std::vector<glm::vec2> &);
	TextureArray(const std::vector<glm::ivec2> &);
	TextureArray(const std::vector<glm::uvec2> &);
	TextureArray(const std::vector<glm::dvec2> &);
	TextureArray(const std::vector<glm::vec3> &);
	TextureArray(const std::vector<glm::ivec3> &);
	TextureArray(const std::vector<glm::uvec3> &);
	TextureArray(const std::vector<glm::dvec3> &);
	TextureArray(const std::vector<glm::vec4> &);
	TextureArray(const std::vector<glm::ivec4> &);
	TextureArray(const std::vector<glm::uvec4> &);
	TextureArray(const std::vector<glm::dvec4> &);
	TextureArray(const TextureArray &) = delete;
	TextureArray(TextureArray &&);

	const TextureArray &operator=(const TextureArray &) = delete;
	const TextureArray &operator=(TextureArray &&);

	template <typename T>
	bool Load(const std::vector<T> &a)
	{
		using Converted = TextureArray_Detail::ConvertTo_t<T>;
		std::vector<Converted> b;
		b.reserve(a.size());
		for (const T &t : a)
		{
			b.emplace_back(t);
		}

		Destroy();
		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_1D, m_TextureID);
		glTexImage1D(
		    GL_TEXTURE_1D,
		    0,
		    TextureArray_Detail::GLInternalStorage_v<Converted>,
		    b.size() * sizeof(TextureArray_Detail::ConvertTo_t<T>),
		    0,
		    TextureArray_Detail::GLStorage_v<Converted>,
		    TextureArray_Detail::GLType_v<Converted>,
		    b.data());
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		return true;
	}

	private:
	public:
	/**
	 * \brief binds the texture
	 *
	 * \param Position the position to load the texture into, defaults to 0
	 */
	void Bind(size_t Position = 0);

	/**
	 * \brief destroys the contained texture
	 */
	void Destroy();
};
