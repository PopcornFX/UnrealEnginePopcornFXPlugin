//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------
#include "PopcornFXMeshVertexFactory.h"

#include "PopcornFXVertexFactoryShaderParameters.h"

#include "Render/PopcornFXShaderUtils.h"

#include "ShaderParameterUtils.h"
#include "SceneView.h"
#include "MeshBatch.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"

#include "MeshMaterialShader.h"
#include "MeshDrawShaderBindings.h"

//----------------------------------------------------------------------------
//
// FPopcornFXMeshVertexFactoryShaderParameters
//
//----------------------------------------------------------------------------

class FPopcornFXMeshVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:

	void	Bind(const FShaderParameterMap& ParameterMap)
	{
	}

	void				GetElementShaderBindings(	const FSceneInterface *scene,
													const FSceneView *view,
													const FMeshMaterialShader *shader,
													const EVertexInputStreamType inputStreamType,
													ERHIFeatureLevel::Type featureLevel,
													const FVertexFactory* vertexFactory,
													const FMeshBatchElement &batchElement,
													class FMeshDrawSingleShaderBindings &shaderBindings,
													FVertexInputStreamArray &vertexStreams) const
	{
	}
};

//----------------------------------------------------------------------------
//
// FPopcornFXMeshVertexFactory
//
//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5)
IMPLEMENT_VERTEX_FACTORY_TYPE(FPopcornFXMeshVertexFactory, PKUE_SHADER_PATH("PopcornFXMeshVertexFactory"),
	EVertexFactoryFlags::UsedWithMaterials | EVertexFactoryFlags::SupportsDynamicLighting); // TODO
#else
IMPLEMENT_VERTEX_FACTORY_TYPE(FPopcornFXMeshVertexFactory, PKUE_SHADER_PATH("PopcornFXMeshVertexFactory"), true, false, true, false, false);
#endif // (ENGINE_MAJOR_VERSION == 5)

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXMeshVertexFactory, SF_Vertex, FPopcornFXMeshVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXMeshVertexFactory, SF_Compute, FPopcornFXMeshVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXMeshVertexFactory, SF_RayHitGroup, FPopcornFXMeshVertexFactoryShaderParameters);

//----------------------------------------------------------------------------

void	FPopcornFXMeshVertexFactory::_SetupInstanceStream(	u32								attributeIndex,
															u32								offset,
															FPopcornFXVertexBufferView		&buffer,
															EVertexElementType				type,
															FVertexDeclarationElementList	&decl)
{
	if (!buffer.Valid())
	{
		FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color);
		decl.Add(AccessStreamComponent(NullColorComponent, attributeIndex));
		return;
	}

	const u32			streamIndex = Streams.Num();
	FVertexStream		*stream = new (Streams) FVertexStream();
	stream->Offset = buffer.Offset();
	stream->Stride = buffer.Stride();
	stream->VertexBuffer = buffer.VertexBufferPtr();

	decl.Add(FVertexElement(streamIndex, offset, type, attributeIndex, buffer.Stride(), true));
}

//----------------------------------------------------------------------------

bool	FPopcornFXMeshVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	return	(Parameters.MaterialParameters.bIsUsedWithMeshParticles || Parameters.MaterialParameters.bIsUsedWithNiagaraMeshParticles || Parameters.MaterialParameters.bIsSpecialEngineMaterial) &&
			Parameters.MaterialParameters.MaterialDomain == EMaterialDomain::MD_Surface;
}

//----------------------------------------------------------------------------

bool	FPopcornFXMeshVertexFactory::IsCompatible(UMaterialInterface *material)
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

void	FPopcornFXMeshVertexFactory::SetData(const FDataType& InData)
{
	check(IsInRenderingThread());
	Data = InData;
	UpdateRHI();
}

//----------------------------------------------------------------------------

void	FPopcornFXMeshVertexFactory::InitRHI()
{
	FVertexDeclarationElementList Elements;

	if (Data.bInitialized)
	{
		Streams.Empty();
		// Particle Matrices
		PK_ASSERT(Data.m_InstancedMatrices.Valid());
		{
			const u32				streamIndex = 0;
			FVertexStream			*stream = new (Streams)FVertexStream();
			stream->Offset = Data.m_InstancedMatrices.Offset();
			stream->Stride = Data.m_InstancedMatrices.Stride();
			stream->VertexBuffer = Data.m_InstancedMatrices.VertexBufferPtr();

			const u32		stride = Data.m_InstancedMatrices.Stride();
			Elements.Add(FVertexElement(streamIndex, sizeof(float) * 4 * 0, VET_Float4, 8, stride, true));
			Elements.Add(FVertexElement(streamIndex, sizeof(float) * 4 * 1, VET_Float4, 9, stride, true));
			Elements.Add(FVertexElement(streamIndex, sizeof(float) * 4 * 2, VET_Float4, 10, stride, true));
			Elements.Add(FVertexElement(streamIndex, sizeof(float) * 4 * 3, VET_Float4, 11, stride, true));
		}

		// Particle colors
		PK_ASSERT(Data.m_InstancedColors.Valid());
		{
			const u32	attributeIndex = 12;
			const u32	offset = 0;
			_SetupInstanceStream(attributeIndex, offset, Data.m_InstancedColors, Data.m_InstancedColors.Stride() == 8 ? VET_Half4 : VET_Float4, Elements);
		}

		// Vertex positions
		if (Data.PositionComponent.VertexBuffer != NULL)
		{
			Elements.Add(AccessStreamComponent(Data.PositionComponent, 0));
		}

		// Vertex Tangents & Normals (only tangent,normal are used by the stream. the binormal is derived in the shader)
		uint8 TangentBasisAttributes[2] = { 1, 2 };
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

		// Particle VAT cursors
		{
			const u32	attributeIndex = 13;
			const u32	offset = 0;
			_SetupInstanceStream(attributeIndex, offset, Data.m_InstancedVATCursors, VET_Float1, Elements);
		}
		// Particle Dynamic parameter 0
		{
			const u32	attributeIndex = 14;
			const u32	offset = 0;
			_SetupInstanceStream(attributeIndex, offset, Data.m_InstancedDynamicParameters0, VET_Float4, Elements);
		}

		// Particle Dynamic parameter 1
		{
			const u32	attributeIndex = 15;
			const u32	offset = 0;
			_SetupInstanceStream(attributeIndex, offset, Data.m_InstancedDynamicParameters1, VET_Float4, Elements);
		}

		// Dyn param 2 & 3 are currently disabled. Will need rework using a single GPU buffer containing all per instance data to remove this vertex attribute limit

		// Particle Dynamic parameter 2
		//{
		//	const u32				streamIndex = 9;
		//	FVertexStream			*stream = new (Streams)FVertexStream();
		//	stream->Offset = Data.m_InstancedDynamicParameters2.Offset();
		//	stream->Stride = Data.m_InstancedDynamicParameters2.Stride();
		//	stream->VertexBuffer = Data.m_InstancedDynamicParameters2.VertexBufferPtr();
		//
		//	const u32		stride = Data.m_InstancedDynamicParameters2.Stride();
		//	Elements.Add(FVertexElement(streamIndex, 0, VET_Float4, 16, stride, true));
		//}

		// Particle Dynamic parameter 3
		//{
		//	const u32			streamIndex = 9;
		//	FVertexStream			*stream = new (Streams)FVertexStream();
		//	stream->Offset = Data.m_InstancedDynamicParameters3.Offset();
		//	stream->Stride = Data.m_InstancedDynamicParameters3.Stride();
		//	stream->VertexBuffer = Data.m_InstancedDynamicParameters3.VertexBufferPtr();
		//
		//	const u32		stride = Data.m_InstancedDynamicParameters3.Stride();
		//	Elements.Add(FVertexElement(streamIndex, 0, VET_Float4, 16, stride, true));
		//}

		if (Streams.Num() > 0)
		{
			InitDeclaration(Elements);
			check(IsValidRef(GetDeclaration()));
		}
	}
}

//----------------------------------------------------------------------------

// static
void	FPopcornFXMeshVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
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
