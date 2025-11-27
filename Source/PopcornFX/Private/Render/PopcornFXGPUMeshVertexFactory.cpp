//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXGPUMeshVertexFactory.h"
#include "PopcornFXMeshVertexFactory.h"
#include "PopcornFXVertexFactoryCommon.h"

#include "PopcornFXVertexFactoryShaderParameters.h"

#include "Render/PopcornFXShaderUtils.h"
#include "MeshMaterialShader.h"
#include "ParticleResources.h"
#include "PipelineStateCache.h"

#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "MeshDrawShaderBindings.h"
#include "GlobalRenderResources.h"
#include "MaterialDomain.h"
#include "DataDrivenShaderPlatformInfo.h"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXGPUMeshVSUniforms, "PopcornFXGPUMeshVSUniforms");

//----------------------------------------------------------------------------
//
// FPopcornFXGPUMeshVertexFactoryShaderParameters
//
//----------------------------------------------------------------------------

IMPLEMENT_TYPE_LAYOUT(FPopcornFXGPUMeshVertexFactoryShaderParameters);

void	FPopcornFXGPUMeshVertexFactoryShaderParameters::Bind(const FShaderParameterMap &ParameterMap)
{
	MeshBatchElementId.Bind(ParameterMap, TEXT("MeshBatchElementId"));
}

//----------------------------------------------------------------------------

void	FPopcornFXGPUMeshVertexFactoryShaderParameters::GetElementShaderBindings(	const FSceneInterface *scene,
																					const FSceneView *view,
																					const FMeshMaterialShader *shader,
																					const EVertexInputStreamType inputStreamType,
																					ERHIFeatureLevel::Type featureLevel,
																					const FVertexFactory* vertexFactory,
																					const FMeshBatchElement &batchElement,
																					class FMeshDrawSingleShaderBindings &shaderBindings,
																					FVertexInputStreamArray &vertexStreams) const
{
	const FPopcornFXGPUMeshVertexFactory	*_vertexFactory = (FPopcornFXGPUMeshVertexFactory*)vertexFactory;
	shaderBindings.Add(MeshBatchElementId, batchElement.UserIndex); // UserIndex set to the ID of the current LOD section from CBatchDrawer_Mesh_GPUBB::_BuildMeshBatchElement
	shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXUniforms>(), _vertexFactory->GetVSUniformBuffer());
	shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXMeshVSUniforms>(), _vertexFactory->GetMeshVSUniformBuffer());
	shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXGPUMeshVSUniforms>(), _vertexFactory->GetGPUMeshVSUniformBuffer());
}

//----------------------------------------------------------------------------
//
// FPopcornFXGPUMeshVertexFactory
//
//----------------------------------------------------------------------------

IMPLEMENT_VERTEX_FACTORY_TYPE(FPopcornFXGPUMeshVertexFactory, PKUE_SHADER_PATH("PopcornFXGPUMeshVertexFactory"),
	EVertexFactoryFlags::UsedWithMaterials | EVertexFactoryFlags::SupportsDynamicLighting); // TODO

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUMeshVertexFactory, SF_Vertex, FPopcornFXGPUMeshVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUMeshVertexFactory, SF_Compute, FPopcornFXGPUMeshVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUMeshVertexFactory, SF_RayHitGroup, FPopcornFXGPUMeshVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUMeshVertexFactory, SF_Pixel, FPopcornFXGPUMeshVertexFactoryShaderParameters);

//----------------------------------------------------------------------------

bool	FPopcornFXGPUMeshVertexFactory::PlatformIsSupported(EShaderPlatform platform)
{
	const bool	platformSupportsByteAddressBuffers =	IsFeatureLevelSupported(platform, ERHIFeatureLevel::SM5) ||
														IsFeatureLevelSupported(platform, ERHIFeatureLevel::ES3_1);

	return platformSupportsByteAddressBuffers;
}

//----------------------------------------------------------------------------

bool	FPopcornFXGPUMeshVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	return	PlatformIsSupported(Parameters.Platform) &&
			(Parameters.MaterialParameters.bIsUsedWithMeshParticles || Parameters.MaterialParameters.bIsUsedWithNiagaraMeshParticles || Parameters.MaterialParameters.bIsSpecialEngineMaterial) &&
			Parameters.MaterialParameters.MaterialDomain == EMaterialDomain::MD_Surface;
}

//----------------------------------------------------------------------------

bool	FPopcornFXGPUMeshVertexFactory::IsCompatible(UMaterialInterface *material)
{
	if (!material->CheckMaterialUsage_Concurrent(MATUSAGE_MeshParticles) &&
		!material->CheckMaterialUsage_Concurrent(MATUSAGE_NiagaraMeshParticles))
		return false;

	const UMaterial	*mat = material->GetMaterial();
	if (mat != null)
		return PK_VERIFY(mat->MaterialDomain == MD_Surface); // If this triggers, it's a miss earlier (Render data factory ?)
	return true;
}

//----------------------------------------------------------------------------

// static
void	FPopcornFXGPUMeshVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	// @FIXME: it wont trigger recompilation !!
	OutEnvironment.SetDefine(TEXT("ENGINE_MINOR_VERSION"), ENGINE_MINOR_VERSION);
	OutEnvironment.SetDefine(TEXT("ENGINE_MAJOR_VERSION"), ENGINE_MAJOR_VERSION);

	// Dynamic parameters..
	OutEnvironment.SetDefine(TEXT("PARTICLE_FACTORY"),TEXT("1"));
	OutEnvironment.SetDefine(TEXT("NIAGARA_PARTICLE_FACTORY"), TEXT("1"));

	// Set a define so we can tell in MaterialTemplate.usf when we are compiling a mesh particle vertex factory
	OutEnvironment.SetDefine(TEXT("PARTICLE_MESH_FACTORY"), TEXT("1"));
}

//----------------------------------------------------------------------------

void	FPopcornFXGPUMeshVertexFactory::SetData(const FDataType& InData)
{
	check(IsInRenderingThread());
	Data = InData;
	UpdateRHI(FRHICommandListExecutor::GetImmediateCommandList());
}

//----------------------------------------------------------------------------

void	FPopcornFXGPUMeshVertexFactory::InitRHI(FRHICommandListBase &RHICmdList)
{
	FVertexDeclarationElementList	Elements;

	if (Data.bInitialized)
	{
		Streams.Empty();
		// Vertex positions
		if (Data.PositionComponent.VertexBuffer != NULL)
		{
			Elements.Add(AccessStreamComponent(Data.PositionComponent, 0));
		}

		// Vertex Tangents & Normals (only tangent,normal are used by the stream. the binormal is derived in the shader)
		const uint8	TangentBasisAttributes[2] = { 1, 2 };
		for (int32 AxisIndex = 0; AxisIndex < 2; AxisIndex++)
		{
			if (Data.TangentBasisComponents[AxisIndex].VertexBuffer != NULL)
			{
				Elements.Add(AccessStreamComponent(Data.TangentBasisComponents[AxisIndex], TangentBasisAttributes[AxisIndex]));
			}
		}

		// Vertex colors
		if (Data.ColorComponent.VertexBuffer != NULL)
		{
			Elements.Add(AccessStreamComponent(Data.ColorComponent, 3));
		}
		else
		{
			//If the mesh has no color component, set the null color buffer on a new stream with a stride of 0.
			//This wastes 4 bytes of bandwidth per vertex, but prevents having to compile out twice the number of vertex factories.
			const FVertexStreamComponent	NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color);
			Elements.Add(AccessStreamComponent(NullColorComponent, 3));
		}

		// Vertex UVs
		if (Data.TextureCoordinates.Num())
		{
			const int32	BaseTexCoordAttribute = 4;
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

		if (Streams.Num() > 0)
		{
			InitDeclaration(Elements);
			check(IsValidRef(GetDeclaration()));
		}
	}
}