#pragma once

#include "MaterialConstants.h"
#include "RenderConstants.h"

struct SMaterial
{
	string Name;

	// Index into constant buffer corresponding to this material.
	int32_t MaterialCBIndex = -1;

	// Index into SRV heap for diffuse texture
	int DiffuseSRVHeapIndex = -1;

	// Dirty flag indicating the material has changed and we need to
	// update the constant buffer. Because we have a material constant
	// buffer for each FrameResource, we have to apply the update to each
	// FrameResource. Thus, when we modify a material we should set
	// NumFramesDirty = gNumFrameResources so that each frame resource
	// gets the update.

	int NumFramesDirty = SRenderConstants::NumFrameResources;

	// Material constant buffer data used for shading.
	SMaterialConstants MaterialConsatnts;
};
