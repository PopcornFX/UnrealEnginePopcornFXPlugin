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

// FIX EDITOR BUILD
void UpdateBufferStats(TRefCountPtr<ID3D11Buffer> Buffer, bool bAllocating) { }
#endif // WITH_EDITOR

//----------------------------------------------------------------------------
#define	DEPTH_32_BIT_CONVERSION 0

// Engine\Source\Runtime\Windows\D3D11RHI\Private\D3D11RHIPrivate.h
/** Find an appropriate DXGI format for the input format and SRGB setting. */
inline static DXGI_FORMAT My_FindShaderResourceDXGIFormat(DXGI_FORMAT InFormat, bool bSRGB)
{
	if (bSRGB)
	{
		switch (InFormat)
		{
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:    return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case DXGI_FORMAT_BC1_TYPELESS:         return DXGI_FORMAT_BC1_UNORM_SRGB;
		case DXGI_FORMAT_BC2_TYPELESS:         return DXGI_FORMAT_BC2_UNORM_SRGB;
		case DXGI_FORMAT_BC3_TYPELESS:         return DXGI_FORMAT_BC3_UNORM_SRGB;
		case DXGI_FORMAT_BC7_TYPELESS:         return DXGI_FORMAT_BC7_UNORM_SRGB;
		};
	}
	else
	{
		switch (InFormat)
		{
		case DXGI_FORMAT_B8G8R8A8_TYPELESS: return DXGI_FORMAT_B8G8R8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_BC1_TYPELESS:      return DXGI_FORMAT_BC1_UNORM;
		case DXGI_FORMAT_BC2_TYPELESS:      return DXGI_FORMAT_BC2_UNORM;
		case DXGI_FORMAT_BC3_TYPELESS:      return DXGI_FORMAT_BC3_UNORM;
		case DXGI_FORMAT_BC7_TYPELESS:      return DXGI_FORMAT_BC7_UNORM;
		};
	}
	switch (InFormat)
	{
	case DXGI_FORMAT_R24G8_TYPELESS: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case DXGI_FORMAT_R32_TYPELESS: return DXGI_FORMAT_R32_FLOAT;
	case DXGI_FORMAT_R16_TYPELESS: return DXGI_FORMAT_R16_UNORM;
#if DEPTH_32_BIT_CONVERSION
		// Changing Depth Buffers to 32 bit on Dingo as D24S8 is actually implemented as a 32 bit buffer in the hardware
	case DXGI_FORMAT_R32G8X24_TYPELESS: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
#endif
	}
	return InFormat;
}

//----------------------------------------------------------------------------

// Source/Runtime/Windows/D3D11RHI/Private/D3D11UAV.cpp
//
// Creates a ***Raw Byte Address*** SRV
//
FShaderResourceViewRHIRef		My_RHICreateShaderResourceView_D3D11(FVBRHIParamRef /*Vertex*/BufferRHI, u32 Stride, u8 Format)
{
#if (ENGINE_MAJOR_VERSION == 5)
	FD3D11Buffer				*VertexBuffer = (FD3D11Buffer*)/*Vertex*/BufferRHI;
#else
	FD3D11VertexBuffer			*VertexBuffer = (FD3D11VertexBuffer*)/*Vertex*/BufferRHI;
#endif // (ENGINE_MAJOR_VERSION == 5)
	check(VertexBuffer);
	check(VertexBuffer->Resource);

	D3D11_BUFFER_DESC BufferDesc;
	VertexBuffer->Resource->GetDesc(&BufferDesc);

	const bool bByteAccessBuffer = (BufferDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS) != 0;

	// Create a Shader Resource View
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	PopcornFX::Mem::Clear(SRVDesc);

	if (bByteAccessBuffer)
	{
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		SRVDesc.BufferEx.NumElements = BufferDesc.ByteWidth / 4;
		SRVDesc.BufferEx.FirstElement = 0;
		SRVDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	}
	else
	{
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		SRVDesc.Buffer.FirstElement = 0;

		SRVDesc.Format = My_FindShaderResourceDXGIFormat((DXGI_FORMAT)GPixelFormats[Format].PlatformFormat, false);
		SRVDesc.Buffer.NumElements = BufferDesc.ByteWidth / Stride;
	}

	TRefCountPtr<ID3D11ShaderResourceView> ShaderResourceView;

	//
	ID3D11Device		*Direct3DDevice = (ID3D11Device*)RHIGetNativeDevice();
	//

	HRESULT hr = Direct3DDevice->CreateShaderResourceView(VertexBuffer->Resource, &SRVDesc, (ID3D11ShaderResourceView**)ShaderResourceView.GetInitReference());
	if (FAILED(hr))
	{
		if (hr == E_OUTOFMEMORY)
		{
			// There appears to be a driver bug that causes SRV creation to fail with an OOM error and then succeed on the next call.
			hr = Direct3DDevice->CreateShaderResourceView(VertexBuffer->Resource, &SRVDesc, (ID3D11ShaderResourceView**)ShaderResourceView.GetInitReference());
		}
		if (FAILED(hr))
		{
			//UE_LOG(LogD3D11RHI, Error, TEXT("Failed to create shader resource view for vertex buffer: ByteWidth=%d NumElements=%d"), BufferDesc.ByteWidth, BufferDesc.ByteWidth / Stride);
			//VerifyD3D11Result(hr, "Direct3DDevice->CreateShaderResourceView", __FILE__, __LINE__, Direct3DDevice);
			PK_ASSERT_NOT_REACHED();
		}
	}

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	return new FD3D11ShaderResourceView(ShaderResourceView, VertexBuffer, VertexBuffer);
#else
	return new FD3D11ShaderResourceView(ShaderResourceView, VertexBuffer);
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
}

//----------------------------------------------------------------------------

// Source/Runtime/Windows/D3D11RHI/Private/D3D11RHIPrivate.h
inline static DXGI_FORMAT My_FindUnorderedAccessDXGIFormat(DXGI_FORMAT InFormat)
{
	switch (InFormat)
	{
	case DXGI_FORMAT_B8G8R8A8_TYPELESS: return DXGI_FORMAT_B8G8R8A8_UNORM;
	case DXGI_FORMAT_R8G8B8A8_TYPELESS: return DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	return InFormat;
}

//----------------------------------------------------------------------------

FUnorderedAccessViewRHIRef		My_RHICreateUnorderedAccessView_D3D11(FVBRHIParamRef /*Vertex*/BufferRHI, u8 Format)
{
	return RHICreateUnorderedAccessView(BufferRHI, Format);
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 4)
// Source/Runtime/Windows/D3D11RHI/Private/D3D11UAV.cpp
//
// UAV from Index Buffer
//
FUnorderedAccessViewRHIRef		My_RHICreateUnorderedAccessView_D3D11(FIBRHIParamRef /*Index*/BufferRHI, u8 Format)
{
	PK_ASSERT(IsValidRef(/*Index*/BufferRHI));

#if (ENGINE_MAJOR_VERSION == 5)
	FD3D11Buffer				*IndexBuffer = (FD3D11Buffer*)/*Index*/BufferRHI;
#else
	FD3D11IndexBuffer			*IndexBuffer = (FD3D11IndexBuffer*)/*Index*/BufferRHI;
#endif // (ENGINE_MAJOR_VERSION == 5)

	D3D11_BUFFER_DESC BufferDesc;
	IndexBuffer->Resource->GetDesc(&BufferDesc);

	const bool bByteAccessBuffer = (BufferDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS) != 0;

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = My_FindUnorderedAccessDXGIFormat((DXGI_FORMAT)GPixelFormats[Format].PlatformFormat);
	UAVDesc.Buffer.FirstElement = 0;

	UAVDesc.Buffer.NumElements = BufferDesc.ByteWidth / GPixelFormats[Format].BlockBytes;
	UAVDesc.Buffer.Flags = 0;

	// It seems UE does not enable RAW_VIEWS on IndexBuffer (for now ?)
	// If this assert Pops up, Great ! just change the shader:
	// from "RWBuffer<uint> OutIndices;"
	// to "RWByteAddressBuffer OutIndices;" with the code that depends on that
	PK_ASSERT(!bByteAccessBuffer);
	if (bByteAccessBuffer)
	{
		UAVDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_RAW;
		UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	}

	//
	ID3D11Device		*Direct3DDevice = (ID3D11Device*)RHIGetNativeDevice();
	//

	TRefCountPtr<ID3D11UnorderedAccessView> UnorderedAccessView;
	//VERIFYD3D11RESULT(Direct3DDevice->CreateUnorderedAccessView(IndexBuffer->Resource, &UAVDesc, (ID3D11UnorderedAccessView**)UnorderedAccessView.GetInitReference()));
	HRESULT hr = Direct3DDevice->CreateUnorderedAccessView(IndexBuffer->Resource, &UAVDesc, (ID3D11UnorderedAccessView**)UnorderedAccessView.GetInitReference());

	PK_ASSERT(!FAILED(hr));

	return new FD3D11UnorderedAccessView(UnorderedAccessView, IndexBuffer);
}
#endif // (ENGINE_MAJOR_VERSION == 4)

//----------------------------------------------------------------------------
//
//		D3D11
//
//----------------------------------------------------------------------------

FShaderResourceViewRHIRef	StreamBufferSRVToRHI(const PopcornFX::SParticleStreamBuffer_D3D11 *stream, u32 bytes, u32 stride, u8 pixelFormat)
{
#if (ENGINE_MAJOR_VERSION == 5)
	FD3D11Buffer				*buffer = static_cast<FD3D11Buffer*>(StreamBufferResourceToRHI(stream, bytes, stride));
#else
	FD3D11VertexBuffer			*buffer = static_cast<FD3D11VertexBuffer*>(StreamBufferResourceToRHI(stream, bytes, stride));
#endif // (ENGINE_MAJOR_VERSION == 5)

	if (pixelFormat == PF_Unknown)
	{
		// Create RHI SRV wrapper from SRV already available
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		FD3D11ShaderResourceView* srv = new FD3D11ShaderResourceView(stream->m_BufferSRV, buffer, buffer);
#else
		FD3D11ShaderResourceView* srv = new FD3D11ShaderResourceView(stream->m_BufferSRV, buffer);
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		PK_ASSERT(_PopcornFXD3DGetRefCount(*(srv->View)) > 1); // Referenced here and by PopcornFX
		return srv;
	}
	else
	{
		// Create a new SRV from resource
		return RHICreateShaderResourceView(buffer, stride, pixelFormat);
	}
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5)
FRHIBuffer						*StreamBufferResourceToRHI(const PopcornFX::SParticleStreamBuffer_D3D11 *stream, u32 bytes, u32 stride)
#else
FRHIVertexBuffer				*StreamBufferResourceToRHI(const PopcornFX::SParticleStreamBuffer_D3D11 *stream, u32 bytes, u32 stride)
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

	const EBufferUsageFlags		bufferUsage = BUF_UnorderedAccess | BUF_ByteAddressBuffer | BUF_ShaderResource;
#if (ENGINE_MAJOR_VERSION == 5)
	FD3D11Buffer				*buffer = new FD3D11Buffer(stream->m_Buffer, bytes, bufferUsage, stride);
#else
	FD3D11VertexBuffer			*buffer = new FD3D11VertexBuffer(stream->m_Buffer, bytes, bufferUsage);
#endif // (ENGINE_MAJOR_VERSION == 5)
	PK_ASSERT(_PopcornFXD3DGetRefCount(*(buffer->Resource)) > 1); // Referenced here and by PopcornFX

	return buffer;
}

#endif // (PK_GPU_D3D11 == 1)
