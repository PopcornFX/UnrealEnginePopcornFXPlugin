//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "Runtime/Launch/Resources/Version.h"

#include "PopcornFXMinimal.h"
#include "PopcornFXBuffer.h"

#include "RenderResource.h"
#include "VertexFactory.h"

class UMaterialInterface;

//----------------------------------------------------------------------------

// Vertex shader uniforms
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXGPUBillboardVSUniforms, POPCORNFX_API)
	SHADER_PARAMETER(int32, CapsulesDC)
	SHADER_PARAMETER(int32, HasSecondUVSet)
	SHADER_PARAMETER(int32, InPositionsOffset)
	SHADER_PARAMETER(int32, InSizesOffset)
	SHADER_PARAMETER(int32, InSize2sOffset)
	SHADER_PARAMETER(int32, InRotationsOffset)
	SHADER_PARAMETER(int32, InAxis0sOffset)
	SHADER_PARAMETER(int32, InAxis1sOffset)
	SHADER_PARAMETER(int32, InTextureIDsOffset)
	SHADER_PARAMETER(int32, InColorsOffset)
	SHADER_PARAMETER(int32, InEmissiveColorsOffset)
	SHADER_PARAMETER(int32, InAlphaCursorsOffset)
	SHADER_PARAMETER(int32, InDynamicParameter1sOffset)
	SHADER_PARAMETER(int32, InDynamicParameter2sOffset)
	SHADER_PARAMETER(int32, InDynamicParameter3sOffset)
	SHADER_PARAMETER(int32, HasSortedIndices)
	SHADER_PARAMETER(int32, DrawRequestID)
	SHADER_PARAMETER(uint32, InIndicesOffset) // >= 0
	SHADER_PARAMETER(uint32, AtlasRectCount)
	SHADER_PARAMETER(FVector4, DrawRequest) // Unbatched, GPU
	SHADER_PARAMETER_SRV(Buffer<uint>, InSimData)
	SHADER_PARAMETER_SRV(Buffer<uint>, InSortedIndices)
	SHADER_PARAMETER_SRV(Buffer<float4>, AtlasBuffer)
	SHADER_PARAMETER_SRV(Buffer<float4>, DrawRequests) // Batched, CPU
END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FPopcornFXGPUBillboardVSUniforms> FPopcornFXGPUBillboardVSUniformsRef;

//----------------------------------------------------------------------------

class	FPopcornFXGPUVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FPopcornFXGPUVertexFactory);

public:
	FPopcornFXGPUVertexFactory(ERHIFeatureLevel::Type InFeatureLevel);
	~FPopcornFXGPUVertexFactory();

	FPopcornFXVertexBufferView		m_InSimData;

	static bool			PlatformIsSupported(EShaderPlatform platform);

#if (ENGINE_MINOR_VERSION >= 25)
	static bool			ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
#else
	static bool			ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);
#endif // (ENGINE_MINOR_VERSION >= 25)

#if (ENGINE_MINOR_VERSION >= 25)
	static void			ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
#else
	static void			ModifyCompilationEnvironment(const FVertexFactoryType* Type, EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment);
#endif // (ENGINE_MINOR_VERSION >= 25)

	static bool			IsCompatible(UMaterialInterface *material);

	/** FRenderResource interface */
	virtual void							InitRHI() override;

	/** Does the vertex factory supports tesselation shaders */
	static bool								SupportsTessellationShaders() { return false; }

	/** Volumetric fog */
	virtual bool							RendersPrimitivesAsCameraFacingSprites() const override { return true; }

#if (ENGINE_MINOR_VERSION < 25)
	/** Construct the corresponding shader parameters */
	static FVertexFactoryShaderParameters	*ConstructShaderParameters(EShaderFrequency shaderFrequency);
#endif // (ENGINE_MINOR_VERSION < 25)

	FRHIUniformBuffer						*GetVSUniformBuffer() { return m_VSUniformBuffer; }

public:
	FUniformBufferRHIRef		m_VSUniformBuffer;
};
