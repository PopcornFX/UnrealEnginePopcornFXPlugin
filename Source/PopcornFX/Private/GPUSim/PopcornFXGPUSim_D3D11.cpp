//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXGPUSim_D3D11.h"

// Most of this file contains editor only hacks because UE4 doesn't export some "low level" RHI symbols
// So our only option is to copy paste implementations
// Right now we create native API resources on PKFX side for GPU simulation, and have to convert those native resources
// In RHI resources, but we don't let RHI create the resources.
// So all those low level function implementations are not exposed by UE4 as it's not a use case for them

#if (PK_GPU_D3D11 == 1)

#include "ShaderCore.h"
#include "DynamicRHI.h"

#if (PK_GPU_D3D11 == 1)
#	include <d3d11.h>
#	include "D3D11RHI.h"
#endif // (PK_GPU_D3D11 == 1)

#include "RenderUtils.h"

#include <pk_particles/include/ps_mediums.h>
#include <pk_particles/include/ps_stream_to_render.h>
#include <pk_kernel/include/kr_thread_pool.h>
#include <pk_particles/include/Storage/D3D11/storage_d3d11.h>

//----------------------------------------------------------------------------
//
//		D3D11
//
//----------------------------------------------------------------------------

DECLARE_LOG_CATEGORY_EXTERN(LogD3D11RHI, Log, All);
#	include "D3D11RHI.h"
#	include "D3D11State.h"
#	include "D3D11Resources.h"

//----------------------------------------------------------------------------
//
//		D3D11 engine source code copy (D3D11RHI)
//
//----------------------------------------------------------------------------

#if WITH_EDITOR

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
FD3D11Buffer::~FD3D11Buffer()
{
}
#else
void	UpdateBufferStats(TRefCountPtr<ID3D11Buffer> Buffer, bool bAllocating) { }
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)

#endif // WITH_EDITOR

//----------------------------------------------------------------------------
//
//		D3D11
//
//----------------------------------------------------------------------------

FShaderResourceViewRHIRef	StreamBufferSRVToRHI(const PopcornFX::SParticleStreamBuffer_D3D11 *stream, u32 stride, u8 pixelFormat)
{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3)
	FRHICommandListBase			&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
#endif

#if (ENGINE_MAJOR_VERSION == 5)
	FD3D11Buffer				*buffer = static_cast<FD3D11Buffer*>(StreamBufferResourceToRHI(stream, stride));
#else
	FD3D11VertexBuffer			*buffer = static_cast<FD3D11VertexBuffer*>(StreamBufferResourceToRHI(stream, stride));
#endif // (ENGINE_MAJOR_VERSION == 5)

	check(pixelFormat != PF_Unknown);
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	return RHICmdList.CreateShaderResourceView(buffer, FRHIViewDesc::CreateBufferSRV()
		.SetTypeFromBuffer(buffer)
		.SetFormat((EPixelFormat)pixelFormat));
#else
	// Create a new SRV from resource
	return RHICreateShaderResourceView(buffer, stride, pixelFormat);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5)
FRHIBuffer						*StreamBufferResourceToRHI(const PopcornFX::SParticleStreamBuffer_D3D11 *stream, u32 stride)
#else
FRHIVertexBuffer				*StreamBufferResourceToRHI(const PopcornFX::SParticleStreamBuffer_D3D11 *stream, u32 stride)
#endif // (ENGINE_MAJOR_VERSION == 5)
{
	D3D11_BUFFER_DESC	desc;
	stream->m_Buffer->GetDesc(&desc);
	PK_ASSERT((desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) == 0); // no structured
	PK_ASSERT((desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) != 0); // is BUF_UnorderedAccess
	PK_ASSERT((desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS) != 0); // is BUF_ByteAddressBuffer
	PK_ASSERT((desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0); // is BUF_ShaderResource
	PK_ASSERT((desc.Usage & D3D11_USAGE_DYNAMIC) == 0); // no BUF_AnyDynamic
	PK_ASSERT((desc.CPUAccessFlags) == 0); // no BUF_AnyDynamic

	// TODO: Unify SParticleStreamBuffer_D3D11 with other APIs, just provide a m_ByteSize member..
	const u32	sizeInBytes = desc.ByteWidth;

	// Fixed #12899: removed BUF_UnorderedAccess | BUF_ByteAddressBuffer from the buffer usage, as it leads to crashes on some AMD GPU hardware.
	// The driver does not properly set the buffer stride as it considers it raw (although the buffer isn't bound as a raw buffer).
	// The BUF_UnorderedAccess could technically be left active, but none of the UE plugin shaders are binding any of the PK sim streams as UAV anyways.
	const EBufferUsageFlags		bufferUsage = BUF_ShaderResource;
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	FD3D11Buffer				*buffer = new FD3D11Buffer(stream->m_Buffer, FRHIBufferDesc(sizeInBytes, stride, bufferUsage));
#elif (ENGINE_MAJOR_VERSION == 5)
	FD3D11Buffer				*buffer = new FD3D11Buffer(stream->m_Buffer, sizeInBytes, bufferUsage, stride);
#else
	FD3D11VertexBuffer			*buffer = new FD3D11VertexBuffer(stream->m_Buffer, sizeInBytes, bufferUsage);
#endif // (ENGINE_MAJOR_VERSION == 5)

	return buffer;
}

#endif // (PK_GPU_D3D11 == 1)
