#pragma once

#include <iostream>

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <glm/ext.hpp>

#include "stb_image.h"

/**
 * \brief a class that manages a texture
 * 
 * copying has been deleted
 */
class Texture {
    GLuint m_TextureID = 0;
	int m_W=0, m_H=0;

	public:

	Texture() = default;
	~Texture() {
		Destroy();
	}
	Texture(std::string Filename, bool Flip=true);
	Texture(const Texture&) = delete;
	Texture(Texture&&);

	const Texture& operator=(const Texture&) = delete;
	const Texture& operator=(Texture&&);

	/**
	 * \brief loads a Texture from a file
	 * 
	 * \param Filename the name of the file to load from
	 * \param Flip wether or not to flip the image on loads
	 * 
	 * \return weather or not the loading succeded
	 * 
	 * if loading the file failed then the Texture will be unchanged
	 */
	bool Load(const std::string& Filename, bool Flip=true);

	enum class WrapDimensions
	{
		None=0,
		X=1,
		Y=2,
		XY=X|Y
	};

	/**
	 * \brief changes the wrapping mode of the texture
	 * 
	 * \param ClampMode the Clamp mode that the texture should use
	 * \param WrapDimensions which dimensions the wrapping should apply to
	 */
	void SetWrapMode(GLint ClampMode, WrapDimensions WrapDimensions=WrapDimensions::XY);
	/**
	 * \brief sets the color that should be used for wrapping mode \ref GL_CLAMP_TO_BORDER
	 * 
	 * \param Color the color
	 */
	void SetWrapColor(glm::vec<4, GLfloat> Color);

	/**
	 * \brief sets the texture filtering mode
	 * 
	 * \param MinOrMag if this should apply to Min or Mag resizing \
	 * only GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER are allowed
	 * \param FilterMode the mode of filtering
	 */
	void SetTextureFiltering(GLint MinOrMag, GLint FilterMode);

	/**
	 * \brief binds the texture
	 * 
	 * \param Position the position to load the texture into, defaults to 0
	 */
	void Bind(size_t Position=0);

	/**
	 * \brief destroys the contained texture
	 */
	void Destroy();
};

Texture::WrapDimensions operator&(const Texture::WrapDimensions lhs, const Texture::WrapDimensions rhs);
