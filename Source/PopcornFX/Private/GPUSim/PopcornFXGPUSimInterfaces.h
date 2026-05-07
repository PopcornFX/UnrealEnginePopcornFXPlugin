//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXSDK.h"
#include <pk_compiler/include/cp_backend.h>

#if (PK_GPU_D3D12 == 1) || (PK_GPU_D3D12 == 1)
#	define PK_GPU_D3D 1
#endif // (PK_GPU_D3D12 == 1) || (PK_GPU_D3D12 == 1)

#ifndef PK_GPU_D3D
#	define PK_GPU_D3D 0
#endif

#if (PK_HAS_GPU != 0)

namespace	PopcornFXGPU
{
#if (PK_GPU_D3D != 0)
	namespace	D3D
	{
#if WITH_EDITOR
		bool	EmitBuiltin_IntersectScene(PopcornFX::CLinkerGPU::SExternalFunctionEmitArgs &args);
#endif // WITH_EDITOR

		bool	Bind_IntersectScene_SceneDepth(PopcornFX::CStringId mangledName, const PopcornFX::SLinkGPUContext &context);
		bool	Bind_IntersectScene_SceneNormal(PopcornFX::CStringId mangledName, const PopcornFX::SLinkGPUContext &context);
		bool	Bind_IntersectScene_ViewUniformBuffer(PopcornFX::CStringId mangledName, const PopcornFX::SLinkGPUContext &context);
	}; // namespace D3D
#endif // (PK_GPU_D3D != 0)
} // namespace PopcornFXGPU

#endif // (PK_HAS_GPU != 0)
