#include "TinyObjLoaderParser.h"

#include "Logger.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "Application.h"
#include "DXMesh/DirectXMesh/DirectXMesh.h"
#include "MeshGenerator/MeshPayload.h"
#include "Profiler.h"
#include "tinyobjloader/tiny_obj_loader.h"

bool OTinyObjParser::ParseMesh(const wstring& Path, SMeshPayloadData& MeshData, ETextureMapType Type)
{
	PROFILE_SCOPE();
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = ""; // Path to material files

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(WStringToUTF8(Path), reader_config))
	{
		if (!reader.Error().empty())
		{
			WIN_LOG(TinyObjLoader, Error, "TinyObjReader: {}", TEXT(reader.Error()));
		}
		return false;
	}

	if (!reader.Warning().empty())
	{
		LOG(TinyObjLoader, Warning, "TinyObjReader: {}", TEXT(reader.Warning()));
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();
	MeshData.Data.reserve(shapes.size());

	std::vector<std::future<SMeshPayloadData>> futures;
	auto compute = [&](uint32_t start, uint32_t end) {
		SMeshPayloadData payload;
		// Loop over shapes
		for (size_t s = start; s < end; s++)
		{
			OGeometryGenerator::SMeshData data;
			data.Name = shapes[s].name;
			data.Vertices.reserve(shapes[s].mesh.indices.size() / 3);
			// Loop over faces(polygon)
			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				const size_t fv = shapes[s].mesh.num_face_vertices[f];

				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++)
				{
					OGeometryGenerator::SGeometryExtendedVertex vertex{};
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					tinyobj::TinyObjPoint point = { attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 0],
						                            attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 1],
						                            attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 2] };
					vertex.Position = { -point.x, point.y, point.z };
					// Check if `normal_index` is zero or positive. negative = no normal data
					if (idx.normal_index >= 0)
					{
						vertex.Normal.x = -attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0];
						vertex.Normal.y = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1];
						vertex.Normal.z = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2];
					}

					if (idx.texcoord_index >= 0)
					{
						vertex.TexC.x = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 0];
						vertex.TexC.y = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 1];
					}

					data.Vertices.push_back(vertex);
					data.Indices32.push_back(static_cast<uint32_t>(data.Indices32.size()));
				}
				auto size = data.Vertices.size();
				auto firstVertex = data.Vertices[size - 3];
				auto secondVertex = data.Vertices[size - 2];
				auto thirdVertex = data.Vertices[size - 1];

				auto v1 = DirectX::XMLoadFloat3(&firstVertex.Position);
				auto v2 = DirectX::XMLoadFloat3(&secondVertex.Position);
				auto v3 = DirectX::XMLoadFloat3(&thirdVertex.Position);

				auto uv0 = DirectX::XMLoadFloat2(&firstVertex.TexC);
				auto uv1 = DirectX::XMLoadFloat2(&secondVertex.TexC);
				auto uv2 = DirectX::XMLoadFloat2(&thirdVertex.TexC);

				DirectX::XMVECTOR deltaUV1 = DirectX::XMVectorSubtract(uv1, uv0);
				DirectX::XMVECTOR deltaUV2 = DirectX::XMVectorSubtract(uv2, uv0);

				auto deltaPos1 = DirectX::XMVectorSubtract(v2, v1);
				auto deltaPos2 = DirectX::XMVectorSubtract(v3, v1);

				float r = 1.0f / (DirectX::XMVectorGetX(deltaUV1) * DirectX::XMVectorGetY(deltaUV2) - DirectX::XMVectorGetY(deltaUV1) * DirectX::XMVectorGetX(deltaUV2));
				auto mult = DirectX::XMVectorMultiply(DirectX::XMVectorReplicate(r),
				                                      DirectX::XMVectorSubtract(
				                                          DirectX::XMVectorMultiply(DirectX::XMVectorReplicate(DirectX::XMVectorGetY(deltaUV2)), deltaPos1),
				                                          DirectX::XMVectorMultiply(DirectX::XMVectorReplicate(DirectX::XMVectorGetY(deltaUV1)), deltaPos2)));

				auto cross = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&firstVertex.Normal), deltaPos1));

				// Orthogonalize relative to normal
				const DirectX::XMVECTOR tangent = DirectX::XMVector4Normalize(DirectX::XMVectorSubtract(mult, cross));
				DirectX::XMFLOAT3 t;
				Put(t, tangent);
				data.Vertices[size - 3].TangentU = t;
				data.Vertices[size - 2].TangentU = t;
				data.Vertices[size - 1].TangentU = t;

				index_offset += fv;
			}

			// per-face material
			const auto id = shapes[s].mesh.material_ids[0];
			if (id >= 0 && id < materials.size())
			{
				const auto& material = materials[id];

				auto diff = OApplication::GetTexturesPath(Path, UTF8ToWString(material.diffuse_texname));
				auto norm = OApplication::GetTexturesPath(Path, UTF8ToWString(material.normal_texname));
				auto height = OApplication::GetTexturesPath(Path, UTF8ToWString(material.bump_texname));
				auto alpha = OApplication::GetTexturesPath(Path, UTF8ToWString(material.alpha_texname));
				auto ambient = OApplication::GetTexturesPath(Path, UTF8ToWString(material.ambient_texname));
				auto specular = OApplication::GetTexturesPath(Path, UTF8ToWString(material.specular_texname));

				data.Material.Name = material.name;
				data.Material.DiffuseMap = diff;
				data.Material.NormalMap = height;
				data.Material.AlphaMap = alpha;
				data.Material.AmbientMap = ambient;
				data.Material.SpecularMap = specular;
				HLSL::MaterialData surf = {
					.AmbientAlbedo = { material.ambient[0], material.ambient[1], material.ambient[2] },
					.Shininess = material.shininess,
					.DiffuseAlbedo = { material.diffuse[0], material.diffuse[1], material.diffuse[2] },
					.IndexOfRefraction = material.ior,
					.SpecularAlbedo = { material.specular[0], material.specular[1], material.specular[2] },
					.Dissolve = material.dissolve,
					.Transmittance = { material.transmittance[0], material.transmittance[1], material.transmittance[2] },
					.Illumination = material.illum,
					.Emission = { material.emission[0], material.emission[1], material.emission[2] },
					.Roughness = material.roughness,
					.Metalness = material.metallic,
					.Sheen = material.sheen,
					.Reflection = 0.0

				};
				data.Material.MaterialSurface = surf;
			}
			payload.TotalIndices += data.Indices32.size();
			payload.TotalVertices += static_cast<uint32_t>(data.Vertices.size());
			payload.Data.push_back(std::move(data));
		}
		return payload;
	};

	auto threads = std::thread::hardware_concurrency();
	if (shapes.size() <= threads)
	{
		for (size_t s = 0; s < shapes.size(); s++)
		{
			futures.push_back(std::async(std::launch::async, compute, s, s + 1));
		}
	}
	else
	{
		auto bunch = shapes.size() / threads;
		uint32_t start = 0;
		uint32_t end = bunch;
		for (size_t i = 0; i < threads; i++)
		{
			futures.push_back(std::async(std::launch::async, compute, start, end));
			start = end;
			end += bunch;
		}
	}

	for (auto& future : futures)
	{
		auto payload = future.get();
		MeshData.TotalIndices += payload.TotalIndices;
		MeshData.TotalVertices += payload.TotalVertices;
		MeshData.Data.insert(MeshData.Data.end(), payload.Data.begin(), payload.Data.end());
	}

	return MeshData.TotalVertices > 0;
}

//TODO Implement parallel
/*
bool OTinyObjParser::ParseMesh(const wstring& Path, SMeshPayloadData& MeshData, ETextureMapType Type)
{
	tinyobj_opt::attrib_t attrib;
	std::vector<tinyobj_opt::shape_t> shapes;
	std::vector<tinyobj_opt::material_t> materials;

	tinyobj_opt::LoadOption option;
	option.verbose = true;
	option.req_num_threads = std::thread::hardware_concurrency();
	if (!parseObj(&attrib, &shapes, &materials, std::filesystem::path(Path), option))
	{
		LOG(TinyObjLoader, Warning, "Couldn't parse the file: {}", Path);
	}

	MeshData.Data.reserve(shapes.size());
	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++)
	{
		LOG(Geometry, Log, "Loading mesh: {}", TEXT(shapes[s].name));
		OGeometryGenerator::SMeshData data;
		data.Name = shapes[s].name;
		data.Vertices.reserve(shapes[s].length / 3);
		size_t index_offset = shapes[s].face_offset; // Start at the correct offset for this shape

		for (size_t f = 0; f < shapes[s].length; f++)
		{
			const size_t fv = attrib.face_num_verts[index_offset + f];

			for (size_t v = 0; v < fv; v++)
			{
				OGeometryGenerator::SGeometryExtendedVertex vertex{};
				// Access to vertex
				tinyobj_opt::index_t idx = attrib.indices[index_offset + v]; // Index in the flattened index array
				vector point = {
					attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 0],
					attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 1],
					attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 2]
				};
				vertex.Position = DirectX::XMFLOAT3(point[0], point[1], point[2]);

				if (idx.normal_index >= 0)
				{
					vertex.Normal.x = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0];
					vertex.Normal.y = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1];
					vertex.Normal.z = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2];
				}

				if (idx.texcoord_index >= 0)
				{
					vertex.TexC.x = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 0];
					vertex.TexC.y = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 1];
				}

				data.Vertices.push_back(vertex);
				data.Indices32.push_back(static_cast<uint32_t>(index_offset + f));
			}
		}

		// per-face material
		const auto id = attrib.material_ids[0];

		const auto& material = materials[id];
		auto diff = OApplication::Get()->GetTexturesPath(Path, UTF8ToWString(material.diffuse_texname));
		auto norm = OApplication::Get()->GetTexturesPath(Path, UTF8ToWString(material.normal_texname));
		auto height = OApplication::Get()->GetTexturesPath(Path, UTF8ToWString(material.bump_texname));
		data.Material.Name = material.name;
		if (!diff.empty())
		{
			data.Material.DiffuseMaps.push_back(diff);
		}
		if (!norm.empty())
		{
			data.Material.NormalMaps.push_back(norm);
		}
		if (!height.empty())
		{
			data.Material.HeightMaps.push_back(height);
		}
		const SMaterialSurface surf = {
			.DiffuseAlbedo = { material.ambient[0], material.ambient[1], material.ambient[2], 1.0 },
			.FresnelR0 = { material.specular[0], material.specular[1], material.specular[2] },
			.Emission = { material.emission[0], material.emission[1], material.emission[2] },
			.Roughness = 1 - (material.shininess / 100),
			.IndexOfRefraction = material.ior,
			.Dissolve = material.dissolve
		};
		data.Material.MaterialSurface = surf;
		MeshData.TotalIndices += data.Indices32.size();
		MeshData.TotalVertices += static_cast<uint32_t>(data.Vertices.size());
		MeshData.Data.push_back(std::move(data));
	}
	return true;
}*/