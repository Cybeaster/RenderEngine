#pragma once
#include "Engine/RenderTarget/RenderObject/RenderObject.h"
#include "Engine/UploadBuffer/UploadBuffer.h"

class OCommandQueue;
struct SRenderTargetParams
{
	weak_ptr<ODevice> Device;
	UINT Width;
	UINT Height;
	DXGI_FORMAT Format;
	EResourceHeapType HeapType;
};

class ORenderTargetBase : public ORenderObjectBase
{
public:
	ORenderTargetBase(const weak_ptr<ODevice>& Device,
	                  UINT Width, UINT Height,
	                  DXGI_FORMAT Format, EResourceHeapType HeapType)
	    : Width(Width), Height(Height), Format(Format), Device(Device), ORenderObjectBase(HeapType)
	{
	}

	ORenderTargetBase(const SRenderTargetParams& Params)
	    : Width(Params.Width), Height(Params.Height), Format(Params.Format), Device(Params.Device), ORenderObjectBase(Params.HeapType)
	{
	}

	ORenderTargetBase(UINT Width, UINT Height, EResourceHeapType HeapType);

	virtual void BuildDescriptors() = 0;
	virtual void BuildResource() = 0;
	virtual SResourceInfo* GetSubresource(uint32_t Idx = 0);
	virtual SResourceInfo* GetResource() = 0;
	D3D12_RESOURCE_DESC GetResourceDesc() const;
	uint32_t GetNumSRVRequired() const override;
	uint32_t GetNumRTVRequired() const override;
	uint32_t GetNumDSVRequired() const override;
	void CopyTo(ORenderTargetBase* Dest, const OCommandQueue* CommandQueue);
	void OnResize(const ResizeEventArgs& Args) override;
	virtual SDescriptorPair GetDSV(uint32_t SubtargetIdx = 0) const;

	virtual void InitRenderObject();

	TUUID GetID() override;
	void SetID(TUUID) override;
	void SetViewport(ID3D12GraphicsCommandList* List) const;
	virtual void PrepareRenderTarget(OCommandQueue* Queue, bool ClearRenderTarget, bool ClearDepth, uint32_t SubtargetIdx = 0);

	void UnsetRenderTarget(OCommandQueue* CommandQueue);
	wstring GetName() const override
	{
		return Name;
	}

	LONG GetWidth() const;
	LONG GetHeight() const;

protected:
	virtual void BuildViewport();

	D3D12_VIEWPORT Viewport;
	D3D12_RECT ScissorRect;
	LONG Width = 0;
	LONG Height = 0;
	DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	weak_ptr<ODevice> Device;
	unordered_set<uint32_t> PreparedTaregts;
	wstring Name = L"";
};

class OOffscreenTexture : public ORenderTargetBase
{
public:
	OOffscreenTexture(const weak_ptr<ODevice>& Device,
	                  UINT Width, UINT Height,
	                  DXGI_FORMAT Format);

	OOffscreenTexture(const OOffscreenTexture& rhs) = delete;
	OOffscreenTexture& operator=(const OOffscreenTexture& rhs) = delete;

	~OOffscreenTexture() override;

	SDescriptorPair GetSRV(uint32_t SubtargetIdx = 0) const override { return SRVHandle; }
	SDescriptorPair GetRTV(uint32_t SubtargetIdx = 0) const override { return RTVHandle; }
	uint32_t GetNumRTVRequired() const override { return 1; }
	uint32_t GetNumSRVRequired() const override { return 1; }

	void BuildDescriptors(IDescriptor* Descriptor) override;
	void InitRenderObject() override;

protected:
	void BuildDescriptors() override;
	void BuildResource() override;

public:
	SResourceInfo* GetResource() override;

private:
	SDescriptorPair SRVHandle;
	SDescriptorPair RTVHandle;

	// Two for ping-ponging the textures.
	TResourceInfo RenderTarget;
};
