//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "RHIDefinitions.h"
#include "RenderResource.h"
#include "VertexFactory.h"

#include "PopcornFXSDK.h"

class	FPopcornFXVertexFactoryShaderParametersVertex : public FVertexFactoryShaderParameters
{
public:
	DECLARE_TYPE_LAYOUT(FPopcornFXVertexFactoryShaderParametersVertex, NonVirtual);

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
	DECLARE_TYPE_LAYOUT(FPopcornFXVertexFactoryShaderParametersPixel, NonVirtual);

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
