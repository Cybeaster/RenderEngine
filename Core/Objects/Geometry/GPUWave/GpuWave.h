#pragma once
#include "EngineHelper.h"
#include "Material.h"
#include "Timer/Timer.h"
class OGPUWave : public ORenderObjectBase
{
public:
	OGPUWave(const shared_ptr<ODevice>& _Device, ID3D12GraphicsCommandList* _List, int32_t _M, int32_t _N, float dx, float dt, float speed, float damping);
	OGPUWave() = default;
	~OGPUWave() = default;

	UINT GetRowCount() const { return NumRows; }
	UINT GetColumnCount() const { return NumCols; }
	UINT GetVertexCount() const { return VertexCount; }
	UINT GetTriangleCount() const { return TriangleCount; }

	float GetWidth() const { return NumCols * SpatialStep; }
	float GetDepth() const { return NumRows * SpatialStep; }
	float GetSpatialStep() const { return SpatialStep; }

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetDisplacementMapHandle() const { return CurrSolSRVHandle.GPUHandle; }
	auto GetDiplacementMapTexelSize() const
	{
		return DirectX::XMFLOAT2(1.0f / NumCols, 1.0f / NumRows);
	}

	SRenderItemParams GetRIParams()
	{
		SRenderItemParams params;
		params.NumberOfInstances = 1;
		params.bFrustumCoolingEnabled = false;
		params.Pickable = false;
		params.Material = FindMaterial(SMaterialNames::Water);
		return params;
	}

	uint32_t GetNumSRVRequired() const override
	{
		return 6;
	}

	void BuildResources();

	void BuildDescriptors(IDescriptor* Descriptor) override;

	void Disturb(ID3D12RootSignature* RootSignature, ID3D12PipelineState* PSO, UINT I, UINT J, float Magnitude);
	void Update(const UpdateEventArgs& Event) override;
	wstring GetName() const override
	{
		return Name;
	}

private:
	void Update(const STimer& Gt, ID3D12RootSignature* RootSignature, ID3D12PipelineState* PSO);

	UINT NumRows;
	UINT NumCols;
	wstring Name = L"GPUWave";
	UINT VertexCount;
	UINT TriangleCount;

	// Simulation constants
	float mK[3];

	float TimeStep;
	float SpatialStep;

	weak_ptr<ODevice> Device;
	ID3D12GraphicsCommandList* CMDList = nullptr;

	SDescriptorPair PrevSolSRVHandle;
	SDescriptorPair CurrSolSRVHandle;
	SDescriptorPair NextSolSRVHandle;

	SDescriptorPair PrevSolUAVHandle;
	SDescriptorPair CurrSolUAVHandle;
	SDescriptorPair NextSolUAVHandle;

	// Two for ping-ponging the textures.
	TResourceInfo PrevSol;
	TResourceInfo CurrSol;
	TResourceInfo NextSol;

	TResourceInfo PrevUploadBuffer;
	TResourceInfo CurrUploadBuffer;
};
