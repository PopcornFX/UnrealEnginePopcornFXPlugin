//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplerShape.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXStats.h"
#include "PopcornFXAttributeList.h"
#include "Assets/PopcornFXMesh.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_geometrics/include/ge_mesh_sampler_accel.h>
#include <pk_particles_toolbox/include/pt_mesh_deformers_skin.h>

#include "Components/SkinnedMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ClothingAsset.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
// Don't include the DestructibleComponent implementation header,
// relies on ApexDestruction plugin which isn't loaded when PopcornFX runtime
// module is loaded. Include the interface instead.
// #	include "DestructibleComponent.h"
#include "DestructibleInterface.h"

#if WITH_EDITOR
#	include "Editor.h"
#	include "Engine/Selection.h"
#endif

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplerShape"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplerShape, Log, All);\

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSamplerShape
//
//----------------------------------------------------------------------------

namespace
{
	using PopcornFX::CShapeDescriptor;
	using PopcornFX::PShapeDescriptor;
	using PopcornFX::PCShapeDescriptor;

	struct SNewShapeParams
	{
		const UPopcornFXAttributeSamplerShape	*self;
	};

	struct SUpdateShapeParams
	{
		const UPopcornFXAttributeSamplerShape	*self;
		CShapeDescriptor						*shape;
	};

	typedef CShapeDescriptor*(*CbNewDescriptor)(const SNewShapeParams &params);
	typedef void(*CbUpdateShapeDescriptor)(const SUpdateShapeParams &params);

	void				_RebuildSamplingStructs(const UPopcornFXAttributeSamplerShape *self, PopcornFX::CShapeDescriptor_Mesh *shapeDesc)
	{
		PopcornFX::CMeshSurfaceSamplerStructuresRandom	*surfaceSampling = self->SamplerSurface();
		PopcornFX::PMeshNew								mesh = shapeDesc->Mesh();
		PopcornFX::CMeshTriangleBatch					triangleBatch = mesh->TriangleBatch();

		PK_ASSERT(mesh != null);

		if (self->Properties.ShapeSamplingMode == EPopcornFXMeshSamplingMode::Type::Weighted)
			mesh->SetupSurfaceSamplingAccelStructs(0, self->Properties.DensityColorChannel, *surfaceSampling);
		else
			surfaceSampling->Build(triangleBatch.m_IStream, triangleBatch.m_VStream.Positions());

		shapeDesc->SetSamplingStructs(surfaceSampling, null);
	}

	// NULL
	template <typename _Descriptor>
	CShapeDescriptor	*_NewDescriptor(const SNewShapeParams &params)
	{
		return null;
	}

	// NULL
	template <typename _Descriptor>
	void				_UpdateShapeDescriptor(const SUpdateShapeParams &params)
	{
	}

	// BOX
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Box>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;

		PK_ASSERT(self->Properties.BoxDimension.GetMin() >= 0.0f);
		const FVector3f	safeBoxDimension = FVector3f(self->Properties.BoxDimension);
		const CFloat3	dim = ToPk(safeBoxDimension) * FPopcornFXPlugin::GlobalScaleRcp();
		return PK_NEW(PopcornFX::CShapeDescriptor_Box(dim));
	}

	// BOX
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Box>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;
		PopcornFX::CShapeDescriptor_Box			*shape = static_cast<PopcornFX::CShapeDescriptor_Box*>(params.shape);

		PK_ASSERT(self->Properties.BoxDimension.GetMin() >= 0.0f);
		const FVector3f	safeBoxDimension = FVector3f(self->Properties.BoxDimension);
		const CFloat3	dim = ToPk(safeBoxDimension) * FPopcornFXPlugin::GlobalScaleRcp();

		shape->SetDimensions(dim);
	}

	// SPHERE
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Sphere>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape		*self = params.self;

		PK_ASSERT(self->Properties.Radius >= 0.0f);
		PK_ASSERT(self->Properties.InnerRadius >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Properties.Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->Properties.InnerRadius, 0.0f) * invGlobalScale;

		return PK_NEW(PopcornFX::CShapeDescriptor_Sphere(radius, innerRadius));
	}

	// SPHERE
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Sphere>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape		*self = params.self;
		PopcornFX::CShapeDescriptor_Sphere			*shape = static_cast<PopcornFX::CShapeDescriptor_Sphere*>(params.shape);

		PK_ASSERT(self->Properties.Radius >= 0.0f);
		PK_ASSERT(self->Properties.InnerRadius >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Properties.Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->Properties.InnerRadius, 0.0f) * invGlobalScale;

		shape->SetInnerRadius(innerRadius);
		shape->SetRadius(radius);
	}

	// ELLIPSOID
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Ellipsoid>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;

		PK_ASSERT(self->Properties.Radius >= 0.0f);
		PK_ASSERT(self->Properties.InnerRadius >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Properties.Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->Properties.InnerRadius, 0.0f) * invGlobalScale;

		return PK_NEW(PopcornFX::CShapeDescriptor_Ellipsoid(radius, innerRadius));
	}

	// ELLIPSOID
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Ellipsoid>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;
		PopcornFX::CShapeDescriptor_Ellipsoid	*shape = static_cast<PopcornFX::CShapeDescriptor_Ellipsoid*>(params.shape);

		PK_ASSERT(self->Properties.Radius >= 0.0f);
		PK_ASSERT(self->Properties.InnerRadius >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Properties.Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->Properties.InnerRadius, 0.0f) * invGlobalScale;

		shape->SetInnerRadius(innerRadius);
		shape->SetRadius(radius);
	}

	// CYLINDRE
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Cylinder>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;

		PK_ASSERT(self->Properties.Radius >= 0.0f);
		PK_ASSERT(self->Properties.InnerRadius >= 0.0f);
		PK_ASSERT(self->Properties.Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Properties.Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->Properties.InnerRadius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Properties.Height, 0.0f) * invGlobalScale;

		return PK_NEW(PopcornFX::CShapeDescriptor_Cylinder(radius, height, innerRadius));
	}

	// CYLINDRE
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Cylinder>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;
		PopcornFX::CShapeDescriptor_Cylinder	*shape = static_cast<PopcornFX::CShapeDescriptor_Cylinder*>(params.shape);

		PK_ASSERT(self->Properties.Radius >= 0.0f);
		PK_ASSERT(self->Properties.InnerRadius >= 0.0f);
		PK_ASSERT(self->Properties.Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Properties.Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->Properties.InnerRadius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Properties.Height, 0.0f) * invGlobalScale;

		shape->SetInnerRadius(innerRadius);
		shape->SetRadius(radius);
		shape->SetHeight(height);
	}

	// CAPSULE
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Capsule>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;

		PK_ASSERT(self->Properties.Radius >= 0.0f);
		PK_ASSERT(self->Properties.InnerRadius >= 0.0f);
		PK_ASSERT(self->Properties.Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Properties.Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->Properties.InnerRadius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Properties.Height, 0.0f) * invGlobalScale;

		return PK_NEW(PopcornFX::CShapeDescriptor_Capsule(radius, height, innerRadius));
	}

	// CAPSULE
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Capsule>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;
		PopcornFX::CShapeDescriptor_Capsule		*shape = static_cast<PopcornFX::CShapeDescriptor_Capsule*>(params.shape);

		PK_ASSERT(self->Properties.Radius >= 0.0f);
		PK_ASSERT(self->Properties.InnerRadius >= 0.0f);
		PK_ASSERT(self->Properties.Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Properties.Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->Properties.InnerRadius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Properties.Height, 0.0f) * invGlobalScale;

		shape->SetInnerRadius(innerRadius);
		shape->SetRadius(radius);
		shape->SetHeight(height);
	}

	// CONE
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Cone>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;

		PK_ASSERT(self->Properties.Radius >= 0.0f);
		PK_ASSERT(self->Properties.Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Properties.Radius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Properties.Height, 0.0f) * invGlobalScale;

		return PK_NEW(PopcornFX::CShapeDescriptor_Cone(radius, height));
	}

	// CONE
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Cone>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;
		PopcornFX::CShapeDescriptor_Cone		*shape = static_cast<PopcornFX::CShapeDescriptor_Cone*>(params.shape);

		PK_ASSERT(self->Properties.Radius >= 0.0f);
		PK_ASSERT(self->Properties.Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Properties.Radius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Properties.Height, 0.0f) * invGlobalScale;

		shape->SetRadius(radius);
		shape->SetHeight(height);
	}

	// MESH
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Mesh>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;

		if (self->Properties.StaticMesh == null)
			return null;

		PK_FIXME("use the ResourceManager with TResourcePtr !!");
		UPopcornFXMesh		*pkMesh = UPopcornFXMesh::FindStaticMesh(self->Properties.StaticMesh);
		if (pkMesh == null)
			return null;

		PopcornFX::PResourceMesh	meshRes = pkMesh->LoadResourceMeshIFN(true);
		if (meshRes == null)
			return null;

		const u32					batchCount = meshRes->BatchList().Count();
		if (!PK_VERIFY(batchCount != 0))
			return null;

		const u32								batchi = PopcornFX::PKMin(u32(PopcornFX::PKMax(self->Properties.StaticMeshSubIndex, 0)), batchCount - 1);
		const PopcornFX::PResourceMeshBatch		&batch = meshRes->BatchList()[batchi];
		if (!PK_VERIFY(batch != null))
			return null;
		if (!PK_VERIFY(batch->RawMesh() != null))
			return null;

		PK_ASSERT(self->Properties.Scale.GetMin() >= 0.0f);
		const FVector3f safeMeshScale = FVector3f(self->Properties.Scale);
		const CFloat3 scale = ToPk(safeMeshScale);

		PopcornFX::CShapeDescriptor_Mesh	*shapeDesc = PK_NEW(PopcornFX::CShapeDescriptor_Mesh(batch->RawMesh(), scale));

		_RebuildSamplingStructs(self, shapeDesc);

		return shapeDesc;
	}

	// MESH
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Mesh>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape		*self = params.self;
		PopcornFX::CShapeDescriptor_Mesh			*shapeDesc = static_cast<PopcornFX::CShapeDescriptor_Mesh*>(params.shape);
		
		PK_ASSERT(self->Properties.Scale.GetMin() >= 0.0f);
		const FVector3f	safeScale = FVector3f(self->Properties.Scale);
		const CFloat3	scale = ToPk(safeScale);

		shapeDesc->SetScale(scale);

		_RebuildSamplingStructs(self, shapeDesc);
	}

	// COLLECTION
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_MeshCollection>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape			*self = params.self;

		PopcornFX::CShapeDescriptor_MeshCollection	*shapeCollection = PK_NEW(PopcornFX::CShapeDescriptor_MeshCollection());
		if (shapeCollection == null)
			return null;
		if (self->Properties.Shapes.Num() == 0) // not an error
			return shapeCollection;

		PK_STATIC_ASSERT(EPopcornFXShapeCollectionSamplingHeuristic::NoWeight			== (u32)PopcornFX::CShapeDescriptor_MeshCollection::NoWeight);
		PK_STATIC_ASSERT(EPopcornFXShapeCollectionSamplingHeuristic::WeightWithVolume	== (u32)PopcornFX::CShapeDescriptor_MeshCollection::WeightWithVolume);
		PK_STATIC_ASSERT(EPopcornFXShapeCollectionSamplingHeuristic::WeightWithSurface	== (u32)PopcornFX::CShapeDescriptor_MeshCollection::WeightWithSurface);
		PK_STATIC_ASSERT(3 == PopcornFX::CShapeDescriptor_MeshCollection::__MaxSamplingHeuristics);
		shapeCollection->SetSamplingHeuristic(static_cast<PopcornFX::CShapeDescriptor_MeshCollection::ESamplingHeuristic>(self->Properties.CollectionSamplingHeuristic.GetValue()));
		shapeCollection->SetUseSubMeshWeights(self->Properties.CollectionUseShapeWeights != 0);
//		shapeCollection->m_PermutateMultiSamples = false;	// Oh god no ! This should almost always be 'true' !!

		const float	invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		for (auto shapeIt = self->Properties.Shapes.CreateConstIterator(); shapeIt; ++shapeIt)
		{
			const APopcornFXAttributeSamplerActor	*attrSampler = *shapeIt;

			if (attrSampler == null)
				continue;
			UPopcornFXAttributeSamplerShape	*shape = Cast<UPopcornFXAttributeSamplerShape>(attrSampler->Sampler);	// This must be a mesh
			if (shape == null || shape->IsCollection())
				continue;

			// Meshes associated with shape collections won't be setup by their default shape
			if (!shape->IsValid())
			{
				PK_ONLY_IF_ASSERTS(bool shapeInitOk = )shape->InitShape();
				PK_ASSERT(shapeInitOk);
			}

			// This won't compile now, it must be a CShapeDescriptor_Mesh*
			PopcornFX::CShapeDescriptor_Mesh	*desc = shape->GetShapeDescriptor();
			if (!PK_VERIFY(desc != null) ||
				!PK_VERIFY(shapeCollection->AddSubMesh(desc)))
				continue;
			// Don't transform the world rotation
			const FVector	transformedLocation = self->GetComponentTransform().InverseTransformPosition(shape->GetComponentLocation());
			FMatrix			localMatrix = FTransform(shape->GetComponentRotation(), transformedLocation, FVector::ZeroVector).ToMatrixNoScale();

			// This first matrix is likely to be incorrect on editor load
			// it will be patched at PreUpdate
			localMatrix.M[3][0] *= invGlobalScale;
			localMatrix.M[3][1] *= invGlobalScale;
			localMatrix.M[3][2] *= invGlobalScale;
			desc->m_LocalTransforms = ToPk(localMatrix);
		}
		return shapeCollection;
	}

	// COLLECTION
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_MeshCollection>(const SUpdateShapeParams &params)
	{
		//PopcornFX::CShapeDescriptor_MeshCollection	*collection = static_cast<PopcornFX::CShapeDescriptor_MeshCollection*>(shape);
	}
#endif

	CbNewDescriptor const		kCbNewDescriptors[] = {
		&_NewDescriptor<PopcornFX::CShapeDescriptor_Box>,		//Box = 0,
		&_NewDescriptor<PopcornFX::CShapeDescriptor_Sphere>,	//Sphere,
		&_NewDescriptor<PopcornFX::CShapeDescriptor_Ellipsoid>,	//Ellipsoid,
		&_NewDescriptor<PopcornFX::CShapeDescriptor_Cylinder>,	//Cylinder,
		&_NewDescriptor<PopcornFX::CShapeDescriptor_Capsule>,	//Capsule,
		&_NewDescriptor<PopcornFX::CShapeDescriptor_Cone>,		//Cone,
		&_NewDescriptor<PopcornFX::CShapeDescriptor_Mesh>,		//StaticMesh,
		&_NewDescriptor<PopcornFX::CShapeDescriptor_Mesh>,		//SkeletalMesh,
		//null,		// Spline
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
		&_NewDescriptor<PopcornFX::CShapeDescriptor_MeshCollection>,	//MeshCollection
#endif
	};

	CbUpdateShapeDescriptor const	kCbUpdateShapeDescriptors[] = {
		&_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Box>,		//Box = 0,
		&_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Sphere>,	//Sphere,
		&_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Ellipsoid>,	//Ellipsoid,
		&_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Cylinder>,	//Cylinder,
		&_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Capsule>,	//Capsule,
		&_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Cone>,		//Cone,
		&_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Mesh>,		//StaticMesh,
		&_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Mesh>,		//SkeletalMesh,
		//null,		// Spline
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
		&_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_MeshCollection>,	//MeshCollection
#endif
	};

	PK_STATIC_ASSERT(PK_ARRAY_COUNT(kCbNewDescriptors) == EPopcornFXAttribSamplerShapeType_Max);
	PK_STATIC_ASSERT(PK_ARRAY_COUNT(kCbUpdateShapeDescriptors) == EPopcornFXAttribSamplerShapeType_Max);
}

#define	POPCORNFX_MAX_ANIM_IDLE_TIME	1.0f

struct FPopcornFXClothSection
{
	u32							m_BaseVertexOffset;
	u32							m_VertexCount;
	u32							m_ClothDataIndex;
	PopcornFX::TArray<u32>		m_Indices;
};

struct FAttributeSamplerShapeData
{
	bool	m_ShouldUpdateTransforms = false;
	bool	m_BoneVisibilityChanged = false;

	float	m_AccumulatedDts = 0.0f;

	UStaticMesh													*m_StaticMesh = null;
	USkeletalMesh												*m_SkeletalMesh = null;
	PopcornFX::PMeshNew											m_Mesh;
	PopcornFX::PParticleSamplerDescriptor_Shape_Default			m_Desc;
	PopcornFX::PShapeDescriptor									m_Shape;
	PopcornFX::CMeshSurfaceSamplerStructuresRandom				m_SamplerSurface;

	PopcornFX::TArray<CFloat4, PopcornFX::TArrayAligned16>		m_DstPositions;
	PopcornFX::TArray<CFloat4, PopcornFX::TArrayAligned16>		m_DstNormals;
	PopcornFX::TArray<CFloat4, PopcornFX::TArrayAligned16>		m_DstTangents;

	PopcornFX::TArray<CFloat4, PopcornFX::TArrayAligned16>		m_OldPositions;
	PopcornFX::TArray<CFloat4, PopcornFX::TArrayAligned16>		m_DstVelocities;

	PopcornFX::TArray<u8, PopcornFX::TArrayAligned16>			m_BoneIndices; // Only if hasMasterPose

	PopcornFX::TArray<CFloat4x4, PopcornFX::TArrayAligned16>	m_BoneInverseMatrices;

	PopcornFX::TArray<FPopcornFXClothSection>	m_ClothSections;
	TMap<int32, FClothSimulData>				m_ClothSimDataCopy;
	FMatrix44f									m_InverseTransforms;

	PopcornFX::CBaseSkinningStreams				*m_SkinningStreamsProxy = null;
	PopcornFX::SSkinContext						m_SkinContext;
	PopcornFX::CSkinAsyncContext				m_AsyncSkinContext;
	PopcornFX::CSkeletonView					*m_SkeletonView = null;

	PopcornFX::SSamplerSourceOverride			m_Override;

	PopcornFX::CDiscreteProbabilityFunction1D_O1::SWorkingBuffers	m_OverrideSurfaceSamplingWorkingBuffers;
	PopcornFX::CMeshSurfaceSamplerStructuresRandom					m_OverrideSurfaceSamplingAccelStructs;

	TWeakObjectPtr<USkinnedMeshComponent>		m_CurrentSkinnedMeshComponent = null;
};

UPopcornFXAttributeSamplerShape::UPopcornFXAttributeSamplerShape(const FObjectInitializer &PCIP)
	: Super(PCIP)
	, m_LastFrameUpdate(0)
{
	bAutoActivate = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
	bTickInEditor = true;
#if WITH_EDITOR
	m_IndirectSelectedThisTick = false;
#endif

	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Shape;

	Properties.ShapeType = EPopcornFXAttribSamplerShapeType::Sphere;
	Properties.BoxDimension = FVector(100.f);
	Properties.Radius = 100.f;
	Properties.Scale = FVector::OneVector;
	Properties.InnerRadius = 0.f;
	Properties.Height = 100.f;
	Properties.StaticMesh = null;
	Properties.StaticMeshSubIndex = 0;
	Properties.Weight = 1.0f;
	// Default skinned mesh only builds positions
	Properties.bPauseSkinning = false;
	Properties.bSkinPositions = true;
	Properties.bSkinNormals = false;
	Properties.bSkinTangents = false;
	Properties.bBuildColors = false;
	Properties.bBuildUVs = false;
	Properties.bComputeVelocities = false;
	Properties.bBuildClothData = false;

	Properties.TargetActor = null;
	Properties.Transforms = EPopcornFXSkinnedTransforms::AttrSamplerRelativeTr;
	Properties.bApplyScale = false;
#if WITH_EDITORONLY_DATA
	Properties.bEditorBuildInitialPose = false;
#endif // WITH_EDITORONLY_DATA

#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	CollectionSamplingHeuristic = EPopcornFXShapeCollectionSamplingHeuristic::NoWeight;
	CollectionUseShapeWeights = 1;
#endif

	Properties.bUseRelativeTransform = true;

	m_Data = new FAttributeSamplerShapeData();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::BeginDestroy()
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

void	UPopcornFXAttributeSamplerShape::SetRadius(float radius)
{
	Properties.Radius = radius;
	UpdateShapeProperties();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::SetWeight(float height)
{
	Properties.Height = height;
	UpdateShapeProperties();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::SetBoxDimension(FVector boxDimensions)
{
	Properties.BoxDimension = boxDimensions;
	UpdateShapeProperties();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::SetInnerRadius(float innerRadius)
{
	Properties.InnerRadius = innerRadius;
	UpdateShapeProperties();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::SetHeight(float height)
{
	Properties.Height = height;
	UpdateShapeProperties();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::SetScale(FVector scale)
{
	Properties.Scale = scale;
	UpdateShapeProperties();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::Clear()
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
	m_Data->m_SkinContext.m_SrcTangents = TStridedMemoryView<const CFloat4>();

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

void	UPopcornFXAttributeSamplerShape::Skin_PreProcess(uint32 vertexStart, uint32 vertexCount, const PopcornFX::SSkinContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::Skin_PreProcess", POPCORNFX_UE_PROFILER_COLOR);

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

void	UPopcornFXAttributeSamplerShape::Skin_PostProcess(uint32 vertexStart, uint32 vertexCount, const PopcornFX::SSkinContext &ctx)
{
	PK_ASSERT(m_Data != null);
	PK_ASSERT(m_Data->m_Mesh != null);

	if (Properties.bBuildClothData && !m_Data->m_ClothSections.Empty())
		FetchClothData(vertexStart, vertexCount);
	if (!Properties.bComputeVelocities)
		return;
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::Skin_PostProcess", POPCORNFX_UE_PROFILER_COLOR);

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

void	UPopcornFXAttributeSamplerShape::Skin_Finish(const PopcornFX::SSkinContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::Skin_Finish", POPCORNFX_UE_PROFILER_FAST_COLOR);

	PK_ASSERT(m_Data != null);
	PK_ASSERT(m_Data->m_Mesh != null);

	// Rebuild sampling structures if bone visibility array has changed
	if (!m_Data->m_BoneVisibilityChanged)
		return;

	if (!Properties.bSkinPositions)
	{
		// @TODO : warn the user that distribution will be invalid if he only checked bSkinNormals or bSkinTangents
		// and he either is sampling a destructible mesh or the target skinned mesh component got one of its bone hidden/unhidden
		return;
	}

	bool rebuildAccelStruct = false;

	if (Properties.ShapeSamplingMode == EPopcornFXMeshSamplingMode::Type::Weighted)
		rebuildAccelStruct = m_Data->m_Mesh->SetupSurfaceSamplingAccelStructs(0, Properties.DensityColorChannel, m_Data->m_OverrideSurfaceSamplingAccelStructs, &m_Data->m_OverrideSurfaceSamplingWorkingBuffers);
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

void	UPopcornFXAttributeSamplerShape::FetchClothData(uint32 vertexStart, uint32 vertexCount)
{
	// Done during skinning job's PostProcess callback
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::FetchClothData", POPCORNFX_UE_PROFILER_COLOR);

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
			PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::FetchClothData::Section", POPCORNFX_UE_PROFILER_COLOR);

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
			CFloat4		*dstNormals = Properties.bSkinNormals ? m_Data->m_DstPositions.RawDataPointer() + realVertexStart : _dummyNormal;

			const u32	dstStride = Properties.bSkinNormals ? 0x10 : 0;

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

bool	UPopcornFXAttributeSamplerShape::Rebuild()
{
	return BuildInitialPose();
}

//----------------------------------------------------------------------------

USkinnedMeshComponent	*UPopcornFXAttributeSamplerShape::ResolveSkinnedMeshComponent()
{
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::ResolveSkinnedMeshComponent", POPCORNFX_UE_PROFILER_FAST_COLOR);

	AActor	*fallbackActor = GetOwner();
	if (!PK_VERIFY(fallbackActor != null))
		return null;
	const AActor			*parent = Properties.TargetActor == null ? fallbackActor : Properties.TargetActor;
	USkinnedMeshComponent	*skinnedMesh = null;
	if (Properties.SkinnedMeshComponentName != NAME_None)
	{
		FObjectPropertyBase	*prop = FindFProperty<FObjectPropertyBase>(parent->GetClass(), Properties.SkinnedMeshComponentName);

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
			UE_LOG(LogPopcornFXAttributeSamplerShape, Warning,
				TEXT("Could not find component 'USkinnedMeshComponent %s.%s' for UPopcornFXAttributeSamplerShape '%s'"),
				*parent->GetName(), (Properties.SkinnedMeshComponentName != NAME_None ? *Properties.SkinnedMeshComponentName.ToString() : TEXT("RootComponent")),
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

PopcornFX::CMeshSurfaceSamplerStructuresRandom	*UPopcornFXAttributeSamplerShape::SamplerSurface() const
{
	return &m_Data->m_SamplerSurface;
}
//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerShape::CanUpdateShapeProperties(EPopcornFXAttribSamplerShapeType::Type newType)
{
	PK_ASSERT(m_Data != null);
	if (m_Data->m_Shape == null)
		return false;
	if (!PK_VERIFY(m_Data->m_Desc != null) ||
		!PK_VERIFY(m_Data->m_Desc->m_Shape != null) ||
		!PK_VERIFY(m_Data->m_Desc->m_Shape == m_Data->m_Shape) ||
		m_Data->m_Shape->ShapeType() != ToPkShapeType(newType))
	{
		// well, if it should not happen, better clear everything now
		m_Data->m_Shape = null;
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::UpdateShapeProperties()
{
	if (!CanUpdateShapeProperties(Properties.ShapeType))
		return;
	PK_ASSERT(m_Data->m_Shape->ShapeType() == ToPkShapeType(Properties.ShapeType));
	(*kCbUpdateShapeDescriptors[Properties.ShapeType.GetValue()])(SUpdateShapeParams{ this, m_Data->m_Shape.Get() });
	m_Data->m_Shape->m_Weight = Properties.Weight;
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

void	UPopcornFXAttributeSamplerShape::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.MemberProperty != null)
	{
		// Don't update shapes because parent class properties were modified
		if (propertyChangedEvent.MemberProperty->GetOwnerClass() != UPopcornFXAttributeSamplerShape::StaticClass())
			return;
		if (!CanUpdateShapeProperties(Properties.ShapeType))
			return;
		const FString	propertyName = propertyChangedEvent.MemberProperty->GetName();
		if (propertyName == TEXT("Weight") ||
			propertyName == TEXT("BoxDimension") ||
			propertyName == TEXT("Radius") ||
			propertyName == TEXT("InnerRadius") ||
			propertyName == TEXT("Height") ||
			propertyName == TEXT("Scale") ||
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
			propertyName == TEXT("CollectionSamplingHeuristic") ||
			propertyName == TEXT("CollectionUseShapeWeights") ||
#endif
			propertyName == TEXT("ShapeSamplingMode") ||
			propertyName == TEXT("ShapeType") ||
			propertyName == TEXT("DensityColorChannel"))
			//(propertyName == TEXT("Shapes") && propertyChangedEvent.ChangeType != EPropertyChangeType::ArrayAdd)
		{
			PK_ASSERT(m_Data->m_Shape->ShapeType() == ToPkShapeType(Properties.ShapeType));
			(*kCbUpdateShapeDescriptors[Properties.ShapeType.GetValue()])(SUpdateShapeParams{this, m_Data->m_Shape.Get()});
			m_Data->m_Shape->m_Weight = Properties.Weight;
		}
		else if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bEditorBuildInitialPose) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, TargetActor) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, SkinnedMeshComponentName))
		{
			Properties.bEditorBuildInitialPose = false;
			BuildInitialPose();
		}
		else if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bSkinPositions) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bSkinNormals) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bSkinTangents) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bBuildColors) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bBuildUVs) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bBuildClothData))
		{
			if (!Properties.bSkinPositions)
				Properties.bComputeVelocities = false;
			BuildInitialPose();
		}
		else if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bComputeVelocities))
		{
			if (Properties.bComputeVelocities)
				Properties.bSkinPositions = true;
			BuildInitialPose();
		}
		else
		{
			// invalidate shape
			m_Data->m_Desc = null;
			m_Data->m_Shape = null;
		}
	}

	Super::PostEditChangeProperty(propertyChangedEvent);

}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesShape *newShapeProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesShape *>(other->GetProperties());
	if (!PK_VERIFY(newShapeProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSamplerShape, Error, TEXT("New properties are null or not Shape properties"));
		return;
	}

	Super::CopyPropertiesFrom(other);

	const FPopcornFXAttributeSamplerPropertiesShape oldProperties = Properties;

	Properties = *newShapeProperties;

	if (!CanUpdateShapeProperties(newShapeProperties->ShapeType))
		return;
	if (newShapeProperties->Weight != oldProperties.Weight ||
		newShapeProperties->BoxDimension != oldProperties.BoxDimension ||
		newShapeProperties->Radius != oldProperties.Radius ||
		newShapeProperties->InnerRadius != oldProperties.InnerRadius ||
		newShapeProperties->Height != oldProperties.Height ||
		newShapeProperties->Scale != oldProperties.Scale ||
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
		newShapeProperties->CollectionSamplingHeuristic != oldProperties.CollectionSamplingHeuristic ||
		newShapeProperties->CollectionUseShapeWeights != oldProperties.CollectionUseShapeWeights ||
#endif
		newShapeProperties->ShapeSamplingMode != oldProperties.ShapeSamplingMode ||
		newShapeProperties->ShapeType != oldProperties.ShapeType ||
		newShapeProperties->DensityColorChannel != oldProperties.DensityColorChannel)
	{
		PK_ASSERT(m_Data->m_Shape->ShapeType() == ToPkShapeType(oldProperties.ShapeType));
		(*kCbUpdateShapeDescriptors[oldProperties.ShapeType.GetValue()])(SUpdateShapeParams{ this, m_Data->m_Shape.Get() });
		m_Data->m_Shape->m_Weight = oldProperties.Weight;
	}
	else if (newShapeProperties->bEditorBuildInitialPose != oldProperties.bEditorBuildInitialPose ||
		newShapeProperties->TargetActor != oldProperties.TargetActor ||
		newShapeProperties->SkinnedMeshComponentName != oldProperties.SkinnedMeshComponentName)
	{
		Properties.bEditorBuildInitialPose = false;
		BuildInitialPose();
	}
	else if (newShapeProperties->bSkinPositions != oldProperties.bSkinPositions ||
		newShapeProperties->bSkinNormals != oldProperties.bSkinNormals ||
		newShapeProperties->bSkinTangents != oldProperties.bSkinTangents ||
		newShapeProperties->bBuildColors != oldProperties.bBuildColors ||
		newShapeProperties->bBuildUVs != oldProperties.bBuildUVs ||
		newShapeProperties->bBuildClothData != oldProperties.bBuildClothData)
	{
		if (!newShapeProperties->bSkinPositions)
			Properties.bComputeVelocities = false;
		BuildInitialPose();
	}
	else if (newShapeProperties->bComputeVelocities != oldProperties.bComputeVelocities)
	{
		if (!newShapeProperties->bComputeVelocities)
			Properties.bSkinPositions = false;
		BuildInitialPose();
	}
	else
	{
		// invalidate shape
		m_Data->m_Desc = null;
		m_Data->m_Shape = null;
	}

}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction *thisTickFunction)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShapeMesh::TickComponent", POPCORNFX_UE_PROFILER_COLOR);

	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	if (Properties.ShapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh)
	{
		PK_ASSERT(m_Data != null);
		m_Data->m_ClothSimDataCopy.Empty();

		// Don't skin anything if we don't have any skinned mesh component assigned
		const USkinnedMeshComponent *skinnedMesh = m_Data->m_CurrentSkinnedMeshComponent.Get();
		if (skinnedMesh == null || m_Data->m_Mesh == null /*Legit, if bPlayOnLoad == false, early out*/)
		{
			PK_ASSERT(!m_Data->m_ShouldUpdateTransforms);
			return;
		}
		PK_ASSERT(Properties.bSkinPositions || Properties.bSkinNormals || Properties.bSkinTangents);
		bool	shouldUpdateTransforms = (skinnedMesh->bRecentlyRendered ||
			skinnedMesh->VisibilityBasedAnimTickOption == EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones)
			&& !Properties.bPauseSkinning;

		// We don't want to interpolate more than POPCORNFX_MAX_ANIM_IDLE_TIME of inactive animation.
		if (m_Data->m_AccumulatedDts < POPCORNFX_MAX_ANIM_IDLE_TIME)
			m_Data->m_AccumulatedDts += GetWorld()->DeltaTimeSeconds;
		if (shouldUpdateTransforms)
		{
			// This means that no emitter is attached to this attr sampler, automatically pause skinning
			if (m_Data->m_AsyncSkinContext.m_SkinMergeJob != null)
			{
				PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::TickComponent AsyncSkinWait", POPCORNFX_UE_PROFILER_COLOR);

				// @TODO : Warn the user that they'll have to unpause the skinner when rehooking an effect to this attr sampler
				PopcornFX::CSkeletalSkinnerSimple::AsyncSkinWait(m_Data->m_AsyncSkinContext, null);
				Properties.bPauseSkinning = true;
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
#if WITH_EDITOR
	else
	{
		PK_ASSERT(GetWorld() != null);
		if (GetWorld()->IsGameWorld())
			return;

		bool				render = FPopcornFXPlugin::Get().SettingsEditor()->bAlwaysRenderAttributeSamplerShapes;

		const USelection	*selectedAssets = GEditor->GetSelectedActors();
		PK_ASSERT(selectedAssets != null);
		bool				isSelected = selectedAssets->IsSelected(GetOwner());
		if (isSelected || m_IndirectSelectedThisTick)
		{
			render = true;
			isSelected = true;
			m_IndirectSelectedThisTick = false;
		}

		if (!render)
			return;

		// Use the TickComponent to render the geometry, because _AttribSampler_PreUpdate()
		// isn't called on attr samplers that are not directly referenced (ie shape collection referencing other attr sampler shapes)
		// So they wont be rendered. Attr sampler collections manually rendering their subMeshes isn't an option (might induce multiple draws for a single attr sampler)
		RenderShapeIFP(isSelected);
	}
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

void	UPopcornFXAttributeSamplerShape::RenderShapeIFP(bool isSelected) const
{
	UWorld *world = GetWorld();
	const FColor	debugColor = isSelected ? kSamplerShapesDebugColorSelected : kSamplerShapesDebugColor;
	const FVector	location = GetComponentTransform().GetLocation();
	const float		height = FGenericPlatformMath::Max(Properties.Height, 0.0f);
	const float		radius = FGenericPlatformMath::Max(Properties.Radius, 0.0f);
	const FVector	upVector = GetUpVector();
	switch (Properties.ShapeType)
	{
	case	EPopcornFXAttribSamplerShapeType::Box:
		DrawDebugBox(world, location, FVector(Properties.BoxDimension * 0.5f), GetComponentTransform().GetRotation(), debugColor);
		break;
	case	EPopcornFXAttribSamplerShapeType::Sphere:
		DrawDebugSphere(world, location, radius, kSamplerShapesDebugSegmentCount, debugColor);
		break;
		//case	EPopcornFXAttribSamplerShapeType::Ellipsoid:
	case	EPopcornFXAttribSamplerShapeType::Cylinder:
		DrawDebugCylinder(GetWorld(), location - upVector * height * 0.5f, location + upVector * height * 0.5f, radius, kSamplerShapesDebugSegmentCount, debugColor);
		break;
	case	EPopcornFXAttribSamplerShapeType::Capsule:
		DrawDebugCapsule(world, location, (height / 2.0f) + radius, radius, GetComponentTransform().GetRotation(), debugColor);
		break;
	case	EPopcornFXAttribSamplerShapeType::Cone:
	{
		const float		sideLen = FGenericPlatformMath::Sqrt(height * height + radius * radius);
		const float		angle = FGenericPlatformMath::Atan(radius / height);
		DrawDebugCone(world, location + upVector * height, -upVector, sideLen, angle, angle, kSamplerShapesDebugSegmentCount, debugColor);
		break;
	}
	//case	EPopcornFXAttribSamplerShapeType::Spline:
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	case	EPopcornFXAttribSamplerShapeType::Collection:
	{
		if (isSelected)
		{
			const u32	subShapeCount = Shapes.Num();
			for (u32 iSubShape = 0; iSubShape < subShapeCount; ++iSubShape)
			{
				const APopcornFXAttributeSamplerActor *subShapeActor = Shapes[iSubShape];
				if (subShapeActor != null && subShapeActor->Sampler != null)
					subShapeActor->Sampler->_AttribSampler_IndirectSelectedThisTick();
			}
		}
		// if not select, the sub shape will render themselves
	}
#endif
	//case	EPopcornFXAttribSamplerShapeType::Sweep:
	default:
	{
		break;
	}
	}
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerShape::SetComponentTickingGroup(USkinnedMeshComponent *skinnedMesh)
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
	const TArray<int32> &GetMasterBoneMap(const USkinnedMeshComponent *skinnedMesh)
	{
		return skinnedMesh->GetLeaderBoneMap();
	}

	const TArray<FSkelMeshRenderSection> &GetSections(const FSkeletalMeshLODRenderData *lodRenderData)
	{
		return lodRenderData->RenderSections;
	}

	const TArray<FTransform> &GetComponentSpaceTransforms(const USkinnedMeshComponent *comp)
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
			m_SkeletalMesh = Cast<USkeletalMesh>(m_SkinnedMeshComponent->GetSkinnedAsset());
			if (m_SkeletalMesh == null)
				return false;

			const FSkeletalMeshRenderData *skelMeshRenderData = m_SkinnedMeshComponent->GetSkeletalMeshRenderData();
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
	bool	_FillBuffers(FAttributeSamplerShapeData *data, SBuildSkinnedMesh &buildDesc, PopcornFX::CMeshVStream &vstream, PopcornFX::CBaseSkinningStreams *skinningStreams)
	{
		PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::FillBuffers", POPCORNFX_UE_PROFILER_COLOR);

		const TStridedMemoryView<CFloat3>	srcPositionsView = vstream.Positions();

		const PopcornFX::TMemoryView<const _IndexType>	srcBoneIndices(skinningStreams->IndexStream<_IndexType>(), skinningStreams->Count());
		const PopcornFX::TMemoryView<const float>		srcBoneWeights(skinningStreams->WeightStream(), skinningStreams->Count());

		_IndexType *__restrict boneIndices = reinterpret_cast<_IndexType *>(data->m_BoneIndices.RawDataPointer());

		const bool	skin = (buildDesc.m_BuildFlags & (Build_Positions | Build_Normals | Build_Tangents)) != 0;
		if (skin)
		{
			PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::Copy src positions/normals/tangents", POPCORNFX_UE_PROFILER_COLOR);

			if (buildDesc.m_BuildFlags & Build_Positions)
			{
				if (!PK_VERIFY(srcPositionsView.Count() == data->m_DstPositions.Count()))
					return false;
				PK_ASSERT(srcPositionsView.Stride() == data->m_DstPositions.Stride());
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
				const TStridedMemoryView<CFloat4>	srcTangentsView = vstream.Tangents();
				if (!PK_VERIFY(srcTangentsView.Count() == data->m_DstTangents.Count()))
					return false;
				PK_ASSERT(srcTangentsView.Stride() == data->m_DstTangents.Stride());
				data->m_SkinContext.m_SrcTangents = srcTangentsView;
				PopcornFX::Mem::Copy(data->m_DstTangents.RawDataPointer(), srcTangentsView.Data(), sizeof(CFloat4) * srcTangentsView.Count());
			}
		}

		const USkinnedMeshComponent *masterPoseComponent = buildDesc.m_SkinnedMeshComponent->LeaderPoseComponent.Get();
		const bool					hasMasterPoseComponent = masterPoseComponent != null;
		if (hasMasterPoseComponent)
		{
			PK_ASSERT(GetMasterBoneMap(buildDesc.m_SkinnedMeshComponent).Num() == Cast<USkeletalMesh>(buildDesc.m_SkinnedMeshComponent->GetSkinnedAsset())->GetRefSkeleton().GetNum());
		}

		PK_ONLY_IF_ASSERTS(u32 totalVertices = 0);

		const float		scale = FPopcornFXPlugin::GlobalScale();
		const float		eSq = KINDA_SMALL_NUMBER * KINDA_SMALL_NUMBER;

		// Pack the vertices bone weights/indices from sparse sections
		const u32		sectionCount = GetSections(buildDesc.m_LODRenderData).Num();
		for (u32 iSection = 0; iSection < sectionCount; ++iSection)
		{
			PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::FillBuffers (Build section)", POPCORNFX_UE_PROFILER_COLOR);

			const FSkelMeshRenderSection &section = GetSections(buildDesc.m_LODRenderData)[iSection];
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
				PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::Section - cloth", POPCORNFX_UE_PROFILER_COLOR);
				const s32	clothingAssetIndex = buildDesc.m_SkeletalMesh->GetClothingAssetIndex(section.ClothingData.AssetGuid);
				if (PK_VERIFY(clothingAssetIndex != INDEX_NONE))
				{
					UClothingAssetCommon *clothAsset = Cast<UClothingAssetCommon>(buildDesc.m_SkeletalMesh->GetMeshClothingAssets()[clothingAssetIndex]);

					if (clothAsset != null && clothAsset->LodData.Num() > 0)
					{
						const FClothLODDataCommon &lodData = clothAsset->LodData[0];
						const FClothPhysicalMeshData &physMeshData = lodData.PhysicalMeshData;

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
						UE_LOG(LogPopcornFXAttributeSamplerShape, Warning, TEXT("Couldn't build cloth data for asset '%s', section %d: No cloth LOD data available"), *buildDesc.m_SkeletalMesh->GetName(), iSection);
					}
				}
			}

			if (skin && (buildClothIndices || hasMasterPoseComponent))
			{
				PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::Section - Bindpose", POPCORNFX_UE_PROFILER_COLOR);

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
							UE_LOG(LogPopcornFXAttributeSamplerShape, Warning, TEXT("Couldn't build cloth LUT for asset '%s', section %d: Legacy APEX cloth assets not supported"), *buildDesc.m_SkeletalMesh->GetName(), iSection);
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

bool	UPopcornFXAttributeSamplerShape::BuildInitialPose()
{
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::BuildInitialPose", POPCORNFX_UE_PROFILER_COLOR);

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
		UE_LOG(LogPopcornFXAttributeSamplerShape, Warning, TEXT("Cannot build mesh '%s' for sampling: Unlimited bone influences not supported for sampling"), *buildDesc.m_SkinnedMeshComponent->GetSkinnedAsset()->GetName());
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
		UE_LOG(LogPopcornFXAttributeSamplerShape, Warning, TEXT("Cannot build mesh '%s' for sampling: PopcornFX mesh was not built"), *buildDesc.m_SkinnedMeshComponent->GetSkinnedAsset()->GetName());
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

	const bool	skin = Properties.bSkinPositions || Properties.bSkinNormals || Properties.bSkinTangents;
	{
		PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::BuildMeshVertexDecl", POPCORNFX_UE_PROFILER_COLOR);

		if (Properties.bSkinPositions)
		{
			if (!PK_VERIFY(m_Data->m_DstPositions.Resize(buildDesc.m_TotalVertexCount)))
				return false;
			m_Data->m_SkinContext.m_DstPositions = PopcornFX::TStridedMemoryView<CFloat3>(reinterpret_cast<CFloat3*>(m_Data->m_DstPositions.RawDataPointer()), m_Data->m_DstPositions.Count(), 16);
			m_Data->m_Override.m_PositionsOverride = m_Data->m_SkinContext.m_DstPositions;
			buildDesc.m_BuildFlags |= Build_Positions;
		}
		if (Properties.bSkinNormals)
		{
			if (!PK_VERIFY(m_Data->m_DstNormals.Resize(buildDesc.m_TotalVertexCount)))
				return false;
			m_Data->m_SkinContext.m_DstNormals = PopcornFX::TStridedMemoryView<CFloat3>(reinterpret_cast<CFloat3*>(m_Data->m_DstNormals.RawDataPointer()), m_Data->m_DstNormals.Count(), 16);
			m_Data->m_Override.m_NormalsOverride = m_Data->m_SkinContext.m_DstNormals;
			buildDesc.m_BuildFlags |= Build_Normals;
		}
		if (Properties.bSkinTangents)
		{
			if (!PK_VERIFY(m_Data->m_DstTangents.Resize(buildDesc.m_TotalVertexCount)))
				return false;
			m_Data->m_SkinContext.m_DstTangents = PopcornFX::TStridedMemoryView<CFloat4>(reinterpret_cast<CFloat4*>(m_Data->m_DstTangents.RawDataPointer()), m_Data->m_DstTangents.Count(), 16);
			m_Data->m_Override.m_TangentsOverride = m_Data->m_DstTangents;
			buildDesc.m_BuildFlags |= Build_Tangents;
		}
		if (Properties.bBuildClothData)
			buildDesc.m_BuildFlags |= Build_Cloth;
		if (Properties.bComputeVelocities)
		{
			// There should be at least position skinning
			PK_ASSERT(Properties.bSkinPositions && skin);
			if (!PK_VERIFY(m_Data->m_DstVelocities.Resize(buildDesc.m_TotalVertexCount)) ||
				!PK_VERIFY(m_Data->m_OldPositions.Resize(buildDesc.m_TotalVertexCount)))
				return false;
			// No need to clear the oldPositions
			PopcornFX::Mem::Clear(m_Data->m_DstVelocities.RawDataPointer(), sizeof(CFloat4) * buildDesc.m_TotalVertexCount);

			m_Data->m_SkinContext.m_CustomProcess_PreSkin = PopcornFX::SSkinContext::CbCustomProcess(this, &UPopcornFXAttributeSamplerShape::Skin_PreProcess);
			m_Data->m_Override.m_VelocitiesOverride = PopcornFX::TStridedMemoryView<CFloat3>(reinterpret_cast<CFloat3*>(m_Data->m_DstVelocities.RawDataPointer()), m_Data->m_DstVelocities.Count(), 16);
		}
		m_Data->m_SkinContext.m_CustomProcess_PostSkin = PopcornFX::SSkinContext::CbCustomProcess(this, &UPopcornFXAttributeSamplerShape::Skin_PostProcess);
		m_Data->m_SkinContext.m_CustomFinish = PopcornFX::SSkinContext::CbCustomFinish(this, &UPopcornFXAttributeSamplerShape::Skin_Finish);
	}

	// No need to create bone weights/indices if we don't skin anything..
	const u32	sectionCount = GetSections(buildDesc.m_LODRenderData).Num();
	const bool	hasMasterPoseComponent = buildDesc.m_SkinnedMeshComponent->LeaderPoseComponent.Get() != null;
	if (skin && hasMasterPoseComponent) // No master pose component? Use directly the src mesh indices
	{
		PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::Alloc skinning buffers", POPCORNFX_UE_PROFILER_COLOR);

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
		PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::BuildAccelStructs", POPCORNFX_UE_PROFILER_COLOR);

		// No need to build sampling structs as we override them
		//meshNew->SetupRuntimeStructsIFN(false);

		bool	success = false;

		if (Properties.ShapeSamplingMode == EPopcornFXMeshSamplingMode::Type::Weighted)
			success = meshNew->SetupSurfaceSamplingAccelStructs(0, Properties.DensityColorChannel, m_Data->m_OverrideSurfaceSamplingAccelStructs, &m_Data->m_OverrideSurfaceSamplingWorkingBuffers);
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

bool	UPopcornFXAttributeSamplerShape::UpdateSkinning()
{
	PK_ASSERT(IsInGameThread());
	PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerShape::UpdateSkinning", POPCORNFX_UE_PROFILER_COLOR);

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
	
	const USkinnedMeshComponent	*baseComponent = skinnedMesh->LeaderPoseComponent.IsValid() ? skinnedMesh->LeaderPoseComponent.Get() : skinnedMesh;
	if (!PK_VERIFY(boneCount <= (u32)GetComponentSpaceTransforms(baseComponent).Num()) || // <= boneCount will be greater if virtual bones are present in the skeleton
		!PK_VERIFY(boneCount == Cast<USkeletalMesh>(skinnedMesh->GetSkinnedAsset())->GetRefBasesInvMatrix().Num()))
	{
		UE_LOG(LogPopcornFXAttributeSamplerShape, Warning, TEXT("Mismatching bone counts: make sure to rebuild bind pose for asset '%s'"), *skinnedMesh->GetSkinnedAsset()->GetName());
		return false;
	}

	const TArray<uint8>			&boneVisibilityStates = skinnedMesh->GetBoneVisibilityStates();
	const USkeletalMesh			*mesh = Cast<USkeletalMesh>(skinnedMesh->GetSkinnedAsset());
	const TArray<FTransform>	&spaceBases = GetComponentSpaceTransforms(baseComponent);

	const TArray<FMatrix44f>	&refBasesInvMatrix=  mesh->GetRefBasesInvMatrix();
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
		if (Properties.bBuildClothData &&
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

void	UPopcornFXAttributeSamplerShape::UpdateTransforms()
{
	m_WorldTr_Previous = m_WorldTr_Current;
	m_Angular_Velocity = FVector3f(0);
	m_Linear_Velocity = FVector3f(ComponentVelocity);

	if (!m_Data->m_CurrentSkinnedMeshComponent.IsValid() &&
		(Properties.Transforms == EPopcornFXSkinnedTransforms::SkinnedComponentRelativeTr ||
			Properties.Transforms == EPopcornFXSkinnedTransforms::SkinnedComponentWorldTr))
	{
		// No transforms update, keep previously created transforms, if any
		return;
	}

	switch (Properties.Transforms)
	{
	case	EPopcornFXSkinnedTransforms::SkinnedComponentRelativeTr:
		if (Properties.bApplyScale)
			m_WorldTr_Current = (FMatrix44f)m_Data->m_CurrentSkinnedMeshComponent->GetRelativeTransform().ToMatrixWithScale();
		else
			m_WorldTr_Current = (FMatrix44f)m_Data->m_CurrentSkinnedMeshComponent->GetRelativeTransform().ToMatrixNoScale();
		break;
	case	EPopcornFXSkinnedTransforms::SkinnedComponentWorldTr:
		if (Properties.bApplyScale)
			m_WorldTr_Current = (FMatrix44f)m_Data->m_CurrentSkinnedMeshComponent->GetComponentTransform().ToMatrixWithScale();
		else
			m_WorldTr_Current = (FMatrix44f)m_Data->m_CurrentSkinnedMeshComponent->GetComponentTransform().ToMatrixNoScale();
		break;
	case	EPopcornFXSkinnedTransforms::AttrSamplerRelativeTr:
		if (Properties.bApplyScale)
			m_WorldTr_Current = (FMatrix44f)GetRelativeTransform().ToMatrixWithScale();
		else
			m_WorldTr_Current = (FMatrix44f)GetRelativeTransform().ToMatrixNoScale();
		break;
	case	EPopcornFXSkinnedTransforms::AttrSamplerWorldTr:
		if (Properties.bApplyScale)
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

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerShape::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	check(m_Data != null);

	const PopcornFX::CResourceDescriptor_Shape	*defaultShapeSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Shape>(defaultSampler);
	if (!PK_VERIFY(defaultShapeSampler != null))
		return null;
	if (Properties.ShapeType == EPopcornFXAttribSamplerShapeType::Type::SkeletalMesh)
	{
		Properties.bPauseSkinning = false;
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
		shapeDesc->m_WorldTr_Current = &_Reinterpret<const CFloat4x4>(m_WorldTr_Current);
		shapeDesc->m_WorldTr_Previous = &_Reinterpret<CFloat4x4>(m_WorldTr_Previous);
		shapeDesc->m_Angular_Velocity = &_Reinterpret<const CFloat3>(m_Angular_Velocity);
		shapeDesc->m_Linear_Velocity = &_Reinterpret<CFloat3>(m_Linear_Velocity);

		UpdateTransforms();

		desc.m_NeedUpdate = true;

		return shapeDesc;
	}

	m_Data->m_SamplerSurface = PopcornFX::CMeshSurfaceSamplerStructuresRandom();

	if (!InitShape())
		return null;
	desc.m_NeedUpdate = true;
	_AttribSampler_PreUpdate(0.f);
	return m_Data->m_Desc.Get();
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerShape::InitShape()
{
	bool	needsInit = false;
	if (m_Data->m_Desc == null)
	{
		m_Data->m_Desc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_Shape_Default());
		needsInit = true;
	}
	PopcornFX::CParticleSamplerDescriptor_Shape_Default	*shapeDesc = m_Data->m_Desc.Get();
	if (!PK_VERIFY(shapeDesc != null))
		return false;
	if (needsInit)
	{
		shapeDesc->m_WorldTr_Current = reinterpret_cast<const CFloat4x4*>(&(m_WorldTr_Current));
		shapeDesc->m_WorldTr_Previous = reinterpret_cast<CFloat4x4*>(&(m_WorldTr_Previous));
		shapeDesc->m_Angular_Velocity = reinterpret_cast<const CFloat3*>(&(m_Angular_Velocity));
		shapeDesc->m_Linear_Velocity = reinterpret_cast<CFloat3*>(&(m_Linear_Velocity));
	}

	if (Properties.ShapeType == EPopcornFXAttribSamplerShapeType::StaticMesh)
	{
		if (Properties.StaticMesh == null)
			return false;
		if (m_Data->m_StaticMesh != Properties.StaticMesh &&
			Properties.StaticMesh != null) // Keep the old valid shape bound
		{
			m_Data->m_StaticMesh = Properties.StaticMesh;
			m_Data->m_Shape = null; // rebuild
		}
	}

	if (m_Data->m_Shape != null)
	{
		if (PK_VERIFY(shapeDesc->m_Shape == m_Data->m_Shape) &&
			PK_VERIFY(m_Data->m_Shape->ShapeType() == ToPkShapeType(Properties.ShapeType)))
		{
			(*kCbUpdateShapeDescriptors[Properties.ShapeType])(SUpdateShapeParams{this, m_Data->m_Shape.Get()}); // Simply update the already existing shape
		}
		else
			m_Data->m_Shape = null; // something wrong happened, rebuild the shape
	}

	if (m_Data->m_Shape == null)
	{
		shapeDesc->m_Shape = null;
		m_Data->m_Shape = (*kCbNewDescriptors[Properties.ShapeType.GetValue()])(SNewShapeParams{this});
		if (!PK_VERIFY(m_Data->m_Shape != null))
			return false;
		m_Data->m_Shape->m_Weight = Properties.Weight;
		shapeDesc->m_Shape = m_Data->m_Shape;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerShape::IsValid() const
{
	return m_Data != null && m_Data->m_Shape != null;
}

//----------------------------------------------------------------------------

PopcornFX::CShapeDescriptor	*UPopcornFXAttributeSamplerShape::GetShapeDescriptor() const
{
	PK_ASSERT(m_Data != null);
	return m_Data->m_Shape.Get();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::_AttribSampler_PreUpdate(float deltaTime)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerShape::_AttribSampler_PreUpdate", POPCORNFX_UE_PROFILER_COLOR);

	if (Properties.ShapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh)
	{
		if (m_LastFrameUpdate == GFrameCounter)
			return;
		m_LastFrameUpdate = GFrameCounter;

		UpdateTransforms();

		if (m_Data->m_CurrentSkinnedMeshComponent.IsValid() &&
			m_Data->m_ShouldUpdateTransforms)
		{
			PK_ASSERT(Properties.bSkinPositions || Properties.bSkinNormals || Properties.bSkinTangents);
			SCOPE_CYCLE_COUNTER(STAT_PopcornFX_SkinningWaitTime);

			PopcornFX::CSkeletalSkinnerSimple::AsyncSkinWait(m_Data->m_AsyncSkinContext, null);

			if (!Properties.bPauseSkinning)
				m_Data->m_AccumulatedDts = 0.0f;
		}
	}
	else
	{
		check(m_Data != null);

		//PK_ASSERT(m_LastFrameUpdate != GFrameCounter);
		if (m_LastFrameUpdate != GFrameCounter)
		{
			m_WorldTr_Previous = m_WorldTr_Current;
			m_LastFrameUpdate = GFrameCounter;
		}

		if (Properties.bUseRelativeTransform)
		{
			m_WorldTr_Current = (FMatrix44f)GetRelativeTransform().ToMatrixNoScale();
		}
		else
		{
			m_WorldTr_Current = (FMatrix44f)GetComponentTransform().ToMatrixNoScale();
		}

		const float	invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
		if (IsCollection() && IsValid())
		{
			PK_ASSERT(m_Data->m_Desc != null);
			PK_ASSERT(m_Data->m_Desc->m_Shape != null);
			PK_ASSERT(m_Data->m_Desc->m_Shape.Get() == m_Data->m_Shape.Get());

			PopcornFX::CShapeDescriptor_MeshCollection *shape = static_cast<PopcornFX::CShapeDescriptor_MeshCollection *>(m_Data->m_Shape.Get());
			PopcornFX::TMemoryView<const PCShapeDescriptor_Mesh>	subMeshes = shape->SubMeshes();

			bool		collectionIsDirty = false;
			u32			iSubShape = 0;
			const u32	shapeCount = Shapes.Num();
			const u32	subShapeCount = subMeshes.Count();
			for (u32 iShape = 0; iShape < shapeCount && iSubShape < subShapeCount; ++iShape)
			{
				const APopcornFXAttributeSamplerActor *attr = Shapes[iShape];

				if (attr == null)
					continue;
				const UPopcornFXAttributeSamplerShape *shapeComp = Cast<UPopcornFXAttributeSamplerShape>(attr->Sampler);
				if (shapeComp == null/* || shapeComp->IsCollection()*/)
					continue;
				PK_TODO("Remove this Gore non-const cast when the popcornfx runtime is modified the proper way ")
					CShapeDescriptor *desc = const_cast<CShapeDescriptor *>(subMeshes[iSubShape].Get());
				PK_ASSERT(desc != null);
				if (desc != shapeComp->GetShapeDescriptor())
				{
					collectionIsDirty = true;
					break;
				}

				// Don't transform the world rotation
				const FVector	transformedLocation = GetComponentTransform().InverseTransformPositionNoScale(shapeComp->GetComponentLocation());
				FMatrix			localMatrix = FTransform(shapeComp->GetComponentRotation(), transformedLocation, FVector::ZeroVector).ToMatrixNoScale();

				localMatrix.M[3][0] *= invGlobalScale;
				localMatrix.M[3][1] *= invGlobalScale;
				localMatrix.M[3][2] *= invGlobalScale;
				desc->m_LocalTransforms = ToPk(localMatrix);
				++iSubShape;
			}
			if (collectionIsDirty)
				m_Data->m_Shape = null;
		}
#endif
		m_WorldTr_Current.M[3][0] *= invGlobalScale;
		m_WorldTr_Current.M[3][1] *= invGlobalScale;
		m_WorldTr_Current.M[3][2] *= invGlobalScale;

		PK_TODO("attribute angular linear velocity");
		m_Angular_Velocity = FVector3f(0);
		m_Linear_Velocity = FVector3f(ComponentVelocity);
	}
}

#undef LOCTEXT_NAMESPACE
