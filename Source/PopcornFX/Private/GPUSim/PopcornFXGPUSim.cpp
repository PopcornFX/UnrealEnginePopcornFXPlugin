//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXGPUSim.h"

#if (PK_HAS_GPU != 0)

// Make sure we don't break with UE4 updates
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 27)
	// Not implemented, make sure RHI feature sets didn't change, and update IFN
	PK_STATIC_ASSERT(false);
#endif

#include <pk_kernel/include/kr_thread_pool.h>
#include <pk_particles/include/ps_stream_to_render.h>

#if (PK_GPU_D3D11 == 1)
#	include <pk_particles/include/Storage/D3D11/storage_d3d11.h>
#elif (PK_GPU_D3D12 == 1)
#	include <pk_particles/include/Storage/D3D12/storage_d3d12.h>
#endif

//----------------------------------------------------------------------------
//
//		Common
//
//----------------------------------------------------------------------------

// Static, cannot change during runtime
SUERenderContext::ERHIAPI		g_PopcornFXRHIAPI = SUERenderContext::_Unsupported;

void	SetupPopcornFXRHIAPI(uint32 API)
{
	PK_ASSERT(API <= SUERenderContext::_Unsupported);
	g_PopcornFXRHIAPI = static_cast<SUERenderContext::ERHIAPI>(API);
}

//----------------------------------------------------------------------------

bool	_IsGpuSupportedOnPlatform(const EShaderPlatform &platform)
{
	const bool	platformSupportsSM5OrEquivalent = IsFeatureLevelSupported(platform, ERHIFeatureLevel::SM5);
	const bool	isStaticShaderPlatform = FStaticShaderPlatformNames::Get().IsStaticPlatform(platform);

	if (isStaticShaderPlatform)
	{
		const FName		platformName = FStaticShaderPlatformNames::Get().GetPlatformName(platform);

		// Special UE4
		return	(
#if (defined(PK_DURANGO) && defined(PK_UNKNOWN)) || (PK_COMPILE_GPU_XBOX_ONE != 0)
				 platformName == FName("UNKNOWN") ||
#endif // (defined(PK_DURANGO) && defined(PK_UNKNOWN)) || (PK_COMPILE_GPU_XBOX_ONE != 0)
#if defined(PK_SCARLETT) || (PK_COMPILE_GPU_UNKNOWN != 0)
				 platformName == FName("UNKNOWN_SM6") ||
#endif // defined(PK_SCARLETT) || (PK_COMPILE_GPU_UNKNOWN != 0)
#if defined(PK_UNKNOWN2) || (PK_COMPILE_GPU_UNKNOWN2 != 0)
				 platformName == FName("UNKNOWN2") ||
#endif // defined(PK_UNKNOWN2) || (PK_COMPILE_GPU_UNKNOWN2 != 0)
				 false) && platformSupportsSM5OrEquivalent;
	}
#if (ENGINE_MAJOR_VERSION == 5)
	const bool	platformSupportsSM6OrEquivalent = IsFeatureLevelSupported(platform, ERHIFeatureLevel::SM6);
	return	(platform == SP_PCD3D_SM5 && platformSupportsSM5OrEquivalent) ||
			(platform == SP_PCD3D_SM6 && platformSupportsSM6OrEquivalent);
#else
	return (
#	if 0
			// GPU sim not supported on Xbox one for now
			platform == SP_XBOXONE_D3D12 ||
#	endif
			platform == SP_PCD3D_SM5
			) &&
			platformSupportsSM5OrEquivalent;
#endif // (ENGINE_MAJOR_VERSION == 5)
}

//----------------------------------------------------------------------------

FShaderResourceViewRHIRef	My_RHICreateShaderResourceView(FVBRHIParamRef VertexBufferRHI, uint32 Stride, uint8 Format)
{
#if (PK_GPU_D3D11 == 1)
	if (g_PopcornFXRHIAPI == SUERenderContext::D3D11)
		return My_RHICreateShaderResourceView_D3D11(VertexBufferRHI, Stride, Format);
#endif // (PK_GPU_D3D11 == 1)
#if (PK_GPU_D3D12 == 1)
	if (g_PopcornFXRHIAPI == SUERenderContext::D3D12)
		return RHICreateShaderResourceView(VertexBufferRHI, Stride, Format);
#endif // (PK_GPU_D3D12 == 1)
	PK_ASSERT_NOT_REACHED();
	return null;
}

//----------------------------------------------------------------------------

FUnorderedAccessViewRHIRef	My_RHICreateUnorderedAccessView(FVBRHIParamRef BufferRHI, uint8 Format)
{
#if (PK_GPU_D3D11 == 1)
	if (g_PopcornFXRHIAPI == SUERenderContext::D3D11)
		return My_RHICreateUnorderedAccessView_D3D11(BufferRHI, Format);
#endif // (PK_GPU_D3D11 == 1)
#if (PK_GPU_D3D12 == 1)
	if (g_PopcornFXRHIAPI == SUERenderContext::D3D12)
		return My_RHICreateUnorderedAccessView_D3D12(BufferRHI, Format);
#endif // (PK_GPU_D3D12 == 1)
	PK_ASSERT_NOT_REACHED();
	return null;
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 4)
FUnorderedAccessViewRHIRef	My_RHICreateUnorderedAccessView(FIBRHIParamRef IndexBufferRHI, uint8 Format)
{
#if (PK_GPU_D3D11 == 1)
	if (g_PopcornFXRHIAPI == SUERenderContext::D3D11)
		return My_RHICreateUnorderedAccessView_D3D11(IndexBufferRHI, Format);
#endif // (PK_GPU_D3D11 == 1)
#if (PK_GPU_D3D12 == 1)
	if (g_PopcornFXRHIAPI == SUERenderContext::D3D12)
		return My_RHICreateUnorderedAccessView_D3D12(IndexBufferRHI, Format);
#endif // (PK_GPU_D3D12 == 1)
	PK_ASSERT_NOT_REACHED();
	return null;
}
#endif // (ENGINE_MAJOR_VERSION == 4)

//----------------------------------------------------------------------------

bool	StreamBufferByteOffset(const PopcornFX::CParticleStreamToRender_GPU &stream_GPU, PopcornFX::CGuid streamId, u32 &streamOffset)
{
	PK_ASSERT(streamId.Valid());

	const PopcornFX::CGuid	_streamOffset = stream_GPU.StreamOffset(streamId);
	streamOffset = _streamOffset;
	return _streamOffset.Valid();
}

//----------------------------------------------------------------------------

template <typename _Type, u32 _Stride>
FShaderResourceViewRHIRef	StreamBufferSRVToRHI(const PopcornFX::CParticleStreamToRender &stream_GPU, PopcornFX::CGuid streamId, PopcornFX::CGuid &streamOffset)
{
	PK_ASSERT(streamId.Valid());

#if (PK_GPU_D3D11 == 1)
	if (g_PopcornFXRHIAPI == SUERenderContext::D3D11)
	{
		const PopcornFX::CParticleStreamToRender_D3D11	&stream = static_cast<const PopcornFX::CParticleStreamToRender_D3D11&>(stream_GPU);

		streamOffset = stream.StreamOffset(streamId);
		return StreamBufferSRVToRHI(&stream.StreamBuffer(), stream.StreamSizeEst() * _Stride, _Stride/*, DXGI_FORMAT_UNKNOWN*/);
	}
#endif // (PK_GPU_D3D11 == 1)
#if (PK_GPU_D3D12 == 1)
	if (g_PopcornFXRHIAPI == SUERenderContext::D3D12)
	{
		const PopcornFX::CParticleStreamToRender_D3D12	&stream = static_cast<const PopcornFX::CParticleStreamToRender_D3D12&>(stream_GPU);

		streamOffset = stream.StreamOffset(streamId);
		return StreamBufferSRVToRHI(&stream.StreamBuffer(), stream.StreamSizeEst() * _Stride);
	}
#endif // (PK_GPU_D3D12 == 1)
	PK_ASSERT_NOT_REACHED();
	return null;
}

#define DEFINE_StreamBufferSRVToRHI(__type, __stride) \
	template FShaderResourceViewRHIRef		StreamBufferSRVToRHI<__type, __stride>(const PopcornFX::CParticleStreamToRender &stream_GPU, PopcornFX::CGuid streamId, PopcornFX::CGuid &streamOffset)

DEFINE_StreamBufferSRVToRHI(CFloat4, 16);
DEFINE_StreamBufferSRVToRHI(CFloat3, 12);
DEFINE_StreamBufferSRVToRHI(CFloat2, 8);
DEFINE_StreamBufferSRVToRHI(float, 4);
#undef DEFINE_StreamBufferSRVToRHI

//----------------------------------------------------------------------------
//
//		D3D
//
//----------------------------------------------------------------------------

#if (PK_GPU_D3D11 == 1 || PK_GPU_D3D12 == 1)

uint32	_PopcornFXD3DGetRefCount(IUnknown &res)
{
	uint32		count = res.AddRef();
	res.Release();
	return count - 1;
}

#endif // (PK_GPU_D3D11 == 1 || PK_GPU_D3D12 == 1)

#endif // (PK_HAS_GPU != 0)
