//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXGPUSim.h"
#include "PopcornFXMinimal.h"

#if (PK_GPU_D3D11 == 1)

class	FD3D11VertexBuffer;

FWD_PK_API_BEGIN
struct SBuffer_D3D11;
class CParticleStreamToRender_D3D11;
FWD_PK_API_END

FShaderResourceViewRHIRef		StreamBufferSRVToRHI(const PopcornFX::SBuffer_D3D11 *stream, u32 stride, u8 pixelFormat = PF_R32_FLOAT);
#if (ENGINE_MAJOR_VERSION == 5)
FRHIBuffer						*StreamBufferResourceToRHI(const PopcornFX::SBuffer_D3D11 *stream, u32 stride);
#else
FRHIVertexBuffer				*StreamBufferResourceToRHI(const PopcornFX::SBuffer_D3D11 *stream, u32 stride);
#endif // (ENGINE_MAJOR_VERSION == 5)

//----------------------------------------------------------------------------

#endif // (PK_GPU_D3D11 == 1)
