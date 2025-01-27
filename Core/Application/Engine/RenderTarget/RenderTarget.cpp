
#include "RenderTarget.h"

#include "Engine/Engine.h"
#include "Window/Window.h"

ORenderTargetBase::ORenderTargetBase(UINT Width, UINT Height, EResourceHeapType HeapType)
    : Width(Width), Height(Height), Format(SRenderConstants::BackBufferFormat), Device(OEngine::Get()->GetDevice()), ORenderObjectBase(HeapType)
{
}

uint32_t ORenderTargetBase::GetNumSRVRequired() const
{
	return 0;
}

uint32_t ORenderTargetBase::GetNumRTVRequired() const
{
	return 0;
}

uint32_t ORenderTargetBase::GetNumDSVRequired() const
{
	return 0;
}

void ORenderTargetBase::CopyTo(ORenderTargetBase* Dest, const OCommandQueue* CommandQueue)
{
	CommandQueue->CopyResourceTo(Dest, this);
}

SDescriptorPair ORenderTargetBase::GetDSV(uint32_t SubtargetIdx) const
{
	return OEngine::Get()->GetWindow().lock()->GetDSV(); // TODO recursive call
}

void ORenderTargetBase::InitRenderObject()
{
	BuildViewport();
}

void ORenderTargetBase::SetViewport(ID3D12GraphicsCommandList* List) const
{
	List->RSSetViewports(1, &Viewport);
	List->RSSetScissorRects(1, &ScissorRect);
}

void ORenderTargetBase::PrepareRenderTarget(OCommandQueue* Queue, bool ClearRenderTarget, bool ClearDepth, uint32_t SubtargetIdx)
{
	if (PreparedTaregts.contains(SubtargetIdx))
	{
		return;
	}
	PreparedTaregts.insert(SubtargetIdx);
	auto backbufferView = GetRTV(SubtargetIdx);
	auto depthStencilView = GetDSV(SubtargetIdx);
	Utils::ResourceBarrier(Queue->GetCommandList().Get(), GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (ClearRenderTarget)
	{
		Queue->ClearRenderTarget(backbufferView, SColor::Blue);
	}
	if (ClearDepth)
	{
		Queue->ClearDepthStencil(depthStencilView);
	}
	LOG(Engine, Log, "Setting render target with address: [{}] in [{}] and depth stencil: [{}]", TEXT(backbufferView.CPUHandle.ptr), GetName(), TEXT(depthStencilView.CPUHandle.ptr));
	Queue->GetCommandList()->OMSetRenderTargets(1, &backbufferView.CPUHandle, true, &depthStencilView.CPUHandle);
}

void ORenderTargetBase::UnsetRenderTarget(OCommandQueue* CommandQueue)
{
	PreparedTaregts.clear();
}
LONG ORenderTargetBase::GetWidth() const
{
	return Width;
}

LONG ORenderTargetBase::GetHeight() const
{
	return Height;
}

TUUID ORenderTargetBase::GetID()
{
	return ID;
}

void ORenderTargetBase::SetID(TUUID Other)
{
	ID = Other;
}

void ORenderTargetBase::BuildViewport()
{
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = static_cast<float>(Width);
	Viewport.Height = static_cast<float>(Height);
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;

	ScissorRect = { 0, 0, Width, Height };
}
void ORenderTargetBase::OnResize(const ResizeEventArgs& Args)
{
	if (Width != Args.Width || Height != Args.Height)
	{
		Width = Args.Width;
		Height = Args.Height;
		BuildViewport();
		BuildResource();
		BuildDescriptors();
	}
}
SResourceInfo* ORenderTargetBase::GetSubresource(uint32_t Idx)
{
	return nullptr;
}

D3D12_RESOURCE_DESC ORenderTargetBase::GetResourceDesc() const
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
	return texDesc;
}

OOffscreenTexture::OOffscreenTexture(const weak_ptr<ODevice>& Device, UINT Width, UINT Height, DXGI_FORMAT Format)
    : ORenderTargetBase(Device, Width, Height, Format, EResourceHeapType::Default)
{
	Name = L"OffscreenTexture";
}
OOffscreenTexture::~OOffscreenTexture()
{
}

void OOffscreenTexture::BuildDescriptors(IDescriptor* Descriptor)
{
	if (const auto descriptor = Cast<SRenderObjectHeap>(Descriptor))
	{
		descriptor->SRVHandle.Offset(SRVHandle);
		descriptor->RTVHandle.Offset(RTVHandle);
		BuildDescriptors();
	}
}

void OOffscreenTexture::InitRenderObject()
{
	ORenderTargetBase::InitRenderObject();
	BuildResource();
}

void OOffscreenTexture::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	auto device = Device.lock();
	device->CreateShaderResourceView(RenderTarget, srvDesc, SRVHandle);
	device->CreateRenderTargetView(RenderTarget, RTVHandle);
}

void OOffscreenTexture::BuildResource()
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

	RenderTarget = Utils::CreateResource(weak_from_this(), L"RenderTarget", Device.lock()->GetDevice(), D3D12_HEAP_TYPE_DEFAULT, texDesc);
}
SResourceInfo* OOffscreenTexture::GetResource()
{
	return RenderTarget.get();
}
