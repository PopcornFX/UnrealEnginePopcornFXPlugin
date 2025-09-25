//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXGPUSim_D3D12.h"

// Most of this file contains editor only hacks because UE4 doesn't export some "low level" RHI symbols
// So our only option is to copy paste implementations
// Right now we create native API resources on PKFX side for GPU simulation, and have to convert those native resources
// In RHI resources, but we don't let RHI create the resources.
// So all those low level function implementations are not exposed by UE4 as it's not a use case for them

#if (PK_GPU_D3D12 == 1)

#include "ShaderCore.h"
#include "DynamicRHI.h"

#if PLATFORM_WINDOWS
#	ifdef WINDOWS_PLATFORM_TYPES_GUARD
#		include "Windows/HideWindowsPlatformTypes.h"
#	endif
#elif PLATFORM_XBOXONE
#	ifdef XBOX_PLATFORM_TYPES_GUARD
#		include "XboxCommonHidePlatformTypes.h"
#	endif // XBOX_PLATFORM_TYPES_GUARD
#endif

#include "D3D12RHIPrivate.h"

#if PLATFORM_WINDOWS
#	include "Windows/AllowWindowsPlatformTypes.h"
#elif PLATFORM_XBOXONE
#	include "XboxCommonAllowPlatformTypes.h"
#endif

#include "RenderUtils.h"

#include <pk_particles/include/ps_mediums.h>
#include <pk_particles/include/ps_stream_to_render.h>
#include <pk_kernel/include/kr_thread_pool.h>
#include <pk_particles/include/Storage/D3D12/storage_d3d12.h>

//----------------------------------------------------------------------------
//
//		D3D12
//
//----------------------------------------------------------------------------

#if WITH_EDITOR // At least this is editor only..

#if UE_BUILD_DEBUG
int64 FD3D12Resource::TotalResourceCount = 0;
int64 FD3D12Resource::NoStateTrackingResourceCount = 0;
#endif

#if ENABLE_RESIDENCY_MANAGEMENT
bool	GEnableResidencyManagement = true;
#endif

//----------------------------------------------------------------------------
//
//		D3D12 engine source code copy (D3D12RHI)
//
//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5)
FD3D12Buffer::~FD3D12Buffer()
{
	if (EnumHasAnyFlags(GetUsage(), EBufferUsageFlags::VertexBuffer) && GetParentDevice())
	{
		FD3D12CommandContext& DefaultContext = GetParentDevice()->GetDefaultCommandContext();
		DefaultContext.StateCache.ClearVertexBuffer(&ResourceLocation);
	}

	//int64 BufferSize = ResourceLocation.GetSize();
	//bool bTransient = ResourceLocation.IsTransient();
	//if (!bTransient)
	//{
	//	UpdateBufferStats((EBufferUsageFlags)GetUsage(), -BufferSize);
	//}
}

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION < 6)
uint32 FD3D12Buffer::GetParentGPUIndex() const
{
	return Parent->GetGPUIndex();
}
#endif // (ENGINE_MINOR_VERSION < 6)

#else
FD3D12VertexBuffer::~FD3D12VertexBuffer()
{
	if (ResourceLocation.GetResource() != nullptr)
	{
		// ! //

		//UpdateBufferStats(&ResourceLocation, false, D3D12_BUFFER_TYPE_VERTEX);

		DEC_MEMORY_STAT_BY(STAT_VertexBufferMemory, ResourceLocation.GetSize());
#if PLATFORM_WINDOWS
		// this is a work-around on Windows. See comment above.
		LLM_SCOPED_PAUSE_TRACKING_WITH_ENUM_AND_AMOUNT(ELLMTag::Meshes, -(int64)ResourceLocation.GetSize(), ELLMTracker::Default, ELLMAllocType::None);
		LLM_SCOPED_PAUSE_TRACKING_WITH_ENUM_AND_AMOUNT(ELLMTag::GraphicsPlatform, -(int64)ResourceLocation.GetSize(), ELLMTracker::Platform, ELLMAllocType::None);
#endif
	}
}
#endif // (ENGINE_MAJOR_VERSION == 5)

//----------------------------------------------------------------------------

FD3D12ResourceLocation::FD3D12ResourceLocation(FD3D12Device* Parent)
	: FD3D12DeviceChild(Parent)
#if (ENGINE_MAJOR_VERSION == 5)
	, Allocator(nullptr)
#else
	, Type(ResourceLocationType::eUndefined)
	, UnderlyingResource(nullptr)
	, ResidencyHandle(nullptr)
	, Allocator(nullptr)
	, MappedBaseAddress(nullptr)
	, GPUVirtualAddress(0)
	, OffsetFromBaseOfResource(0)
	, Size(0)
	, bTransient(false)
#endif // (ENGINE_MAJOR_VERSION == 5)
{
	FMemory::Memzero(AllocatorData);
}

FD3D12ResourceLocation::~FD3D12ResourceLocation()
{
	//ReleaseResource();
}

void FD3D12ResourceLocation::SetResource(FD3D12Resource* Value)
{
	check(UnderlyingResource == nullptr);
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION < 4)
	check(ResidencyHandle == nullptr);
#endif

	if (Type == ResourceLocationType::eStandAlone)
	{
		GPUVirtualAddress = Value->GetGPUVirtualAddress();
	}

	UnderlyingResource = Value;
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION < 4)
	ResidencyHandle = &UnderlyingResource->GetResidencyHandle();
#elif (ENGINE_MAJOR_VERSION == 4)
	ResidencyHandle = UnderlyingResource->GetResidencyHandle();
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION < 4)
}

//----------------------------------------------------------------------------

namespace	D3D12RHI
{
	void VerifyD3D12Result(HRESULT Result, const ANSICHAR* Code, const ANSICHAR* Filename, uint32 Line, ID3D12Device* Device, FString Message)
	{
		check(FAILED(Result));
	}
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 4)
void FD3D12Resource::StartTrackingForResidency()
{
#if ENABLE_RESIDENCY_MANAGEMENT

	if (!bRequiresResidencyTracking)
	{
		return;
	}

	checkf(IsGPUOnly(HeapType), TEXT("Residency tracking is not expected for CPU-accessible resources"));
	checkf(!Desc.bBackBuffer, TEXT("Residency tracking is not expected for back buffers"));
	checkf(!Desc.bExternal, TEXT("Residency tracking is not expected for externally-owned resources"));

	if (!IsPlacedResource() && !IsReservedResource())
	{
		checkf(!ResidencyHandle, TEXT("Residency tracking is already initialzied for this resource"));
		ResidencyHandle = new FD3D12ResidencyHandle;

		D3D12_RESOURCE_ALLOCATION_INFO Info;

		Info = GetParentDevice()->GetDevice()->GetResourceAllocationInfo(0, 1, &Desc);
		checkf(Info.SizeInBytes != UINT64_MAX, TEXT("D3D12 GetResourceAllocationInfo failed - likely a resource was requested that has invalid allocation info (e.g. is an invalid texture size)"));
		D3DX12Residency::Initialize(*ResidencyHandle, Resource.GetReference(), Info.SizeInBytes, this);
		D3DX12Residency::BeginTrackingObject(GetParentDevice()->GetResidencyManager(), *ResidencyHandle);
	}
#endif // ENABLE_RESIDENCY_MANAGEMENT
}
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 4)

//----------------------------------------------------------------------------

FD3D12Resource::FD3D12Resource(FD3D12Device* ParentDevice,
	FRHIGPUMask VisibleNodes,
	ID3D12Resource* InResource,
	D3D12_RESOURCE_STATES InitialState,
	ED3D12ResourceStateMode InResourceStateMode,
	D3D12_RESOURCE_STATES InDefaultResourceState,
#if (ENGINE_MAJOR_VERSION == 5)
	const FD3D12ResourceDesc& InDesc,
#else
	D3D12_RESOURCE_DESC const& InDesc,
#endif // (ENGINE_MAJOR_VERSION == 5)
	FD3D12Heap* InHeap,
	D3D12_HEAP_TYPE InHeapType)
	: FD3D12DeviceChild(ParentDevice)
	, FD3D12MultiNodeGPUObject(ParentDevice->GetGPUMask(), VisibleNodes)
	, Resource(InResource)
	, Heap(InHeap)
#if (ENGINE_MAJOR_VERSION == 5)
	, Desc(InDesc)
	, HeapType(InHeapType)
#	if (ENGINE_MINOR_VERSION >= 3)
	, PlaneCount(UE::DXGIUtilities::GetPlaneCount(Desc.Format))
#	else
	, PlaneCount(::GetPlaneCount(Desc.Format))
#	endif
	, bRequiresResourceStateTracking(true)
	, bDepthStencil(false)
	, bDeferDelete(true)
#	if (ENGINE_MINOR_VERSION < 4)
	, bBackBuffer(false)
#endif
#else
	, ResidencyHandle()
	, Desc(InDesc)
	, PlaneCount(::GetPlaneCount(Desc.Format))
	, SubresourceCount(0)
	, DefaultResourceState(D3D12_RESOURCE_STATE_TBD)
	, bRequiresResourceStateTracking(true)
	, bDepthStencil(false)
	, bDeferDelete(true)
#	if PLATFORM_USE_BACKBUFFER_WRITE_TRANSITION_TRACKING
	, bBackBuffer(false)
#	endif // #if PLATFORM_USE_BACKBUFFER_WRITE_TRANSITION_TRACKING
	, HeapType(InHeapType)
	, GPUVirtualAddress(0)
	, ResourceBaseAddress(nullptr)
#endif // (ENGINE_MAJOR_VERSION == 5)
{
#if UE_BUILD_DEBUG
	FPlatformAtomics::InterlockedIncrement(&TotalResourceCount);
#endif

	if (Resource)
	{
		GPUVirtualAddress = Resource->GetGPUVirtualAddress();
	}

	InitalizeResourceState(InitialState, InResourceStateMode, InDefaultResourceState);
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
#if (ENGINE_MINOR_VERSION == 3)
	if (!IsPlacedResource())
	{
		ResidencyHandle = MakeUnique<FD3D12ResidencyHandle>();
	}
#else
	StartTrackingForResidency();
#endif // (ENGINE_MINOR_VERSION == 3)
	if (Desc.bReservedResource)
	{
		checkf(Heap == nullptr, TEXT("Reserved resources are not expected to have a heap"));
		ReservedResourceData = MakeUnique<FD3D12ReservedResourceData>();
	}
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
}

//----------------------------------------------------------------------------

FD3D12Resource::~FD3D12Resource()
{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	if (!IsPlacedResource() && D3DX12Residency::IsInitialized(*ResidencyHandle))
	{
		D3DX12Residency::EndTrackingObject(GetParentDevice()->GetResidencyManager(), *ResidencyHandle);
	}
#else
	if (D3DX12Residency::IsInitialized(ResidencyHandle))
	{
		D3DX12Residency::EndTrackingObject(GetParentDevice()->GetResidencyManager(), ResidencyHandle);
	}
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION <= 5)
void	CResourceState::Initialize(uint32 SubresourceCount)
{
	check(0 == m_SubresourceState.Num());

	// Allocate space for per-subresource tracking structures
	check(SubresourceCount > 0);
	m_SubresourceState.SetNumUninitialized(SubresourceCount);
	check(m_SubresourceState.Num() == SubresourceCount);

	// All subresources start out in an unknown state
	SetResourceState(D3D12_RESOURCE_STATE_TBD);
}

//----------------------------------------------------------------------------

void	CResourceState::SetResourceState(D3D12_RESOURCE_STATES State)
{
	m_AllSubresourcesSame = 1;

	m_ResourceState = State;

	// State is now tracked per-resource, so m_SubresourceState should not be read.
#if UE_BUILD_DEBUG
	const uint32 numSubresourceStates = m_SubresourceState.Num();
	for (uint32 i = 0; i < numSubresourceStates; i++)
	{
		m_SubresourceState[i] = D3D12_RESOURCE_STATE_CORRUPT;
	}
#endif
}
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION <= 5)

//----------------------------------------------------------------------------

void SetName(ID3D12Object* const Object, const TCHAR* const Name)
{
#if NAME_OBJECTS
	if (Object)
	{
		VERIFYD3D12RESULT(Object->SetName(Name));
	}
#else
	UNREFERENCED_PARAMETER(Object);
	UNREFERENCED_PARAMETER(Name);
#endif
}

//----------------------------------------------------------------------------

ID3D12Device* FD3D12Device::GetDevice()
{
	return GetParentAdapter()->GetD3DDevice();
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MAJOR_VERSION < 3)
// Source/Runtime/D3D12RHI/Private/D3D12UAV.cpp

template<typename ResourceType>
inline FD3D12UnorderedAccessView* CreateUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc, ResourceType* Resource)
{
	FD3D12Adapter* Adapter = Resource->GetParentDevice()->GetParentAdapter();

	return Adapter->CreateLinkedViews<ResourceType, FD3D12UnorderedAccessView>(Resource, [&Desc](ResourceType* Resource)
	{
		FD3D12Device* Device = Resource->GetParentDevice();
		FD3D12Resource* CounterResource = nullptr;

		return new FD3D12UnorderedAccessView(Device, Desc, Resource->ResourceLocation, CounterResource);
	});
}
#endif // (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MAJOR_VERSION < 3)

//----------------------------------------------------------------------------
//
//		D3D12
//
//----------------------------------------------------------------------------

FShaderResourceViewRHIRef	StreamBufferSRVToRHI(const PopcornFX::SBuffer_D3D12 *stream, u32 stride, u8 pixelFormat)
{
#if (ENGINE_MAJOR_VERSION == 5)
	FRHIBuffer			*buffer = StreamBufferResourceToRHI(stream, stride);
	FD3D12Buffer		*bufferD3D12 = FD3D12DynamicRHI::ResourceCast(buffer);
#else
	FRHIVertexBuffer	*buffer = StreamBufferResourceToRHI(stream, stride);
	FD3D12VertexBuffer	*bufferD3D12 = FD3D12DynamicRHI::ResourceCast(buffer);
#endif // (ENGINE_MAJOR_VERSION == 5)

	PK_ASSERT(bufferD3D12 != null);

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
	FRHICommandListBase &RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	if (bufferD3D12 && EnumHasAnyFlags(bufferD3D12->GetDesc().Usage, BUF_ByteAddressBuffer))
	{
		return RHICmdList.CreateShaderResourceView(bufferD3D12, FRHIViewDesc::CreateBufferSRV()
			.SetType(FRHIViewDesc::EBufferType::Raw)
		);
	}
	else
	{
		return RHICmdList.CreateShaderResourceView(bufferD3D12, FRHIViewDesc::CreateBufferSRV()
			.SetType(FRHIViewDesc::EBufferType::Typed)
			.SetFormat(EPixelFormat(pixelFormat))
		);
	}
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3)
	FRHICommandListBase			&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	return RHICmdList.CreateShaderResourceView(bufferD3D12, sizeof(uint32), pixelFormat);
#else
	return RHICreateShaderResourceView(bufferD3D12, sizeof(uint32), pixelFormat);
#endif
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5)
FRHIBuffer			*StreamBufferResourceToRHI(const PopcornFX::SBuffer_D3D12 *stream, u32 stride)
#else
FRHIVertexBuffer	*StreamBufferResourceToRHI(const PopcornFX::SBuffer_D3D12 *stream, u32 stride)
#endif // (ENGINE_MAJOR_VERSION == 5)
{
	D3D12_RESOURCE_DESC	desc = stream->m_Resource->GetDesc();
	PK_ASSERT(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
	PK_ASSERT(desc.Height == 1);
	PK_ASSERT(desc.DepthOrArraySize == 1);
	PK_ASSERT(desc.MipLevels == 1);
	PK_ASSERT(desc.Format == DXGI_FORMAT_UNKNOWN);
	PK_ASSERT(desc.SampleDesc.Count == 1);
	PK_ASSERT(desc.SampleDesc.Quality == 0);
	PK_ASSERT(desc.Layout == D3D12_TEXTURE_LAYOUT_ROW_MAJOR);
	PK_ASSERT(desc.Flags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	PK_ASSERT(stream->m_State == (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) || stream->m_State == (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_COPY_SOURCE)); // PK_BUFFER_GPU_D3D12_DEFAULT_STATE, currently not exposed

	FD3D12DynamicRHI	*dynamicRHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
	FD3D12Device		*device = dynamicRHI->GetAdapter().GetDevice(0);

	// Fixed #12899: removed BUF_UnorderedAccess | BUF_ByteAddressBuffer from the buffer usage, as it leads to crashes on some AMD GPU hardware.
	// The driver does not properly set the buffer stride as it considers it raw (although the buffer isn't bound as a raw buffer).
	// The BUF_UnorderedAccess could technically be left active, but none of the UE plugin shaders are binding any of the PK sim streams as UAV anyways.
	const EBufferUsageFlags			bufferUsage = BUF_ShaderResource;
	const D3D12_RESOURCE_STATES		resourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	FD3D12Resource			*resource = new FD3D12Resource(device, device->GetVisibilityMask(), stream->m_Resource, resourceState, ED3D12ResourceStateMode::Default, resourceState, desc, NULL, D3D12_HEAP_TYPE_DEFAULT);
	FD3D12Adapter			*adapter = device->GetParentAdapter();
#if (ENGINE_MAJOR_VERSION == 5)
	FD3D12Buffer			*buffer = adapter->CreateLinkedObject<FD3D12Buffer>(device->GetVisibilityMask(), [&](FD3D12Device* device, void* empty = nullptr)
		{
#if (ENGINE_MINOR_VERSION >= 6)
			FD3D12Buffer	*newBuffer = new FD3D12Buffer(device, FRHIBufferCreateDesc(TEXT("PopcornFXBuffer"), stream->m_ByteSize, stride, bufferUsage));
#elif (ENGINE_MINOR_VERSION >= 3)
			FD3D12Buffer	*newBuffer = new FD3D12Buffer(device, FRHIBufferDesc(stream->m_ByteSize, stride, bufferUsage));
#else
			FD3D12Buffer	*newBuffer = new FD3D12Buffer(device, stream->m_ByteSize, bufferUsage, stride);
#endif // (ENGINE_MINOR_VERSION >= 3)
			return newBuffer;
		});
#else
	FD3D12VertexBuffer		*buffer = adapter->CreateLinkedObject<FD3D12VertexBuffer>(device->GetVisibilityMask(), [&](FD3D12Device* device)
		{
			FD3D12VertexBuffer	*newBuffer = new FD3D12VertexBuffer(device, stride, stream->m_ByteSize, bufferUsage);
			return newBuffer;
		});
#endif // (ENGINE_MAJOR_VERSION == 5)

	resource->AddRef();
	buffer->ResourceLocation.AsFastAllocation(resource, stream->m_ByteSize, resource->GetGPUVirtualAddress(), NULL, 0 /* resourceOffsetBase */, stream->m_ByteOffset);

	// Fix crash in UE 5.4+ builds: Resource requires residency tracking, but StartTrackingForResidency() was not called.
#if !WITH_EDITOR && ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	buffer->GetResource()->StartTrackingForResidency();
#endif

	return buffer;
}

#endif // (PK_GPU_D3D12 == 1)
