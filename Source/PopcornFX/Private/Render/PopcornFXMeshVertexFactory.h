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

class	FPopcornFXMeshVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FPopcornFXMeshVertexFactory);

public:
	struct FDataType : public FStaticMeshDataType
	{
		// Instanced particle streams:

		FPopcornFXVertexBufferView	m_InstancedMatrices;
		FPopcornFXVertexBufferView	m_InstancedColors;
		FPopcornFXVertexBufferView	m_InstancedVATCursors;
		FPopcornFXVertexBufferView	m_InstancedDynamicParameters0;
		FPopcornFXVertexBufferView	m_InstancedDynamicParameters1;
		FPopcornFXVertexBufferView	m_InstancedDynamicParameters2;
		//FPopcornFXVertexBufferView	m_InstancedDynamicParameters3;

		bool bInitialized;

		FDataType()
			: bInitialized(false)
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

protected:
	FDataType			Data;

private:
	void				_SetupInstanceStream(	u32								attributeIndex,
												u32								offset,
												FPopcornFXVertexBufferView		&buffer,
												EVertexElementType				type,
												FVertexDeclarationElementList	&decl);
};
