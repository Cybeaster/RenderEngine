

#ifndef MAX_DIFFUSE_MAPS
#define MAX_DIFFUSE_MAPS 3
#endif

#ifndef MAX_NORMAL_MAPS
#define MAX_NORMAL_MAPS 3
#endif

#ifndef MAX_HEIGHT_MAPS
#define MAX_HEIGHT_MAPS 3
#endif

#ifndef MAX_SHADOW_MAPS
#define MAX_SHADOW_MAPS 2
#endif

#define TEXTURE_MAPS_NUM 256
#define SHADOW_MAPS_NUM 2

#define EPSILON 0.0001f

// Include structures and functions for lighting.
#include "LightingUtils.hlsl"
#include "Samplers.hlsl"
struct InstanceData
{
	float4x4 World;
	float4x4 TexTransform;
	uint MaterialIndex;
	float2 DisplacementMapTexelSize;
	float GridSpatialStep;
};

struct TextureData
{
	uint bIsEnabled;
	uint TextureIndex;
};

struct MaterialData
{
	float3 AmbientAlbedo;
	float3 DiffuseAlbedo;
	float3 SpecularAlbedo;
	float3 Transmittance;
    float3 Emission;
	float Shininess;
	float IndexOfRefraction;
	float Dissolve;
	int Illumination;
	float Roughness;
	float Metalness;
	float Sheen;

	float4x4 MatTransform;
	uint DiffuseMapCount;
	uint NormalMapCount;
	uint HeightMapCount;
	float pad0;

	uint DiffuseMapIndex[MAX_DIFFUSE_MAPS];
	float pad01;

	uint NormalMapIndex[MAX_NORMAL_MAPS];
	float pad02;

	uint HeightMapIndex[MAX_HEIGHT_MAPS];
	TextureData AlphaMap;
	TextureData SpecularMap;
	TextureData AmbientMap;
	float pad4;
	float pad5;

};


Texture2D gTextureMaps[TEXTURE_MAPS_NUM] : register(t0);
Texture2D gShadowMaps[SHADOW_MAPS_NUM] : register(t1,space2);

TextureCube gCubeMap : register(t1,space3);
Texture2D gSsaoMap   : register(t2,space3);

StructuredBuffer<InstanceData> gInstanceData : register(t0, space4);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space4);

StructuredBuffer<SpotLight> gSpotLights : register(t2, space4);
StructuredBuffer<PointLight> gPointLights : register(t3, space4);
StructuredBuffer<DirectionalLight> gDirectionalLights : register(t4, space4);

cbuffer cbPass : register(b0)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float4x4 gViewProjTex;
	float3 gEyePosW;

	float cbPerPassPad1; // Use this to pad gEyePosW to 16 bytes

	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	float4 gAmbientLight;
	float4 gFogColor;
	float gFogStart;
	float gFogRange;

	uint gNumDirLights;

	float cbPerPassPad2; // Padding to align following uints
	uint gNumPointLights;
	uint gNumSpotLights;
	float cbPerPassPad3; // Padding to ensure the cbuffer ends on a 16-byte boundary
	float cbPerPassPad4; // Padding to ensure the cbuffer ends on a 16-byte boundary
	bool gSSAOEnabled;
};


bool IsTangentValid(float3 TangentW)
{
    float sum = abs(TangentW.x) + abs(TangentW.y) + abs(TangentW.z);
	return sum > EPSILON;
}

float3 NormalSampleToWorldSpace(float3 NormalMapSample, float3 UnitNormalW, float3 TangentW)
{
	float3 normalT = NormalMapSample * 2.0f - 1.0f;
	float3 N = UnitNormalW;
	/*
	*  This code makes sure T is orthonormal to N by subtracting off any
	    component of T along the direction N
	*/
	float3 T = normalize(TangentW - dot(TangentW, N) * N);
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);
	float3 bumpedNormal = mul(normalT, TBN);
	return bumpedNormal;
}

struct BumpedData
{
    float4 NormalMapSample;
	float3 BumpedNormalW;
};

BumpedData ComputeNormalMaps(float3 NormalW, float3 TangentW, MaterialData matData, float2 TexC)
{
    BumpedData data = (BumpedData)0.0;
	data.BumpedNormalW = NormalW;
	data.NormalMapSample = float4(0.f, 0.f, 1.0f, 1.0f);
    uint numNormalMaps = matData.NormalMapCount;
    for (uint i = 0; i < numNormalMaps; ++i)
    {
    	int normalMapIndex = matData.NormalMapIndex[i];
    	data.NormalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, TexC);
    	data.BumpedNormalW = NormalSampleToWorldSpace(data.NormalMapSample.rgb, normalize(NormalW), TangentW);
    }
	return data;
}

float3 ComputeNormalMap(uint Index, float3 NormalW, float3 TangentW, MaterialData matData, float2 TexC, out float NormalMapSampleA)
{
	float3 bumpedNormalW = NormalW;
	float4 normalMapSample = float4(0.f, 0.f, 1.0f, 1.0f);
	NormalMapSampleA = 1.0f;
	if (IsTangentValid(TangentW))
	{
		int normalMapIndex = matData.NormalMapIndex[Index];
		normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, TexC);
		bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, NormalW, TangentW);
		NormalMapSampleA = normalMapSample.a;
	}
	return bumpedNormalW;
}

float3 ComputeHeightMaps(MaterialData matData, float3 NormalW, float3 TangentW, float2 TexC,float Offset )
{
	uint numHeightMaps = matData.HeightMapCount;
	float3 pos = float3(0.0f, 0.0f, 0.0f);
	for (uint j = 0; j < numHeightMaps; ++j)
	{
		int heightMapIndex = matData.HeightMapIndex[j];
		float heightMapSample = gTextureMaps[heightMapIndex].SampleLevel(gsamAnisotropicWrap, TexC,Offset).r;
		pos += heightMapSample;
	}
	return pos;
}

float4 ComputeDiffuseMaps(MaterialData MatData, float4 DiffuseAlbedo, float2 TexC)
{
	uint numDiffuseMaps = MatData.DiffuseMapCount;
	for (uint k = 0; k < numDiffuseMaps; ++k)
	{
		int diffuseMapIndex = MatData.DiffuseMapIndex[k];
		DiffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, TexC);
	}
	return DiffuseAlbedo;
}

void FindShadowPosition(out float4 ShadowPositions[MAX_SHADOW_MAPS], float4 PosW, out uint LightIndices[MAX_SHADOW_MAPS])
{
    uint indx = 0;
    if(gNumDirLights > 0)
    {
        LightIndices[indx] = gDirectionalLights[0].ShadowMapIndex;
        ShadowPositions[indx] = mul(PosW,  gDirectionalLights[0].Transform);
        indx++;
    }

    if(gNumSpotLights > 0)
    {
        LightIndices[indx] = gSpotLights[0].ShadowMapIndex;
        ShadowPositions[indx] = mul(PosW,  gSpotLights[0].Transform);
        indx++;
    }
}

float CalcShadowFactor(float4 ShadowPosH,uint ShadowMapIndex)
{
    //Perform projection
    ShadowPosH.xyz /= ShadowPosH.w;

    //depth in NDC space
    float depth = ShadowPosH.z;

    Texture2D shadowMap = gShadowMaps[ShadowMapIndex];
    //Get shadow map dimensions
    uint width, height, numMips;
    shadowMap.GetDimensions(0, width, height, numMips);

    //texel size
    float dx = 1.0f / (float)width;
    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    [unroll]
    for(int i = 0; i < 9; ++i)
    {
       percentLit += shadowMap.SampleCmpLevelZero(gsamShadow,
                  ShadowPosH.xy + offsets[i], depth).r;
    }
    return percentLit / 9.0f;
}

void GetShadowFactor(out float ShadowFactors[MAX_SHADOW_MAPS],uint ShadowMapIndices[MAX_SHADOW_MAPS] ,float4 ShadowPositions[MAX_SHADOW_MAPS])
{
        for (uint i = 0; i < MAX_SHADOW_MAPS; ++i)
        {
            ShadowFactors[i] = CalcShadowFactor(ShadowPositions[i],ShadowMapIndices[i]);;
        }

}

bool IsNormalized(float3 vector)
{
    float lengthSquared = dot(vector, vector);
    return abs(lengthSquared - 1.0) < EPSILON;
}

bool AreOrthogonal(float3 vec1, float3 vec2)
 {
    float dotProduct = dot(vec1, vec2);
    return abs(dotProduct) < EPSILON;
}

float GetAlphaValue(MaterialData Data, float2 TexC)
{
	if(Data.AlphaMap.bIsEnabled == 1)
	{
		return gTextureMaps[Data.AlphaMap.TextureIndex].Sample(gsamAnisotropicWrap, TexC).r;
	}
	else
	{
		return Data.Dissolve;
	}
}

float4 GetSpecularValue(MaterialData Data, float2 TexC)
{
	if(Data.SpecularMap.bIsEnabled == 1)
	{
		return gTextureMaps[Data.SpecularMap.TextureIndex].Sample(gsamAnisotropicWrap, TexC);
	}
	else
	{
		return float4(1.0,1.0,1.0,1.0);
	}
}

float4 GetAmbientValue(MaterialData Data, float2 TexC)
{
	if(Data.AmbientMap.bIsEnabled == 1)
	{
		return gTextureMaps[Data.AmbientMap.TextureIndex].Sample(gsamAnisotropicWrap, TexC);
	}
	else
	{
		return float4(1.0,1.0,1.0,1.0);
	}
}

float3 GammaCorrect(float3 Color)
{
    return pow(Color, 1.0 / 2.2);
}

float CalcFresnelR0(float Ior)
{
    return pow((1.0 - Ior) / (1.0 + Ior), 2.0);
}