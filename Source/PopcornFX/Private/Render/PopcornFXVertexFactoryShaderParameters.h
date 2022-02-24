//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "RenderResource.h"
#include "VertexFactory.h"

#include "PopcornFXSDK.h"

class	FPopcornFXVertexFactoryShaderParametersVertex : public FVertexFactoryShaderParameters
{
public:
#if (ENGINE_MAJOR_VERSION == 5)
	DECLARE_TYPE_LAYOUT(FPopcornFXVertexFactoryShaderParametersVertex, NonVirtual);
#else
	DECLARE_INLINE_TYPE_LAYOUT(FPopcornFXVertexFactoryShaderParametersVertex, NonVirtual);
#endif // (ENGINE_MAJOR_VERSION == 5)

			void			GetElementShaderBindings(	const FSceneInterface *scene,
														const FSceneView *view,
														const FMeshMaterialShader *shader,
														const EVertexInputStreamType inputStreamType,
														ERHIFeatureLevel::Type featureLevel,
														const FVertexFactory *vertexFactory,
														const FMeshBatchElement &batchElement,
														class FMeshDrawSingleShaderBindings &shaderBindings,
														FVertexInputStreamArray &vertexStreams) const;
};

class	FPopcornFXVertexFactoryShaderParametersPixel : public FVertexFactoryShaderParameters
{
public:
#if (ENGINE_MAJOR_VERSION == 5)
	DECLARE_TYPE_LAYOUT(FPopcornFXVertexFactoryShaderParametersPixel, NonVirtual);
#else
	DECLARE_INLINE_TYPE_LAYOUT(FPopcornFXVertexFactoryShaderParametersPixel, NonVirtual);
#endif // (ENGINE_MAJOR_VERSION == 5)

	void					GetElementShaderBindings(	const FSceneInterface *scene,
														const FSceneView *view,
														const FMeshMaterialShader *shader,
														const EVertexInputStreamType inputStreamType,
														ERHIFeatureLevel::Type featureLevel,
														const FVertexFactory *vertexFactory,
														const FMeshBatchElement &batchElement,
														class FMeshDrawSingleShaderBindings &shaderBindings,
														FVertexInputStreamArray &vertexStreams) const;
};
