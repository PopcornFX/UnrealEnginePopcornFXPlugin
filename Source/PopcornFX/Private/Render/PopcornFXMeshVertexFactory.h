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

// If FPopcornFXMeshVertexFactory is unused, it will be optimized out in Game build, and crash when trying to find it ...
//#define UNUSED_PopcornFXMeshVertexFactory

#ifndef UNUSED_PopcornFXMeshVertexFactory

struct FPopcornFXMeshUserData
{
	bool								m_Instanced;
	TStridedMemoryView<const CFloat4x4>	m_Instance_Matrices;
	TStridedMemoryView<const CFloat4>	m_Instance_Param_DiffuseColors;
	TStridedMemoryView<const float>		m_Instance_Param_VATCursors;
	TStridedMemoryView<const CFloat4>	m_Instance_Param_DynamicParameter0;
	TStridedMemoryView<const CFloat4>	m_Instance_Param_DynamicParameter1;
	TStridedMemoryView<const CFloat4>	m_Instance_Param_DynamicParameter2;
	//TStridedMemoryView<const CFloat4>	m_Instance_Param_DynamicParameter3;
};

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

#if (ENGINE_MINOR_VERSION >= 25)
	static bool			ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
#else
	static bool			ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);
#endif // (ENGINE_MINOR_VERSION >= 25)
	static bool			IsCompatible(UMaterialInterface *material);

	virtual void		InitRHI() override;
	void				SetData(const FDataType& InData);

#if (ENGINE_MINOR_VERSION < 25)
	static FVertexFactoryShaderParameters		*ConstructShaderParameters(EShaderFrequency ShaderFrequency);
#endif // (ENGINE_MINOR_VERSION < 25)

#if (ENGINE_MINOR_VERSION >= 25)
	static void									ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
#else
	static void									ModifyCompilationEnvironment(const FVertexFactoryType* Type, EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment);
#endif // (ENGINE_MINOR_VERSION >= 25)

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

#endif // UNUSED_PopcornFXMeshVertexFactory
