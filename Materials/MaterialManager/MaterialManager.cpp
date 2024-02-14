#include "MaterialManager.h"

#include "../../Config/ConfigReader.h"
#include "../../Textures/Texture.h"
#include "../../Utils/EngineHelper.h"
#include "Application.h"
#include "Logger.h"
#include "Settings.h"
#include "TextureConstants.h"

#include <future>

void OMaterialManager::CreateMaterial(const string& Name, STexture* Texture, const SMaterialSurface& Surface, bool Notify /*= false*/)
{
	auto mat = make_unique<SMaterial>();
	mat->Name = Name;
	mat->MaterialCBIndex = Materials.size();
	mat->TexturePath = Texture->FileName;
	mat->DiffuseTexture = Texture;
	mat->MaterialSurface = Surface;
	AddMaterial(Name, mat, Notify);
}

OMaterialManager::OMaterialManager()
{
	MaterialsConfigParser = make_unique<OMaterialsConfigParser>(OApplication::Get()->GetConfigPath("MaterialsConfigPath"));
}

void OMaterialManager::AddMaterial(string Name, unique_ptr<SMaterial>& Material, bool Notify /*= false*/)
{
	if (Materials.contains(Name))
	{
		LOG(Engine, Warning, "Material with this name already exists!");
		return;
	}
	Materials[Name] = move(Material);
	if (Notify)
	{
		MaterialsRebuld.Broadcast();
	}
}

const OMaterialManager::TMaterialsMap& OMaterialManager::GetMaterials() const
{
	return Materials;
}

SMaterial* OMaterialManager::FindMaterial(const string& Name) const
{
	if (!Materials.contains(Name))
	{
		LOG(Engine, Error, "Material not found!");
		return Name != STextureNames::Debug ? FindMaterial(STextureNames::Debug) : nullptr;
		;
	}
	return Materials.at(Name).get();
}

uint32_t OMaterialManager::GetMaterialCBIndex(const string& Name)
{
	const auto material = FindMaterial(Name);
	if (!material)
	{
		LOG(Engine, Error, "Material not found!");
		return Name != STextureNames::Debug ? GetMaterialCBIndex(STextureNames::Debug) : -1;
	}
	return material->MaterialCBIndex;
}

uint32_t OMaterialManager::GetNumMaterials()
{
	return Materials.size();
}

void OMaterialManager::LoadMaterialsFromCache()
{
	Materials = std::move(MaterialsConfigParser->LoadMaterials());
	uint32_t it = 0;
	for (auto& val : Materials | std::views::values)
	{
		auto& mat = val;
		mat->DiffuseTexture = FindOrCreateTexture(val->TexturePath);
		ENSURE(mat->DiffuseTexture->HeapIdx != -1);
		mat->MaterialCBIndex = it;
		++it;
	}
	MaterialsRebuld.Broadcast();
}

void OMaterialManager::SaveMaterials() const
{
	//TODO possible data run
	std::unordered_map<string, SMaterial*> materials;
	for (const auto& [fst, snd] : this->Materials)
	{
		materials[fst] = snd.get();
	}

	std::thread([&, localMat = std::move(materials)]() {
		MaterialsConfigParser->AddMaterials(localMat);
	}).detach();
}

void OMaterialManager::BuildMaterialsFromTextures(const std::unordered_map<string, unique_ptr<STexture>>& Textures)
{
	for (auto& texture : Textures)
	{
		CreateMaterial(texture.first, texture.second.get(), SMaterialSurface());
	}
	MaterialsRebuld.Broadcast();
}

void OMaterialManager::OnMaterialChanged(const string& Name)
{
	Materials[Name]->NumFramesDirty = SRenderConstants::NumFrameResources;
}
