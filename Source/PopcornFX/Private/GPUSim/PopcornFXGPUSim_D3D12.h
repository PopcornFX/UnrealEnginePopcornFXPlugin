//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXGPUSim.h"
#include "PopcornFXMinimal.h"

#if (PK_GPU_D3D12 == 1)

#include "D3D12RHI.h"

class	FD3D12VertexBuffer;

FWD_PK_API_BEGIN
struct SBuffer_D3D12;
class CParticleStreamToRender_D3D12;
FWD_PK_API_END

FShaderResourceViewRHIRef		StreamBufferSRVToRHI(const PopcornFX::SBuffer_D3D12 *stream, u32 stride, u8 pixelFormat = PF_R32_FLOAT);
FRHIBuffer						*StreamBufferResourceToRHI(const PopcornFX::SBuffer_D3D12 *stream, u32 stride);

//----------------------------------------------------------------------------
//
//	External (UE owned) resources residency
//
//----------------------------------------------------------------------------

#if (((ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8) || (ENGINE_MAJOR_VERSION == 6)) && ENABLE_RESIDENCY_MANAGEMENT)
#	define PK_D3D12_MANAGE_EXTERNAL_RESIDENCY	1
#else
#	define PK_D3D12_MANAGE_EXTERNAL_RESIDENCY	0
#endif

#if (PK_D3D12_MANAGE_EXTERNAL_RESIDENCY != 0)
void	PopcornFXD3D12_DeclareExternalResidency(FRHITexture *texture);
#endif // (PK_D3D12_MANAGE_EXTERNAL_RESIDENCY != 0)

#endif // (PK_GPU_D3D12 == 1)
