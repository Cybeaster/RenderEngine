#pragma once
#include "Engine/RenderTarget/RenderTarget.h"
class ONormalTangentDebugTarget : public ORenderTargetBase
{
public:
	ONormalTangentDebugTarget(ID3D12Device* Device, int Width, int Height, DXGI_FORMAT Format);
	uint32_t GetNumRTVRequired() const override;
	uint32_t GetNumSRVRequired() const override;
	void InitRenderObject() override;
	void BuildDescriptors() override;
	void BuildDescriptors(IDescriptor* Descriptor) override;
	void BuildResource() override;
	SResourceInfo* GetResource() override;

	SDescriptorPair GetSRV(uint32_t SubtargetIdx) const override;
	SDescriptorPair GetRTV(uint32_t SubtargetIdx) const override;

private:
	SResourceInfo Target;

	SDescriptorPair RTV;
	SDescriptorPair SRV;
};
