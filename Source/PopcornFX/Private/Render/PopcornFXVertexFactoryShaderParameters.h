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
#if (ENGINE_MINOR_VERSION >= 25)
	DECLARE_INLINE_TYPE_LAYOUT(FPopcornFXVertexFactoryShaderParametersVertex, NonVirtual);
#endif // (ENGINE_MINOR_VERSION >= 25)

#if (ENGINE_MINOR_VERSION < 25)
	virtual void	Bind(const FShaderParameterMap& ParameterMap) override;
	virtual void	Serialize(FArchive& Ar) override;
	virtual
#endif // (ENGINE_MINOR_VERSION < 25)
			void			GetElementShaderBindings(	const FSceneInterface *scene,
														const FSceneView *view,
														const FMeshMaterialShader *shader,
														const EVertexInputStreamType inputStreamType,
														ERHIFeatureLevel::Type featureLevel,
														const FVertexFactory *vertexFactory,
														const FMeshBatchElement &batchElement,
														class FMeshDrawSingleShaderBindings &shaderBindings,
														FVertexInputStreamArray &vertexStreams) const
#if (ENGINE_MINOR_VERSION >= 25)
		;
#else
		override;
#endif // (ENGINE_MINOR_VERSION < 25)
};

class	FPopcornFXVertexFactoryShaderParametersPixel : public FVertexFactoryShaderParameters
{
public:
#if (ENGINE_MINOR_VERSION >= 25)
	DECLARE_INLINE_TYPE_LAYOUT(FPopcornFXVertexFactoryShaderParametersPixel, NonVirtual);
#endif // (ENGINE_MINOR_VERSION >= 25)

#if (ENGINE_MINOR_VERSION >= 25)
#else
	virtual void	Bind(const FShaderParameterMap& ParameterMap) override;
	virtual void	Serialize(FArchive& Ar) override;
	virtual
#endif // (ENGINE_MINOR_VERSION >= 25)
	void					GetElementShaderBindings(	const FSceneInterface *scene,
														const FSceneView *view,
														const FMeshMaterialShader *shader,
														const EVertexInputStreamType inputStreamType,
														ERHIFeatureLevel::Type featureLevel,
														const FVertexFactory *vertexFactory,
														const FMeshBatchElement &batchElement,
														class FMeshDrawSingleShaderBindings &shaderBindings,
														FVertexInputStreamArray &vertexStreams) const
#if (ENGINE_MINOR_VERSION >= 25)
	;
#else
	override;
#endif // (ENGINE_MINOR_VERSION >= 25)
};
