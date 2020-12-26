#define TINYOBJLOADER_IMPLEMENTATION
#include "Mesh.hpp"

class MTLLoader : public tinyobj::MaterialReader
{
	public:
	MTLLoader() = default;
	~MTLLoader() = default;
	bool operator()(
	    const std::string &matId,
	    std::vector<tinyobj::material_t> *materials,
	    std::map<std::string, int> *matMap,
	    std::string *warn,
	    std::string *err) override;
};

struct Index : public tinyobj::index_t
{
	int Material;
};

bool operator==(const Index &lhs, const Index &rhs)
{
	return lhs.normal_index == rhs.normal_index &&
	       lhs.texcoord_index == rhs.texcoord_index &&
	       lhs.vertex_index == rhs.vertex_index && lhs.Material == rhs.Material;
}

void Mesh::LoadMesh(
    std::string Filename,
    std::vector<std::string> &DiffuseFiles,
    std::vector<std::string> &SpecularFiles,
    std::vector<std::string> &BumpFiles,
    std::vector<std::string> &DispFiles,
    bool SaveToCPU)
{
	DiffuseFiles.clear();
	SpecularFiles.clear();
	BumpFiles.clear();
	DispFiles.clear();

	m_Triangles.clear();

	tinyobj::attrib_t MeshAttributes;
	std::vector<tinyobj::shape_t> Shapes;
	std::vector<tinyobj::material_t> MeshMaterials;

	std::string Warn, Error;

	isfstream File(Filename, "r+");

	MTLLoader MTL;
	bool Result = tinyobj::LoadObj(
	    &MeshAttributes,
	    &Shapes,
	    &MeshMaterials,
	    &Warn,
	    &Error,
	    &File,
	    &MTL);

	assert(Shapes.size() == 1);

	if (!Warn.empty())
	{
		std::cout << "Warning: " << Warn << '\n';
	}
	if (!Error.empty())
	{
		std::cerr << "Error: " << Error << '\n';
	}

	if (Result == false)
	{
		std::cout << "Failed to load meshfile: " << Filename << '\n';
		return;
	}

	std::vector<GLfloat> Positions;
	std::vector<GLfloat> UVCoords;
	std::vector<GLfloat> Normals;
	std::vector<GLfloat> AmbientColor;
	std::vector<GLfloat> DiffuseColor;
	std::vector<GLfloat> SpecularColor;
	std::vector<GLfloat> Shininess;
	std::vector<std::vector<glm::vec3>> Tangents;
	std::vector<GLfloat> TangentsAvaregedAndSplit;
	std::vector<std::vector<glm::vec3>> BiTangents;
	std::vector<GLfloat> BiTangentsAvaregedAndSplit;

	glBindVertexArray(m_VertexArray);

	m_IndexBuffer.clear();
	m_IndexAmount.size();
	std::vector<std::vector<GLuint>> ExpandedIndexes;
	std::vector<std::vector<Index>> StoredIndexes;

	// this is needed for forcing materials to be per vertex, and tangents
	for (size_t i = 0; i < Shapes.size(); i++)
	{
		StoredIndexes.emplace_back();
		Tangents.emplace_back();
		BiTangents.emplace_back();
		for (size_t j = 0; j < Shapes[i].mesh.num_face_vertices.size(); j++)
		{
			StoredIndexes[i].emplace_back();
			StoredIndexes[i].emplace_back();
			StoredIndexes[i].emplace_back();
			Tangents[i].emplace_back();
			Tangents[i].emplace_back();
			Tangents[i].emplace_back();
			BiTangents[i].emplace_back();
			BiTangents[i].emplace_back();
			BiTangents[i].emplace_back();
			StoredIndexes[i][j * 3].normal_index =
			    Shapes[i].mesh.indices[j * 3].normal_index;
			StoredIndexes[i][j * 3 + 1].normal_index =
			    Shapes[i].mesh.indices[j * 3 + 1].normal_index;
			StoredIndexes[i][j * 3 + 2].normal_index =
			    Shapes[i].mesh.indices[j * 3 + 2].normal_index;
			StoredIndexes[i][j * 3].texcoord_index =
			    Shapes[i].mesh.indices[j * 3].texcoord_index;
			StoredIndexes[i][j * 3 + 1].texcoord_index =
			    Shapes[i].mesh.indices[j * 3 + 1].texcoord_index;
			StoredIndexes[i][j * 3 + 2].texcoord_index =
			    Shapes[i].mesh.indices[j * 3 + 2].texcoord_index;
			StoredIndexes[i][j * 3].vertex_index =
			    Shapes[i].mesh.indices[j * 3].vertex_index;
			StoredIndexes[i][j * 3 + 1].vertex_index =
			    Shapes[i].mesh.indices[j * 3 + 1].vertex_index;
			StoredIndexes[i][j * 3 + 2].vertex_index =
			    Shapes[i].mesh.indices[j * 3 + 2].vertex_index;

			if (SaveToCPU && Shapes[i].mesh.indices[j * 3].normal_index &&
			    Shapes[i].mesh.indices[j * 3 + 2].normal_index &&
			    Shapes[i].mesh.indices[j * 3 + 2].normal_index)
			{
				m_Triangles.push_back(
				    {MeshAttributes.vertices
				         [Shapes[i].mesh.indices[j * 3].vertex_index * 3],
				     MeshAttributes.vertices
				         [Shapes[i].mesh.indices[j * 3].vertex_index * 3 + 1],
				     MeshAttributes.vertices
				         [Shapes[i].mesh.indices[j * 3].vertex_index * 3 + 2]});
				m_Triangles.push_back(
				    {MeshAttributes.vertices
				         [Shapes[i].mesh.indices[j * 3 + 1].vertex_index * 3],
				     MeshAttributes.vertices
				         [Shapes[i].mesh.indices[j * 3 + 1].vertex_index * 3 +
				          1],
				     MeshAttributes.vertices
				         [Shapes[i].mesh.indices[j * 3 + 1].vertex_index * 3 +
				          2]});
				m_Triangles.push_back(
				    {MeshAttributes.vertices
				         [Shapes[i].mesh.indices[j * 3 + 2].vertex_index * 3],
				     MeshAttributes.vertices
				         [Shapes[i].mesh.indices[j * 3 + 2].vertex_index * 3 +
				          1],
				     MeshAttributes.vertices
				         [Shapes[i].mesh.indices[j * 3 + 2].vertex_index * 3 +
				          2]});
			}

			StoredIndexes[i][j * 3].Material = Shapes[i].mesh.material_ids[j];
			StoredIndexes[i][j * 3 + 1].Material =
			    Shapes[i].mesh.material_ids[j];
			StoredIndexes[i][j * 3 + 2].Material =
			    Shapes[i].mesh.material_ids[j];

			if (StoredIndexes[i][j * 3].vertex_index != -1 &&
			    StoredIndexes[i][j * 3 + 1].vertex_index != -1 &&
			    StoredIndexes[i][j * 3 + 2].vertex_index != -1)
			{
				Index &Index0 = StoredIndexes[i][j * 3];
				Index &Index1 = StoredIndexes[i][j * 3 + 1];
				Index &Index2 = StoredIndexes[i][j * 3 + 2];

				glm::vec3 pos0, pos1, pos2;
				pos0.x = MeshAttributes.vertices[Index0.vertex_index * 3];
				pos0.y = MeshAttributes.vertices[Index0.vertex_index * 3 + 1];
				pos0.z = MeshAttributes.vertices[Index0.vertex_index * 3 + 2];

				pos1.x = MeshAttributes.vertices[Index1.vertex_index * 3];
				pos1.y = MeshAttributes.vertices[Index1.vertex_index * 3 + 1];
				pos1.z = MeshAttributes.vertices[Index1.vertex_index * 3 + 2];

				pos2.x = MeshAttributes.vertices[Index2.vertex_index * 3];
				pos2.y = MeshAttributes.vertices[Index2.vertex_index * 3 + 1];
				pos2.z = MeshAttributes.vertices[Index2.vertex_index * 3 + 2];

				if (StoredIndexes[i][j * 3].normal_index == -1 ||
				    StoredIndexes[i][j * 3 + 1].normal_index == -1 ||
				    StoredIndexes[i][j * 3 + 2].normal_index == -1)
				{
					glm::dvec3 Normal = glm::cross(pos1 - pos0, pos2 - pos0);
					MeshAttributes.normals.push_back(Normal.x);
					MeshAttributes.normals.push_back(Normal.y);
					MeshAttributes.normals.push_back(Normal.z);
					size_t NewIndex = (MeshAttributes.normals.size() - 2) / 3;
					Index0.normal_index = NewIndex;
					Index1.normal_index = NewIndex;
					Index2.normal_index = NewIndex;
				}

				glm::vec2 uv0 = {0, 0}, uv1 = {1, 0}, uv2 = {0, 1};
				if (Index0.texcoord_index != -1 &&
				    Index1.texcoord_index != -1 && Index2.texcoord_index != -1)
				{
					uv0.x = MeshAttributes.texcoords[Index0.texcoord_index * 2];
					uv0.y =
					    MeshAttributes.texcoords[Index0.texcoord_index * 2 + 1];

					uv1.x = MeshAttributes.texcoords[Index1.texcoord_index * 2];
					uv1.y =
					    MeshAttributes.texcoords[Index1.texcoord_index * 2 + 1];

					uv2.x = MeshAttributes.texcoords[Index2.texcoord_index * 2];
					uv2.y =
					    MeshAttributes.texcoords[Index2.texcoord_index * 2 + 1];
				}

				glm::vec3 edge1 = pos1 - pos0;
				glm::vec3 edge2 = pos2 - pos0;
				glm::vec2 dUV1 = uv1 - uv0;
				glm::vec2 dUV2 = uv2 - uv0;

				glm::vec3 Tangent, BiTangent;
				float f = 1.0f / (dUV1.x * dUV2.y - dUV2.x * dUV1.y);
				Tangent.x = f * (edge1.x * dUV2.y - edge2.x * dUV1.y);
				Tangent.y = f * (edge1.y * dUV2.y - edge2.y * dUV1.y);
				Tangent.z = f * (edge1.z * dUV2.y - edge2.z * dUV1.y);
				Tangent = glm::normalize(Tangent);

				BiTangent.x = f * (-dUV2.x * edge1.x + dUV1.x * edge2.x);
				BiTangent.y = f * (-dUV2.x * edge1.y + dUV1.x * edge2.y);
				BiTangent.z = f * (-dUV2.x * edge1.z + dUV1.x * edge2.z);
				BiTangent = glm::normalize(BiTangent);

				Tangents[i][j * 3] = Tangent;
				Tangents[i][j * 3 + 1] = Tangent;
				Tangents[i][j * 3 + 2] = Tangent;

				BiTangents[i][j * 3] = BiTangent;
				BiTangents[i][j * 3 + 1] = BiTangent;
				BiTangents[i][j * 3 + 2] = BiTangent;
			}
		}
	}

	for (size_t i = 0; i < Shapes.size(); i++)
	{

		m_IndexBuffer.push_back(0);
		ExpandedIndexes.emplace_back();

		for (size_t j = 0; j < Shapes[i].mesh.indices.size(); j++)
		{
			Sint64 Found = -1;
			for (size_t k = 0; k < ExpandedIndexes[i].size(); k++)
			{
				if (StoredIndexes[i][j] == StoredIndexes[i][k])
				{
					Found = k;
					break;
				}
			}
			if (Found != -1)
			{
				ExpandedIndexes[i].push_back(ExpandedIndexes[i][Found]);
			}
			else
			{
				if (StoredIndexes[i][j].vertex_index != -1)
				{
					Positions.push_back(
					    MeshAttributes
					        .vertices[StoredIndexes[i][j].vertex_index * 3]);
					Positions.push_back(
					    MeshAttributes.vertices
					        [StoredIndexes[i][j].vertex_index * 3 + 1]);
					Positions.push_back(
					    MeshAttributes.vertices
					        [StoredIndexes[i][j].vertex_index * 3 + 2]);
				}
				else
				{
					Positions.push_back(0);
					Positions.push_back(0);
					Positions.push_back(0);
				}

				if (StoredIndexes[i][j].texcoord_index != -1)
				{
					UVCoords.push_back(
					    MeshAttributes
					        .texcoords[StoredIndexes[i][j].texcoord_index * 2]);
					UVCoords.push_back(
					    MeshAttributes.texcoords
					        [StoredIndexes[i][j].texcoord_index * 2 + 1]);
				}
				else
				{
					UVCoords.push_back(0);
					UVCoords.push_back(0);
				}

				if (StoredIndexes[i][j].normal_index != -1)
				{
					Normals.push_back(
					    MeshAttributes
					        .normals[StoredIndexes[i][j].normal_index * 3]);
					Normals.push_back(
					    MeshAttributes
					        .normals[StoredIndexes[i][j].normal_index * 3 + 1]);
					Normals.push_back(
					    MeshAttributes
					        .normals[StoredIndexes[i][j].normal_index * 3 + 2]);
				}
				else
				{
					Normals.push_back(0);
					Normals.push_back(0);
					Normals.push_back(0);
				}

				if (StoredIndexes[i][j].Material != -1)
				{
					Shininess.push_back(
					    MeshMaterials[StoredIndexes[i][j].Material].shininess);
					AmbientColor.push_back(
					    MeshMaterials[StoredIndexes[i][j].Material].ambient[0]);
					AmbientColor.push_back(
					    MeshMaterials[StoredIndexes[i][j].Material].ambient[1]);
					AmbientColor.push_back(
					    MeshMaterials[StoredIndexes[i][j].Material].ambient[2]);
					DiffuseColor.push_back(
					    MeshMaterials[StoredIndexes[i][j].Material].diffuse[0]);
					DiffuseColor.push_back(
					    MeshMaterials[StoredIndexes[i][j].Material].diffuse[1]);
					DiffuseColor.push_back(
					    MeshMaterials[StoredIndexes[i][j].Material].diffuse[2]);
					SpecularColor.push_back(
					    MeshMaterials[StoredIndexes[i][j].Material]
					        .specular[0]);
					SpecularColor.push_back(
					    MeshMaterials[StoredIndexes[i][j].Material]
					        .specular[1]);
					SpecularColor.push_back(
					    MeshMaterials[StoredIndexes[i][j].Material]
					        .specular[2]);
				}
				else
				{
					AmbientColor.push_back(0.2);
					AmbientColor.push_back(0.2);
					AmbientColor.push_back(0.2);
					DiffuseColor.push_back(0.8);
					DiffuseColor.push_back(0.8);
					DiffuseColor.push_back(0.8);
					SpecularColor.push_back(1.0);
					SpecularColor.push_back(1.0);
					SpecularColor.push_back(1.0);
					Shininess.push_back(1.0);
				}

				TangentsAvaregedAndSplit.push_back(Tangents[i][j].x);
				TangentsAvaregedAndSplit.push_back(Tangents[i][j].y);
				TangentsAvaregedAndSplit.push_back(Tangents[i][j].z);

				BiTangentsAvaregedAndSplit.push_back(BiTangents[i][j].x);
				BiTangentsAvaregedAndSplit.push_back(BiTangents[i][j].y);
				BiTangentsAvaregedAndSplit.push_back(BiTangents[i][j].z);

				ExpandedIndexes[i].push_back((Positions.size() - 1) / 3);
			}
		}

		glGenBuffers(1, &m_IndexBuffer[i]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer[i]);
		glBufferData(
		    GL_ELEMENT_ARRAY_BUFFER,
		    ExpandedIndexes[i].size() * sizeof(GLuint),
		    ExpandedIndexes[i].data(),
		    GL_STATIC_DRAW);
		m_IndexAmount.push_back(ExpandedIndexes[i].size());

		DiffuseFiles.push_back(
		    MeshMaterials[Shapes[i].mesh.material_ids[0]].diffuse_texname);
		SpecularFiles.push_back(
		    MeshMaterials[Shapes[i].mesh.material_ids[0]].specular_texname);
		BumpFiles.push_back(
		    MeshMaterials[Shapes[i].mesh.material_ids[0]].bump_texname);
		DispFiles.push_back(
		    MeshMaterials[Shapes[i].mesh.material_ids[0]].displacement_texname);
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer[0]);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    Positions.size() * sizeof(GLfloat),
	    Positions.data(),
	    GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer[1]);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    UVCoords.size() * sizeof(GLfloat),
	    UVCoords.data(),
	    GL_STATIC_DRAW);

	glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer[2]);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    Normals.size() * sizeof(GLfloat),
	    Normals.data(),
	    GL_STATIC_DRAW);

	glVertexAttribPointer(2, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer[3]);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    Shininess.size() * sizeof(GLfloat),
	    Shininess.data(),
	    GL_STATIC_DRAW);
	glVertexAttribPointer(3, 1, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer[4]);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    TangentsAvaregedAndSplit.size() * sizeof(GLfloat),
	    TangentsAvaregedAndSplit.data(),
	    GL_STATIC_DRAW);
	glVertexAttribPointer(4, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(4);

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer[5]);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    BiTangentsAvaregedAndSplit.size() * sizeof(GLfloat),
	    BiTangentsAvaregedAndSplit.data(),
	    GL_STATIC_DRAW);
	glVertexAttribPointer(5, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(5);

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer[6]);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    AmbientColor.size() * sizeof(GLfloat),
	    AmbientColor.data(),
	    GL_STATIC_DRAW);
	glVertexAttribPointer(6, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(6);

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer[7]);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    DiffuseColor.size() * sizeof(GLfloat),
	    DiffuseColor.data(),
	    GL_STATIC_DRAW);
	glVertexAttribPointer(7, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(7);

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer[8]);
	glBufferData(
	    GL_ARRAY_BUFFER,
	    SpecularColor.size() * sizeof(GLfloat),
	    SpecularColor.data(),
	    GL_STATIC_DRAW);
	glVertexAttribPointer(8, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(8);

	MostExtremeVertexes.clear();
	for (size_t i = 0; i < MeshAttributes.vertices.size() / 3; i += 3)
	{
		glm::vec3 Vertex = {
		    MeshAttributes.vertices[i * 3],
		    MeshAttributes.vertices[i * 3 + 1],
		    MeshAttributes.vertices[i * 3 + 2]};
		if (i == 0)
		{
			MostExtremeVertexes[Side::PosY] = Vertex;
			MostExtremeVertexes[Side::NegY] = Vertex;
			MostExtremeVertexes[Side::PosZ] = Vertex;
			MostExtremeVertexes[Side::NegZ] = Vertex;
			MostExtremeVertexes[Side::PosX] = Vertex;
			MostExtremeVertexes[Side::NegX] = Vertex;
			continue;
		}

		if (MostExtremeVertexes[Side::PosY].y < Vertex.y)
		{
			MostExtremeVertexes[Side::PosY] = Vertex;
		}

		if (MostExtremeVertexes[Side::NegY].y > Vertex.y)
		{
			MostExtremeVertexes[Side::NegY] = Vertex;
		}

		if (MostExtremeVertexes[Side::PosZ].z < Vertex.z)
		{
			MostExtremeVertexes[Side::PosZ] = Vertex;
		}

		if (MostExtremeVertexes[Side::NegZ].z > Vertex.z)
		{
			MostExtremeVertexes[Side::NegZ] = Vertex;
		}

		if (MostExtremeVertexes[Side::PosX].x < Vertex.x)
		{
			MostExtremeVertexes[Side::PosX] = Vertex;
		}

		if (MostExtremeVertexes[Side::NegX].x > Vertex.x)
		{
			MostExtremeVertexes[Side::NegX] = Vertex;
		}
	}
}

glm::vec3 Mesh::GetMostExtremeVertex(Side VertexSide)
{
	return MostExtremeVertexes[VertexSide];
}

bool MTLLoader::operator()(
    const std::string &matId,
    std::vector<tinyobj::material_t> *materials,
    std::map<std::string, int> *matMap,
    std::string *warn,
    std::string *err)
{
	(void)err;
	(void)matId;

	std::string Filename = matId;
	if (Filename[0] != '/' && Filename[0] != '\\')
	{
		Filename = "res/" + Filename;
	}

	isfstream File(Filename, "r+");

	LoadMtl(matMap, materials, &File, warn, err);

	return true;
}