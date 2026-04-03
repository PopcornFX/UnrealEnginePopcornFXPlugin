//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXGPUSim.h"
#include "PopcornFXMinimal.h"

#if (PK_GPU_D3D12 == 1)

class	FD3D12VertexBuffer;

FWD_PK_API_BEGIN
struct SBuffer_D3D12;
class CParticleStreamToRender_D3D12;
FWD_PK_API_END

FShaderResourceViewRHIRef		StreamBufferSRVToRHI(const PopcornFX::SBuffer_D3D12 *stream, u32 stride, u8 pixelFormat = PF_R32_FLOAT);
FRHIBuffer						*StreamBufferResourceToRHI(const PopcornFX::SBuffer_D3D12 *stream, u32 stride);

#endif // (PK_GPU_D3D12 == 1)
