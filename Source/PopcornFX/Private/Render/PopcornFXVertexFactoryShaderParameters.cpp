//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXVertexFactoryShaderParameters.h"
#include "PopcornFXVertexFactory.h"

#include "PopcornFXCustomVersion.h"

#include "ShaderParameterUtils.h"
#include "MeshBatch.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"

#include "PopcornFXSDK.h"

//----------------------------------------------------------------------------
//
//	Vertex
//
//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION < 25)
void	FPopcornFXVertexFactoryShaderParametersVertex::Bind(const FShaderParameterMap& ParameterMap)
{
}

void	FPopcornFXVertexFactoryShaderParametersVertex::Serialize(FArchive& Ar)
{
}
#endif	// (ENGINE_MINOR_VERSION < 25)

void	FPopcornFXVertexFactoryShaderParametersVertex::GetElementShaderBindings(const FSceneInterface *scene,
																				const FSceneView *view,
																				const FMeshMaterialShader *shader,
																				const EVertexInputStreamType inputStreamType,
																				ERHIFeatureLevel::Type featureLevel,
																				const FVertexFactory *vertexFactory,
																				const FMeshBatchElement &batchElement,
																				class FMeshDrawSingleShaderBindings &shaderBindings,
																				FVertexInputStreamArray &vertexStreams) const
{
	FPopcornFXVertexFactory	*_vertexFactory = (FPopcornFXVertexFactory*)vertexFactory;
	shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXBillboardVSUniforms>(), _vertexFactory->GetVSUniformBuffer());
	shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXBillboardCommonUniforms>(), _vertexFactory->GetCommonUniformBuffer());
}

//----------------------------------------------------------------------------
//
//	Pixel
//
//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION < 25)
void	FPopcornFXVertexFactoryShaderParametersPixel::Bind(const FShaderParameterMap& ParameterMap)
{
}

void	FPopcornFXVertexFactoryShaderParametersPixel::Serialize(FArchive& Ar)
{
}
#endif	// (ENGINE_MINOR_VERSION < 25)

void	FPopcornFXVertexFactoryShaderParametersPixel::GetElementShaderBindings(const FSceneInterface *scene,
																				const FSceneView *view,
																				const FMeshMaterialShader *shader,
																				const EVertexInputStreamType inputStreamType,
																				ERHIFeatureLevel::Type featureLevel,
																				const FVertexFactory *vertexFactory,
																				const FMeshBatchElement &batchElement,
																				class FMeshDrawSingleShaderBindings &shaderBindings,
																				FVertexInputStreamArray &vertexStreams) const
{
	FPopcornFXVertexFactory	*_vertexFactory = (FPopcornFXVertexFactory*)vertexFactory;
	shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXBillboardCommonUniforms>(), _vertexFactory->GetCommonUniformBuffer());
}

//----------------------------------------------------------------------------
