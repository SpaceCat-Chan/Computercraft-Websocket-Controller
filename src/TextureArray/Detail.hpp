#pragma once

#include <GL/glew.h>
#include <glm/ext.hpp>

namespace TextureArray_Detail
{
template <typename T>
struct GLType
{
};
template <>
struct GLType<GLfloat>
{
	constexpr static GLenum Value = GL_FLOAT;
};
template <>
struct GLType<GLint>
{
	constexpr static GLenum Value = GL_INT;
};
template <>
struct GLType<GLuint>
{
	constexpr static GLenum Value = GL_UNSIGNED_INT;
};
template <>
struct GLType<glm::vec1>
{
	constexpr static GLenum Value = GL_FLOAT;
};
template <>
struct GLType<glm::ivec1>
{
	constexpr static GLenum Value = GL_INT;
};
template <>
struct GLType<glm::uvec1>
{
	constexpr static GLenum Value = GL_UNSIGNED_INT;
};
template <>
struct GLType<glm::vec2>
{
	constexpr static GLenum Value = GL_FLOAT;
};
template <>
struct GLType<glm::ivec2>
{
	constexpr static GLenum Value = GL_INT;
};
template <>
struct GLType<glm::uvec2>
{
	constexpr static GLenum Value = GL_UNSIGNED_INT;
};
template <>
struct GLType<glm::vec3>
{
	constexpr static GLenum Value = GL_FLOAT;
};
template <>
struct GLType<glm::ivec3>
{
	constexpr static GLenum Value = GL_INT;
};
template <>
struct GLType<glm::uvec3>
{
	constexpr static GLenum Value = GL_UNSIGNED_INT;
};
template <>
struct GLType<glm::vec4>
{
	constexpr static GLenum Value = GL_FLOAT;
};
template <>
struct GLType<glm::ivec4>
{
	constexpr static GLenum Value = GL_INT;
};
template <>
struct GLType<glm::uvec4>
{
	constexpr static GLenum Value = GL_UNSIGNED_INT;
};

template <typename T>
struct GLStorage
{
};

template <>
struct GLStorage<GLfloat>
{
	constexpr static GLenum Value = GL_RED;
};
template <>
struct GLStorage<GLint>
{
	constexpr static GLenum Value = GL_RED;
};
template <>
struct GLStorage<GLuint>
{
	constexpr static GLenum Value = GL_RED_INTEGER;
};
template <>
struct GLStorage<glm::vec1>
{
	constexpr static GLenum Value = GL_RED;
};
template <>
struct GLStorage<glm::ivec1>
{
	constexpr static GLenum Value = GL_RED;
};
template <>
struct GLStorage<glm::uvec1>
{
	constexpr static GLenum Value = GL_RED_INTEGER;
};
template <>
struct GLStorage<glm::vec2>
{
	constexpr static GLenum Value = GL_RG;
};
template <>
struct GLStorage<glm::ivec2>
{
	constexpr static GLenum Value = GL_RG;
};
template <>
struct GLStorage<glm::uvec2>
{
	constexpr static GLenum Value = GL_RG_INTEGER;
};
template <>
struct GLStorage<glm::vec3>
{
	constexpr static GLenum Value = GL_RGB;
};
template <>
struct GLStorage<glm::ivec3>
{
	constexpr static GLenum Value = GL_RGB;
};
template <>
struct GLStorage<glm::uvec3>
{
	constexpr static GLenum Value = GL_RGB_INTEGER;
};
template <>
struct GLStorage<glm::vec4>
{
	constexpr static GLenum Value = GL_RGBA;
};
template <>
struct GLStorage<glm::ivec4>
{
	constexpr static GLenum Value = GL_RGBA;
};
template <>
struct GLStorage<glm::uvec4>
{
	constexpr static GLenum Value = GL_RGBA_INTEGER;
};

template <typename T>
struct GLInternalStorage
{
};

template <>
struct GLInternalStorage<GLfloat>
{
	constexpr static GLenum Value = GL_R32F;
};
template <>
struct GLInternalStorage<GLint>
{
	constexpr static GLenum Value = GL_R32I;
};
template <>
struct GLInternalStorage<GLuint>
{
	constexpr static GLenum Value = GL_R32UI;
};
template <>
struct GLInternalStorage<glm::vec1>
{
	constexpr static GLenum Value = GL_R32F;
};
template <>
struct GLInternalStorage<glm::ivec1>
{
	constexpr static GLenum Value = GL_R32I;
};
template <>
struct GLInternalStorage<glm::uvec1>
{
	constexpr static GLenum Value = GL_R32UI;
};
template <>
struct GLInternalStorage<glm::vec2>
{
	constexpr static GLenum Value = GL_RG32F;
};
template <>
struct GLInternalStorage<glm::ivec2>
{
	constexpr static GLenum Value = GL_RG32I;
};
template <>
struct GLInternalStorage<glm::uvec2>
{
	constexpr static GLenum Value = GL_RG32UI;
};
template <>
struct GLInternalStorage<glm::vec3>
{
	constexpr static GLenum Value = GL_RGB32F;
};
template <>
struct GLInternalStorage<glm::ivec3>
{
	constexpr static GLenum Value = GL_RGB32I;
};
template <>
struct GLInternalStorage<glm::uvec3>
{
	constexpr static GLenum Value = GL_RGB32UI;
};
template <>
struct GLInternalStorage<glm::vec4>
{
	constexpr static GLenum Value = GL_RGBA32F;
};
template <>
struct GLInternalStorage<glm::ivec4>
{
	constexpr static GLenum Value = GL_RGBA32I;
};
template <>
struct GLInternalStorage<glm::uvec4>
{
	constexpr static GLenum Value = GL_RGBA32UI;
};

template <typename T>
inline constexpr GLenum GLType_v = GLType<T>::Value;
template <typename T>
inline constexpr GLenum GLStorage_v = GLStorage<T>::Value;
template <typename T>
inline constexpr GLenum GLInternalStorage_v = GLInternalStorage<T>::Value;

} // namespace TextureArray_Detail
