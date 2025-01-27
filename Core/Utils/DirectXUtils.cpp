#include "DirectXUtils.h"

#include "Engine/RenderTarget/RenderObject/RenderObject.h"
#include "Logger.h"

UINT Utils::CalcBufferByteSize(const UINT ByteSize)
{
	return (ByteSize + 255) & ~255;
}

ComPtr<ID3DBlob> Utils::CompileShader(const std::wstring& FileName, const D3D_SHADER_MACRO* Defines, const std::string& EntryPoint, const std::string& Target)
{
	UINT compileFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;
	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(FileName.c_str(), Defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(), Target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
	{
		OutputDebugStringA(static_cast<char*>(errors->GetBufferPointer()));
	}
	THROW_IF_FAILED(hr);
	return byteCode;
}

ComPtr<ID3DBlob> Utils::LoadBinary(const wstring& FileName)
{
	std::fstream fIn(FileName, std::ios::in | std::ios::binary);
	fIn.seekg(0, std::ios_base::end);
	std::fstream::pos_type size = static_cast<int>(fIn.tellg());
	fIn.seekg(0, std::ios_base::beg);

	ComPtr<ID3DBlob> blob;
	THROW_IF_FAILED(D3DCreateBlob(size, blob.GetAddressOf()));

	fIn.read(static_cast<char*>(blob->GetBufferPointer()), size);
	fIn.close();
	return blob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Utils::CreateDefaultBuffer(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList, const void* InitData, UINT64 ByteSize, ComPtr<ID3D12Resource>& UploadBuffer)
{
	ComPtr<ID3D12Resource> defaultBuffer;

	const auto property = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(ByteSize);
	// Create the actual default buffer resource.
	THROW_IF_FAILED(Device->CreateCommittedResource(&property,
	                                                D3D12_HEAP_FLAG_NONE,
	                                                &buffer,
	                                                D3D12_RESOURCE_STATE_COMMON,
	                                                nullptr,
	                                                IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	const auto uploadProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap.
	THROW_IF_FAILED(Device->CreateCommittedResource(
	    &uploadProperty,
	    D3D12_HEAP_FLAG_NONE,
	    &buffer,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(UploadBuffer.GetAddressOf())));

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = InitData;
	subResourceData.RowPitch = ByteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	auto commonCpyTransition = CD3DX12_RESOURCE_BARRIER::Transition(
	    defaultBuffer.Get(),
	    D3D12_RESOURCE_STATE_COMMON,
	    D3D12_RESOURCE_STATE_COPY_DEST);

	auto cpyDestGenericReadTransition = CD3DX12_RESOURCE_BARRIER::Transition(
	    defaultBuffer.Get(),
	    D3D12_RESOURCE_STATE_COPY_DEST,
	    D3D12_RESOURCE_STATE_GENERIC_READ);
	// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	CommandList->ResourceBarrier(1, &commonCpyTransition);

	UpdateSubresources(CommandList, defaultBuffer.Get(), UploadBuffer.Get(), 0, 0, 1, &subResourceData);

	CommandList->ResourceBarrier(1, &cpyDestGenericReadTransition);

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.
	return defaultBuffer;
}
vector<CD3DX12_STATIC_SAMPLER_DESC> Utils::GetStaticSamplers()
{
	//clang-format off
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
	    0, // shaderRegister
	    D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
	    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
	    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
	    D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
	    1, // shaderRegister
	    D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
	    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
	    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
	    D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
	    2, // shaderRegister
	    D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
	    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
	    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
	    D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
	    3, // shaderRegister
	    D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
	    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
	    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
	    D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
	    4, // shaderRegister
	    D3D12_FILTER_ANISOTROPIC, // filter
	    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
	    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
	    D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressW
	    0.0f, // mipLODBias
	    8); // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
	    5, // shaderRegister
	    D3D12_FILTER_ANISOTROPIC, // filter
	    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
	    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
	    D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressW
	    0.0f, // mipLODBias
	    8); // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
	    6, // shaderRegister
	    D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
	    D3D12_TEXTURE_ADDRESS_MODE_BORDER, // addressU
	    D3D12_TEXTURE_ADDRESS_MODE_BORDER, // addressV
	    D3D12_TEXTURE_ADDRESS_MODE_BORDER, // addressW
	    0.0f, // mipLODBias
	    16, // maxAnisotropy
	    D3D12_COMPARISON_FUNC_LESS_EQUAL,
	    D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(
	    7, // shaderRegister
	    D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
	    D3D12_TEXTURE_ADDRESS_MODE_BORDER, // addressU
	    D3D12_TEXTURE_ADDRESS_MODE_BORDER, // addressV
	    D3D12_TEXTURE_ADDRESS_MODE_BORDER, // addressW
	    0.0f,
	    0,
	    D3D12_COMPARISON_FUNC_LESS_EQUAL,
	    D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);
	return {
		pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp, shadow, depthMapSam
	};
	//clang-format on
}

D3D12_RESOURCE_STATES Utils::ResourceBarrier(ID3D12GraphicsCommandList* List, SResourceInfo* Resource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After)
{
	D3D12_RESOURCE_STATES localBefore = Resource->CurrentState;
	auto context = Resource->Context.lock();
	if (localBefore != Before)
	{
		LOG(Debug, Warning, "ResourceBarrier: Resource state mismatch on resource {}!", context->GetName());
	}

	if (localBefore == After)
	{
		LOG(Debug, Warning, "ResourceBarrier: Resource states must be different {}!", context->GetName());
		return localBefore;
	}
	Resource->CurrentState = After;

	const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(Resource->Resource.Get(), localBefore, After);
	List->ResourceBarrier(1, &barrier);
	return localBefore;
}

D3D12_RESOURCE_STATES Utils::ResourceBarrier(ID3D12GraphicsCommandList* List, SResourceInfo* Resource, D3D12_RESOURCE_STATES After)
{
	auto context = Resource->Context.lock();
	if (Resource->CurrentState == After)
	{
		LOG(Debug, Warning, "ResourceBarrier: Resource states must be different {}!", context->GetName());
		return After;
	}

	LOG(Debug, Log, "ResourceBarrier: Transitioning resource {}", context->GetName());
	auto old = Resource->CurrentState;
	const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(Resource->Resource.Get(), Resource->CurrentState, After);
	Resource->CurrentState = After;
	List->ResourceBarrier(1, &barrier);
	return old;
}

void Utils::BuildRootSignature(const shared_ptr<ODevice>& Device, ComPtr<ID3D12RootSignature>& RootSignature, const D3D12_ROOT_SIGNATURE_DESC& Desc)
{
	// Create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	THROW_IF_FAILED(D3D12SerializeRootSignature(&Desc,
	                                            D3D_ROOT_SIGNATURE_VERSION_1,
	                                            serializedRootSig.GetAddressOf(),
	                                            errorBlob.GetAddressOf()));

	CreateRootSignature(Device, RootSignature, serializedRootSig, errorBlob);
}

void Utils::BuildRootSignature(const shared_ptr<ODevice>& Device, ComPtr<ID3D12RootSignature>& RootSignature, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& Desc)
{
	// Create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&Desc,
	                                                     serializedRootSig.GetAddressOf(),
	                                                     errorBlob.GetAddressOf()));
	CreateRootSignature(Device, RootSignature, serializedRootSig, errorBlob);
}

void Utils::CreateRootSignature(const shared_ptr<ODevice>& Device, ComPtr<ID3D12RootSignature>& RootSignature, const ComPtr<ID3DBlob>& SerializedRootSig, const ComPtr<ID3DBlob>& ErrorBlob)
{
	if (ErrorBlob != nullptr)
	{
		::OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer()));
	}

	THROW_IF_FAILED(Device->GetDevice()->CreateRootSignature(0,
	                                                         SerializedRootSig->GetBufferPointer(),
	                                                         SerializedRootSig->GetBufferSize(),
	                                                         IID_PPV_ARGS(&RootSignature)));
}

DXGI_FORMAT Utils::MaskToFormat(const uint32_t Mask)
{
	switch (Mask)
	{
	case 1: // 0001: Only the first component is used (e.g., x or R).
		return DXGI_FORMAT_R32_FLOAT;
	case 3: // 0011: First and second components are used (e.g., xy or RG).
		return DXGI_FORMAT_R32G32_FLOAT;
	case 7: // 0111: First, second, and third components are used (e.g., xyz or RGB).
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case 15: // 1111: All four components are used (e.g., xyzw or RGBA).
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		// Add more cases here if you're handling other types of data (e.g., integers or 16-bit floats).
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

bool Utils::MatricesEqual(const DirectX::XMFLOAT4X4& mat1, const DirectX::XMFLOAT4X4& mat2, float epsilon)
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (fabs(mat1.m[i][j] - mat2.m[i][j]) > epsilon)
			{
				return false;
			}
		}
	}
	return true;
}

TResourceInfo Utils::CreateResource(const weak_ptr<IRenderObject>& Owner, const wstring& AppendName, ID3D12Device* Device, const D3D12_HEAP_TYPE HeapProperties, const D3D12_RESOURCE_DESC& Desc, const D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue)
{
	auto owner = Owner.lock();
	SResourceInfo info{
		.CurrentState = InitialState,
		.Context = Owner,
		.Name = owner->GetName() + L"_" + AppendName
	};
	const auto defaultHeap = CD3DX12_HEAP_PROPERTIES(HeapProperties);
	THROW_IF_FAILED(Device->CreateCommittedResource(&defaultHeap,
	                                                D3D12_HEAP_FLAG_NONE,
	                                                &Desc,
	                                                InitialState,
	                                                ClearValue,
	                                                IID_PPV_ARGS(&info.Resource)));
	info.Resource->SetName(info.Name.c_str());
	return make_shared<SResourceInfo>(info);
}
TResourceInfo Utils::CreateResource(const weak_ptr<IRenderObject>& Owner, const wstring& AppendName, ID3D12Device* Device, D3D12_HEAP_TYPE HeapProperties, const D3D12_RESOURCE_DESC& Desc, D3D12_RESOURCE_STATES InitialState, ID3D12GraphicsCommandList* CMDList, const D3D12_CLEAR_VALUE* ClearValue)
{
	auto resource = CreateResource(Owner, AppendName, Device, HeapProperties, Desc, D3D12_RESOURCE_STATE_COMMON, ClearValue);
	ResourceBarrier(CMDList, resource.get(), D3D12_RESOURCE_STATE_COMMON, InitialState);
	return resource;
}

TResourceInfo Utils::CreateResource(const weak_ptr<IRenderObject>& Owner, const wstring& AppendName, ID3D12Device* Device, D3D12_RESOURCE_FLAGS Flags, D3D12_RESOURCE_STATES InitialState, const D3D12_HEAP_PROPERTIES& HeapProps, const uint64_t Size)
{
	auto lock = Owner.lock();
	SResourceInfo info{
		.CurrentState = InitialState,
		.Context = Owner,
		.Name = lock->GetName() + L"_" + AppendName
	};

	D3D12_RESOURCE_DESC bufDesc = {};
	bufDesc.Alignment = 0;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Flags = Flags;
	bufDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufDesc.Height = 1;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufDesc.MipLevels = 1;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.SampleDesc.Quality = 0;
	bufDesc.Width = Size;

	THROW_IF_FAILED(Device->CreateCommittedResource(&HeapProps,
	                                                D3D12_HEAP_FLAG_NONE,
	                                                &bufDesc,
	                                                InitialState,
	                                                nullptr,
	                                                IID_PPV_ARGS(&info.Resource)));
	info.Resource->SetName(info.Name.c_str());
	return make_shared<SResourceInfo>(info);
}