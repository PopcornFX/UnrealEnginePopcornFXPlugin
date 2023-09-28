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

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 2)
#	include "DataDrivenShaderPlatformInfo.h"
#	if (ENGINE_MINOR_VERSION >= 3)
#		include "RHIStaticShaderPlatformNames.h"
#	endif
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

bool	StreamBufferByteOffset(const PopcornFX::CParticleStreamToRender_GPU &stream_GPU, PopcornFX::CGuid streamId, u32 &streamOffset)
{
	PK_ASSERT(streamId.Valid());

	const PopcornFX::CGuid	_streamOffset = stream_GPU.StreamOffset(streamId);
	streamOffset = _streamOffset;
	return _streamOffset.Valid();
}

#endif // (PK_HAS_GPU != 0)
