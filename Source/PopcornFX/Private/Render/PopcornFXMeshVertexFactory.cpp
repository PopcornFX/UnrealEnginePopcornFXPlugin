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

#if (ENGINE_MINOR_VERSION >= 25)
#	include "MeshMaterialShader.h"
#endif // (ENGINE_MINOR_VERSION >= 25)
#include "MeshDrawShaderBindings.h"

#ifndef UNUSED_PopcornFXMeshVertexFactory

//----------------------------------------------------------------------------
//
// FPopcornFXMeshVertexFactoryShaderParameters
//
//----------------------------------------------------------------------------

class FPopcornFXMeshVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:

#if (ENGINE_MINOR_VERSION >= 25)
	void	Bind(const FShaderParameterMap& ParameterMap)
	{
	}
#else
	virtual void	Bind(const FShaderParameterMap& ParameterMap) override
	{
		Transform1.Bind(ParameterMap, TEXT("Transform1"));
		Transform2.Bind(ParameterMap, TEXT("Transform2"));
		Transform3.Bind(ParameterMap, TEXT("Transform3"));
		Transform4.Bind(ParameterMap, TEXT("Transform4"));
		//DynamicParameter.Bind(ParameterMap, TEXT("DynamicParameter"));
		ParticleColor.Bind(ParameterMap, TEXT("ParticleColor"));
		DynamicParameter1.Bind(ParameterMap, TEXT("DynamicParameter"));
		DynamicParameter1.Bind(ParameterMap, TEXT("DynamicParameter1"));
		DynamicParameter2.Bind(ParameterMap, TEXT("DynamicParameter2"));
		//DynamicParameter3.Bind(ParameterMap, TEXT("DynamicParameter3"));
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Ar << Transform1;
		Ar << Transform2;
		Ar << Transform3;
		Ar << Transform4;
		//Ar << DynamicParameter;
		Ar << ParticleColor;
		Ar << DynamicParameter0;
		Ar << DynamicParameter1;
		Ar << DynamicParameter2;
		//Ar << DynamicParameter3;
	}
#endif // (ENGINE_MINOR_VERSION < 25)

#if (ENGINE_MINOR_VERSION < 25)
	virtual
#endif // (ENGINE_MINOR_VERSION < 25)
	void				GetElementShaderBindings(	const FSceneInterface *scene,
													const FSceneView *view,
													const FMeshMaterialShader *shader,
													const EVertexInputStreamType inputStreamType,
													ERHIFeatureLevel::Type featureLevel,
													const FVertexFactory* vertexFactory,
													const FMeshBatchElement &batchElement,
													class FMeshDrawSingleShaderBindings &shaderBindings,
													FVertexInputStreamArray &vertexStreams) const
#if (ENGINE_MINOR_VERSION < 25)
													override
#endif // (ENGINE_MINOR_VERSION < 25)
	{
#if (ENGINE_MINOR_VERSION < 25)
		const FPopcornFXMeshUserData	*instanceDatas = static_cast<const FPopcornFXMeshUserData*>(batchElement.UserData);
		PK_ASSERT(instanceDatas != null);
		if (instanceDatas == null)
			return;
		if (!instanceDatas->m_Instanced)
		{
			const bool		hasColors = !instanceDatas->m_Instance_Param_DiffuseColors.Empty();
			const bool		hasVATCursors = !instanceDatas->m_Instance_Param_VATCursors.Empty();
			const bool		hasDynParam0s = !instanceDatas->m_Instance_Param_DynamicParameter0.Empty();
			const bool		hasDynParam1s = !instanceDatas->m_Instance_Param_DynamicParameter1.Empty();
			const bool		hasDynParam2s = !instanceDatas->m_Instance_Param_DynamicParameter2.Empty();
			const u32		particlei = batchElement.UserIndex;

			if (PK_VERIFY(particlei < instanceDatas->m_Instance_Matrices.Count()))
			{
				const CFloat4x4		&matrix = instanceDatas->m_Instance_Matrices[particlei];

				shaderBindings.Add(Transform1, ToUE(matrix.XAxis()));
				shaderBindings.Add(Transform2, ToUE(matrix.YAxis()));
				shaderBindings.Add(Transform3, ToUE(matrix.ZAxis()));
				shaderBindings.Add(Transform4, ToUE(matrix.WAxis()));

				if (hasColors)
					shaderBindings.Add(ParticleColor, ToUE(instanceDatas->m_Instance_Param_DiffuseColors[particlei]));
				if (hasVATCursors)
					shaderBindings.Add(VATCursor, instanceDatas->m_Instance_Param_VATCursors[particlei]);
				if (hasDynParam0s)
					shaderBindings.Add(DynamicParameter0, ToUE(instanceDatas->m_Instance_Param_DynamicParameter0[particlei]));
				if (hasDynParam1s)
					shaderBindings.Add(DynamicParameter1, ToUE(instanceDatas->m_Instance_Param_DynamicParameter1[particlei]));
				if (hasDynParam2s)
					shaderBindings.Add(DynamicParameter2, ToUE(instanceDatas->m_Instance_Param_DynamicParameter2[particlei]));
			}
		}
#endif // (ENGINE_MINOR_VERSION < 25)
	}

private:
#	if (ENGINE_MINOR_VERSION < 25)
	// Used only when instancing is off (ES2)
	FShaderParameter	Transform1;
	FShaderParameter	Transform2;
	FShaderParameter	Transform3;
	FShaderParameter	Transform4;
	FShaderParameter	ParticleColor;
	FShaderParameter	VATCursor;
	FShaderParameter	DynamicParameter0;
	FShaderParameter	DynamicParameter1;
	FShaderParameter	DynamicParameter2;
	//FShaderParameter	DynamicParameter3;
#	endif // (ENGINE_MINOR_VERSION < 25)
};

//----------------------------------------------------------------------------
//
// FPopcornFXMeshVertexFactory
//
//----------------------------------------------------------------------------

IMPLEMENT_VERTEX_FACTORY_TYPE(FPopcornFXMeshVertexFactory, PKUE_SHADER_PATH("PopcornFXMeshVertexFactory"), true, false, true, false, false);

#if (ENGINE_MINOR_VERSION >= 25)
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXMeshVertexFactory, SF_Vertex, FPopcornFXMeshVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXMeshVertexFactory, SF_Compute, FPopcornFXMeshVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXMeshVertexFactory, SF_RayHitGroup, FPopcornFXMeshVertexFactoryShaderParameters);
#endif // (ENGINE_MINOR_VERSION >= 25)
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

#if (ENGINE_MINOR_VERSION >= 25)
bool	FPopcornFXMeshVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	return	(Parameters.MaterialParameters.bIsUsedWithMeshParticles || Parameters.MaterialParameters.bIsUsedWithNiagaraMeshParticles || Parameters.MaterialParameters.bIsSpecialEngineMaterial) &&
			Parameters.MaterialParameters.MaterialDomain == EMaterialDomain::MD_Surface;
}
#else
bool	FPopcornFXMeshVertexFactory::ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial *Material, const class FShaderType *ShaderType)
{
	return (Material->IsUsedWithMeshParticles() || Material->IsUsedWithNiagaraMeshParticles() || Material->IsSpecialEngineMaterial()) &&
			Material->GetMaterialDomain() == EMaterialDomain::MD_Surface;
}
#endif // (ENGINE_MINOR_VERSION >= 25)

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

#if (ENGINE_MINOR_VERSION < 25)
	const bool	bInstanced = GRHISupportsInstancing; // Supported everyone since 4.25
#endif // (ENGINE_MINOR_VERSION < 25)
	if (Data.bInitialized)
	{
		Streams.Empty();
#if (ENGINE_MINOR_VERSION < 25)
		if (bInstanced)
		{
#endif // (ENGINE_MINOR_VERSION < 25)
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
#if (ENGINE_MINOR_VERSION < 25)
		}
#endif // (ENGINE_MINOR_VERSION < 25)

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


#if (ENGINE_MINOR_VERSION < 25)
		if (bInstanced)
		{
#endif // (ENGINE_MINOR_VERSION < 25)
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
#if (ENGINE_MINOR_VERSION < 25)
		}
#endif // (ENGINE_MINOR_VERSION < 25)

		if (Streams.Num() > 0)
		{
			InitDeclaration(Elements);
			check(IsValidRef(GetDeclaration()));
		}
	}
}

//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION < 25)
// static
FVertexFactoryShaderParameters* FPopcornFXMeshVertexFactory::ConstructShaderParameters(EShaderFrequency shaderFrequency)
{
	if (shaderFrequency == SF_Vertex)
	{
		return new FPopcornFXMeshVertexFactoryShaderParameters();
	}
#if RHI_RAYTRACING
	else if (shaderFrequency == SF_Compute)
	{
		return new FPopcornFXMeshVertexFactoryShaderParameters();
	}
	else if (shaderFrequency == SF_RayHitGroup)
	{
		return new FPopcornFXMeshVertexFactoryShaderParameters();
	}
#endif // (RHI_RAYTRACING)
	return null;
}
#endif // (ENGINE_MINOR_VERSION < 25)

//----------------------------------------------------------------------------

// static
#if (ENGINE_MINOR_VERSION >= 25)
void	FPopcornFXMeshVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);
#else
void	FPopcornFXMeshVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryType* Type, EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(Type, Platform, Material, OutEnvironment);
#endif // (ENGINE_MINOR_VERSION >= 25)

	// @FIXME: it wont trigger recompilation !!
	OutEnvironment.SetDefine(TEXT("ENGINE_MINOR_VERSION"), ENGINE_MINOR_VERSION);

	// Dynamic parameters..
	OutEnvironment.SetDefine(TEXT("PARTICLE_FACTORY"),TEXT("1"));
	OutEnvironment.SetDefine(TEXT("NIAGARA_PARTICLE_FACTORY"), TEXT("1"));

	// Set a define so we can tell in MaterialTemplate.usf when we are compiling a mesh particle vertex factory
	OutEnvironment.SetDefine(TEXT("PARTICLE_MESH_FACTORY"), TEXT("1"));

#if (ENGINE_MINOR_VERSION >= 25)
	const bool	bInstanced = true; // Supported everywhere since 4.25
#else
	const bool	bInstanced = GRHISupportsInstancing;
#endif // (ENGINE_MINOR_VERSION >= 25)

	OutEnvironment.SetDefine(TEXT("PARTICLE_MESH_INSTANCED"), (uint32)(bInstanced ? 1 : 0));
}

#endif // UNUSED_PopcornFXMeshVertexFactory

//----------------------------------------------------------------------------
