#define STB_IMAGE_IMPLEMENTATION
#include "TextureArray.hpp"

TextureArray::TextureArray(const std::vector<GLfloat> &a) { Load(a); }
TextureArray::TextureArray(
    const std::vector<TextureArray::or_other<GLfloat, float, double>> &a)
{
	Load(a);
}
TextureArray::TextureArray(const std::vector<GLint> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<GLuint> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::vec1> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::ivec1> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::uvec1> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::dvec1> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::vec2> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::ivec2> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::uvec2> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::dvec2> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::vec3> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::ivec3> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::uvec3> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::dvec3> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::vec4> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::ivec4> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::uvec4> &a) { Load(a); }
TextureArray::TextureArray(const std::vector<glm::dvec4> &a) { Load(a); }

TextureArray::TextureArray(TextureArray &&Move)
{
	Destroy();
	m_TextureID = Move.m_TextureID;
	Move.m_TextureID = 0;
	m_W = Move.m_W;
}

const TextureArray &TextureArray::operator=(TextureArray &&Move)
{
	Destroy();
	m_TextureID = Move.m_TextureID;
	Move.m_TextureID = 0;
	m_W = Move.m_W;

	return *this;
}

void TextureArray::Bind(size_t Position /*=0*/)
{
	glActiveTexture(GL_TEXTURE0 + Position);
	glBindTexture(GL_TEXTURE_1D, m_TextureID);
}

void TextureArray::Destroy()
{
	if (m_TextureID == 0)
		return;
	glDeleteTextures(1, &m_TextureID);
}
