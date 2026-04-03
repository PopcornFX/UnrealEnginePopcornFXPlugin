//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "Runtime/Launch/Resources/Version.h"

#include "PopcornFXMinimal.h"
#include "PopcornFXBuffer.h"

#include "RenderResource.h"
#include "VertexFactory.h"
#include "Components.h"
#include "Serialization/MemoryLayout.h"

#include "PopcornFXHelper.h"

class UMaterialInterface;

//----------------------------------------------------------------------------

// Vertex shader uniforms
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXGPUMeshVSUniforms, POPCORNFX_API)
	SHADER_PARAMETER(uint32,						DrawRequest)
	SHADER_PARAMETER(uint32,						IndirectionOffsetsIndex)
	SHADER_PARAMETER_SRV(StructuredBuffer<uint>,	Indirection)
	SHADER_PARAMETER_SRV(StructuredBuffer<uint>,	IndirectionOffsets)
	SHADER_PARAMETER_SRV(StructuredBuffer<float4>,	Matrices)
	SHADER_PARAMETER_SRV(StructuredBuffer<uint>,	MatricesOffsets)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FPopcornFXGPUMeshVSUniforms> FPopcornFXGPUMeshVSUniformsRef;

//----------------------------------------------------------------------------

class FPopcornFXGPUMeshVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	DECLARE_TYPE_LAYOUT(FPopcornFXGPUMeshVertexFactoryShaderParameters, NonVirtual);

public:
	void	Bind(const FShaderParameterMap &ParameterMap);
	void	GetElementShaderBindings(	const FSceneInterface *scene,
										const FSceneView *view,
										const FMeshMaterialShader *shader,
										const EVertexInputStreamType inputStreamType,
										ERHIFeatureLevel::Type featureLevel,
										const FVertexFactory* vertexFactory,
										const FMeshBatchElement &batchElement,
										class FMeshDrawSingleShaderBindings &shaderBindings,
										FVertexInputStreamArray &vertexStreams) const;

	LAYOUT_FIELD(FShaderParameter, MeshBatchElementId);
};

//----------------------------------------------------------------------------

class	FPopcornFXGPUMeshVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FPopcornFXGPUMeshVertexFactory);

public:
	struct FDataType : public FStaticMeshDataType
	{
		bool	bInitialized;

		FDataType()
			:	bInitialized(false)
		{
		}
	};

	FPopcornFXGPUMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
		: FVertexFactory(InFeatureLevel)
	{
	}
	
	static bool			PlatformIsSupported(EShaderPlatform platform);
	static bool			ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static bool			IsCompatible(UMaterialInterface *material);

	virtual void		InitRHI(FRHICommandListBase &RHICmdList) override;
	void				SetData(const FDataType& InData);

	static void			ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	static bool			SupportsTessellationShaders() { return true; }

	FRHIUniformBuffer	*GetVSUniformBuffer() const { return m_VSUniformBuffer; }
	FRHIUniformBuffer	*GetMeshVSUniformBuffer() const { return m_MeshVSUniformBuffer; }
	FRHIUniformBuffer	*GetGPUMeshVSUniformBuffer() const { return m_GPUMeshVSUniformBuffer; }

protected:
	FDataType	Data;

public:
	FUniformBufferRHIRef	m_VSUniformBuffer;
	FUniformBufferRHIRef	m_MeshVSUniformBuffer;
	FUniformBufferRHIRef	m_GPUMeshVSUniformBuffer;
};
