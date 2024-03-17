#pragma once
#include "Engine/UploadBuffer/UploadBuffer.h"
#include "InstanceData.h"
#include "Logger.h"
#include "MaterialData.h"
#include "ObjectConstants.h"
#include "Vertex.h"

#include <Types.h>
#include <d3d12.h>
#include <wrl/client.h>

struct SFrameResource
{
	SFrameResource(ID3D12Device* Device, IRenderObject* Owner);

	SFrameResource(const SFrameResource&) = delete;

	SFrameResource& operator=(const SFrameResource&) = delete;

	~SFrameResource();

	// We cannot update a cbuffer until the GPU is done processing the
	// commands that reference it. So each frame needs their own cbuffers.

	void SetPass(UINT PassCount);
	void SetInstances(UINT InstanceCount);
	void SetMaterials(UINT MaterialCount);
	void SetDirectionalLight(UINT LightCount);
	void SetPointLight(UINT LightCount);
	void SetSpotLight(UINT LightCount);
	unique_ptr<OUploadBuffer<SPassConstants>> PassCB = nullptr;
	unique_ptr<OUploadBuffer<SMaterialData>> MaterialBuffer = nullptr;
	std::unique_ptr<OUploadBuffer<SInstanceData>> InstanceBuffer = nullptr;

	TUploadBuffer<SDirectionalLight> DirectionalLightBuffer;
	TUploadBuffer<SPointLight> PointLightBuffer;
	TUploadBuffer<SSpotLight> SpotLightBuffer;

	// Fence value to mark commands up to this fence point. This lets us
	// check if these frame resources are still in use by the GPU.
	UINT64 Fence = 0;
	IRenderObject* Owner = nullptr;
	ID3D12Device* Device = nullptr;

};

