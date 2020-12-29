#include "TexturedMesh.hpp"

TexturedMesh::TexturedMesh(std::string Filename, bool SaveToCPU)
{
	Load(Filename, SaveToCPU);
}

TexturedMesh::TexturedMesh(TexturedMesh &&Move)
{
	m_Mesh = std::move(Move.m_Mesh);

	m_Diffuse = std::move(Move.m_Diffuse);
	m_Specular = std::move(Move.m_Specular);

	m_Bump = std::move(Move.m_Bump);
	m_UsesBump = std::move(Move.m_UsesBump);

	m_Disp = std::move(Move.m_Disp);
	m_UsesDisp = std::move(Move.m_UsesDisp);
}

const TexturedMesh &TexturedMesh::operator=(TexturedMesh &&Move)
{
	m_Mesh = std::move(Move.m_Mesh);

	m_Diffuse = std::move(Move.m_Diffuse);
	m_Specular = std::move(Move.m_Specular);

	m_Bump = std::move(Move.m_Bump);
	m_UsesBump = std::move(Move.m_UsesBump);

	m_Disp = std::move(Move.m_Disp);
	m_UsesDisp = std::move(Move.m_UsesDisp);

	return *this;
}

void TexturedMesh::Load(std::string Filename, bool SaveToCPU)
{
	m_Diffuse.clear();
	m_Specular.clear();
	m_Bump.clear();
	m_UsesBump.clear();
	m_Disp.clear();
	m_UsesDisp.clear();

	std::vector<std::string> DiffuseFiles, SpecularFiles, BumpFiles, DispFiles;

	m_Mesh.LoadMesh(Filename, DiffuseFiles, SpecularFiles, BumpFiles, DispFiles, SaveToCPU);

	for (size_t i = 0; i < DiffuseFiles.size(); i++)
	{
		m_Diffuse.push_back(Texture(DiffuseFiles[i]));
		m_Specular.push_back(Texture(SpecularFiles[i]));
		m_Bump.push_back(Texture(BumpFiles[i]));
		m_UsesBump.push_back(BumpFiles[i] != "");
		m_Disp.push_back(Texture(DispFiles[i]));
		m_UsesDisp.push_back(DispFiles[i] != "");
	}
}

void TexturedMesh::Bind(size_t Index /*=0*/)
{
	m_Mesh.Bind(Index);
}

void TexturedMesh::Bind(size_t Index, Shader &ShaderProgram) {
	m_Mesh.Bind(Index);

	ShaderProgram.SetUniform("u_Texture", 0);
	ShaderProgram.SetUniform("u_Specular", 1);
	ShaderProgram.SetUniform("u_Bump", 2);
	ShaderProgram.SetUniform("u_Disp", 3);
	ShaderProgram.SetUniform("u_UseBumpMap", m_UsesBump.at(Index));
	ShaderProgram.SetUniform("u_UseDispMap", m_UsesDisp.at(Index));

	m_Diffuse.at(Index).Bind(0);
	m_Specular.at(Index).Bind(1);
	m_Bump.at(Index).Bind(2);
	m_Disp.at(Index).Bind(3);
}