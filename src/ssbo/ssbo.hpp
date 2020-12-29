#pragma once

#include <stdexcept>

#include <GL/glew.h>

#include "TextureArray/Detail.hpp"

template <typename T>
class ssbo
{
	GLuint m_id=0;
	uint64_t m_view_id=0;
	size_t Size=0;

	public:
	ssbo() noexcept { glCreateBuffers(1, &m_id); }
	~ssbo() noexcept { glDeleteBuffers(1, &m_id); }
	void Bind() noexcept { glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id); }
	void Bind(GLuint Target) noexcept
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, Target, m_id);
	}
	void BindPartial(GLuint Target, GLuint Offset, size_t Amount)
	{
		if (Amount + Offset > Size)
		{
			throw std::out_of_range{
			    "Offset or Amount is bigger than ssbo storage"};
		}
		glBindBufferRange(
		    GL_SHADER_STORAGE_BUFFER,
		    Target,
		    m_id,
		    Offset * sizeof(T),
		    Amount * sizeof(T));
	}
	void Unbind() noexcept { glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); }
	size_t size() const noexcept { return Size; }
	void LoadData(std::vector<T> a, GLenum usage) noexcept
	{
		glNamedBufferData(m_id, a.size() * sizeof(T), a.data(), usage);
		Size = a.size();
	}
	void ReplacePart(std::vector<T> a, size_t Offset)
	{
		if (a.size() + Offset > Size)
		{
			throw std::out_of_range{
			    "Offset or Array is bigger than ssbo storage"};
		}
		glNamedBufferSubData(
		    m_id,
		    Offset * sizeof(T),
		    a.size() * sizeof(T),
		    a.data());
	}

	void realloc(size_t size, GLenum usage)
	{
		glNamedBufferData(m_id, size * sizeof(T), nullptr, usage);
		Size = size;
	}

	void clear(T DefaultValue)
	{
		glClearNamedBufferData(
		    m_id,
		    TextureArray_Detail::GLInternalStorage_v<T>,
		    TextureArray_Detail::GLStorage_v<T>,
		    TextureArray_Detail::GLType_v<T>,
		    &DefaultValue);
	}

	class BufferView
	{
		uint64_t m_view_id;
		T *m_data;
		ssbo<T> &m_buffer;
		BufferView(uint32_t view_id, T *data, ssbo<T> &buffer)
		    : m_view_id(view_id), m_data(data), m_buffer(buffer)
		{
		}

		friend class ssbo;


		public:
		~BufferView() { m_buffer.Unmap(); }

		T &operator[](uint32_t index)
		{
			if (m_buffer.m_view_id > m_view_id)
			{
				std::cout << "BufferView Invalid\n";
				static T a;
				return a;
			}
			return m_data[index];
		}
	};

	BufferView Map(size_t length, size_t offset, GLenum access)
	{
		void *Mapped = glMapNamedBufferRange(m_id, offset, length, access);
		return BufferView(m_view_id, static_cast<T *>(Mapped), *this);
	}
	void Unmap()
	{
		m_view_id++;
		glUnmapNamedBuffer(m_id);
	}
};