//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXVertexFactoryShaderParameters.h"
#include "PopcornFXVertexFactory.h"
#include "PopcornFXVertexFactoryCommon.h"

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
	shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXUniforms>(), _vertexFactory->GetVSUniformBuffer());
	shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXBillboardVSUniforms>(), _vertexFactory->GetBillboardVSUniformBuffer());
	shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXBillboardCommonUniforms>(), _vertexFactory->GetBillboardCommonUniformBuffer());
}

//----------------------------------------------------------------------------
//
//	Pixel
//
//----------------------------------------------------------------------------

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
	shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXUniforms>(), _vertexFactory->GetVSUniformBuffer());
	shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXBillboardCommonUniforms>(), _vertexFactory->GetBillboardCommonUniformBuffer());
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5)
	IMPLEMENT_TYPE_LAYOUT(FPopcornFXVertexFactoryShaderParametersVertex);
	IMPLEMENT_TYPE_LAYOUT(FPopcornFXVertexFactoryShaderParametersPixel);
#endif // (ENGINE_MAJOR_VERSION == 5)

//----------------------------------------------------------------------------

