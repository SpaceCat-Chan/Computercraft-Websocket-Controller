#include "Shader.hpp"

Shader::Shader()
{
	m_ShaderProgram = glCreateProgram();
}

Shader::~Shader()
{
	for (size_t i = 0; i < m_Shaders.size(); i++)
	{
		glDeleteShader(m_Shaders[i]);
	}
	glDeleteProgram(m_ShaderProgram);
}

Shader::Shader(Shader &&Move)
{
	m_Shaders = std::move(Move.m_Shaders);
	m_ShaderProgram = std::move(Move.m_ShaderProgram);
	Move.m_ShaderProgram = 0;
	m_Linked = Move.m_Linked;
}

const Shader &Shader::operator=(Shader &&Move)
{
	m_Shaders = std::move(Move.m_Shaders);
	m_ShaderProgram = std::move(Move.m_ShaderProgram);
	m_Linked = Move.m_Linked;

	return *this;
};

void Shader::Link()
{
	glLinkProgram(m_ShaderProgram);
	m_Linked = true;
}

void Shader::Bind()
{
	if (!m_Linked)
	{
		Link();
	}
	glUseProgram(m_ShaderProgram);
}

bool Shader::AddShader(std::string Source, GLenum type)
{
	GLint NewShader = glCreateShader(type);
	const GLchar *SourceC = Source.c_str();
	GLint size = Source.size();
	glShaderSource(NewShader, 1, &SourceC, &size);
	glCompileShader(NewShader);

	GLint Status;
	glGetShaderiv(NewShader, GL_COMPILE_STATUS, &Status);
	if (Status != GL_TRUE)
	{
		GLsizei LogSize;
		glGetShaderiv(NewShader, GL_INFO_LOG_LENGTH, &LogSize);

		char *Log = new char[LogSize];
		glGetShaderInfoLog(NewShader, LogSize, &LogSize, Log);

		std::cerr << Log;

		glDeleteShader(NewShader);
		return false;
	}
	glAttachShader(m_ShaderProgram, NewShader);
	m_Linked = false;
	m_Shaders.push_back(NewShader);
	return true;
}

bool Shader::AddShaderFile(std::string Name, GLenum type)
{
	isfstream File(Name, "r");

	std::string Line;
	std::stringstream Source;
	while (getline(File, Line))
	{
		Source << Line << '\n';
	}
	return AddShader(Source.str(), type);
}

void Shader::SetUniform(std::string Name, GLfloat v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform1f(Position, v0);
}

void Shader::SetUniform(std::string Name, GLfloat v0, GLfloat v1)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform2f(Position, v0, v1);
}

void Shader::SetUniform(std::string Name, GLfloat v0, GLfloat v1, GLfloat v2)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform3f(Position, v0, v1, v2);
}

void Shader::SetUniform(std::string Name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform4f(Position, v0, v1, v2, v3);
}

void Shader::SetUniform(std::string Name, GLint v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform1i(Position, v0);
}

void Shader::SetUniform(std::string Name, GLint v0, GLint v1)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform2i(Position, v0, v1);
}

void Shader::SetUniform(std::string Name, GLint v0, GLint v1, GLint v2)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform3i(Position, v0, v1, v2);
}

void Shader::SetUniform(std::string Name, GLint v0, GLint v1, GLint v2, GLint v3)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform4i(Position, v0, v1, v2, v3);
}

void Shader::SetUniform(std::string Name, GLuint v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform1ui(Position, v0);
}

void Shader::SetUniform(std::string Name, GLuint v0, GLuint v1)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform2ui(Position, v0, v1);
}

void Shader::SetUniform(std::string Name, GLuint v0, GLuint v1, GLuint v2)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform3ui(Position, v0, v1, v2);
}

void Shader::SetUniform(std::string Name, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform4ui(Position, v0, v1, v2, v3);
}

void Shader::SetUniform(std::string Name, glm::dvec1 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform1f(Position, v0.x);
}

void Shader::SetUniform(std::string Name, glm::dvec2 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform2f(Position, v0.x, v0.y);
}

void Shader::SetUniform(std::string Name, glm::dvec3 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform3f(Position, v0.x, v0.y, v0.z);
}

void Shader::SetUniform(std::string Name, glm::dvec4 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform4f(Position, v0.x, v0.y, v0.z, v0.w);
}

void Shader::SetUniform(std::string Name, glm::dmat2x2 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glm::mat2x2 Matrix = v0;
	glUniformMatrix2fv(Position, 1, GL_FALSE, glm::value_ptr(Matrix));
}

void Shader::SetUniform(std::string Name, glm::dmat3x3 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glm::mat3x3 Matrix = v0;
	glUniformMatrix3fv(Position, 1, GL_FALSE, glm::value_ptr(Matrix));
}

void Shader::SetUniform(std::string Name, glm::dmat4x4 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glm::mat4x4 Matrix = v0;
	glUniformMatrix4fv(Position, 1, GL_FALSE, glm::value_ptr(Matrix));
}

void Shader::SetUniform(std::string Name, glm::dmat2x3 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glm::mat2x3 Matrix = v0;
	glUniformMatrix2x3fv(Position, 1, GL_FALSE, glm::value_ptr(Matrix));
}

void Shader::SetUniform(std::string Name, glm::dmat3x2 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glm::mat3x2 Matrix = v0;
	glUniformMatrix3x2fv(Position, 1, GL_FALSE, glm::value_ptr(Matrix));
}

void Shader::SetUniform(std::string Name, glm::dmat2x4 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glm::mat2x4 Matrix = v0;
	glUniformMatrix2x4fv(Position, 1, GL_FALSE, glm::value_ptr(Matrix));
}

void Shader::SetUniform(std::string Name, glm::dmat4x2 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glm::mat4x2 Matrix = v0;
	glUniformMatrix4x2fv(Position, 1, GL_FALSE, glm::value_ptr(Matrix));
}

void Shader::SetUniform(std::string Name, glm::dmat3x4 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glm::mat3x4 Matrix = v0;
	glUniformMatrix3x4fv(Position, 1, GL_FALSE, glm::value_ptr(Matrix));
}

void Shader::SetUniform(std::string Name, glm::dmat4x3 v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glm::mat4x3 Matrix = v0;
	glUniformMatrix4x3fv(Position, 1, GL_FALSE, glm::value_ptr(Matrix));
}

void Shader::SetUniform(std::string Name, std::vector<GLfloat> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform1fv(Position, v0.size(), v0.data());
}

void Shader::SetUniform(std::string Name, std::vector<std::conditional_t<!is_same_v<GLfloat, float>, float, double>> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLfloat> Converted;
	Converted.reserve(v0.size() * 2);
	for (auto &v : v0)
	{
		Converted.push_back(v);
	}
	glUniform1fv(Position, v0.size(), Converted.data());
}
void Shader::SetUniform(std::string Name, std::vector<GLint> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform1iv(Position, v0.size(), v0.data());
}
void Shader::SetUniform(std::string Name, std::vector<GLuint> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	glUniform1uiv(Position, v0.size(), v0.data());
}

void Shader::SetUniform(std::string Name, std::vector<glm::vec2> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLfloat> Converted;
	Converted.reserve(v0.size() * 2);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
	}
	glUniform2fv(Position, v0.size(), Converted.data());
}
void Shader::SetUniform(std::string Name, std::vector<glm::dvec2> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLfloat> Converted;
	Converted.reserve(v0.size() * 2);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
	}
	glUniform2fv(Position, v0.size(), Converted.data());
}
void Shader::SetUniform(std::string Name, std::vector<glm::ivec2> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLint> Converted;
	Converted.reserve(v0.size() * 2);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
	}
	glUniform2iv(Position, v0.size(), Converted.data());
}
void Shader::SetUniform(std::string Name, std::vector<glm::uvec2> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLuint> Converted;
	Converted.reserve(v0.size() * 2);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
	}
	glUniform2uiv(Position, v0.size(), Converted.data());
}

void Shader::SetUniform(std::string Name, std::vector<glm::vec3> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLfloat> Converted;
	Converted.reserve(v0.size() * 3);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
		Converted.push_back(v.z);
	}
	glUniform3fv(Position, v0.size(), Converted.data());
}
void Shader::SetUniform(std::string Name, std::vector<glm::dvec3> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLfloat> Converted;
	Converted.reserve(v0.size() * 3);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
		Converted.push_back(v.z);
	}
	glUniform3fv(Position, v0.size(), Converted.data());
}
void Shader::SetUniform(std::string Name, std::vector<glm::ivec3> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLint> Converted;
	Converted.reserve(v0.size() * 3);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
		Converted.push_back(v.z);
	}
	glUniform3iv(Position, v0.size(), Converted.data());
}
void Shader::SetUniform(std::string Name, std::vector<glm::uvec3> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLuint> Converted;
	Converted.reserve(v0.size() * 3);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
		Converted.push_back(v.z);
	}
	glUniform3uiv(Position, v0.size(), Converted.data());
}

void Shader::SetUniform(std::string Name, std::vector<glm::vec4> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLfloat> Converted;
	Converted.reserve(v0.size() * 4);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
		Converted.push_back(v.z);
		Converted.push_back(v.w);
	}
	glUniform4fv(Position, v0.size(), Converted.data());
}
void Shader::SetUniform(std::string Name, std::vector<glm::dvec4> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLfloat> Converted;
	Converted.reserve(v0.size() * 4);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
		Converted.push_back(v.z);
		Converted.push_back(v.w);
	}
	glUniform4fv(Position, v0.size(), Converted.data());
}
void Shader::SetUniform(std::string Name, std::vector<glm::ivec4> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLint> Converted;
	Converted.reserve(v0.size() * 4);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
		Converted.push_back(v.z);
		Converted.push_back(v.w);
	}
	glUniform4iv(Position, v0.size(), Converted.data());
}
void Shader::SetUniform(std::string Name, std::vector<glm::uvec4> v0)
{
	Bind();
	GLint Position = glGetUniformLocation(m_ShaderProgram, Name.c_str());
	std::vector<GLuint> Converted;
	Converted.reserve(v0.size() * 4);
	for (auto &v : v0)
	{
		Converted.push_back(v.x);
		Converted.push_back(v.y);
		Converted.push_back(v.z);
		Converted.push_back(v.w);
	}
	glUniform4uiv(Position, v0.size(), Converted.data());
}