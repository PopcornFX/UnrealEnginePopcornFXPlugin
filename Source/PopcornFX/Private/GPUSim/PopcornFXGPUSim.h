//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "RHI.h"
#include "RHIResources.h"

#include "PopcornFXSDK.h"

#include "Render/RenderTypesPolicies.h"

	//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5)
	typedef FRHIBuffer*						FVBRHIParamRef;
	typedef FRHIBuffer*						FIBRHIParamRef;
#else
	typedef FRHIVertexBuffer*				FVBRHIParamRef;
	typedef FRHIIndexBuffer*				FIBRHIParamRef;
#endif
	typedef FRHIComputeShader*				FCSRHIParamRef;
	typedef FRHIShaderResourceView*			FSRVRHIParamRef;
	typedef FRHIUnorderedAccessView*		FUAVRHIParamRef;

#if (PK_HAS_GPU != 0)

	FWD_PK_API_BEGIN
	class	CParticleStreamToRender;
	class	CParticleStreamToRender_GPU;
	FWD_PK_API_END

	extern SUERenderContext::ERHIAPI			g_PopcornFXRHIAPI;

	void							SetupPopcornFXRHIAPI(uint32 API);

	template <typename _Type, uint32 _Stride>
	FShaderResourceViewRHIRef		StreamBufferSRVToRHI(const PopcornFX::CParticleStreamToRender &stream_GPU, PopcornFX::CGuid streamId, PopcornFX::CGuid &offset);

	bool							StreamBufferByteOffset(const PopcornFX::CParticleStreamToRender_GPU &stream_GPU, PopcornFX::CGuid streamId, u32 &streamOffset);

	//----------------------------------------------------------------------------

#	if (PK_GPU_D3D11 == 1)
	uint32	_PopcornFXD3DGetRefCount(IUnknown &res);
#	endif // ((PK_GPU_D3D11 == 1) || (PK_GPU_D3D12 == 1))

	//----------------------------------------------------------------------------

	bool	_IsGpuSupportedOnPlatform(const EShaderPlatform &platform);

#	if (PK_GPU_D3D11 == 1)
#		include "GPUSim/PopcornFXGPUSim_D3D11.h"
#	endif // (PK_GPU_D3D11 == 1)
#	if (PK_GPU_D3D12 == 1)
#		include "GPUSim/PopcornFXGPUSim_D3D12.h"
#	endif // (PK_GPU_D3D12 == 1)

#endif // (PK_HAS_GPU != 0)
