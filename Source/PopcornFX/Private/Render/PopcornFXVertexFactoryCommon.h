//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "RHIDefinitions.h"
#include "RenderResource.h"
#include "VertexFactory.h"

//----------------------------------------------------------------------------

// Vertex shader uniforms
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXUniforms, POPCORNFX_API)
	SHADER_PARAMETER_SRV(Buffer<uint>, InSimData)
	SHADER_PARAMETER(uint32, DynamicParameterMask)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FPopcornFXUniforms> FPopcornFXUniformsRef;

//----------------------------------------------------------------------------
