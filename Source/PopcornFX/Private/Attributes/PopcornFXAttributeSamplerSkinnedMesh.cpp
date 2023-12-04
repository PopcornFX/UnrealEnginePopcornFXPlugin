//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplerSkinnedMesh.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXStats.h"
#include "PopcornFXAttributeList.h"
#include "PopcornFXAttributeSampler.h"
#include "PopcornFXCustomVersion.h"
#include "Assets/PopcornFXMesh.h"
#include "ClothingAsset.h"

#include "Components/SkinnedMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "StaticMeshResources.h"
#include "SkeletalRenderPublic.h"

#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"

#include "Rendering/SkinWeightVertexBuffer.h"

// Don't include the DestructibleComponent implementation header,
// relies on ApexDestruction plugin which isn't loaded when PopcornFX runtime
// module is loaded. Include the interface instead.
// #	include "DestructibleComponent.h"
#include "DestructibleInterface.h"

#include "Engine/SkeletalMesh.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers_shape.h>
#include <pk_geometrics/include/ge_mesh_resource.h>
#include <pk_geometrics/include/ge_mesh.h>
#include <pk_geometrics/include/ge_mesh_sampler_accel.h>
#include <pk_particles_toolbox/include/pt_mesh_deformers_skin.h>
#include <pk_kernel/include/kr_profiler.h>
#include <pk_kernel/include/kr_containers_onstack.h>

//----------------------------------------------------------------------------
//
// UE version wrappers
//
//----------------------------------------------------------------------------

namespace
{
	const TArray<FMatrix44f>	&SkeletalMeshRefBasesInvMatrices(const USkeletalMesh *skeletalMesh)
	{
		check(skeletalMesh != null);
#if ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5)
		return skeletalMesh->GetRefBasesInvMatrix();
#else
		return skeletalMesh->RefBasesInvMatrix;
#endif // ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5)
	}

	const FReferenceSkeleton	&SkeletalMeshRefSkeleton(const USkeletalMesh *skeletalMesh)
	{
		check(skeletalMesh != null);
#if ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5)
		return skeletalMesh->GetRefSkeleton();
#else
		return skeletalMesh->RefSkeleton;
#endif // ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5)
	}

	const TArray<UClothingAssetBase*>	&SkeletalMeshClothingAssets(const USkeletalMesh* skeletalMesh)
	{
		check(skeletalMesh != null);
#if ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5)
		return skeletalMesh->GetMeshClothingAssets();
#else
		return skeletalMesh->MeshClothingAssets;
#endif // ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5)
	}
}

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplerSkinned, Log, All);

#define	POPCORNFX_MAX_ANIM_IDLE_TIME	1.0f

struct FPopcornFXClothSection
{
	u32							m_BaseVertexOffset;
	u32							m_VertexCount;
	u32							m_ClothDataIndex;
	PopcornFX::TArray<u32>		m_Indices;
};

//----------------------------------------------------------------------------

struct FAttributeSamplerSkinnedMeshData
{
	bool	m_ShouldUpdateTransforms = false;
	bool	m_BoneVisibilityChanged = false;

	float	m_AccumulatedDts = 0.0f;

	PopcornFX::PMeshNew										m_Mesh;
	PopcornFX::PParticleSamplerDescriptor_Shape_Default		m_Desc;

	PopcornFX::TArray<CFloat4, PopcornFX::TArrayAligned16>	m_DstPositions;
	PopcornFX::TArray<CFloat4, PopcornFX::TArrayAligned16>	m_DstNormals;
	PopcornFX::TArray<CFloat4, PopcornFX::TArrayAligned16>	m_DstTangents;

	PopcornFX::TArray<CFloat4, PopcornFX::TArrayAligned16>	m_OldPositions;
	PopcornFX::TArray<CFloat4, PopcornFX::TArrayAligned16>	m_DstVelocities;

	PopcornFX::TArray<u8, PopcornFX::TArrayAligned16>		m_BoneIndices; // Only if hasMasterPose

	PopcornFX::TArray<CFloat4x4, PopcornFX::TArrayAligned16>	m_BoneInverseMatrices;

	PopcornFX::TArray<FPopcornFXClothSection>	m_ClothSections;
	TMap<int32, FClothSimulData>				m_ClothSimDataCopy;
	FMatrix44f										m_InverseTransforms;

	PopcornFX::CBaseSkinningStreams				*m_SkinningStreamsProxy = null;
	PopcornFX::SSkinContext						m_SkinContext;
	PopcornFX::CSkinAsyncContext				m_AsyncSkinContext;
	PopcornFX::CSkeletonView					*m_SkeletonView = null;

	PopcornFX::SSamplerSourceOverride			m_Override;

	PopcornFX::CDiscreteProbabilityFunction1D_O1::SWorkingBuffers	m_OverrideSurfaceSamplingWorkingBuffers;
	PopcornFX::CMeshSurfaceSamplerStructuresRandom					m_OverrideSurfaceSamplingAccelStructs;

	TWeakObjectPtr<USkinnedMeshComponent>		m_CurrentSkinnedMeshComponent = null;
};

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerSkinnedMesh::UPopcornFXAttributeSamplerSkinnedMesh(const FObjectInitializer &PCIP)
	: Super(PCIP)
	, m_LastFrameUpdate(0)
{
	bAutoActivate = true;

	// Default skinned mesh only builds positions
	bPauseSkinning = false;
	bSkinPositions = true;
	bSkinNormals = false;
	bSkinTangents = false;
	bBuildColors = false;
	bBuildUVs = false;
	bComputeVelocities = false;
	bBuildClothData = false;

	// We don't know the target yet, TickGroup will be set at ResolveSkinnedMeshComponent
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;

	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Shape;

	TargetActor = null;
	bUseSkinnedMeshTransforms_DEPRECATED = false;
	bUseRelativeTransform_DEPRECATED = true;
	Transforms = EPopcornFXSkinnedTransforms::AttrSamplerRelativeTr;
	bApplyScale = false;
#if WITH_EDITORONLY_DATA
	bEditorBuildInitialPose = false;
#endif // WITH_EDITORONLY_DATA

	m_Data = new FAttributeSamplerSkinnedMeshData();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerSkinnedMesh::BeginDestroy()
{
	Super::BeginDestroy();
	if (m_Data != null)
	{
		Clear();
		delete m_Data;
		m_Data = null;
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerSkinnedMesh::Clear()
{
	PK_ASSERT(m_Data != null);

	PK_SAFE_DELETE(m_Data->m_SkeletonView);
	PK_SAFE_DELETE(m_Data->m_SkinningStreamsProxy);

	m_Data->m_DstPositions.Clear();
	m_Data->m_DstNormals.Clear();
	m_Data->m_DstTangents.Clear();
	m_Data->m_DstVelocities.Clear();
	m_Data->m_OldPositions.Clear();
	m_Data->m_BoneInverseMatrices.Clear();

	m_Data->m_ClothSections.Clear();
	m_Data->m_ClothSimDataCopy.Empty();

	m_Data->m_ShouldUpdateTransforms = false;
	m_Data->m_BoneVisibilityChanged = false;

	m_Data->m_AccumulatedDts = 0.0f;

	m_Data->m_SkinContext.m_SrcPositions = TStridedMemoryView<const CFloat3>();
	m_Data->m_SkinContext.m_SrcNormals = TStridedMemoryView<const CFloat3>();
	m_Data->m_SkinContext.m_SrcTangents = TStridedMemoryView<const CFloat3>();

	m_Data->m_SkinContext.m_DstPositions = TStridedMemoryView<CFloat3>();
	m_Data->m_SkinContext.m_DstNormals = TStridedMemoryView<CFloat3>();
	m_Data->m_SkinContext.m_DstTangents = TStridedMemoryView<CFloat4>();

	m_Data->m_Override.m_PositionsOverride = TStridedMemoryView<const CFloat3>();
	m_Data->m_Override.m_NormalsOverride = TStridedMemoryView<const CFloat3>();
	m_Data->m_Override.m_TangentsOverride = TStridedMemoryView<const CFloat4>();

	if (m_Data->m_CurrentSkinnedMeshComponent != null)
	{
		RemoveTickPrerequisiteComponent(m_Data->m_CurrentSkinnedMeshComponent.Get());
		m_Data->m_CurrentSkinnedMeshComponent = null;
	}
	PrimaryComponentTick.RemovePrerequisite(GetWorld(), GetWorld()->EndPhysicsTickFunction);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerSkinnedMesh::Skin_PreProcess(uint32 vertexStart, uint32 vertexCount, const PopcornFX::SSkinContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::Skin_PreProcess", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(m_Data != null);
	PK_ASSERT(vertexStart + vertexCount <= m_Data->m_DstPositions.Count());
	PK_ASSERT(m_Data->m_DstPositions.Count() == m_Data->m_OldPositions.Count());
	PK_ASSERT(m_Data->m_DstPositions.Count() == m_Data->m_DstVelocities.Count());

	PopcornFX::TStridedMemoryView<const CFloat3>	src = ctx.m_DstPositions.Slice(vertexStart, vertexCount);

	PopcornFX::TStridedMemoryView<CFloat3>	dst = PopcornFX::TStridedMemoryView<CFloat3>(reinterpret_cast<CFloat3*>(m_Data->m_OldPositions.RawDataPointer()), m_Data->m_OldPositions.Count(), 16).Slice(vertexStart, vertexCount);

	PK_ASSERT(src.Stride() == 0x10 && dst.Stride() == 0x10);
	PopcornFX::Mem::Copy(dst.Data(), src.Data(), dst.Count() * dst.Stride());
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerSkinnedMesh::Skin_PostProcess(uint32 vertexStart, uint32 vertexCount, const PopcornFX::SSkinContext &ctx)
{
	PK_ASSERT(m_Data != null);
	PK_ASSERT(m_Data->m_Mesh != null);

	if (bBuildClothData && !m_Data->m_ClothSections.Empty())
		FetchClothData(vertexStart, vertexCount);
	if (!bComputeVelocities)
		return;
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::Skin_PostProcess", POPCORNFX_UE_PROFILER_COLOR);

	PopcornFX::TStridedMemoryView<const CFloat3>	posCur = ctx.m_DstPositions.Slice(vertexStart, vertexCount);
	PopcornFX::TStridedMemoryView<const CFloat3>	posOld = PopcornFX::TStridedMemoryView<CFloat3>(reinterpret_cast<CFloat3*>(m_Data->m_OldPositions.RawDataPointer()), m_Data->m_OldPositions.Count(), 16).Slice(vertexStart, vertexCount);
	PopcornFX::TStridedMemoryView<CFloat3>			vel = PopcornFX::TStridedMemoryView<CFloat3>(reinterpret_cast<CFloat3*>(m_Data->m_DstVelocities.RawDataPointer()), m_Data->m_DstVelocities.Count(), 16).Slice(vertexStart, vertexCount);

	if (vel.Empty())
		return;
	if (m_Data->m_AccumulatedDts >= POPCORNFX_MAX_ANIM_IDLE_TIME)
	{
		PopcornFX::Mem::Clear(vel.Data(), vel.CoveredBytes());
		return;
	}
	PK_ASSERT(posCur.Stride() == 0x10);
	PK_ASSERT(posOld.Stride() == 0x10);
	PK_ASSERT(vel.Stride() == 0x10);

	CFloat3			* __restrict dstVelPtr = vel.Data();
	const CFloat3	*curPosPtr = posCur.Data();
	const CFloat3	*oldPosPtr = posOld.Data();

	PK_ASSERT(PopcornFX::Mem::IsAligned<0x10>(dstVelPtr));
	PK_ASSERT(PopcornFX::Mem::IsAligned<0x10>(curPosPtr));
	PK_ASSERT(PopcornFX::Mem::IsAligned<0x10>(oldPosPtr));

	const float				invDt = 1.0f / m_Data->m_AccumulatedDts;
	const VectorRegister4f	iDt = MakeVectorRegister(invDt, invDt, invDt, invDt);

	for (u32 iVertex = 0; iVertex < vertexCount; ++iVertex)
	{
		const VectorRegister4f	curPos = VectorLoad(reinterpret_cast<const FVector4f*>(curPosPtr));
		const VectorRegister4f	oldPos = VectorLoad(reinterpret_cast<const FVector4f*>(oldPosPtr));
		const VectorRegister4f	v = VectorMultiply(VectorSubtract(curPos, oldPos), iDt);

		VectorStore(v, reinterpret_cast<FVector4f*>(dstVelPtr));

		dstVelPtr = PopcornFX::Mem::AdvanceRawPointer(dstVelPtr, 0x10);
		curPosPtr = PopcornFX::Mem::AdvanceRawPointer(curPosPtr, 0x10);
		oldPosPtr = PopcornFX::Mem::AdvanceRawPointer(oldPosPtr, 0x10);
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerSkinnedMesh::Skin_Finish(const PopcornFX::SSkinContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::Skin_Finish", POPCORNFX_UE_PROFILER_FAST_COLOR);

	PK_ASSERT(m_Data != null);
	PK_ASSERT(m_Data->m_Mesh != null);

	// Rebuild sampling structures if bone visibility array has changed
	if (!m_Data->m_BoneVisibilityChanged)
		return;
	if (!PK_VERIFY(bSkinPositions))
	{
		// @TODO : warn the user that distribution will be invalid if he only checked bSkinNormals or bSkinTangents
		// and he either is sampling a destructible mesh or the target skinned mesh component got one of its bone hidden/unhidden
		return;
	}

	bool rebuildAccelStruct = false;

	if (MeshSamplingMode == EPopcornFXMeshSamplingMode::Type::Weighted)
		rebuildAccelStruct = m_Data->m_Mesh->SetupSurfaceSamplingAccelStructs(0, DensityColorChannel, m_Data->m_OverrideSurfaceSamplingAccelStructs, &m_Data->m_OverrideSurfaceSamplingWorkingBuffers);
	else
		rebuildAccelStruct = m_Data->m_OverrideSurfaceSamplingAccelStructs.Build(m_Data->m_Mesh->TriangleBatch().m_IStream, m_Data->m_SkinContext.m_DstPositions, &m_Data->m_OverrideSurfaceSamplingWorkingBuffers);

	if (!PK_VERIFY(rebuildAccelStruct))
	{
		// @TODO : warn the user that it couldn't rebuild accel strucs
	}
}

//----------------------------------------------------------------------------

namespace	PopcornFXAttributeSamplers
{
	// Simpler solution to VectorTransformVector (assumes W == 1)
	VectorRegister4f	_TransformVector(VectorRegister4f &v, const FMatrix44f *m)
	{
		const VectorRegister4f	*M = (const VectorRegister4f*)m;
		VectorRegister4f		x, y, z;

		x = VectorReplicate(v, 0);
		y = VectorReplicate(v, 1);
		z = VectorReplicate(v, 2);

		x = VectorMultiply(x, M[0]);
		y = VectorMultiply(y, M[1]);
		z = VectorMultiply(z, M[2]);

		x = VectorAdd(x, y);
		z = VectorAdd(z, M[3]);
		x = VectorAdd(x, z);

		return x;
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerSkinnedMesh::FetchClothData(uint32 vertexStart, uint32 vertexCount)
{
	// Done during skinning job's PostProcess callback
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::FetchClothData", POPCORNFX_UE_PROFILER_COLOR);

	const TMap<int32, FClothSimulData>	&clothData = m_Data->m_ClothSimDataCopy;
	if (clothData.Num() == 0)
		return;

	const float			invScale = FPopcornFXPlugin::GlobalScaleRcp();
	const FMatrix44f	localM = m_Data->m_InverseTransforms * invScale;
	for (u32 iSection = 0; iSection < m_Data->m_ClothSections.Count(); ++iSection)
	{
		const FPopcornFXClothSection	&section = m_Data->m_ClothSections[iSection];

		const u32	baseVertexOffset = section.m_BaseVertexOffset;
		if (baseVertexOffset > vertexStart + vertexCount ||
			baseVertexOffset + section.m_VertexCount <= vertexStart)
			continue;
		if (!PK_VERIFY(clothData.Contains(section.m_ClothDataIndex)))
			continue;
		{
			PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::FetchClothData::Section", POPCORNFX_UE_PROFILER_COLOR);

			const FClothSimulData	&data = clothData[section.m_ClothDataIndex];
			PK_ASSERT(data.Positions.Num() > 0);
			PK_ASSERT(data.Positions.Num() == data.Normals.Num());
			PK_ASSERT(section.m_VertexCount == section.m_Indices.Count());

			const u32	realVertexStart = PopcornFX::PKMax(vertexStart, baseVertexOffset);
			const u32	realVertexEnd = PopcornFX::PKMin(baseVertexOffset + section.m_VertexCount, vertexStart + vertexCount);
			const u32	realVertexCount = realVertexEnd - realVertexStart;
			const u32	indicesStart = realVertexStart - baseVertexOffset;

			const FVector3f	*srcPositions = data.Positions.GetData();
			const FVector3f	*srcNormals = data.Normals.GetData();
			const u32		*srcIndices = section.m_Indices.RawDataPointer() + indicesStart;

			CFloat4		_dummyNormal[1];
			CFloat4		*dstPositions = m_Data->m_DstPositions.RawDataPointer() + realVertexStart;
			CFloat4		*dstNormals = bSkinNormals ? m_Data->m_DstPositions.RawDataPointer() + realVertexStart : _dummyNormal;

			const u32	dstStride = bSkinNormals ? 0x10 : 0;

			for (u32 iVertex = 0; iVertex < realVertexCount; ++iVertex)
			{
				const u32	simIndex = srcIndices[iVertex];

				VectorRegister4f		srcPos = VectorLoadFloat3(srcPositions + simIndex);
				const VectorRegister4f	srcNormal = VectorLoadFloat3(srcNormals + simIndex);

				srcPos = PopcornFXAttributeSamplers::_TransformVector(srcPos, &localM);

				VectorStore(srcPos, reinterpret_cast<FVector4f*>(dstPositions));
				VectorStore(srcNormal, reinterpret_cast<FVector4f*>(dstNormals));

				dstPositions = PopcornFX::Mem::AdvanceRawPointer(dstPositions, 0x10);
				dstNormals = PopcornFX::Mem::AdvanceRawPointer(dstNormals, dstStride);
			}
		}
	}
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerSkinnedMesh::Rebuild()
{
	return BuildInitialPose();
}

//----------------------------------------------------------------------------
#if WITH_EDITOR

void	UPopcornFXAttributeSamplerSkinnedMesh::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != NULL)
	{
		if (PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerSkinnedMesh, bEditorBuildInitialPose) ||
			PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerSkinnedMesh, TargetActor) ||
			PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerSkinnedMesh, SkinnedMeshComponentName))
		{
			bEditorBuildInitialPose = false;
			BuildInitialPose();
		}
		else if (PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerSkinnedMesh, bSkinPositions) ||
				 PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerSkinnedMesh, bSkinNormals) ||
				 PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerSkinnedMesh, bSkinTangents) ||
				 PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerSkinnedMesh, bBuildColors) ||
				 PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerSkinnedMesh, bBuildUVs) ||
				 PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerSkinnedMesh, bBuildClothData))
		{
			if (!bSkinPositions)
				bComputeVelocities = false;
			BuildInitialPose();
		}
		else if (PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerSkinnedMesh, bComputeVelocities))
		{
			if (bComputeVelocities)
				bSkinPositions = true;
			BuildInitialPose();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif // WITH_EDITOR
//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerSkinnedMesh::_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	check(m_Data != null);

	const PopcornFX::CResourceDescriptor_Shape	*defaultShapeSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Shape>(defaultSampler);
	if (!PK_VERIFY(defaultShapeSampler != null))
		return null;
	bPauseSkinning = false;
	if (m_Data->m_Mesh == null)
	{
		if (!BuildInitialPose())
			return null;
	}

	if (m_Data->m_Desc == null)
		m_Data->m_Desc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_Shape_Default());
	PopcornFX::CParticleSamplerDescriptor_Shape_Default	*shapeDesc = m_Data->m_Desc.Get();
	if (!PK_VERIFY(shapeDesc != null))
		return null;

	PopcornFX::CShapeDescriptor_Mesh	*descMesh = PK_NEW(PopcornFX::CShapeDescriptor_Mesh(m_Data->m_Mesh));
	if (!PK_VERIFY(descMesh != null))
		return null;

	descMesh->SetSamplingStructs(&m_Data->m_OverrideSurfaceSamplingAccelStructs, null);
	descMesh->SetMesh(m_Data->m_Mesh, &(m_Data->m_Override));

	shapeDesc->m_Shape = descMesh;
	shapeDesc->m_WorldTr_Current =	&_Reinterpret<const CFloat4x4>(m_WorldTr_Current);
	shapeDesc->m_WorldTr_Previous = &_Reinterpret<CFloat4x4>(m_WorldTr_Previous);
	shapeDesc->m_Angular_Velocity = &_Reinterpret<const CFloat3>(m_Angular_Velocity);
	shapeDesc->m_Linear_Velocity =	&_Reinterpret<CFloat3>(m_Linear_Velocity);

	UpdateTransforms();

	desc.m_NeedUpdate = true;

	return shapeDesc;
}

//----------------------------------------------------------------------------

USkinnedMeshComponent	*UPopcornFXAttributeSamplerSkinnedMesh::ResolveSkinnedMeshComponent()
{
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::ResolveSkinnedMeshComponent", POPCORNFX_UE_PROFILER_FAST_COLOR);

	AActor	*fallbackActor = GetOwner();
	if (!PK_VERIFY(fallbackActor != null))
		return null;
	const AActor			*parent = TargetActor == null ? fallbackActor : TargetActor;
	USkinnedMeshComponent	*skinnedMesh = null;
	if (SkinnedMeshComponentName != NAME_None)
	{
		FObjectPropertyBase	*prop = FindFProperty<FObjectPropertyBase>(parent->GetClass(), SkinnedMeshComponentName);

		if (prop != null)
			skinnedMesh = Cast<USkinnedMeshComponent>(prop->GetObjectPropertyValue_InContainer(parent));
	}
	else
	{
		skinnedMesh = Cast<USkinnedMeshComponent>(parent->GetRootComponent());
	}
	if (skinnedMesh == null)
	{
		const bool	enableLogOnError = true;
		if (enableLogOnError)
		{
			// always log, must have a USkinnedMeshComponent or useless
			UE_LOG(LogPopcornFXAttributeSamplerSkinned, Warning,
				TEXT("Could not find component 'USkinnedMeshComponent %s.%s' for UPopcornFXAttributeSamplerSkinnedMesh '%s'"),
				*parent->GetName(), (SkinnedMeshComponentName != NAME_None ? *SkinnedMeshComponentName.ToString() : TEXT("RootComponent")),
				*GetFullName());
		}
		return null;
	}
	return skinnedMesh;
}

//----------------------------------------------------------------------------

static CFloat3	ToPk(const FPackedNormal &packedNormal)
{
	FVector3f			unpackedVector;
	VectorRegister4f	normal = packedNormal.GetVectorRegister();
	VectorStoreFloat3(normal, &unpackedVector);
	return ToPk(unpackedVector);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerSkinnedMesh::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FPopcornFXCustomVersion::GUID);

	if (Ar.IsLoading())
	{
		// Fix old bool parameters to enum
		const int32		version = GetLinkerCustomVersion(FPopcornFXCustomVersion::GUID);
		if (version < FPopcornFXCustomVersion::AttributeSamplers_ChangeTransforms)
		{
			if (bUseSkinnedMeshTransforms_DEPRECATED)
			{
				if (bUseRelativeTransform_DEPRECATED)
					Transforms = EPopcornFXSkinnedTransforms::SkinnedComponentRelativeTr;
				else
					Transforms = EPopcornFXSkinnedTransforms::SkinnedComponentWorldTr;
			}
			else
			{
				if (bUseRelativeTransform_DEPRECATED)
					Transforms = EPopcornFXSkinnedTransforms::AttrSamplerRelativeTr;
				else
					Transforms = EPopcornFXSkinnedTransforms::AttrSamplerWorldTr;
			}
			UE_LOG(LogPopcornFXAttributeSamplerSkinned, Log, TEXT("Upgraded from UseSkinnedMeshTransforms:%d UseRelativeTransform:%d to Transforms:%d"), bUseSkinnedMeshTransforms_DEPRECATED, bUseRelativeTransform_DEPRECATED, int(Transforms));
		}
	}
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerSkinnedMesh::SetComponentTickingGroup(USkinnedMeshComponent *skinnedMesh)
{
	if (Cast<USkeletalMeshComponent>(skinnedMesh) != null)
	{
		// Bone transforms are executed in the PrePhysics tick group,
		// So we set skinned mesh attr sampler components to be ticked in the same ticking group,
		// And this component will have the target skinned mesh component as prerequisite
		PrimaryComponentTick.TickGroup = TG_PrePhysics;
		AddTickPrerequisiteComponent(skinnedMesh);
	}
	else if (Cast<IDestructibleInterface>(skinnedMesh) != null)
	{
		// Transforms are updated in TG_EndPhysics after APEX update for Destructible components
		PrimaryComponentTick.TickGroup = TG_EndPhysics;
		PrimaryComponentTick.AddPrerequisite(GetWorld(), GetWorld()->EndPhysicsTickFunction);
	}
	else
	{
		// We don't handle PoseableMesh, yet ?
		PK_ASSERT_NOT_REACHED();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

namespace
{
	const TArray<int32>		&GetMasterBoneMap(const USkinnedMeshComponent *skinnedMesh)
	{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		return skinnedMesh->GetLeaderBoneMap();
#else
		return skinnedMesh->GetMasterBoneMap();
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	}

	const TArray<FSkelMeshRenderSection>	&GetSections(const FSkeletalMeshLODRenderData *lodRenderData)
	{
		return lodRenderData->RenderSections;
	}

	const TArray<FTransform>	&GetComponentSpaceTransforms(const USkinnedMeshComponent *comp)
	{
		return comp->GetComponentSpaceTransforms();
	}

	enum
	{
		None = 0,
		Build_Positions = 0x1,
		Build_Normals = 0x2,
		Build_Tangents = 0x4,
		Build_Cloth = 0x8,
	};

	struct	SBuildSkinnedMesh
	{
		USkinnedMeshComponent				*m_SkinnedMeshComponent = null;
		USkeletalMesh						*m_SkeletalMesh = null;
		const FSkeletalMeshLODRenderData	*m_LODRenderData = null;

		u32		m_TotalVertexCount;

		u32		m_MaxInfluenceCount = 0;
		u32		m_TotalBoneCount;
		bool	m_SmallBoneIndices;

		u32		m_BuildFlags = 0;

		bool	Initialize(USkinnedMeshComponent *skinnedMesh)
		{
			PK_NAMEDSCOPEDPROFILE_C("InitBuildDesc", POPCORNFX_UE_PROFILER_COLOR);

			m_SkinnedMeshComponent = skinnedMesh;
			if (m_SkinnedMeshComponent == null)
				return false;
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
			m_SkeletalMesh = Cast<USkeletalMesh>(m_SkinnedMeshComponent->GetSkinnedAsset());
#else
			m_SkeletalMesh = m_SkinnedMeshComponent->SkeletalMesh;
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
			if (m_SkeletalMesh == null)
				return false;

			const FSkeletalMeshRenderData	*skelMeshRenderData = m_SkinnedMeshComponent->GetSkeletalMeshRenderData();
			if (!PK_VERIFY(skelMeshRenderData != null && skelMeshRenderData->LODRenderData.Num() > 0))
				return false;
			m_LODRenderData = &skelMeshRenderData->LODRenderData[0];

			m_TotalBoneCount = m_LODRenderData->RequiredBones.Num();
			if (!PK_VERIFY(m_TotalBoneCount > 0))
				return false;
			m_SmallBoneIndices = m_TotalBoneCount <= 256;

			m_TotalVertexCount = m_LODRenderData->GetNumVertices();

			if (!PK_VERIFY(m_TotalVertexCount > 0))
				return false;
			return true;
		}
	};

	template<typename _IndexType>
	bool	_FillBuffers(FAttributeSamplerSkinnedMeshData *data, SBuildSkinnedMesh &buildDesc, PopcornFX::CMeshVStream &vstream, PopcornFX::CBaseSkinningStreams *skinningStreams)
	{
		PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::FillBuffers", POPCORNFX_UE_PROFILER_COLOR);

		const TStridedMemoryView<CFloat3>	srcPositionsView = vstream.Positions();

		const PopcornFX::TMemoryView<const _IndexType>	srcBoneIndices(skinningStreams->IndexStream<_IndexType>(), skinningStreams->Count());
		const PopcornFX::TMemoryView<const float>		srcBoneWeights(skinningStreams->WeightStream(), skinningStreams->Count());

		_IndexType							* __restrict boneIndices = reinterpret_cast<_IndexType*>(data->m_BoneIndices.RawDataPointer());

		const bool	skin = (buildDesc.m_BuildFlags & (Build_Positions | Build_Tangents | Build_Normals)) != 0;
		if (skin)
		{
			PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::Copy src positions/normals/tangents", POPCORNFX_UE_PROFILER_COLOR);
			PK_ASSERT(srcPositionsView.Stride() == data->m_DstPositions.Stride());

			if (buildDesc.m_BuildFlags & Build_Positions)
			{
				if (!PK_VERIFY(srcPositionsView.Count() == data->m_DstPositions.Count()))
					return false;
				data->m_SkinContext.m_SrcPositions = srcPositionsView;
				PopcornFX::Mem::Copy(data->m_DstPositions.RawDataPointer(), srcPositionsView.Data(), sizeof(CFloat4) * srcPositionsView.Count());
			}
			if (buildDesc.m_BuildFlags & Build_Normals)
			{
				const TStridedMemoryView<CFloat3>	srcNormalsView = vstream.Normals();
				if (!PK_VERIFY(srcNormalsView.Count() == data->m_DstNormals.Count()))
					return false;
				PK_ASSERT(srcNormalsView.Stride() == data->m_DstNormals.Stride());
				data->m_SkinContext.m_SrcNormals = srcNormalsView;
				PopcornFX::Mem::Copy(data->m_DstNormals.RawDataPointer(), srcNormalsView.Data(), sizeof(CFloat4) * srcNormalsView.Count());
			}
			if (buildDesc.m_BuildFlags & Build_Tangents)
			{
				const TStridedMemoryView<CFloat3>	srcTangentsView = TStridedMemoryView<CFloat3>::Reinterpret(vstream.Tangents());
				if (!PK_VERIFY(srcTangentsView.Count() == data->m_DstTangents.Count()))
					return false;
				PK_ASSERT(srcTangentsView.Stride() == data->m_DstTangents.Stride());
				data->m_SkinContext.m_SrcTangents = srcTangentsView;
				PopcornFX::Mem::Copy(data->m_DstTangents.RawDataPointer(), srcTangentsView.Data(), sizeof(CFloat4) * srcTangentsView.Count());
			}
		}

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		const USkinnedMeshComponent	*masterPoseComponent = buildDesc.m_SkinnedMeshComponent->LeaderPoseComponent.Get();
#else
		const USkinnedMeshComponent	*masterPoseComponent = buildDesc.m_SkinnedMeshComponent->MasterPoseComponent.Get();
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		const bool					hasMasterPoseComponent = masterPoseComponent != null;
		if (hasMasterPoseComponent)
		{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
			PK_ASSERT(GetMasterBoneMap(buildDesc.m_SkinnedMeshComponent).Num() == SkeletalMeshRefSkeleton(Cast<USkeletalMesh>(buildDesc.m_SkinnedMeshComponent->GetSkinnedAsset())).GetNum());
#else
			PK_ASSERT(GetMasterBoneMap(buildDesc.m_SkinnedMeshComponent).Num() == SkeletalMeshRefSkeleton(buildDesc.m_SkinnedMeshComponent->SkeletalMesh).GetNum());
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		}

		PK_ONLY_IF_ASSERTS(u32 totalVertices = 0);

		const float		scale = FPopcornFXPlugin::GlobalScale();
		const float		eSq = KINDA_SMALL_NUMBER * KINDA_SMALL_NUMBER;

		// Pack the vertices bone weights/indices from sparse sections
		const u32		sectionCount = GetSections(buildDesc.m_LODRenderData).Num();
		for (u32 iSection = 0; iSection < sectionCount; ++iSection)
		{
			PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::FillBuffers (Build section)", POPCORNFX_UE_PROFILER_COLOR);

			const FSkelMeshRenderSection	&section = GetSections(buildDesc.m_LODRenderData)[iSection];
			PK_ONLY_IF_ASSERTS(const u32 numSectionVertices = section.GetNumVertices());

			const u32	sectionOffset = section.BaseVertexIndex;
			const u32	numVertices = section.GetNumVertices();
			const u32	sectionInfluenceCount = section.MaxBoneInfluences;

			FPopcornFXClothSection		clothSection;
			const bool					validClothSection = (buildDesc.m_BuildFlags & Build_Cloth) && section.HasClothingData() && skin;

			bool									buildClothIndices = false;
			PopcornFX::TMemoryView<const FVector3f>	clothVertices;
			if (validClothSection)
			{
				PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::Section - cloth", POPCORNFX_UE_PROFILER_COLOR);
				const s32	clothingAssetIndex = buildDesc.m_SkeletalMesh->GetClothingAssetIndex(section.ClothingData.AssetGuid);
				if (PK_VERIFY(clothingAssetIndex != INDEX_NONE))
				{
					UClothingAssetCommon	*clothAsset = Cast<UClothingAssetCommon>(SkeletalMeshClothingAssets(buildDesc.m_SkeletalMesh)[clothingAssetIndex]);

					if (clothAsset != null && clothAsset->LodData.Num() > 0)
					{
						const FClothLODDataCommon			&lodData = clothAsset->LodData[0];
						const FClothPhysicalMeshData		&physMeshData = lodData.PhysicalMeshData;

						clothVertices = PopcornFX::TMemoryView<const FVector3f>(physMeshData.Vertices.GetData(), physMeshData.Vertices.Num());

						if (PK_VERIFY(clothSection.m_Indices.Resize(numVertices)))
						{
							buildClothIndices = true;
							clothSection.m_BaseVertexOffset = section.BaseVertexIndex;
							clothSection.m_ClothDataIndex = clothingAssetIndex;
							clothSection.m_VertexCount = numVertices;
						}
					}
					else
					{
						UE_LOG(LogPopcornFXAttributeSamplerSkinned, Warning, TEXT("Couldn't build cloth data for asset '%s', section %d: No cloth LOD data available"), *buildDesc.m_SkeletalMesh->GetName(), iSection);
					}
				}
			}

			if (skin && (buildClothIndices || hasMasterPoseComponent))
			{
				PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::Section - Bindpose", POPCORNFX_UE_PROFILER_COLOR);

				for (u32 iVertex = 0; iVertex < numVertices; ++iVertex)
				{
					const u32	offsetNoInfluences = sectionOffset + iVertex;
					PK_ASSERT(offsetNoInfluences < buildDesc.m_TotalVertexCount);

					if (buildClothIndices)
					{
						// Expensive step, but necessary to have less runtime overhead
						s32				index = INDEX_NONE;
						const u32		clothVertexCount = clothVertices.Count();
						const FVector3f	pos = ToUE(srcPositionsView[offsetNoInfluences]) * scale;
						for (u32 iClothVertex = 0; iClothVertex < clothVertexCount; ++iClothVertex)
						{
							if ((clothVertices[iClothVertex] - pos).SizeSquared() <= eSq)
							{
								index = iClothVertex;
								break;
							}
						}
						buildClothIndices = index != INDEX_NONE;
						if (buildClothIndices)
							clothSection.m_Indices[iVertex] = index;
						else
						{
							UE_LOG(LogPopcornFXAttributeSamplerSkinned, Warning, TEXT("Couldn't build cloth LUT for asset '%s', section %d: Legacy APEX cloth assets not supported"), *buildDesc.m_SkeletalMesh->GetName(), iSection);
						}
					}
					if (validClothSection) // Avoid skinning streams altogether
						continue;

					if (hasMasterPoseComponent)
					{
						const u32	offsetInfluences = offsetNoInfluences * buildDesc.m_MaxInfluenceCount;
						PK_ASSERT(offsetInfluences + buildDesc.m_MaxInfluenceCount <= srcBoneWeights.Count());
						for (u32 iInfluence = 0; iInfluence < sectionInfluenceCount; ++iInfluence)
						{
							const _IndexType	sectionBoneIndex = srcBoneIndices[offsetInfluences + iInfluence];
							const _IndexType	masterBoneMapIndex = GetMasterBoneMap(buildDesc.m_SkinnedMeshComponent)[sectionBoneIndex];
							PK_ASSERT(masterBoneMapIndex >= 0x0 && masterBoneMapIndex <= 0xFF);

							boneIndices[offsetInfluences + iInfluence] = masterBoneMapIndex;

							// We can stop copying data, weights are sorted
							if (srcBoneWeights[offsetInfluences + iInfluence] == 0.0f)
								break;
						}
					}
				}
			}
			if (buildClothIndices)
			{
				PK_PARANOID_ASSERT(validClothSection);
				data->m_ClothSections.PushBack(clothSection);
			}

			PK_ONLY_IF_ASSERTS(totalVertices += numSectionVertices);
		}
		PK_ASSERT(totalVertices == buildDesc.m_TotalVertexCount);
		return true;
	}

} // namespace

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerSkinnedMesh::BuildInitialPose()
{
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::BuildInitialPose", POPCORNFX_UE_PROFILER_COLOR);

	check(m_Data != null);
	Clear();

	PK_TODO("Big endian");
#if !PLATFORM_LITTLE_ENDIAN
	PK_ASSERT_NOT_REACHED();
	return false;
#endif

	SBuildSkinnedMesh	buildDesc;
	if (!buildDesc.Initialize(ResolveSkinnedMeshComponent()))
		return false;

	if (buildDesc.m_LODRenderData->SkinWeightVertexBuffer.GetBoneInfluenceType() == UnlimitedBoneInfluence)
	{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		UE_LOG(LogPopcornFXAttributeSamplerSkinned, Warning, TEXT("Cannot build mesh '%s' for sampling: Unlimited bone influences not supported for sampling"), *buildDesc.m_SkinnedMeshComponent->GetSkinnedAsset()->GetName());
#else
		UE_LOG(LogPopcornFXAttributeSamplerSkinned, Warning, TEXT("Cannot build mesh '%s' for sampling: Unlimited bone influences not supported for sampling"), *buildDesc.m_SkinnedMeshComponent->SkeletalMesh->GetName());
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		return false;
	}

	UPopcornFXMesh		*pkMesh = UPopcornFXMesh::FindSkeletalMesh(buildDesc.m_SkeletalMesh);
	if (!PK_VERIFY(pkMesh != null))
		return false;

	PopcornFX::PResourceMesh	meshRes = pkMesh->LoadResourceMeshIFN(true);
	if (meshRes == null ||
		meshRes->BatchList().Count() != 1 ||
		meshRes->BatchList().First() == null)
	{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		UE_LOG(LogPopcornFXAttributeSamplerSkinned, Warning, TEXT("Cannot build mesh '%s' for sampling: PopcornFX mesh was not built"), *buildDesc.m_SkinnedMeshComponent->GetSkinnedAsset()->GetName());
#else
		UE_LOG(LogPopcornFXAttributeSamplerSkinned, Warning, TEXT("Cannot build mesh '%s' for sampling: PopcornFX mesh was not built"), *buildDesc.m_SkinnedMeshComponent->SkeletalMesh->GetName());
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		return false;
	}

	PopcornFX::PMeshNew				meshNew = meshRes->BatchList()[0]->RawMesh();
	if (!PK_VERIFY(meshNew != null))
		return false;
	PopcornFX::CBaseSkinningStreams	*skinningStreams = meshRes->BatchList()[0]->m_OptimizedStreams;
	if (!PK_VERIFY(skinningStreams != null))
		return false;

	PopcornFX::CMeshTriangleBatch	&triBatch = meshNew->TriangleBatch();
	PopcornFX::CMeshVStream			&vstream = triBatch.m_VStream;
	PopcornFX::CMeshIStream			&istream = triBatch.m_IStream;

	PopcornFX::TMemoryView<const float>		srcBoneWeights(skinningStreams->WeightStream(), skinningStreams->Count());
	PK_ASSERT(skinningStreams->VertexCount() == buildDesc.m_TotalVertexCount);

	// TODO: We have to make sure those streams remain valid while this attr sampler is active: Reset when src .uasset is removed?

	const bool	skin = bSkinPositions || bSkinNormals || bSkinTangents;
	{
		PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::BuildMeshVertexDecl", POPCORNFX_UE_PROFILER_COLOR);

		if (bSkinPositions)
		{
			if (!PK_VERIFY(m_Data->m_DstPositions.Resize(buildDesc.m_TotalVertexCount)))
				return false;
			m_Data->m_SkinContext.m_DstPositions = PopcornFX::TStridedMemoryView<CFloat3>(reinterpret_cast<CFloat3*>(m_Data->m_DstPositions.RawDataPointer()), m_Data->m_DstPositions.Count(), 16);
			m_Data->m_Override.m_PositionsOverride = m_Data->m_SkinContext.m_DstPositions;
			buildDesc.m_BuildFlags |= Build_Positions;
		}
		if (bSkinNormals)
		{
			if (!PK_VERIFY(m_Data->m_DstNormals.Resize(buildDesc.m_TotalVertexCount)))
				return false;
			m_Data->m_SkinContext.m_DstNormals = PopcornFX::TStridedMemoryView<CFloat3>(reinterpret_cast<CFloat3*>(m_Data->m_DstNormals.RawDataPointer()), m_Data->m_DstNormals.Count(), 16);
			m_Data->m_Override.m_NormalsOverride = m_Data->m_SkinContext.m_DstNormals;
			buildDesc.m_BuildFlags |= Build_Normals;
		}
		if (bSkinTangents)
		{
			if (!PK_VERIFY(m_Data->m_DstTangents.Resize(buildDesc.m_TotalVertexCount)))
				return false;
			m_Data->m_SkinContext.m_DstTangents = PopcornFX::TStridedMemoryView<CFloat4>(reinterpret_cast<CFloat4*>(m_Data->m_DstTangents.RawDataPointer()), m_Data->m_DstTangents.Count(), 16);
			m_Data->m_Override.m_TangentsOverride = m_Data->m_DstTangents;
			buildDesc.m_BuildFlags |= Build_Tangents;
		}
		if (bBuildClothData)
			buildDesc.m_BuildFlags |= Build_Cloth;
		if (bComputeVelocities)
		{
			// There should be at least position skinning
			PK_ASSERT(bSkinPositions && skin);
			if (!PK_VERIFY(m_Data->m_DstVelocities.Resize(buildDesc.m_TotalVertexCount)) ||
				!PK_VERIFY(m_Data->m_OldPositions.Resize(buildDesc.m_TotalVertexCount)))
				return false;
			// No need to clear the oldPositions
			PopcornFX::Mem::Clear(m_Data->m_DstVelocities.RawDataPointer(), sizeof(CFloat4) * buildDesc.m_TotalVertexCount);

			m_Data->m_SkinContext.m_CustomProcess_PreSkin = PopcornFX::SSkinContext::CbCustomProcess(this, &UPopcornFXAttributeSamplerSkinnedMesh::Skin_PreProcess);
			m_Data->m_Override.m_VelocitiesOverride = PopcornFX::TStridedMemoryView<CFloat3>(reinterpret_cast<CFloat3*>(m_Data->m_DstVelocities.RawDataPointer()), m_Data->m_DstVelocities.Count(), 16);
		}
		m_Data->m_SkinContext.m_CustomProcess_PostSkin = PopcornFX::SSkinContext::CbCustomProcess(this, &UPopcornFXAttributeSamplerSkinnedMesh::Skin_PostProcess);
		m_Data->m_SkinContext.m_CustomFinish = PopcornFX::SSkinContext::CbCustomFinish(this, &UPopcornFXAttributeSamplerSkinnedMesh::Skin_Finish);
	}

	// No need to create bone weights/indices if we don't skin anything..
	const u32	sectionCount = GetSections(buildDesc.m_LODRenderData).Num();
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	const bool	hasMasterPoseComponent = buildDesc.m_SkinnedMeshComponent->LeaderPoseComponent.Get() != null;
#else
	const bool	hasMasterPoseComponent = buildDesc.m_SkinnedMeshComponent->MasterPoseComponent.Get() != null;
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	if (skin && hasMasterPoseComponent) // No master pose component? Use directly the src mesh indices
	{
		PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::Alloc skinning buffers", POPCORNFX_UE_PROFILER_COLOR);

		for (u32 iSection = 0; iSection < sectionCount; ++iSection)
		{
			const FSkelMeshRenderSection	&section = GetSections(buildDesc.m_LODRenderData)[iSection];
			PK_ASSERT(section.MaxBoneInfluences > 0);
			if (section.MaxBoneInfluences > static_cast<int32>(buildDesc.m_MaxInfluenceCount))
				buildDesc.m_MaxInfluenceCount = section.MaxBoneInfluences;
		}

		const u32	totalDataCount = buildDesc.m_MaxInfluenceCount * buildDesc.m_TotalVertexCount;
		// Allocate necessary space for bone weights/indices
		if (!PK_VERIFY(totalDataCount > 0))
			return false;
		const u32	boneRealDataCount = totalDataCount * (buildDesc.m_SmallBoneIndices ? 1 : 2);
		if (!PK_VERIFY(m_Data->m_BoneIndices.Resize(boneRealDataCount)))
			return false;
		PopcornFX::Mem::Clear(m_Data->m_BoneIndices.RawDataPointer(), sizeof(u8) * boneRealDataCount);
	}
	if (buildDesc.m_SmallBoneIndices)
	{
		if (!PK_VERIFY(_FillBuffers<u8>(m_Data, buildDesc, vstream, skinningStreams)))
			return false;
	}
	else
	{
		if (!PK_VERIFY(_FillBuffers<u16>(m_Data, buildDesc, vstream, skinningStreams)))
			return false;
	}

	{
		PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::BuildAccelStructs", POPCORNFX_UE_PROFILER_COLOR);

		// No need to build sampling structs as we override them
		//meshNew->SetupRuntimeStructsIFN(false);

		bool	success = false;

		if (MeshSamplingMode == EPopcornFXMeshSamplingMode::Type::Weighted)
			success = meshNew->SetupSurfaceSamplingAccelStructs(0, DensityColorChannel, m_Data->m_OverrideSurfaceSamplingAccelStructs, &m_Data->m_OverrideSurfaceSamplingWorkingBuffers);
		else
			success = m_Data->m_OverrideSurfaceSamplingAccelStructs.Build(istream, vstream.Positions(), &m_Data->m_OverrideSurfaceSamplingWorkingBuffers);

		if (!PK_VERIFY(success))
			return false;
	}

	if (skin)
	{
		PK_ASSERT(m_Data->m_SkeletonView == null);
		PK_ASSERT(m_Data->m_SkinningStreamsProxy == null);

		if (!m_Data->m_BoneIndices.Empty()) // hasMasterPoseComponent
		{
			if (buildDesc.m_SmallBoneIndices)
			{
				PopcornFX::TBaseSkinningStreamsProxy<u8>	*proxy = PK_NEW(PopcornFX::TBaseSkinningStreamsProxy<u8>);
				if (!PK_VERIFY(proxy != null))
					return false;
				if (!PK_VERIFY(proxy->Setup(buildDesc.m_TotalVertexCount, srcBoneWeights, m_Data->m_BoneIndices)))
				{
					PK_DELETE(proxy);
					return false;
				}
				m_Data->m_SkinningStreamsProxy = proxy;
			}
			else
			{
				PopcornFX::TBaseSkinningStreamsProxy<u16>	*proxy = PK_NEW(PopcornFX::TBaseSkinningStreamsProxy<u16>);
				if (!PK_VERIFY(proxy != null))
					return false;
				if (!PK_VERIFY(proxy->Setup(buildDesc.m_TotalVertexCount, srcBoneWeights, PopcornFX::TMemoryView<const u16>(reinterpret_cast<const u16*>(m_Data->m_BoneIndices.RawDataPointer()), m_Data->m_BoneIndices.Count() / 2))))
				{
					PK_DELETE(proxy);
					return false;
				}
				m_Data->m_SkinningStreamsProxy = proxy;
			}
		}
		m_Data->m_SkinContext.m_SkinningStreams = m_Data->m_SkinningStreamsProxy != null ? m_Data->m_SkinningStreamsProxy : skinningStreams;

		if (!PK_VERIFY(m_Data->m_BoneInverseMatrices.Resize(buildDesc.m_TotalBoneCount)))
			return false;
		m_Data->m_SkeletonView = PK_NEW(PopcornFX::CSkeletonView(buildDesc.m_TotalBoneCount, null, m_Data->m_BoneInverseMatrices.RawDataPointer()));
		if (!PK_VERIFY(m_Data->m_SkeletonView != null))
			return false;

		// If everything is OK, we assign this component as dependency
		if (!PK_VERIFY(SetComponentTickingGroup(buildDesc.m_SkinnedMeshComponent)))
			return false;
		m_Data->m_CurrentSkinnedMeshComponent = buildDesc.m_SkinnedMeshComponent;
	}
	m_Data->m_Mesh = meshNew;
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerSkinnedMesh::UpdateSkinning()
{
	PK_ASSERT(IsInGameThread());
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::UpdateSkinning", POPCORNFX_UE_PROFILER_COLOR);

	PK_TODO("Don't skin several times the same skinned mesh component");

	// Do we have to resolve the skeletal mesh each frame ?
	USkinnedMeshComponent	*skinnedMesh = m_Data->m_CurrentSkinnedMeshComponent.Get();
	if (!PK_VERIFY(skinnedMesh != null))
		return false;

	// Don't even start the skinning if the skeleton view is invalid
	if (!PK_VERIFY(m_Data->m_SkeletonView != null))
		return false;
	const u32	boneCount = m_Data->m_BoneInverseMatrices.Count();
	if (!PK_VERIFY(boneCount > 0))
		return false;
	const FVector3f	invScale(FPopcornFXPlugin::GlobalScaleRcp());
	
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	const USkinnedMeshComponent	*baseComponent = skinnedMesh->LeaderPoseComponent.IsValid() ? skinnedMesh->LeaderPoseComponent.Get() : skinnedMesh;
#else
	const USkinnedMeshComponent	*baseComponent = skinnedMesh->MasterPoseComponent.IsValid() ? skinnedMesh->MasterPoseComponent.Get() : skinnedMesh;
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	if (!PK_VERIFY(boneCount <= (u32)GetComponentSpaceTransforms(baseComponent).Num()) || // <= boneCount will be greater if virtual bones are present in the skeleton
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		!PK_VERIFY(boneCount == SkeletalMeshRefBasesInvMatrices(Cast<USkeletalMesh>(skinnedMesh->GetSkinnedAsset())).Num()))
#else
		!PK_VERIFY(boneCount == SkeletalMeshRefBasesInvMatrices(skinnedMesh->SkeletalMesh).Num()))
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		UE_LOG(LogPopcornFXAttributeSamplerSkinned, Warning, TEXT("Mismatching bone counts: make sure to rebuild bind pose for asset '%s'"), *skinnedMesh->GetSkinnedAsset()->GetName());
#else
		UE_LOG(LogPopcornFXAttributeSamplerSkinned, Warning, TEXT("Mismatching bone counts: make sure to rebuild bind pose for asset '%s'"), *skinnedMesh->SkeletalMesh->GetName());
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		return false;
	}

	const TArray<uint8>			&boneVisibilityStates = skinnedMesh->GetBoneVisibilityStates();
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	const USkeletalMesh			*mesh = Cast<USkeletalMesh>(skinnedMesh->GetSkinnedAsset());
#else
	const USkeletalMesh			*mesh = skinnedMesh->SkeletalMesh;
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	const TArray<FTransform>	&spaceBases = GetComponentSpaceTransforms(baseComponent);

	const TArray<FMatrix44f>	&refBasesInvMatrix=  SkeletalMeshRefBasesInvMatrices(mesh);
	PK_ASSERT(boneCount <= (u32)boneVisibilityStates.Num());
	m_Data->m_BoneVisibilityChanged = false;
	for (u32 iBone = 0; iBone < boneCount; ++iBone)
	{
		// Consider only purely visible vertices (hierarchy of bones is respected)
		const bool	isBoneVisible = boneVisibilityStates[iBone] == EBoneVisibilityStatus::BVS_Visible;
		const bool	wasBoneVisible = m_Data->m_BoneInverseMatrices[iBone] != CFloat4x4::ZERO;
		if (isBoneVisible)
		{
			FMatrix44f	matrix = refBasesInvMatrix[iBone] * (FMatrix44f)spaceBases[iBone].ToMatrixWithScale();

			matrix.ScaleTranslation(invScale);
			m_Data->m_BoneInverseMatrices[iBone] = ToPk(matrix);
		}
		else
			m_Data->m_BoneInverseMatrices[iBone] = CFloat4x4::ZERO;
		m_Data->m_BoneVisibilityChanged |= isBoneVisible != wasBoneVisible;
	}

	USkeletalMeshComponent	*skelMesh = Cast<USkeletalMeshComponent>(skinnedMesh);
	if (skelMesh != null && !skelMesh->bDisableClothSimulation)
	{
		if (bBuildClothData &&
			!m_Data->m_ClothSections.Empty())
		{
			SCOPE_CYCLE_COUNTER(STAT_PopcornFX_FetchClothData); // Time cloth data copy

			const TMap<int32, FClothSimulData>	&simData = skelMesh->GetCurrentClothingData_GameThread();
			const FMatrix44f					&inverseTr = (FMatrix44f)skelMesh->GetComponentToWorld().Inverse().ToMatrixWithScale();

			m_Data->m_InverseTransforms = inverseTr;
			m_Data->m_ClothSimDataCopy = simData;
		}
	}

	// Launch skinning tasks
	PopcornFX::CSkeletalSkinnerSimple::AsyncSkinStart(m_Data->m_AsyncSkinContext, *m_Data->m_SkeletonView, m_Data->m_SkinContext);

	return true;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerSkinnedMesh::TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction *thisTickFunction)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::TickComponent", POPCORNFX_UE_PROFILER_COLOR);

	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	PK_ASSERT(m_Data != null);
	m_Data->m_ClothSimDataCopy.Empty();

	// Don't skin anything if we don't have any skinned mesh component assigned
	const USkinnedMeshComponent	*skinnedMesh = m_Data->m_CurrentSkinnedMeshComponent.Get();
	if (skinnedMesh == null || m_Data->m_Mesh == null /*Legit, if bPlayOnLoad == false, early out*/)
	{
		PK_ASSERT(!m_Data->m_ShouldUpdateTransforms);
		return;
	}
	PK_ASSERT(bSkinPositions || bSkinNormals || bSkinTangents);
	bool	shouldUpdateTransforms = (skinnedMesh->bRecentlyRendered ||
	skinnedMesh->VisibilityBasedAnimTickOption == EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones)
	&& !bPauseSkinning;

	// We don't want to interpolate more than POPCORNFX_MAX_ANIM_IDLE_TIME of inactive animation.
	if (m_Data->m_AccumulatedDts < POPCORNFX_MAX_ANIM_IDLE_TIME)
		m_Data->m_AccumulatedDts += GetWorld()->DeltaTimeSeconds;
	if (shouldUpdateTransforms)
	{
		// This means that no emitter is attached to this attr sampler, automatically pause skinning
		if (m_Data->m_AsyncSkinContext.m_SkinMergeJob != null)
		{
			PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::TickComponent AsyncSkinWait", POPCORNFX_UE_PROFILER_COLOR);

			// @TODO : Warn the user that he'll have to unpause the skinner when rehooking an effect to this attr sampler
			PopcornFX::CSkeletalSkinnerSimple::AsyncSkinWait(m_Data->m_AsyncSkinContext, null);
			bPauseSkinning = true;
			return;
		}
		shouldUpdateTransforms = UpdateSkinning();
	}
	else if (m_Data->m_ShouldUpdateTransforms)
	{
		// Clear velocities
		PopcornFX::Mem::Clear(m_Data->m_DstVelocities.RawDataPointer(), sizeof(CFloat4) * m_Data->m_DstVelocities.Count());
	}
	m_Data->m_ShouldUpdateTransforms = shouldUpdateTransforms;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerSkinnedMesh::UpdateTransforms()
{
	m_WorldTr_Previous = m_WorldTr_Current;
	m_Angular_Velocity = FVector3f(0);
	m_Linear_Velocity = FVector3f(ComponentVelocity);

	if (!m_Data->m_CurrentSkinnedMeshComponent.IsValid() &&
		(Transforms == EPopcornFXSkinnedTransforms::SkinnedComponentRelativeTr ||
		 Transforms == EPopcornFXSkinnedTransforms::SkinnedComponentWorldTr))
	{
		// No transforms update, keep previously created transforms, if any
		return;
	}

	switch (Transforms)
	{
		case	EPopcornFXSkinnedTransforms::SkinnedComponentRelativeTr:
			if (bApplyScale)
				m_WorldTr_Current = (FMatrix44f)m_Data->m_CurrentSkinnedMeshComponent->GetRelativeTransform().ToMatrixWithScale();
			else
				m_WorldTr_Current = (FMatrix44f)m_Data->m_CurrentSkinnedMeshComponent->GetRelativeTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSkinnedTransforms::SkinnedComponentWorldTr:
			if (bApplyScale)
				m_WorldTr_Current = (FMatrix44f)m_Data->m_CurrentSkinnedMeshComponent->GetComponentTransform().ToMatrixWithScale();
			else
				m_WorldTr_Current = (FMatrix44f)m_Data->m_CurrentSkinnedMeshComponent->GetComponentTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSkinnedTransforms::AttrSamplerRelativeTr:
			if (bApplyScale)
				m_WorldTr_Current = (FMatrix44f)GetRelativeTransform().ToMatrixWithScale();
			else
				m_WorldTr_Current = (FMatrix44f)GetRelativeTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSkinnedTransforms::AttrSamplerWorldTr:
			if (bApplyScale)
				m_WorldTr_Current = (FMatrix44f)GetComponentTransform().ToMatrixWithScale();
			else
				m_WorldTr_Current = (FMatrix44f)GetComponentTransform().ToMatrixNoScale();
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			break;
	}

	const float		pkScaleRcp = FPopcornFXPlugin::GlobalScaleRcp();
	m_WorldTr_Current.M[3][0] *= pkScaleRcp;
	m_WorldTr_Current.M[3][1] *= pkScaleRcp;
	m_WorldTr_Current.M[3][2] *= pkScaleRcp;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerSkinnedMesh::_AttribSampler_PreUpdate(float deltaTime)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::_AttribSampler_PreUpdate", POPCORNFX_UE_PROFILER_COLOR);

	if (m_LastFrameUpdate == GFrameCounter)
		return;
	m_LastFrameUpdate = GFrameCounter;

	UpdateTransforms();

	if (m_Data->m_CurrentSkinnedMeshComponent.IsValid() &&
		m_Data->m_ShouldUpdateTransforms)
	{
		PK_ASSERT(bSkinPositions || bSkinNormals || bSkinTangents);
		SCOPE_CYCLE_COUNTER(STAT_PopcornFX_SkinningWaitTime);

		PopcornFX::CSkeletalSkinnerSimple::AsyncSkinWait(m_Data->m_AsyncSkinContext, null);

		if (!bPauseSkinning)
			m_Data->m_AccumulatedDts = 0.0f;
	}
}

//----------------------------------------------------------------------------
