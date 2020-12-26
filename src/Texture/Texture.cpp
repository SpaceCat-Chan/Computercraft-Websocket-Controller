#define STB_IMAGE_IMPLEMENTATION
#include "Texture.hpp"


Texture::Texture(std::string Filename, bool Flip/*=true*/) {
	Load(Filename, Flip);
}

Texture::Texture(Texture&& Move) {
	m_TextureID = Move.m_TextureID;
	Move.m_TextureID = 0;
	m_W = Move.m_W;
	m_H = Move.m_H;
}

const Texture& Texture::operator=(Texture &&Move) {
	m_TextureID = Move.m_TextureID;
	Move.m_TextureID = 0;
	m_W = Move.m_W;
	m_H = Move.m_H;

	return *this;
}

bool Texture::Load(const std::string& Filename, bool Flip/*=true*/) {
	stbi_set_flip_vertically_on_load(Flip);
	unsigned char *Image = stbi_load(Filename.c_str(), &m_W, &m_H, nullptr, STBI_rgb_alpha);
	if(Image) {
		Destroy();
		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_W, m_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, Image);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		stbi_image_free(Image);
		return true;
	}
	else {
		stbi_image_free(Image);
		return false;
	}
}

void Texture::SetWrapMode(GLint WrappingMode, WrapDimensions WrapDimensions)
{
	if(WrapDimensions != WrapDimensions::None && m_TextureID)
	{
		Bind();
		if((WrapDimensions & WrapDimensions::X) != WrapDimensions::None)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WrappingMode);
		}
		if((WrapDimensions & WrapDimensions::Y) != WrapDimensions::None)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WrappingMode);
		}
	}
}

void Texture::SetWrapColor(glm::vec<4, GLfloat> Color)
{
	if(m_TextureID)
	{
		Bind();
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(Color));
	}
}

void Texture::SetTextureFiltering(GLint MinOrMag, GLint FilterMode)
{
	if(MinOrMag != GL_TEXTURE_MIN_FILTER && MinOrMag != GL_TEXTURE_MAG_FILTER)
	{
		std::cout << "Texture::SetTextureFiltering Warning: invalid MinOrMag Value: " << MinOrMag << '\n';
		return; //no bad values thank you
	}
	if(m_TextureID)
	{
		Bind();
		glTexParameteri(GL_TEXTURE_2D, MinOrMag, FilterMode);
	}
}

void Texture::Bind(size_t Position/*=0*/) {
	glActiveTexture(GL_TEXTURE0 + Position);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
}

void Texture::Destroy() {
	if (m_TextureID == 0) return;
	glDeleteTextures(1, &m_TextureID);
}

Texture::WrapDimensions operator&(const Texture::WrapDimensions lhs, const Texture::WrapDimensions rhs)
{
	return Texture::WrapDimensions(static_cast<int>(lhs) & static_cast<int>(rhs));
}