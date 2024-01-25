#pragma once
#include "DXHelper.h"
class ORenderTarget
{
public:
	ORenderTarget(ID3D12Device* Device,
	              UINT Width, UINT Height,
	              DXGI_FORMAT Format);

	ORenderTarget(const ORenderTarget& rhs) = delete;
	ORenderTarget& operator=(const ORenderTarget& rhs) = delete;
	~ORenderTarget() = default;

	ID3D12Resource* GetResource() const { return OffscreenTex.Get(); }
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSRV() const { return GpuSrv; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return CpuRtv; }

	void BuildDescriptors(
	    CD3DX12_CPU_DESCRIPTOR_HANDLE _CpuSrv,
	    CD3DX12_GPU_DESCRIPTOR_HANDLE _GpuSrv,
	    CD3DX12_CPU_DESCRIPTOR_HANDLE _CpuRtv);

	void OnResize(UINT NewWidth, UINT NewHeight);

private:
	void BuildDescriptors();
	void BuildResource();

	ID3D12Device* Device = nullptr;

	UINT Width = 0;
	UINT Height = 0;

	DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE CpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CpuRtv;

	// Two for ping-ponging the textures.
	ComPtr<ID3D12Resource> OffscreenTex = nullptr;
};
