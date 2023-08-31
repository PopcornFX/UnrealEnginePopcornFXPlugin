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

#include "PopcornFXHelper.h"

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
	SHADER_PARAMETER(FVector4f, DrawRequest) // Unbatched, GPU
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

	static bool			ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);

	static void			ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	static bool			IsCompatible(UMaterialInterface *material);

	/** FRenderResource interface */
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	virtual void							InitRHI(FRHICommandListBase &RHICmdList) override;
#else
	virtual void							InitRHI() override;
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)

	/** Does the vertex factory supports tesselation shaders */
	static bool								SupportsTessellationShaders() { return false; }

	/** Volumetric fog */
	virtual bool							RendersPrimitivesAsCameraFacingSprites() const override { return true; }

	FRHIUniformBuffer						*GetVSUniformBuffer() { return m_VSUniformBuffer; }
	FRHIUniformBuffer						*GetGPUBillboardVSUniformBuffer() { return m_GPUBillboardVSUniformBuffer; }

public:
	FUniformBufferRHIRef		m_VSUniformBuffer;
	FUniformBufferRHIRef		m_GPUBillboardVSUniformBuffer;
};
