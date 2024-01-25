//
// Created by Cybea on 24/01/2024.
//

#include "RenderTarget.h"
ORenderTarget::ORenderTarget(ID3D12Device* Device, UINT Width, UINT Height, DXGI_FORMAT Format)
    : Device(Device), Width(Width), Height(Height), Format(Format)
{
	BuildResource();
}

void ORenderTarget::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _CpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE _GpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE _CpuRtv)
{
	CpuSrv = _CpuSrv;
	GpuSrv = _GpuSrv;
	CpuRtv = _CpuRtv;

	BuildDescriptors();
}

void ORenderTarget::OnResize(UINT NewWidth, UINT NewHeight)
{
	if (Width != NewWidth || Height != NewHeight)
	{
		Width = NewWidth;
		Height = NewHeight;

		BuildResource();
		BuildDescriptors();
	}
}

void ORenderTarget::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	Device->CreateShaderResourceView(OffscreenTex.Get(), &srvDesc, CpuSrv);
	Device->CreateRenderTargetView(OffscreenTex.Get(), nullptr, CpuRtv);
}

void ORenderTarget::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = Width;
	texDesc.Height = Height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = Format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	const auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	THROW_IF_FAILED(Device->CreateCommittedResource(&defaultHeap,
	                                                D3D12_HEAP_FLAG_NONE,
	                                                &texDesc,
	                                                D3D12_RESOURCE_STATE_GENERIC_READ,
	                                                nullptr,
	                                                IID_PPV_ARGS(&OffscreenTex)));
}
