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

#define		PK_PREBUILD_BONE_TRANSFORMS	1

//----------------------------------------------------------------------------

// Skeletal mesh shader uniforms
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXSkelMeshUniforms, POPCORNFX_API)
	SHADER_PARAMETER(int32, InColorsOffset)
	SHADER_PARAMETER(int32, InEmissiveColorsOffset)
	SHADER_PARAMETER(int32, InAlphaCursorsOffset)
	SHADER_PARAMETER(int32, InTextureIDsOffset)
	SHADER_PARAMETER(int32, InVATCursorsOffset)
	SHADER_PARAMETER(int32, InVATCursorNextsOffset)
	SHADER_PARAMETER(int32, InVATTracksOffset)
	SHADER_PARAMETER(int32, InVATTrackNextsOffset)
	SHADER_PARAMETER(int32, InVATTransitionCursorsOffset)
	SHADER_PARAMETER(int32, InPrevVATCursorsOffset)
	SHADER_PARAMETER(int32, InPrevVATCursorNextsOffset)
	SHADER_PARAMETER(int32, InPrevVATTracksOffset)
	SHADER_PARAMETER(int32, InPrevVATTrackNextsOffset)
	SHADER_PARAMETER(int32, InPrevVATTransitionCursorsOffset)
	SHADER_PARAMETER(int32, InDynamicParameter0sOffset)
	SHADER_PARAMETER(int32, InDynamicParameter1sOffset)
	SHADER_PARAMETER(int32, InDynamicParameter2sOffset)
	SHADER_PARAMETER(int32, InDynamicParameter3sOffset)
	SHADER_PARAMETER(int32, InMatricesOffset)
	SHADER_PARAMETER(int32, InPrevMatricesOffset)

	SHADER_PARAMETER(uint32, AtlasRectCount)
	SHADER_PARAMETER_SRV(Buffer<float4>, AtlasBuffer)

	SHADER_PARAMETER(uint32, SkinningBufferStride)
	SHADER_PARAMETER(uint32, BoneMatricesOffset)

	SHADER_PARAMETER_SRV(Buffer<uint>, BoneIndicesReorder)
	SHADER_PARAMETER_TEXTURE(Texture2D<half4>, SkeletalAnimationTexture)
	SHADER_PARAMETER_SAMPLER(SamplerState, SkeletalAnimationSampler)
#	if (ENGINE_MAJOR_VERSION == 5)
	SHADER_PARAMETER(FVector2f, InvSkeletalAnimationTextureDimensions)
	SHADER_PARAMETER(FVector3f, SkeletalAnimationPosBoundsMin)
	SHADER_PARAMETER(FVector3f, SkeletalAnimationPosBoundsMax)
#	else
	SHADER_PARAMETER(FVector2D, InvSkeletalAnimationTextureDimensions)
	SHADER_PARAMETER(FVector, SkeletalAnimationPosBoundsMin)
	SHADER_PARAMETER(FVector, SkeletalAnimationPosBoundsMax)
#	endif // (ENGINE_MAJOR_VERSION == 5)
	SHADER_PARAMETER(float, SkeletalAnimationYOffsetPerTrack)
	SHADER_PARAMETER(uint32, LinearInterpolateTransforms)
	SHADER_PARAMETER(uint32, LinearInterpolateTracks)

END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FPopcornFXSkelMeshUniforms> FPopcornFXSkelMeshUniformsRef;

//----------------------------------------------------------------------------

struct	FPopcornFXComputeBoneTransformsCS_Params
{
	u32							m_TotalParticleCount = 0;
	u32							m_TotalBoneCount = 0;
	FRHIUniformBuffer			*m_UniformBuffer = null;
	FRHIUniformBuffer			*m_MeshUniformBuffer = null;
	FUnorderedAccessViewRHIRef	m_BoneMatrices;
	FUnorderedAccessViewRHIRef	m_PrevBoneMatrices;
};

//----------------------------------------------------------------------------

class	FPopcornFXComputeBoneTransformsCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPopcornFXComputeBoneTransformsCS, Global);

public:
	typedef FGlobalShader	Super;

	static bool			ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters);
	static void			ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment);

	FPopcornFXComputeBoneTransformsCS(const ShaderMetaType::CompiledShaderInitializerType &Initializer);
	FPopcornFXComputeBoneTransformsCS() { }

	void			Dispatch(FRHICommandList &RHICmdList, const FPopcornFXComputeBoneTransformsCS_Params &params);
public:
	LAYOUT_FIELD(FShaderParameter, TotalParticleCount);
	LAYOUT_FIELD(FShaderParameter, TotalBoneCount);
	LAYOUT_FIELD(FShaderResourceParameter, BoneMatrices);
};

//----------------------------------------------------------------------------

class	FPopcornFXComputeMBBoneTransformsCS : public FPopcornFXComputeBoneTransformsCS
{
	DECLARE_SHADER_TYPE(FPopcornFXComputeMBBoneTransformsCS, Global);

public:
	typedef FPopcornFXComputeBoneTransformsCS	Super;

	static bool			ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters);
	static void			ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment);

	FPopcornFXComputeMBBoneTransformsCS(const ShaderMetaType::CompiledShaderInitializerType &Initializer);
	FPopcornFXComputeMBBoneTransformsCS() { }

	void				Dispatch(FRHICommandList &RHICmdList, const FPopcornFXComputeBoneTransformsCS_Params &params);
public:
	LAYOUT_FIELD(FShaderResourceParameter, PrevBoneMatrices);
};

//----------------------------------------------------------------------------

class	FPopcornFXSkelMeshVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FPopcornFXSkelMeshVertexFactory);

public:
	struct FDataType : public FStaticMeshDataType
	{
		// Bone matrices
		FShaderResourceViewRHIRef	m_BoneMatrices;

		// Previous Bone matrices
		FShaderResourceViewRHIRef	m_PreviousBoneMatrices;

		/** The stream to read the bone indices from */
		FVertexStreamComponent		m_BoneIndices;

		/** The stream to read the bone weights from */
		FVertexStreamComponent		m_BoneWeights;

		/** If the bone indices are 16 or 8-bit format */
		bool						m_Use16BitBoneIndex = 0;

		bool						bInitialized = false;
	};

	FPopcornFXSkelMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
	:	FVertexFactory(InFeatureLevel)
	{
	}

	static bool			ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static bool			IsCompatible(UMaterialInterface *material);

	virtual void		InitRHI() override;
	void				SetData(const FDataType& InData);
	const FDataType		&GetData() const { return Data; }

	static void			ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	static bool			SupportsTessellationShaders() { return true; }

	FRHIUniformBuffer	*GetVSUniformBuffer() { return m_VSUniformBuffer; }
	FRHIUniformBuffer	*GetSkelMeshVSUniformBuffer() { return m_SkelMeshVSUniformBuffer; }

protected:
	FDataType			Data;

public:
	FUniformBufferRHIRef	m_VSUniformBuffer;
	FUniformBufferRHIRef	m_SkelMeshVSUniformBuffer;
};
