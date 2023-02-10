//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------
#include "PopcornFXSkeletalMeshVertexFactory.h"
#include "PopcornFXVertexFactoryCommon.h"

#include "Render/PopcornFXShaderUtils.h"

#include "ShaderParameterUtils.h"
#include "SceneView.h"
#include "MeshBatch.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"

#include "GPUSkinPublicDefs.h"

#include "MeshMaterialShader.h"
#include "MeshDrawShaderBindings.h"

#include "PopcornFXSDK.h"

//----------------------------------------------------------------------------

#define PK_CS_PCOUNT_THREADGROUP_SIZE	32U
#define PK_CS_BCOUNT_THREADGROUP_SIZE	32U

//----------------------------------------------------------------------------

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXSkelMeshUniforms, "PopcornFXSkelMeshUniforms");

//----------------------------------------------------------------------------
//
// FPopcornFXSkelMeshVertexFactoryShaderParameters
//
//----------------------------------------------------------------------------

class FPopcornFXSkelMeshVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
#if (ENGINE_MAJOR_VERSION == 5)
	DECLARE_TYPE_LAYOUT(FPopcornFXSkelMeshVertexFactoryShaderParameters, NonVirtual);
#else
	DECLARE_INLINE_TYPE_LAYOUT(FPopcornFXSkelMeshVertexFactoryShaderParameters, NonVirtual);
#endif // (ENGINE_MAJOR_VERSION == 5)

	void	Bind(const FShaderParameterMap &ParameterMap)
	{
#if (PK_PREBUILD_BONE_TRANSFORMS != 0)
		BoneMatrices.Bind(ParameterMap, TEXT("BoneMatrices"));
		PreviousBoneMatrices.Bind(ParameterMap, TEXT("PreviousBoneMatrices"));
#endif // (PK_PREBUILD_BONE_TRANSFORMS != 0)
		BoneIndicesReorderOffset.Bind(ParameterMap, TEXT("BoneIndicesReorderOffset"));
	}

	void	GetElementShaderBindings(	const FSceneInterface					*scene,
										const FSceneView						*view,
										const FMeshMaterialShader				*shader,
										const EVertexInputStreamType			inputStreamType,
										ERHIFeatureLevel::Type					featureLevel,
										const FVertexFactory					*vertexFactory,
										const FMeshBatchElement					&batchElement,
										class FMeshDrawSingleShaderBindings		&shaderBindings,
										FVertexInputStreamArray					&vertexStreams) const
	{
		FPopcornFXSkelMeshVertexFactory	*_vertexFactory = (FPopcornFXSkelMeshVertexFactory*)vertexFactory;
		shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXUniforms>(), _vertexFactory->GetVSUniformBuffer());
		shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXSkelMeshUniforms>(), _vertexFactory->GetSkelMeshVSUniformBuffer());

#if (PK_PREBUILD_BONE_TRANSFORMS != 0)
		shaderBindings.Add(BoneMatrices, _vertexFactory->GetData().m_BoneMatrices);
		shaderBindings.Add(PreviousBoneMatrices, _vertexFactory->GetData().m_PreviousBoneMatrices);
#endif // (PK_PREBUILD_BONE_TRANSFORMS != 0)

		const u64	userData = reinterpret_cast<u64>(batchElement.UserData);
		const u32	boneIndicesReorderOffset = userData;
		shaderBindings.Add(BoneIndicesReorderOffset, boneIndicesReorderOffset);
	}

private:
#if (PK_PREBUILD_BONE_TRANSFORMS != 0)
	LAYOUT_FIELD(FShaderResourceParameter, BoneMatrices);
	LAYOUT_FIELD(FShaderResourceParameter, PreviousBoneMatrices);
#endif // (PK_PREBUILD_BONE_TRANSFORMS != 0)
	LAYOUT_FIELD(FShaderParameter, BoneIndicesReorderOffset);
};

//----------------------------------------------------------------------------
//
// FPopcornFXComputeBoneTransformsCS
//
//----------------------------------------------------------------------------

IMPLEMENT_SHADER_TYPE(, FPopcornFXComputeBoneTransformsCS, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXComputeBoneTransformsComputeShader")), TEXT("Main"), SF_Compute);

// static
bool	FPopcornFXComputeBoneTransformsCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
{
	return true;
}

//----------------------------------------------------------------------------

//static
void	FPopcornFXComputeBoneTransformsCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment)
{
	Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	// @FIXME: it wont trigger recompilation !!
	OutEnvironment.SetDefine(TEXT("PK_GPU_THREADGROUP_SIZE_X"), PK_CS_PCOUNT_THREADGROUP_SIZE);
	OutEnvironment.SetDefine(TEXT("PK_GPU_THREADGROUP_SIZE_Y"), PK_CS_BCOUNT_THREADGROUP_SIZE);

	OutEnvironment.SetDefine(TEXT("SAMPLE_VAT_TEXTURES"), PK_PREBUILD_BONE_TRANSFORMS);

	OutEnvironment.SetDefine(TEXT("COMPUTE_PREV_MATRICES"), 0);
}

//----------------------------------------------------------------------------

FPopcornFXComputeBoneTransformsCS::FPopcornFXComputeBoneTransformsCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	: FGlobalShader(Initializer)
{
	TotalParticleCount.Bind(Initializer.ParameterMap, TEXT("TotalParticleCount"));
	TotalBoneCount.Bind(Initializer.ParameterMap, TEXT("TotalBoneCount"));
	BoneMatrices.Bind(Initializer.ParameterMap, TEXT("BoneMatrices"));
}

//----------------------------------------------------------------------------

void	FPopcornFXComputeBoneTransformsCS::Dispatch(FRHICommandList& RHICmdList, const FPopcornFXComputeBoneTransformsCS_Params &params)
{
	FCSRHIParamRef	shader = RHICmdList.GetBoundComputeShader();

	SetShaderValue(RHICmdList, shader, TotalParticleCount, params.m_TotalParticleCount);
	SetShaderValue(RHICmdList, shader, TotalBoneCount, params.m_TotalBoneCount);

	SetUniformBufferParameter(RHICmdList, shader, GetUniformBufferParameter<FPopcornFXUniforms>(), params.m_UniformBuffer);
	SetUniformBufferParameter(RHICmdList, shader, GetUniformBufferParameter<FPopcornFXSkelMeshUniforms>(), params.m_MeshUniformBuffer);
	SetUAVParameter(RHICmdList, shader, BoneMatrices, params.m_BoneMatrices);

	{
		PK_NAMEDSCOPEDPROFILE("FPopcornFXComputeBoneTransformsCS::Dispatch");
		RHICmdList.DispatchComputeShader(	FMath::DivideAndRoundUp(params.m_TotalParticleCount, PK_CS_PCOUNT_THREADGROUP_SIZE),
											FMath::DivideAndRoundUp(params.m_TotalBoneCount, PK_CS_BCOUNT_THREADGROUP_SIZE),
											1);
	}
}

//----------------------------------------------------------------------------
//
// FPopcornFXComputeMBBoneTransformsCS
//
//----------------------------------------------------------------------------

IMPLEMENT_SHADER_TYPE(, FPopcornFXComputeMBBoneTransformsCS, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXComputeBoneTransformsComputeShader")), TEXT("Main"), SF_Compute);

// static
bool	FPopcornFXComputeMBBoneTransformsCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
{
	return true;
}

//----------------------------------------------------------------------------

//static
void	FPopcornFXComputeMBBoneTransformsCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment)
{
	Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);
}

//----------------------------------------------------------------------------

FPopcornFXComputeMBBoneTransformsCS::FPopcornFXComputeMBBoneTransformsCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	: FPopcornFXComputeBoneTransformsCS(Initializer)
{
	PrevBoneMatrices.Bind(Initializer.ParameterMap, TEXT("PrevBoneMatrices"));
}

//----------------------------------------------------------------------------

void	FPopcornFXComputeMBBoneTransformsCS::Dispatch(FRHICommandList& RHICmdList, const FPopcornFXComputeBoneTransformsCS_Params &params)
{
	FCSRHIParamRef	shader = RHICmdList.GetBoundComputeShader();

	SetShaderValue(RHICmdList, shader, TotalParticleCount, params.m_TotalParticleCount);
	SetShaderValue(RHICmdList, shader, TotalBoneCount, params.m_TotalBoneCount);

	SetUniformBufferParameter(RHICmdList, shader, GetUniformBufferParameter<FPopcornFXUniforms>(), params.m_UniformBuffer);
	SetUniformBufferParameter(RHICmdList, shader, GetUniformBufferParameter<FPopcornFXSkelMeshUniforms>(), params.m_MeshUniformBuffer);
	SetUAVParameter(RHICmdList, shader, BoneMatrices, params.m_BoneMatrices);
	SetUAVParameter(RHICmdList, shader, PrevBoneMatrices, params.m_PrevBoneMatrices);

	{
		PK_NAMEDSCOPEDPROFILE("FPopcornFXComputeMBBoneTransformsCS::Dispatch");
		RHICmdList.DispatchComputeShader(	FMath::DivideAndRoundUp(params.m_TotalParticleCount, PK_CS_PCOUNT_THREADGROUP_SIZE),
											FMath::DivideAndRoundUp(params.m_TotalBoneCount, PK_CS_BCOUNT_THREADGROUP_SIZE),
											1);
	}
}

//----------------------------------------------------------------------------
//
// FPopcornFXSkelMeshVertexFactory
//
//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5)
IMPLEMENT_VERTEX_FACTORY_TYPE(FPopcornFXSkelMeshVertexFactory, PKUE_SHADER_PATH("PopcornFXSkeletalMeshVertexFactory"),
	EVertexFactoryFlags::UsedWithMaterials | EVertexFactoryFlags::SupportsDynamicLighting); // TODO
#else
IMPLEMENT_VERTEX_FACTORY_TYPE(FPopcornFXSkelMeshVertexFactory, PKUE_SHADER_PATH("PopcornFXSkeletalMeshVertexFactory"), true, false, true, false, false);
#endif // (ENGINE_MAJOR_VERSION == 5)

#if (ENGINE_MAJOR_VERSION == 5)
IMPLEMENT_TYPE_LAYOUT(FPopcornFXSkelMeshVertexFactoryShaderParameters);
#endif // (ENGINE_MAJOR_VERSION == 5)

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXSkelMeshVertexFactory, SF_Vertex, FPopcornFXSkelMeshVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXSkelMeshVertexFactory, SF_Compute, FPopcornFXSkelMeshVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXSkelMeshVertexFactory, SF_RayHitGroup, FPopcornFXSkelMeshVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXSkelMeshVertexFactory, SF_Pixel, FPopcornFXSkelMeshVertexFactoryShaderParameters);

//----------------------------------------------------------------------------

bool	FPopcornFXSkelMeshVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	return	(Parameters.MaterialParameters.bIsUsedWithMeshParticles || Parameters.MaterialParameters.bIsUsedWithNiagaraMeshParticles || Parameters.MaterialParameters.bIsSpecialEngineMaterial) &&
			Parameters.MaterialParameters.MaterialDomain == EMaterialDomain::MD_Surface;
}

//----------------------------------------------------------------------------

bool	FPopcornFXSkelMeshVertexFactory::IsCompatible(UMaterialInterface *material)
{
	if (!material->CheckMaterialUsage_Concurrent(MATUSAGE_MeshParticles) &&
		!material->CheckMaterialUsage_Concurrent(MATUSAGE_NiagaraMeshParticles))
		return false;

	UMaterial	*mat = material->GetMaterial();
	if (mat != null)
		return PK_VERIFY(mat->MaterialDomain == MD_Surface); // If this triggers, it's a miss earlier (Render data factory ?)
	return true;
}

//----------------------------------------------------------------------------

void	FPopcornFXSkelMeshVertexFactory::SetData(const FDataType& InData)
{
	check(IsInRenderingThread());
	Data = InData;
	UpdateRHI();
}

//----------------------------------------------------------------------------

void	FPopcornFXSkelMeshVertexFactory::InitRHI()
{
	FVertexDeclarationElementList Elements;

	if (Data.bInitialized)
	{
		Streams.Empty();

		// Vertex positions
		if (Data.PositionComponent.VertexBuffer != null)
		{
			Elements.Add(AccessStreamComponent(Data.PositionComponent, 0));
		}

		// Vertex Tangents & Normals (only tangent,normal are used by the stream. the binormal is derived in the shader)
		uint8 TangentBasisAttributes[2] = { 1, 2 };
		for (int32 AxisIndex = 0; AxisIndex < 2; AxisIndex++)
		{
			if (Data.TangentBasisComponents[AxisIndex].VertexBuffer != null)
			{
				Elements.Add(AccessStreamComponent(Data.TangentBasisComponents[AxisIndex], TangentBasisAttributes[AxisIndex]));
			}
		}

		// Vertex colors
		if (Data.ColorComponent.VertexBuffer != null)
		{
			Elements.Add(AccessStreamComponent(Data.ColorComponent, 3));
		}
		else
		{
			//If the mesh has no color component, set the null color buffer on a new stream with a stride of 0.
			//This wastes 4 bytes of bandwidth per vertex, but prevents having to compile out twice the number of vertex factories.
			FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color);
			Elements.Add(AccessStreamComponent(NullColorComponent, 3));
		}

		// Vertex UVs
		if (Data.TextureCoordinates.Num())
		{
			const int32 BaseTexCoordAttribute = 4;
			for (int32 CoordinateIndex = 0; CoordinateIndex < Data.TextureCoordinates.Num(); CoordinateIndex++)
			{
				Elements.Add(AccessStreamComponent(
					Data.TextureCoordinates[CoordinateIndex],
					BaseTexCoordAttribute + CoordinateIndex
					));
			}

			for (int32 CoordinateIndex = Data.TextureCoordinates.Num(); CoordinateIndex < MAX_TEXCOORDS; CoordinateIndex++)
			{
				Elements.Add(AccessStreamComponent(
					Data.TextureCoordinates[Data.TextureCoordinates.Num() - 1],
					BaseTexCoordAttribute + CoordinateIndex
					));
			}
		}

		// Skinning infos
		{
			// bone indices decls
			Elements.Add(AccessStreamComponent(Data.m_BoneIndices, 8));

			// bone weights decls
			Elements.Add(AccessStreamComponent(Data.m_BoneWeights, 9));
		}

		if (Streams.Num() > 0)
		{
			InitDeclaration(Elements);
			check(IsValidRef(GetDeclaration()));
		}
	}
}

//----------------------------------------------------------------------------

// static
void	FPopcornFXSkelMeshVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	// @FIXME: it wont trigger recompilation !!
	OutEnvironment.SetDefine(TEXT("ENGINE_MINOR_VERSION"), ENGINE_MINOR_VERSION);
	OutEnvironment.SetDefine(TEXT("ENGINE_MAJOR_VERSION"), ENGINE_MAJOR_VERSION);

	OutEnvironment.SetDefine(TEXT("PREBUILD_BONE_TRANSFORMS"), PK_PREBUILD_BONE_TRANSFORMS);
	OutEnvironment.SetDefine(TEXT("SAMPLE_VAT_TEXTURES"), !PK_PREBUILD_BONE_TRANSFORMS);

	// Dynamic parameters..
	OutEnvironment.SetDefine(TEXT("PARTICLE_FACTORY"),TEXT("1"));
	OutEnvironment.SetDefine(TEXT("NIAGARA_PARTICLE_FACTORY"), TEXT("1"));

	// Set a define so we can tell in MaterialTemplate.usf when we are compiling a mesh particle vertex factory
	OutEnvironment.SetDefine(TEXT("PARTICLE_MESH_FACTORY"), TEXT("1"));
}

//----------------------------------------------------------------------------
