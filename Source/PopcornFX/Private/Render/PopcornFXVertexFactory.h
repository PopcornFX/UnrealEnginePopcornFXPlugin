//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXBuffer.h"
#include "GPUSim/PopcornFXBillboarderCS.h"

#include "RenderResource.h"
#include "VertexFactory.h"

#include "PopcornFXSDK.h"
#include <pk_defs.h>
#include <pk_kernel/include/kr_memoryviews.h>

class UMaterialInterface;

//----------------------------------------------------------------------------

// Vertex shader uniforms
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXBillboardVSUniforms, POPCORNFX_API)
	SHADER_PARAMETER(uint32, RendererType)
	SHADER_PARAMETER(uint32, CapsulesOffset)
	SHADER_PARAMETER(uint32, TotalParticleCount)
	//SHADER_PARAMETER(int, InTextureIDsOffset)
	SHADER_PARAMETER(int32, InColorsOffset)
	SHADER_PARAMETER(int32, InEmissiveColorsOffset)
	SHADER_PARAMETER(int32, InAlphaCursorsOffset)
	SHADER_PARAMETER(int32, InDynamicParameter1sOffset)
	SHADER_PARAMETER(int32, InDynamicParameter2sOffset)
	SHADER_PARAMETER(int32, InDynamicParameter3sOffset)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FPopcornFXBillboardVSUniforms>	FPopcornFXBillboardVSUniformsRef;

//----------------------------------------------------------------------------

// Shader uniforms for several stages
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXBillboardCommonUniforms, POPCORNFX_API)
	SHADER_PARAMETER(int32, HasSecondUVSet)
	SHADER_PARAMETER(int32, FlipUVs) // Are the UVs flipped (u <-> v)
	SHADER_PARAMETER(int32, NeedsBTN) // Do we need the BTN matrix
	SHADER_PARAMETER(int32, CorrectRibbonDeformation)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FPopcornFXBillboardCommonUniforms>	FPopcornFXBillboardCommonUniformsRef;

//----------------------------------------------------------------------------

class	FPopcornFXVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FPopcornFXVertexFactory);

public:
	FPopcornFXVertexFactory(ERHIFeatureLevel::Type InFeatureLevel);

	~FPopcornFXVertexFactory();

	FPopcornFXVertexBufferView		m_Positions;
	FPopcornFXVertexBufferView		m_Normals;
	FPopcornFXVertexBufferView		m_Tangents;
	FPopcornFXVertexBufferView		m_Texcoords;
	FPopcornFXVertexBufferView		m_Texcoord2s;
	FPopcornFXVertexBufferView		m_UVFactors;
	FPopcornFXVertexBufferView		m_UVScalesAndOffsets;
	FPopcornFXVertexBufferView		m_AtlasIDs; // This should be an additional input

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

	FRHIUniformBuffer						*GetVSUniformBuffer() { return m_VSUniformBuffer; }
	FRHIUniformBuffer						*GetBillboardVSUniformBuffer() { return m_BillboardVSUniformBuffer; }
	FRHIUniformBuffer						*GetBillboardCommonUniformBuffer() { return m_BillboardCommonUniformBuffer; }

public:
	FUniformBufferRHIRef					m_VSUniformBuffer;
	FUniformBufferRHIRef					m_BillboardVSUniformBuffer;
	FUniformBufferRHIRef					m_BillboardCommonUniformBuffer;

private:
	void			_SetupStream(	u32								attributeIndex,
									FPopcornFXVertexBufferView		&buffer,
									EVertexElementType				type,
									FVertexDeclarationElementList	&decl);
};
