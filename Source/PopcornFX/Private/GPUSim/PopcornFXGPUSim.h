//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#if (PK_HAS_GPU != 0)

#	include "RHI.h"
#	include "RHIResources.h"

#	include "PopcornFXSDK.h"

#	include "Render/RenderTypesPolicies.h"

	//----------------------------------------------------------------------------

	FWD_PK_API_BEGIN
	class	CParticleStreamToRender;
	class	CParticleStreamToRender_GPU;
	FWD_PK_API_END

	extern SUERenderContext::ERHIAPI			g_PopcornFXRHIAPI;

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

	void							SetupPopcornFXRHIAPI(uint32 API);

	// RHICreateShaderResourceView + handles Byte Access Buffers
	FShaderResourceViewRHIRef		My_RHICreateShaderResourceView(FVBRHIParamRef VertexBufferRHI, uint32 Stride, uint8 Format);

	// RHICreateUnorderedAccessView
	FUnorderedAccessViewRHIRef		My_RHICreateUnorderedAccessView(FVBRHIParamRef VertexBufferRHI, uint8 Format);

#if (ENGINE_MAJOR_VERSION == 4)
	// RHICreateUnorderedAccessView for index buffers
	FUnorderedAccessViewRHIRef		My_RHICreateUnorderedAccessView(FIBRHIParamRef IndexBufferRHI, uint8 Format);
#endif // (ENGINE_MAJOR_VERSION == 4)

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
