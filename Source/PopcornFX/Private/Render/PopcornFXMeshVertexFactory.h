//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXVertexFactory.h"

#include "RenderResource.h"
#include "Components.h"

#include "PopcornFXSDK.h"

//----------------------------------------------------------------------------

// Vertex shader uniforms
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXMeshVSUniforms, POPCORNFX_API)
	SHADER_PARAMETER(int32, InColorsOffset)
	SHADER_PARAMETER(int32, InEmissiveColorsOffset)
	SHADER_PARAMETER(int32, InAlphaCursorsOffset)
	SHADER_PARAMETER(int32, InTextureIDsOffset)
	SHADER_PARAMETER(int32, InVATCursorsOffset)
	SHADER_PARAMETER(int32, InDynamicParameter0sOffset)
	SHADER_PARAMETER(int32, InDynamicParameter1sOffset)
	SHADER_PARAMETER(int32, InDynamicParameter2sOffset)
	SHADER_PARAMETER(int32, InDynamicParameter3sOffset)
	SHADER_PARAMETER(uint32, AtlasRectCount)
	SHADER_PARAMETER_SRV(Buffer<float4>, AtlasBuffer)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FPopcornFXMeshVSUniforms> FPopcornFXMeshVSUniformsRef;

//----------------------------------------------------------------------------

class	FPopcornFXMeshVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FPopcornFXMeshVertexFactory);

public:
	struct FDataType : public FStaticMeshDataType
	{
		// Instanced particle streams:
		FPopcornFXVertexBufferView	m_InstancedMatrices;

		bool						bInitialized;

		FDataType()
		:	bInitialized(false)
		{

		}
	};

	FPopcornFXMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
	: FVertexFactory(InFeatureLevel)
	{
	}

	static bool			ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static bool			IsCompatible(UMaterialInterface *material);

	virtual void		InitRHI() override;
	void				SetData(const FDataType& InData);

	static void			ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	static bool			SupportsTessellationShaders() { return true; }

	FRHIUniformBuffer	*GetVSUniformBuffer() { return m_VSUniformBuffer; }
	FRHIUniformBuffer	*GetMeshVSUniformBuffer() { return m_MeshVSUniformBuffer; }

protected:
	FDataType			Data;

public:
	FUniformBufferRHIRef	m_VSUniformBuffer;
	FUniformBufferRHIRef	m_MeshVSUniformBuffer;
};
