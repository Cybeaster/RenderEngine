
#pragma once
#include "DirectX/HLSL/HlslTypes.h"
#include "DirectX/Light/Light.h"
#include "Engine/RenderTarget/RenderObject/RenderObject.h"
#include "RenderItemComponentBase.h"

class OCSM;
class OShadowMap;
struct SFrameResource;
ENUM(ELightType,
     Directional,
     Point,
     Spot)

inline string ToString(const ELightType& Type)
{
	switch (Type)
	{
	case ELightType::Directional:
		return "Directional";
		break;
	case ELightType::Point:
		return "Point";
		break;
	case ELightType::Spot:
		return "Spot";
		break;
	}
	return "Unknown";
}

class OLightComponent : public OSceneComponent
{
public:
	OLightComponent();

	void Tick(UpdateEventArgs Arg) override;
	void Init(ORenderItem* Other) override;
	virtual void SetLightSourceData() = 0;

	int NumFramesDirty = SRenderConstants::NumFrameResources;

	virtual void SetPassConstant(SPassConstants& OutConstant) = 0;
	virtual ELightType GetLightType() const = 0;
	virtual void UpdateFrameResource(const SFrameResource* FrameResource) = 0;

	void UpdateLightData();

	virtual int32_t GetLightIndex() const = 0;
	DirectX::XMVECTOR GetGlobalPosition() const;
	void MarkDirty();
	bool UseDirty();
	bool TryUpdate();

	SDelegate<void> OnLightChanged;

private:
	bool bIsDirty = true;
};

//Directional Light component
class ODirectionalLightComponent final : public OLightComponent
{
public:
	ODirectionalLightComponent(uint32_t InIdx);

	void SetLightSourceData() override;
	ELightType GetLightType() const override;
	int32_t GetLightIndex() const override;
	void UpdateFrameResource(const SFrameResource* FrameResource) override;
	void SetPassConstant(SPassConstants& OutConstant) override;
	void Tick(UpdateEventArgs Arg) override;
	void SetDirectionalLight(const SDirectionalLightPayload& Light);
	void InitFrameResource(const TUploadBufferData<HLSL::DirectionalLight>& Spot);
	void SetCSM(const weak_ptr<OCSM>& InCSM);
	shared_ptr<OCSM> GetCSM() const;
	void SetShadowMapIndices(array<UINT, MAX_CSM_PER_FRAME> Maps);

	HLSL::DirectionalLight& GetDirectionalLight();
	array<float, 3> CascadeSplits;

	void SetCascadeLambda(float InLambda);
	float GetCascadeLambda() const;
	void UpdateCascadeSplits();

	float RadiusScale = 1.f;
	DirectX::XMFLOAT3 AnimationDelta = { 0.0f, 0.0f, 0.0f };

private:
	float CascadeSplitLambda = 0.65;
	TUploadBufferData<HLSL::DirectionalLight> DirLightBufferInfo;
	HLSL::DirectionalLight DirectionalLight;
	weak_ptr<OCSM> CSM;
};

//Point light component
class OPointLightComponent final : public OLightComponent
{
public:
	OPointLightComponent(uint32_t InIdx);
	ELightType GetLightType() const override;
	HLSL::PointLight& GetPointLight();
	void SetPointLight(const SPointLightPayload& Light);
	void SetShadowMapIndex(UINT Index);
	int32_t GetLightIndex() const override;
	void UpdateFrameResource(const SFrameResource* FrameResource) override;
	void InitFrameResource(const TUploadBufferData<HLSL::PointLight>& Spot);
	void SetPassConstant(SPassConstants& OutConstant) override;
	void SetLightSourceData() override;

private:
	HLSL::PointLight PointLight = {};
	TUploadBufferData<HLSL::PointLight> PointLightBufferInfo;
	weak_ptr<OShadowMap> ShadowMap = {};
};

class OSpotLightComponent final : public OLightComponent
{
public:
	OSpotLightComponent(uint32_t InIdx);
	void SetSpotLight(const SSpotLightPayload& Light);
	void SetShadowMapIndex(UINT Index);
	int32_t GetLightIndex() const override;
	void UpdateFrameResource(const SFrameResource* FrameResource) override;
	void InitFrameResource(const TUploadBufferData<HLSL::SpotLight>& Spot);
	ELightType GetLightType() const override;
	void SetPassConstant(SPassConstants& OutConstant) override;
	void SetLightSourceData() override;
	HLSL::SpotLight& GetSpotLight();
	void SetShadowMap(const shared_ptr<OShadowMap>& InShadow);

private:
	TUploadBufferData<HLSL::SpotLight> SpotLightBufferInfo;
	HLSL::SpotLight SpotLight = {};
	weak_ptr<OShadowMap> ShadowMap = {};
};