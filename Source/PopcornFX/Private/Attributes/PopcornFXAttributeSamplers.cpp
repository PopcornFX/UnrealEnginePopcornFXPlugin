//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "PopcornFXSDK.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXAttributeSamplerActor.h"
#include "PopcornFXAttributeList.h"
#include "PopcornFXAttributeSampler.h"
#include "PopcornFXAttributeSamplerShape.h"
#include "PopcornFXAttributeSamplerSkinnedMesh.h"
#include "PopcornFXAttributeSamplerImage.h"
#include "PopcornFXAttributeSamplerText.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXAttributeSamplerCurveDynamic.h"
#include "PopcornFXAttributeSamplerAnimTrack.h"
#include "PopcornFXAttributeSamplerVectorField.h"
#include "GPUSim/PopcornFXGPUSim.h"
#include "Assets/PopcornFXMesh.h"
#include "Assets/PopcornFXTextureAtlas.h"
#include "Internal/ResourceHandlerImage_UE.h"

#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"
#include "DrawDebugHelpers.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveLinearColor.h"
#include "Math/InterpCurve.h"
#include "VectorField/VectorFieldStatic.h"
#include "Serialization/BulkData.h"
#include "Curves/RichCurve.h"

#if WITH_EDITOR
#	include "Editor.h"
#	include "Engine/Selection.h"
#endif

#include "PopcornFXSDK.h"
#include <pk_imaging/include/im_image.h>
#include <pk_particles/include/ps_descriptor.h>
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_kernel/include/kr_containers_array.h>
#include <pk_kernel/include/kr_containers_onstack.h>
#include <pk_kernel/include/kr_refcounted_buffer.h>
#include <pk_kernel/include/kr_mem_utils.h>
#include <pk_maths/include/pk_maths_interpolable.h>
#include <pk_geometrics/include/ge_rectangle_list.h>
#include <pk_geometrics/include/ge_mesh_sampler_accel.h>

#include <pk_kernel/include/kr_containers_array.h>
#include <pk_maths/include/pk_maths_fp16.h>

#include <pk_particles/include/ps_font_metrics.h>

#if (PK_GPU_D3D12 != 0)
#	include <pk_particles/include/Samplers/D3D12/image_gpu_d3d12.h>
#	include <pk_particles/include/Samplers/D3D12/rectangle_list_gpu_d3d12.h>
#	include "Windows/HideWindowsPlatformTypes.h"
#	include "D3D12RHIPrivate.h"
#	include "D3D12Util.h"
#endif // (PK_GPU_D3D12 != 0)

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSampler"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSampler, Log, All);

//----------------------------------------------------------------------------
//
// APopcornFXAttributeSamplerActor
//
//----------------------------------------------------------------------------

APopcornFXAttributeSamplerActor::APopcornFXAttributeSamplerActor(const FObjectInitializer &PCIP)
:	Super(PCIP)
,	Sampler(null)
,	m_SamplerComponentType(EPopcornFXAttributeSamplerComponentType::Shape)
{
#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (!IsRunningCommandlet() && SpriteComponent != null)
	{
		struct FConstructorStatics
		{
			FName ID_Effects;
			FText NAME_Effects;
			FConstructorStatics()
				: ID_Effects(TEXT("Effects")) // do not change, recognize by the engine
				, NAME_Effects(NSLOCTEXT("SpriteCategory", "Effects", "Effects")) // do not change, recognize by the engine
			{}
		};
		static FConstructorStatics ConstructorStatics;
		// SpriteComponent->RelativeScale3D = FVector(0.5f, 0.5f, 0.5f);

		SpriteComponent->SetRelativeScale3D(FVector(1.5f));
		SpriteComponent->bHiddenInGame = true;
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SpriteInfo.Category=ConstructorStatics.ID_Effects;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Effects;
		SpriteComponent->bReceivesDecals = false;
		// SpriteComponent->SetupAttachment() // later, when sampler will be ready
	}
#endif

	SetFlags(RF_Transactional);
}

//----------------------------------------------------------------------------

void	APopcornFXAttributeSamplerActor::_CtorRootSamplerComponent(const FObjectInitializer &PCIP, EPopcornFXAttributeSamplerComponentType::Type samplerType)
{
	m_SamplerComponentType = samplerType;
	m_IsValidSpecializedActor = true;

	UClass		*samplerClass = UPopcornFXAttributeSampler::SamplerComponentClass(m_SamplerComponentType);
	Sampler = static_cast<UPopcornFXAttributeSampler*>(
		PCIP.CreateDefaultSubobject(
			this, TEXT("Sampler"),
			UPopcornFXAttributeSampler::StaticClass(), samplerClass, true, false));

	RootComponent = Sampler;

#if WITH_EDITORONLY_DATA
	if (!IsRunningCommandlet() && SpriteComponent != null)
		SpriteComponent->SetupAttachment(RootComponent);
#endif
}

//----------------------------------------------------------------------------

APopcornFXAttributeSamplerShapeActor::APopcornFXAttributeSamplerShapeActor(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	_CtorRootSamplerComponent(PCIP, EPopcornFXAttributeSamplerComponentType::Shape);
}

APopcornFXAttributeSamplerSkinnedMeshActor::APopcornFXAttributeSamplerSkinnedMeshActor(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	_CtorRootSamplerComponent(PCIP, EPopcornFXAttributeSamplerComponentType::SkinnedMesh);
}

APopcornFXAttributeSamplerImageActor::APopcornFXAttributeSamplerImageActor(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	_CtorRootSamplerComponent(PCIP, EPopcornFXAttributeSamplerComponentType::Image);
}

APopcornFXAttributeSamplerCurveActor::APopcornFXAttributeSamplerCurveActor(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	_CtorRootSamplerComponent(PCIP, EPopcornFXAttributeSamplerComponentType::Curve);
}

APopcornFXAttributeSamplerAnimTrackActor::APopcornFXAttributeSamplerAnimTrackActor(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	_CtorRootSamplerComponent(PCIP, EPopcornFXAttributeSamplerComponentType::AnimTrack);
}

APopcornFXAttributeSamplerTextActor::APopcornFXAttributeSamplerTextActor(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	_CtorRootSamplerComponent(PCIP, EPopcornFXAttributeSamplerComponentType::Text);
}

APopcornFXAttributeSamplerVectorFieldActor::APopcornFXAttributeSamplerVectorFieldActor(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	_CtorRootSamplerComponent(PCIP, EPopcornFXAttributeSamplerComponentType::Turbulence);
}

//----------------------------------------------------------------------------
#if WITH_EDITOR
//----------------------------------------------------------------------------

#if WITH_EDITORONLY_DATA
void		APopcornFXAttributeSamplerActor::ReloadSprite()
{
	if (IsRunningCommandlet())
		return;
	if (!PK_VERIFY(SpriteComponent != null))
		return;

	FString		spritePath;
	if (m_IsValidSpecializedActor)
	{
		FString	spriteName;
		switch (m_SamplerComponentType)
		{
		case	EPopcornFXAttributeSamplerComponentType::Shape:
			spriteName = "AttributeSampler_Shape";
			break;
		case	EPopcornFXAttributeSamplerComponentType::SkinnedMesh:
			spriteName = "AttributeSampler_SkeletalMesh";
			break;
		case	EPopcornFXAttributeSamplerComponentType::Image:
			spriteName = "AttributeSampler_Image";
			break;
		case	EPopcornFXAttributeSamplerComponentType::AnimTrack:
			spriteName = "Attributesampler_AnimTrack";
			break;
		case EPopcornFXAttributeSamplerComponentType::Curve:
			spriteName = "AttributeSampler_Curve";
			break;
		case	EPopcornFXAttributeSamplerComponentType::Text:
			spriteName = "AttributeSampler_Text";
			break;
		case	EPopcornFXAttributeSamplerComponentType::Turbulence:
			spriteName = "AttributeSampler_VectorField";
			break;
		default:
			spriteName = "AttributeSampler_Shape";
			break;
		}
		spritePath = "/PopcornFX/SlateBrushes/" + spriteName + "." + spriteName;
	}
	else
	{
		// Baad, should be replaced by the actual Actor !
		spritePath = "/PopcornFX/SlateBrushes/BadIcon.BadIcon";
	}

	if (!spritePath.IsEmpty())
	{
		SpriteComponent->SetSprite(LoadObject<UTexture2D>(null, *spritePath));
		SpriteComponent->MarkForNeededEndOfFrameRecreate();
	}

	if (RootComponent != null && SpriteComponent->GetAttachmentRoot() != RootComponent)
	{
		SpriteComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale, NAME_None);
	}
}
#endif // WITH_EDITORONLY_DATA

//----------------------------------------------------------------------------

void	APopcornFXAttributeSamplerActor::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.Property != NULL)
	{
		if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(APopcornFXAttributeSamplerActor, m_SamplerComponentType) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(APopcornFXAttributeSamplerActor, Sampler))
		{
			ReloadSprite();
		}
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
//----------------------------------------------------------------------------

#if WITH_EDITORONLY_DATA
void	APopcornFXAttributeSamplerActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	ReloadSprite();
}
#endif

//----------------------------------------------------------------------------

void		APopcornFXAttributeSamplerActor::PostLoad()
{
	Super::PostLoad();
#if WITH_EDITOR
	if (!m_IsValidSpecializedActor) // else, the saved Sprite should already be the right one
	{
		SpriteComponent->ConditionalPostLoad();
		Sampler->ConditionalPostLoad();
		ReloadSprite();
	}
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------------

void		APopcornFXAttributeSamplerActor::PostActorCreated()
{
	Super::PostActorCreated();
#if WITH_EDITOR
	ReloadSprite();
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSampler
//
//----------------------------------------------------------------------------

// static
UClass		*UPopcornFXAttributeSampler::SamplerComponentClass(EPopcornFXAttributeSamplerComponentType::Type type)
{
	switch (type)
	{
	case	EPopcornFXAttributeSamplerComponentType::Shape:
		return UPopcornFXAttributeSamplerShape::StaticClass();
	case	EPopcornFXAttributeSamplerComponentType::SkinnedMesh:
		return UPopcornFXAttributeSamplerSkinnedMesh::StaticClass();
	case	EPopcornFXAttributeSamplerComponentType::Image:
		return UPopcornFXAttributeSamplerImage::StaticClass();
	case	EPopcornFXAttributeSamplerComponentType::Curve:
		return UPopcornFXAttributeSamplerCurve::StaticClass();
	case	EPopcornFXAttributeSamplerComponentType::AnimTrack:
		return UPopcornFXAttributeSamplerAnimTrack::StaticClass();
	case	EPopcornFXAttributeSamplerComponentType::Text:
		return UPopcornFXAttributeSamplerText::StaticClass();
	case	EPopcornFXAttributeSamplerComponentType::Turbulence:
		return UPopcornFXAttributeSamplerVectorField::StaticClass();
	}
	// dummy attribute sampler
	return UPopcornFXAttributeSampler::StaticClass();
}

UPopcornFXAttributeSampler::UPopcornFXAttributeSampler(const FObjectInitializer &PCIP)
:	Super(PCIP)
,	m_SamplerType(EPopcornFXAttributeSamplerType::None)
{
	SetFlags(RF_Transactional);
}

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

		if (self->ShapeSamplingMode == EPopcornFXMeshSamplingMode::Type::Weighted)
			mesh->SetupSurfaceSamplingAccelStructs(0, self->DensityColorChannel, *surfaceSampling);
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

		PK_ASSERT(self->BoxDimension.GetMin() >= 0.0f);
		const FVector3f	safeBoxDimension = FVector3f(self->BoxDimension);
		const CFloat3	dim = ToPk(safeBoxDimension) * FPopcornFXPlugin::GlobalScaleRcp();
		return PK_NEW(PopcornFX::CShapeDescriptor_Box(dim));
	}

	// BOX
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Box>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;
		PopcornFX::CShapeDescriptor_Box			*shape = static_cast<PopcornFX::CShapeDescriptor_Box*>(params.shape);

		PK_ASSERT(self->BoxDimension.GetMin() >= 0.0f);
		const FVector3f	safeBoxDimension = FVector3f(self->BoxDimension);
		const CFloat3	dim = ToPk(safeBoxDimension) * FPopcornFXPlugin::GlobalScaleRcp();

		shape->SetDimensions(dim);
	}

	// SPHERE
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Sphere>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape		*self = params.self;

		PK_ASSERT(self->Radius >= 0.0f);
		PK_ASSERT(self->InnerRadius >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->InnerRadius, 0.0f) * invGlobalScale;

		return PK_NEW(PopcornFX::CShapeDescriptor_Sphere(radius, innerRadius));
	}

	// SPHERE
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Sphere>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape		*self = params.self;
		PopcornFX::CShapeDescriptor_Sphere			*shape = static_cast<PopcornFX::CShapeDescriptor_Sphere*>(params.shape);

		PK_ASSERT(self->Radius >= 0.0f);
		PK_ASSERT(self->InnerRadius >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->InnerRadius, 0.0f) * invGlobalScale;

		shape->SetInnerRadius(innerRadius);
		shape->SetRadius(radius);
	}

	// ELLIPSOID
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Ellipsoid>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;

		PK_ASSERT(self->Radius >= 0.0f);
		PK_ASSERT(self->InnerRadius >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->InnerRadius, 0.0f) * invGlobalScale;

		return PK_NEW(PopcornFX::CShapeDescriptor_Ellipsoid(radius, innerRadius));
	}

	// ELLIPSOID
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Ellipsoid>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;
		PopcornFX::CShapeDescriptor_Ellipsoid	*shape = static_cast<PopcornFX::CShapeDescriptor_Ellipsoid*>(params.shape);

		PK_ASSERT(self->Radius >= 0.0f);
		PK_ASSERT(self->InnerRadius >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->InnerRadius, 0.0f) * invGlobalScale;

		shape->SetInnerRadius(innerRadius);
		shape->SetRadius(radius);
	}

	// CYLINDRE
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Cylinder>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;

		PK_ASSERT(self->Radius >= 0.0f);
		PK_ASSERT(self->InnerRadius >= 0.0f);
		PK_ASSERT(self->Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->InnerRadius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Height, 0.0f) * invGlobalScale;

		return PK_NEW(PopcornFX::CShapeDescriptor_Cylinder(radius, height, innerRadius));
	}

	// CYLINDRE
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Cylinder>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;
		PopcornFX::CShapeDescriptor_Cylinder	*shape = static_cast<PopcornFX::CShapeDescriptor_Cylinder*>(params.shape);

		PK_ASSERT(self->Radius >= 0.0f);
		PK_ASSERT(self->InnerRadius >= 0.0f);
		PK_ASSERT(self->Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->InnerRadius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Height, 0.0f) * invGlobalScale;

		shape->SetInnerRadius(innerRadius);
		shape->SetRadius(radius);
		shape->SetHeight(height);
	}

	// CAPSULE
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Capsule>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;

		PK_ASSERT(self->Radius >= 0.0f);
		PK_ASSERT(self->InnerRadius >= 0.0f);
		PK_ASSERT(self->Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->InnerRadius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Height, 0.0f) * invGlobalScale;

		return PK_NEW(PopcornFX::CShapeDescriptor_Capsule(radius, height, innerRadius));
	}

	// CAPSULE
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Capsule>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;
		PopcornFX::CShapeDescriptor_Capsule		*shape = static_cast<PopcornFX::CShapeDescriptor_Capsule*>(params.shape);

		PK_ASSERT(self->Radius >= 0.0f);
		PK_ASSERT(self->InnerRadius >= 0.0f);
		PK_ASSERT(self->Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Radius, 0.0f) * invGlobalScale;
		const float			innerRadius = FGenericPlatformMath::Max(self->InnerRadius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Height, 0.0f) * invGlobalScale;

		shape->SetInnerRadius(innerRadius);
		shape->SetRadius(radius);
		shape->SetHeight(height);
	}

	// CONE
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Cone>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;

		PK_ASSERT(self->Radius >= 0.0f);
		PK_ASSERT(self->Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Radius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Height, 0.0f) * invGlobalScale;

		return PK_NEW(PopcornFX::CShapeDescriptor_Cone(radius, height));
	}

	// CONE
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Cone>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;
		PopcornFX::CShapeDescriptor_Cone		*shape = static_cast<PopcornFX::CShapeDescriptor_Cone*>(params.shape);

		PK_ASSERT(self->Radius >= 0.0f);
		PK_ASSERT(self->Height >= 0.0f);

		const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const float			radius = FGenericPlatformMath::Max(self->Radius, 0.0f) * invGlobalScale;
		const float			height = FGenericPlatformMath::Max(self->Height, 0.0f) * invGlobalScale;

		shape->SetRadius(radius);
		shape->SetHeight(height);
	}

	// MESH
	template <>
	CShapeDescriptor	*_NewDescriptor<PopcornFX::CShapeDescriptor_Mesh>(const SNewShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape	*self = params.self;

		if (self->StaticMesh == null)
			return null;

		PK_FIXME("use the ResourceManager with TResourcePtr !!");
		UPopcornFXMesh		*pkMesh = UPopcornFXMesh::FindStaticMesh(self->StaticMesh);
		if (pkMesh == null)
			return null;

		PopcornFX::PResourceMesh	meshRes = pkMesh->LoadResourceMeshIFN(true);
		if (meshRes == null)
			return null;

		const u32					batchCount = meshRes->BatchList().Count();
		if (!PK_VERIFY(batchCount != 0))
			return null;

		const u32								batchi = PopcornFX::PKMin(u32(PopcornFX::PKMax(self->StaticMeshSubIndex, 0)), batchCount - 1);
		const PopcornFX::PResourceMeshBatch		&batch = meshRes->BatchList()[batchi];
		if (!PK_VERIFY(batch != null))
			return null;
		if (!PK_VERIFY(batch->RawMesh() != null))
			return null;

		PopcornFX::CShapeDescriptor_Mesh	*shapeDesc = PK_NEW(PopcornFX::CShapeDescriptor_Mesh(batch->RawMesh()));

		_RebuildSamplingStructs(self, shapeDesc);

		return shapeDesc;
	}

	// MESH
	template <>
	void				_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Mesh>(const SUpdateShapeParams &params)
	{
		const UPopcornFXAttributeSamplerShape		*self = params.self;
		PopcornFX::CShapeDescriptor_Mesh			*shapeDesc = static_cast<PopcornFX::CShapeDescriptor_Mesh*>(params.shape);

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
		if (self->Shapes.Num() == 0) // not an error
			return shapeCollection;

		PK_STATIC_ASSERT(EPopcornFXShapeCollectionSamplingHeuristic::NoWeight			== (u32)PopcornFX::CShapeDescriptor_MeshCollection::NoWeight);
		PK_STATIC_ASSERT(EPopcornFXShapeCollectionSamplingHeuristic::WeightWithVolume	== (u32)PopcornFX::CShapeDescriptor_MeshCollection::WeightWithVolume);
		PK_STATIC_ASSERT(EPopcornFXShapeCollectionSamplingHeuristic::WeightWithSurface	== (u32)PopcornFX::CShapeDescriptor_MeshCollection::WeightWithSurface);
		PK_STATIC_ASSERT(3 == PopcornFX::CShapeDescriptor_MeshCollection::__MaxSamplingHeuristics);
		shapeCollection->SetSamplingHeuristic(static_cast<PopcornFX::CShapeDescriptor_MeshCollection::ESamplingHeuristic>(self->CollectionSamplingHeuristic.GetValue()));
		shapeCollection->SetUseSubMeshWeights(self->CollectionUseShapeWeights != 0);
//		shapeCollection->m_PermutateMultiSamples = false;	// Oh god no ! This should almost always be 'true' !!

		const float	invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		for (auto shapeIt = self->Shapes.CreateConstIterator(); shapeIt; ++shapeIt)
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
		&_NewDescriptor<PopcornFX::CShapeDescriptor_Mesh>,		//Mesh,
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
		&_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_Mesh>,		//Mesh,
		//null,		// Spline
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
		&_UpdateShapeDescriptor<PopcornFX::CShapeDescriptor_MeshCollection>,	//MeshCollection
#endif
	};

	PK_STATIC_ASSERT(PK_ARRAY_COUNT(kCbNewDescriptors) == EPopcornFXAttribSamplerShapeType_Max);
	PK_STATIC_ASSERT(PK_ARRAY_COUNT(kCbUpdateShapeDescriptors) == EPopcornFXAttribSamplerShapeType_Max);
}

struct FAttributeSamplerShapeData
{
	UStaticMesh												*m_StaticMesh = null;
	PopcornFX::PParticleSamplerDescriptor_Shape_Default		m_Desc;
	PopcornFX::PShapeDescriptor								m_Shape;
	PopcornFX::CMeshSurfaceSamplerStructuresRandom			m_SamplerSurface;
};

UPopcornFXAttributeSamplerShape::UPopcornFXAttributeSamplerShape(const FObjectInitializer &PCIP)
	: Super(PCIP)
	, m_LastFrameUpdate(0)
{
	bAutoActivate = true;

#if WITH_EDITOR
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	m_IndirectSelectedThisTick = false;
#endif // WITH_EDITOR

	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Shape;

	ShapeType = EPopcornFXAttribSamplerShapeType::Sphere;
	BoxDimension = FVector(100.f);
	Radius = 100.f;
	InnerRadius = 0.f;
	Height = 100.f;
	StaticMesh = null;
	StaticMeshSubIndex = 0;
	Weight = 1.0f;

#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	CollectionSamplingHeuristic = EPopcornFXShapeCollectionSamplingHeuristic::NoWeight;
	CollectionUseShapeWeights = 1;
#endif

	bUseRelativeTransform = true;

	m_Data = new FAttributeSamplerShapeData();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::BeginDestroy()
{
	Super::BeginDestroy();
	if (m_Data != null)
	{
		delete m_Data;
		m_Data = null;
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::SetRadius(float radius)
{
	Radius = radius;
	UpdateShapeProperties();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::SetWeight(float height)
{
	Height = height;
	UpdateShapeProperties();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::SetBoxDimension(FVector boxDimensions)
{
	BoxDimension = boxDimensions;
	UpdateShapeProperties();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::SetInnerRadius(float innerRadius)
{
	InnerRadius = innerRadius;
	UpdateShapeProperties();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::SetHeight(float height)
{
	Height = height;
	UpdateShapeProperties();
}

//----------------------------------------------------------------------------

PopcornFX::CMeshSurfaceSamplerStructuresRandom	*UPopcornFXAttributeSamplerShape::SamplerSurface() const
{
	return &m_Data->m_SamplerSurface;
}
//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerShape::CanUpdateShapeProperties()
{
	PK_ASSERT(m_Data != null);
	if (m_Data->m_Shape == null)
		return false;
	if (!PK_VERIFY(m_Data->m_Desc != null) ||
		!PK_VERIFY(m_Data->m_Desc->m_Shape != null) ||
		!PK_VERIFY(m_Data->m_Desc->m_Shape == m_Data->m_Shape) ||
		m_Data->m_Shape->ShapeType() != ToPkShapeType(ShapeType))
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
	if (!CanUpdateShapeProperties())
		return;
	PK_ASSERT(m_Data->m_Shape->ShapeType() == ToPkShapeType(ShapeType));
	(*kCbUpdateShapeDescriptors[ShapeType.GetValue()])(SUpdateShapeParams{ this, m_Data->m_Shape.Get() });
	m_Data->m_Shape->m_Weight = Weight;
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

// source_tree/hellheaven_tools/hh_components/HH-NativeEditor/src/NEdModuleConfig.cpp
// Shapes_BaseColor
static const FColor		kSamplerShapesDebugColor = FLinearColor(0.1f, 0.3f, 0.15f, 1.f).ToFColor(false);
static const FColor		kSamplerShapesDebugColorSelected = FLinearColor(0.2f, 0.5f, 0.75f, 1.f).ToFColor(false);
static const int32		kSamplerShapesDebugSegmentCount = 32;

void	UPopcornFXAttributeSamplerShape::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.MemberProperty != null)
	{
		// Don't update shapes because parent class properties were modified
		if (propertyChangedEvent.MemberProperty->GetOwnerClass() != UPopcornFXAttributeSamplerShape::StaticClass())
			return;
		if (!CanUpdateShapeProperties())
			return;
		const FString	propertyName = propertyChangedEvent.MemberProperty->GetName();
		if (propertyName == TEXT("Weight") ||
			propertyName == TEXT("BoxDimension") ||
			propertyName == TEXT("Radius") ||
			propertyName == TEXT("InnerRadius") ||
			propertyName == TEXT("Height") ||
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
			propertyName == TEXT("CollectionSamplingHeuristic") ||
			propertyName == TEXT("CollectionUseShapeWeights") ||
#endif
			propertyName == TEXT("ShapeSamplingMode") ||
			propertyName == TEXT("DensityColorChannel"))
			//(propertyName == TEXT("Shapes") && propertyChangedEvent.ChangeType != EPropertyChangeType::ArrayAdd)
		{
			PK_ASSERT(m_Data->m_Shape->ShapeType() == ToPkShapeType(ShapeType));
			(*kCbUpdateShapeDescriptors[ShapeType.GetValue()])(SUpdateShapeParams{this, m_Data->m_Shape.Get()});
			m_Data->m_Shape->m_Weight = Weight;
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

void	UPopcornFXAttributeSamplerShape::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction *thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);

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

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerShape::RenderShapeIFP(bool isSelected) const
{
	UWorld			*world = GetWorld();
	const FColor	debugColor = isSelected ? kSamplerShapesDebugColorSelected : kSamplerShapesDebugColor;
	const FVector	location = GetComponentTransform().GetLocation();
	const float		height = FGenericPlatformMath::Max(Height, 0.0f);
	const float		radius = FGenericPlatformMath::Max(Radius, 0.0f);
	const FVector	upVector = GetUpVector();
	switch (ShapeType)
	{
		case	EPopcornFXAttribSamplerShapeType::Box:
			DrawDebugBox(world, location, FVector(BoxDimension * 0.5f), GetComponentTransform().GetRotation(), debugColor);
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
					const APopcornFXAttributeSamplerActor	*subShapeActor = Shapes[iSubShape];
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

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerShape::_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	check(m_Data != null);

	m_Data->m_SamplerSurface = PopcornFX::CMeshSurfaceSamplerStructuresRandom();

	const PopcornFX::CResourceDescriptor_Shape	*defaultShapeSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Shape>(defaultSampler);
	if (!PK_VERIFY(defaultShapeSampler != null))
		return null;
	if (!PK_VERIFY(InitShape()))
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

	if (ShapeType == EPopcornFXAttribSamplerShapeType::Mesh)
	{
		if (m_Data->m_StaticMesh != StaticMesh &&
			StaticMesh != null) // Keep the old valid shape bound
		{
			m_Data->m_StaticMesh = StaticMesh;
			m_Data->m_Shape = null; // rebuild
		}
	}

	if (m_Data->m_Shape != null)
	{
		if (PK_VERIFY(shapeDesc->m_Shape == m_Data->m_Shape) &&
			PK_VERIFY(m_Data->m_Shape->ShapeType() == ToPkShapeType(ShapeType)))
		{
			(*kCbUpdateShapeDescriptors[ShapeType])(SUpdateShapeParams{this, m_Data->m_Shape.Get()}); // Simply update the already existing shape
		}
		else
			m_Data->m_Shape = null; // something wrong happened, rebuild the shape
	}

	if (m_Data->m_Shape == null)
	{
		shapeDesc->m_Shape = null;
		m_Data->m_Shape = (*kCbNewDescriptors[ShapeType.GetValue()])(SNewShapeParams{this});
		if (!PK_VERIFY(m_Data->m_Shape != null))
			return false;
		m_Data->m_Shape->m_Weight = Weight;
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
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerShape::_AttribSampler_PreUpdate", POPCORNFX_UE_PROFILER_COLOR);
	check(m_Data != null);

	//PK_ASSERT(m_LastFrameUpdate != GFrameCounter);
	if (m_LastFrameUpdate != GFrameCounter)
	{
		m_WorldTr_Previous = m_WorldTr_Current;
		m_LastFrameUpdate = GFrameCounter;
	}

	if (bUseRelativeTransform)
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

		PopcornFX::CShapeDescriptor_MeshCollection			*shape = static_cast<PopcornFX::CShapeDescriptor_MeshCollection*>(m_Data->m_Shape.Get());
		PopcornFX::TMemoryView<const PCShapeDescriptor_Mesh>	subMeshes = shape->SubMeshes();

		bool		collectionIsDirty = false;
		u32			iSubShape = 0;
		const u32	shapeCount = Shapes.Num();
		const u32	subShapeCount = subMeshes.Count();
		for (u32 iShape = 0; iShape < shapeCount && iSubShape < subShapeCount; ++iShape)
		{
			const APopcornFXAttributeSamplerActor	*attr = Shapes[iShape];

			if (attr == null)
				continue;
			const UPopcornFXAttributeSamplerShape	*shapeComp = Cast<UPopcornFXAttributeSamplerShape>(attr->Sampler);
			if (shapeComp == null/* || shapeComp->IsCollection()*/)
				continue;
			PK_TODO("Remove this Gore non-const cast when the popcornfx runtime is modified the proper way ")
			CShapeDescriptor	*desc = const_cast<CShapeDescriptor*>(subMeshes[iSubShape].Get());
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

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSamplerCurve
//
//----------------------------------------------------------------------------

#define CURVE_MINIMUM_DELTA	0.01f

//----------------------------------------------------------------------------

struct FAttributeSamplerCurveData
{
	PopcornFX::PParticleSamplerDescriptor_Curve_Default			m_CurveDesc;
	PopcornFX::PParticleSamplerDescriptor_DoubleCurve_Default	m_DoubleCurveDesc;
	PopcornFX::CCurveDescriptor									*m_Curve0 = null;
	PopcornFX::CCurveDescriptor									*m_Curve1 = null;

	bool	m_NeedsReload;

	~FAttributeSamplerCurveData()
	{
		PK_ASSERT(m_Curve0 == null);
		PK_ASSERT(m_Curve1 == null);
	}
};

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::SetCurveDimension(TEnumAsByte<EAttributeSamplerCurveDimension::Type> InCurveDimension)
{
	if (CurveDimension == InCurveDimension)
		return;
	SecondCurve1D = null;
	//SecondCurve2D = null;
	SecondCurve3D = null;
	SecondCurve4D = null;
	Curve1D = null;
	//Curve2D = null;
	Curve3D = null;
	Curve4D = null;
	CurveDimension = InCurveDimension;
	m_Data->m_NeedsReload = true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurve::SetCurve(class UCurveBase *InCurve, bool InIsSecondCurve)
{
	if (!PK_VERIFY(InCurve != null))
		return false;
	if (InIsSecondCurve)
	{
		SecondCurve1D = null;
		//SecondCurve2D = null;
		SecondCurve3D = null;
		SecondCurve4D = null;
	}
	else
	{
		Curve1D = null;
		//Curve2D = null;
		Curve3D = null;
		Curve4D = null;
	}
	bool			ok = false;
	switch (CurveDimension)
	{
	case	EAttributeSamplerCurveDimension::Float1:
		if (InIsSecondCurve)
			ok = ((SecondCurve1D = Cast<UCurveFloat>(InCurve)) != null);
		else
			ok = ((Curve1D = Cast<UCurveFloat>(InCurve)) != null);
		break;
	case	EAttributeSamplerCurveDimension::Float2:
		PK_ASSERT_NOT_REACHED();
		ok = false;
		break;
	case	EAttributeSamplerCurveDimension::Float3:
		if (InIsSecondCurve)
			ok = ((SecondCurve3D = Cast<UCurveVector>(InCurve)) != null);
		else
			ok = ((Curve3D = Cast<UCurveVector>(InCurve)) != null);
		break;
	case	EAttributeSamplerCurveDimension::Float4:
		if (InIsSecondCurve)
			ok = ((SecondCurve4D = Cast<UCurveLinearColor>(InCurve)) != null);
		else
			ok = ((Curve4D = Cast<UCurveLinearColor>(InCurve)) != null);
		break;
	}
	m_Data->m_NeedsReload = true;
	return ok;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerCurve::UPopcornFXAttributeSamplerCurve(const FObjectInitializer &PCIP)
: Super(PCIP)
{
	bAutoActivate = true;
	bIsDoubleCurve = false;
	CurveDimension = EAttributeSamplerCurveDimension::Float1;

	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Curve;

	Curve1D = null;
	Curve3D = null;
	Curve4D = null;

	m_Data = new FAttributeSamplerCurveData();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::BeginDestroy()
{
	if (m_Data != null)
	{
		if (m_Data->m_DoubleCurveDesc != null)
		{
			m_Data->m_DoubleCurveDesc->m_Curve0 = null;
			m_Data->m_DoubleCurveDesc->m_Curve1 = null;
		}
		else if (m_Data->m_CurveDesc != null)
			m_Data->m_CurveDesc->m_Curve0 = null;
		PK_SAFE_DELETE(m_Data->m_Curve0);
		PK_SAFE_DELETE(m_Data->m_Curve1);
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------
#if WITH_EDITOR

void	UPopcornFXAttributeSamplerCurve::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.Property != null)
	{
		// Always rebuild for now
		m_Data->m_NeedsReload = true;
	}
	Super::PostEditChangeProperty(propertyChangedEvent);
}

#endif // WITH_EDITOR
//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::FetchCurveData(const FRichCurve *curve, PopcornFX::CCurveDescriptor *curveDescriptor, uint32 axis)
{
	const PopcornFX::TArray<float, PopcornFX::TArrayAligned16>	&times = curveDescriptor->m_Times;
	PopcornFX::TArray<float, PopcornFX::TArrayAligned16>		&values = curveDescriptor->m_FloatValues;
	PopcornFX::TArray<float, PopcornFX::TArrayAligned16>		&tangents = curveDescriptor->m_FloatTangents;
	TArray<FRichCurveKey>::TConstIterator						keyIt = curve->GetKeyIterator();

	bool		isLinear = false;
	float		previousTangent = 0.0f;
	float		previousValue = keyIt ? (*keyIt).Value - CURVE_MINIMUM_DELTA : 0.0f;
	const u32	keyCount = times.Count();
	for (u32 i = 0; i < keyCount; ++i)
	{
		const int32	index = i * CurveDimension;
		const int32	tangentIndex = index * 2 + axis;
		const float	delta = i > 0 ? times[i] - times[i - 1] : 0.0f;

		if (keyIt && times[i] == (*keyIt).Time)
		{
			const FRichCurveKey	&key = *keyIt++;

			values[index + axis] = key.Value;
			if (key.InterpMode == ERichCurveInterpMode::RCIM_Linear)
			{
				const float	nextValue = i + 1 < keyCount ? curve->Eval(times[i + 1]) : key.Value;
				const float	arriveTangent = key.Value - previousValue;
				const float	leaveTangent = nextValue - key.Value;

				tangents[tangentIndex] = isLinear ? arriveTangent : key.ArriveTangent * delta;
				tangents[tangentIndex + CurveDimension] = leaveTangent;
				previousTangent = leaveTangent;
				previousValue = key.Value;
			}
			else
			{
				const float	nextSampledValue = curve->Eval(times[i] + CURVE_MINIMUM_DELTA);
				const float	prevSampledValue = curve->Eval(times[i] - CURVE_MINIMUM_DELTA);
				const float	nextTime = i + 1 < times.Count() ? times[i + 1] : times[i] + CURVE_MINIMUM_DELTA;
				const float	nextDelta = nextTime - times[i];

				tangents[tangentIndex] = isLinear ? previousTangent : ((nextSampledValue - prevSampledValue) / (2.0f * CURVE_MINIMUM_DELTA)) * delta;
				tangents[tangentIndex + CurveDimension] = key.LeaveTangent * nextDelta;
			}
			isLinear = key.InterpMode == ERichCurveInterpMode::RCIM_Linear;
		}
		else
		{
			const float	value = curve->Eval(times[i]);

			// We do have to eval the key value because each axis can have independant
			// key count and times..
			values[index + axis] = value;
			if (isLinear)
			{
				const float	nextValue = i + 1 < keyCount ? curve->Eval(times[i + 1]) : value;
				const float	arriveTangent = value - previousValue;
				const float	leaveTangent = nextValue - value;

				tangents[tangentIndex] = arriveTangent;
				tangents[tangentIndex + CurveDimension] = leaveTangent;
				previousTangent = leaveTangent;
				previousValue = value;
			}
			else
			{
				const float	nextSampledValue = curve->Eval(times[i] + CURVE_MINIMUM_DELTA);
				const float	prevSampledValue = curve->Eval(times[i] - CURVE_MINIMUM_DELTA);
				const float	nextTime = i + 1 < times.Count() ? times[i + 1] : times[i] + CURVE_MINIMUM_DELTA;
				const float	nextDelta = nextTime - times[i];

				tangents[tangentIndex] = ((nextSampledValue - prevSampledValue) / (2.0f * CURVE_MINIMUM_DELTA)) * delta;
				tangents[tangentIndex + CurveDimension] = ((nextSampledValue - prevSampledValue) / (2.0f * CURVE_MINIMUM_DELTA)) * nextDelta;
			}
		}
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::GetAssociatedCurves(UCurveBase *&curve0, UCurveBase *&curve1)
{
	switch (CurveDimension)
	{
		case	EAttributeSamplerCurveDimension::Float1:
		{
			curve0 = Curve1D;
			curve1 = SecondCurve1D;
			break;
		}
		case	EAttributeSamplerCurveDimension::Float2:
		{
			PK_ASSERT_NOT_REACHED();
			break;
		}
		case	EAttributeSamplerCurveDimension::Float3:
		{
			curve0 = Curve3D;
			curve1 = SecondCurve3D;
			break;
		}
		case	EAttributeSamplerCurveDimension::Float4:
		{
			curve0 = Curve4D;
			curve1 = SecondCurve4D;
			break;
		}
	}
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurve::SetupCurve(PopcornFX::CCurveDescriptor *curveDescriptor, UCurveBase *curve)
{
	static const float	kMaximumKey = 1.0f - CURVE_MINIMUM_DELTA;
	static const float	kMinimumKey = CURVE_MINIMUM_DELTA;

	TArray<FRichCurveEditInfo>	curves = curve->GetCurves();
	if (!PK_VERIFY(curves.Num() == CurveDimension))
		return false;

	curveDescriptor->m_Order = (u32)CurveDimension;
	curveDescriptor->m_Interpolator = PopcornFX::CInterpolableVectorArray::Interpolator_Hermite;

	TArray<float>	collectedTimes;
	// Fetch every independant keys
	for (int32 i = 0; i < CurveDimension; ++i)
	{
		const FRichCurve	*fcurve = static_cast<const FRichCurve*>(curves[i].CurveToEdit);
		PK_ASSERT(fcurve != null);
		for (auto keyIt = fcurve->GetKeyIterator(); keyIt; ++keyIt)
			collectedTimes.AddUnique((*keyIt).Time);
	}
	collectedTimes.RemoveAll([](const float &time) { return time > 1.0f; });
	collectedTimes.Sort();

	// Not enough keys: return (should handle this ? init curve at 0 ?)
	if (collectedTimes.Num() <= 0)
		return false;
	if (collectedTimes.Last() < kMaximumKey)
		collectedTimes.Add(1.0f);
	if (collectedTimes[0] > kMinimumKey)
		collectedTimes.Insert(0.0f, 0);

	// Resize the curve if necessary
	const u32	keyCount = collectedTimes.Num();
	if (curveDescriptor->m_Times.Count() != keyCount)
	{
		if (!PK_VERIFY(curveDescriptor->Resize(keyCount)))
			return false;
	}
	PopcornFX::TArray<float, PopcornFX::TArrayAligned16>	&times = curveDescriptor->m_Times;
	PK_ASSERT(times.Count() == collectedTimes.Num());
	PK_ASSERT(times.RawDataPointer() != null);
	PK_ASSERT(collectedTimes.GetData() != null);
	memcpy(times.RawDataPointer(), collectedTimes.GetData(), keyCount * sizeof(float));

	float	minValues[4] = { 0 };
	float	maxValues[4] = { 0 };
	for (int32 i = 0; i < CurveDimension; ++i)
	{
		const FRichCurve	*fcurve = static_cast<const FRichCurve*>(curves[i].CurveToEdit);
		PK_ASSERT(fcurve != null);

		fcurve->GetValueRange(minValues[i], maxValues[i]);
		FetchCurveData(fcurve, curveDescriptor, i);
	}
	curveDescriptor->m_MinEvalLimits = CFloat4(minValues[0], minValues[1], minValues[2], minValues[3]);
	curveDescriptor->m_MaxEvalLimits = CFloat4(maxValues[0], maxValues[1], maxValues[2], maxValues[3]);
	curveDescriptor->m_Interpolator = PopcornFX::CInterpolableVectorArray::Interpolator_Hermite;
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurve::RebuildCurvesData()
{
	UCurveBase	*curve0 = null;
	UCurveBase	*curve1 = null;

	GetAssociatedCurves(curve0, curve1);
	if (curve0 == null || (bIsDoubleCurve && curve1 == null))
		return false;
	bool	success = true;

	success &= SetupCurve(m_Data->m_Curve0, curve0);
	success &= !bIsDoubleCurve || SetupCurve(m_Data->m_Curve1, curve1);
	return success;
}

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerCurve::_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_TODO("Determine when this should be set to true : has the curve asset changed ?")
	const PopcornFX::CResourceDescriptor_Curve			*defaultCurveSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Curve>(defaultSampler);
	const PopcornFX::CResourceDescriptor_DoubleCurve	*defaultDoubleCurveSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_DoubleCurve>(defaultSampler);
	if (!PK_VERIFY(defaultCurveSampler != null || defaultDoubleCurveSampler != null))
		return null;
	PK_ASSERT(defaultCurveSampler == null || defaultDoubleCurveSampler == null);
	m_Data->m_NeedsReload = true;
	const bool	defaultIsDoubleCurve = defaultDoubleCurveSampler != null;
	if (m_Data->m_NeedsReload)
	{
		if (defaultIsDoubleCurve != bIsDoubleCurve)
			return null;
		if (m_Data->m_Curve0 == null)
			m_Data->m_Curve0 = PK_NEW(PopcornFX::CCurveDescriptor());
		if (bIsDoubleCurve)
		{
			if (m_Data->m_Curve1 == null)
				m_Data->m_Curve1 = PK_NEW(PopcornFX::CCurveDescriptor());
		}
		else if (m_Data->m_Curve1 != null)
			m_Data->m_Curve1 = null;
		if (!RebuildCurvesData())
			return null;
		m_Data->m_NeedsReload = false;
		if (m_Data->m_CurveDesc == null &&
			m_Data->m_DoubleCurveDesc == null)
		{
			if (defaultIsDoubleCurve)
			{
				m_Data->m_DoubleCurveDesc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_DoubleCurve_Default());
				if (PK_VERIFY(m_Data->m_DoubleCurveDesc != null))
				{
					m_Data->m_DoubleCurveDesc->m_Curve0 = m_Data->m_Curve0;
					m_Data->m_DoubleCurveDesc->m_Curve1 = m_Data->m_Curve1;
				}
			}
			else
			{
				m_Data->m_CurveDesc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_Curve_Default());
				if (PK_VERIFY(m_Data->m_CurveDesc != null))
					m_Data->m_CurveDesc->m_Curve0 = m_Data->m_Curve0;
			}
		}
	}
	if (defaultIsDoubleCurve)
		return m_Data->m_DoubleCurveDesc.Get();
	return m_Data->m_CurveDesc.Get();
	//return defaultIsDoubleCurve ? m_Data->m_DoubleCurveDesc.Get() : m_Data->m_CurveDesc.Get();
}

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSamplerCurveDynamic
//
//----------------------------------------------------------------------------

struct FAttributeSamplerCurveDynamicData
{
	PopcornFX::PParticleSamplerDescriptor_Curve_Default	m_Desc;
	PopcornFX::CCurveDescriptor							*m_Curve0 = null;

	PopcornFX::TArray<float>	m_Times;
	PopcornFX::TArray<float>	m_Tangents;
	PopcornFX::TArray<float>	m_Values;
	bool						m_DirtyValues;

	~FAttributeSamplerCurveDynamicData()
	{
		PK_ASSERT(m_Curve0 == null);
	}
};

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerCurveDynamic::UPopcornFXAttributeSamplerCurveDynamic(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	bAutoActivate = true;

	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Curve;

	m_Data = new FAttributeSamplerCurveDynamicData();

	CurveDimension = EAttributeSamplerCurveDimension::Float1;
	CurveInterpolator = ECurveDynamicInterpolator::Linear;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurveDynamic::BeginDestroy()
{
	if (m_Data != null)
	{
		if (m_Data->m_Desc != null)
			m_Data->m_Desc->m_Curve0 = null;
		PK_SAFE_DELETE(m_Data->m_Curve0);
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::CreateCurveIFN()
{
	PK_ASSERT(m_Data != null);
	if (m_Data->m_Curve0 == null)
	{
		m_Data->m_Curve0 = PK_NEW(PopcornFX::CCurveDescriptor());
		if (PK_VERIFY(m_Data->m_Curve0 != null))
		{
			m_Data->m_DirtyValues = true;
			m_Data->m_Curve0->m_Order = (u32)CurveDimension;
			m_Data->m_Curve0->m_Interpolator = CurveInterpolator == ECurveDynamicInterpolator::Linear ?
				PopcornFX::CInterpolableVectorArray::Interpolator_Linear :
				PopcornFX::CInterpolableVectorArray::Interpolator_Hermite;

			//m_Data->m_Curve0->SetEvalLimits(CFloat4(0.0f), CFloat4(1.0f));
		}
	}
	return m_Data->m_Curve0 != null;
}

//----------------------------------------------------------------------------

template <class _Type>
bool	UPopcornFXAttributeSamplerCurveDynamic::SetValuesGeneric(const TArray<_Type> &values)
{
	const u32	valueCount = values.Num();
	if (valueCount < 2)
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Couldn't SetValues, Values should contain at least 2 items."));
		return false;
	}
	if ((m_Data->m_Values.Count() / (u32)CurveDimension) != values.Num())
	{
		if (!PK_VERIFY(m_Data->m_Values.Resize(valueCount * (u32)CurveDimension)))
			return false;
	}
	PopcornFX::Mem::Copy(m_Data->m_Values.RawDataPointer(), values.GetData(), sizeof(float) * (u32)CurveDimension * valueCount);
	m_Data->m_DirtyValues = true;
	return true;
}

//----------------------------------------------------------------------------

template <class _Type>
bool	UPopcornFXAttributeSamplerCurveDynamic::SetTangentsGeneric(const TArray<_Type> &arriveTangents, const TArray<_Type> &leaveTangents)
{
	if (arriveTangents.Num() != leaveTangents.Num())
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Couldn't SetTangents: ArriveTangents count differs from LeaveTangents"));
		return false;
	}
	const u32	tangentCount = arriveTangents.Num();
	if (tangentCount < 2)
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Couldn't SetValues, Values should contain at least 2 items."));
		return false;
	}
	const u32	dim = (u32)CurveDimension;
	if ((m_Data->m_Tangents.Count() / dim / 2) != tangentCount)
	{
		if (!PK_VERIFY(m_Data->m_Tangents.Resize(tangentCount * dim * 2)))
			return false;
	}
	const float	*prevTangents = reinterpret_cast<const float*>(arriveTangents.GetData());
	const float	*nextTangents = reinterpret_cast<const float*>(leaveTangents.GetData());
	float		*dstTangents = m_Data->m_Tangents.RawDataPointer();

	for (u32 iTangent = 0; iTangent < tangentCount; ++iTangent)
	{
		for (u32 iDim = 0; iDim < dim; ++iDim)
			*dstTangents++ = *prevTangents++;
		for (u32 iDim = 0; iDim < dim; ++iDim)
			*dstTangents++ = *nextTangents++;
	}
	m_Data->m_DirtyValues = true;
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetTimes(const TArray<float> &Times)
{
	PK_ASSERT(m_Data != null);

	PK_ONLY_IF_ASSERTS(
		for (s32 iTime = 0; iTime < Times.Num(); ++iTime)
			PK_ASSERT(Times[iTime] >= 0.0f && Times[iTime] <= 1.0f);
	);

	if (Times.Num() < 2)
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Couldn't SetTimes: Times should contain at least 2 items."));
		return false;
	}
	const u32	timesCount = Times.Num();
	if (PK_VERIFY(m_Data->m_Times.Resize(timesCount)))
	{
		m_Data->m_DirtyValues = true;
		PopcornFX::Mem::Copy(m_Data->m_Times.RawDataPointer(), Times.GetData(), sizeof(float) * timesCount);
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetValues1D(const TArray<float> &Values)
{
	PK_ASSERT(m_Data != null);
	if (CurveDimension != EAttributeSamplerCurveDimension::Float1)
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Couldn't SetValues1D: Curve doesn't have Float1 dimension"));
		return false;
	}
	return SetValuesGeneric<float>(Values);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetValues3D(const TArray<FVector> &Values)
{
	PK_ASSERT(m_Data != null);
	if (CurveDimension != EAttributeSamplerCurveDimension::Float3)
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Couldn't SetValues3D: Curve doesn't have Float3 dimension"));
		return false;
	}
	return SetValuesGeneric<FVector>(Values);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetValues4D(const TArray<FLinearColor> &Values)
{
	PK_ASSERT(m_Data != null);
	if (CurveDimension != EAttributeSamplerCurveDimension::Float4)
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Couldn't SetValues4D: Curve doesn't have Float4 dimension"));
		return false;
	}
	return SetValuesGeneric<FLinearColor>(Values);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetTangents1D(const TArray<float> &ArriveTangents, const TArray<float> &LeaveTangents)
{
	PK_ASSERT(m_Data != null);
	if (CurveInterpolator != ECurveDynamicInterpolator::Spline)
		return true; // No need to set tangents
	if (CurveDimension != EAttributeSamplerCurveDimension::Float1)
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Couldn't SetTangents1D: Curve doesn't have Float1 dimension"));
		return false;
	}
	return SetTangentsGeneric<float>(ArriveTangents, LeaveTangents);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetTangents3D(const TArray<FVector> &ArriveTangents, const TArray<FVector> &LeaveTangents)
{
	PK_ASSERT(m_Data != null);
	if (CurveInterpolator != ECurveDynamicInterpolator::Spline)
		return true; // No need to set tangents
	if (CurveDimension != EAttributeSamplerCurveDimension::Float3)
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Couldn't SetTangents3D: Curve doesn't have Float3 dimension"));
		return false;
	}
	return SetTangentsGeneric<FVector>(ArriveTangents, LeaveTangents);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetTangents4D(const TArray<FLinearColor> &ArriveTangents, const TArray<FLinearColor> &LeaveTangents)
{
	PK_ASSERT(m_Data != null);
	if (CurveInterpolator != ECurveDynamicInterpolator::Spline)
		return true; // No need to set tangents
	if (CurveDimension != EAttributeSamplerCurveDimension::Float4)
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Couldn't SetTangents4D: Curve doesn't have Float4 dimension"));
		return false;
	}
	return SetTangentsGeneric<FLinearColor>(ArriveTangents, LeaveTangents);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurveDynamic::_AttribSampler_PreUpdate(float deltaTime)
{
	PK_ASSERT(m_Data != null);

	PopcornFX::CCurveDescriptor	*curve = m_Data->m_Curve0;
	if (curve == null || !m_Data->m_DirtyValues)
		return;
	if (m_Data->m_Values.Empty())
		return;

	m_Data->m_DirtyValues = false;

	const bool	hasTimes = !m_Data->m_Times.Empty();
	const bool	hasTangents = !m_Data->m_Tangents.Empty();
	if (!hasTimes)
	{
		// Generate the times based on the value count (minus one: the first value is at time 0.0f, and the last is at time 1.0f)
		const u32	timesCount = m_Data->m_Values.Count() / (u32)CurveDimension;
		if (!PK_VERIFY(m_Data->m_Times.Resize(timesCount)))
			return;

		float		*times = m_Data->m_Times.RawDataPointer();
		const float	step = 1.0f / (timesCount - 1);
		for (u32 iTime = 0; iTime < timesCount; ++iTime)
			*times++ = step * iTime;
	}

	if (!CreateCurveIFN())
		return;
	const u32	valueCount = m_Data->m_Times.Count();
	if (!PK_VERIFY(curve->Resize(valueCount)))
		return;

	if ((hasTangents && m_Data->m_Tangents.Count() / 2 != m_Data->m_Values.Count()) ||
		(hasTimes && m_Data->m_Times.Count() != m_Data->m_Values.Count() / (u32)CurveDimension))
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Tangents/Times count differs from values. Make sure to set the same number of values/tangents/times"));
		return;
	}
	PopcornFX::Mem::Copy(curve->m_Times.RawDataPointer(), m_Data->m_Times.RawDataPointer(), sizeof(float) * valueCount);
	PopcornFX::Mem::Copy(curve->m_FloatValues.RawDataPointer(), m_Data->m_Values.RawDataPointer(), sizeof(float) * (u32)CurveDimension * valueCount);

	if (CurveInterpolator == ECurveDynamicInterpolator::Spline)
	{
		const u32	tangentsByteCount = sizeof(float) * (u32)CurveDimension * 2 * valueCount;

		if (hasTangents)
			PopcornFX::Mem::Copy(curve->m_FloatTangents.RawDataPointer(), m_Data->m_Tangents.RawDataPointer(), tangentsByteCount);
		else
			PopcornFX::Mem::Clear(curve->m_FloatTangents.RawDataPointer(), tangentsByteCount);
	}
}

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerCurveDynamic::_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_ASSERT(m_Data != null);
	const PopcornFX::CResourceDescriptor_Curve			*defaultCurveSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Curve>(defaultSampler);
	const PopcornFX::CResourceDescriptor_DoubleCurve	*defaultDoubleCurveSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_DoubleCurve>(defaultSampler);
	if (!PK_VERIFY(defaultCurveSampler != null || defaultDoubleCurveSampler != null))
		return null;
	PK_ASSERT(defaultCurveSampler == null || defaultDoubleCurveSampler != null);

	const bool	defaultIsDoubleCurve = defaultDoubleCurveSampler != null;
	if (defaultIsDoubleCurve)
	{
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Couldn't setup UPopcornFXAttributeSamplerCurveDynamic: Source curve is DoubleCurve, not supported by dynamic curve attr sampler."));
		return null;
	}
	if (!CreateCurveIFN())
		return null;
	if (m_Data->m_Desc == null)
		m_Data->m_Desc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_Curve_Default(m_Data->m_Curve0));
	PK_ASSERT(m_Data->m_Desc->m_Curve0 == m_Data->m_Curve0);
	desc.m_NeedUpdate = true;
	_AttribSampler_PreUpdate(0.f);
	return m_Data->m_Desc.Get();
}

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSamplerText
//
//----------------------------------------------------------------------------

struct FAttributeSamplerTextData
{
	PopcornFX::PParticleSamplerDescriptor_Text_Default	m_Desc;

	bool	m_NeedsReload;
};

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerText::SetText(FString InText)
{
	m_Data->m_NeedsReload = true;
	Text = InText;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerText::UPopcornFXAttributeSamplerText(const FObjectInitializer &PCIP)
: Super(PCIP)
{
	bAutoActivate = true;

	Text = "";
	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Text;

	m_Data = new FAttributeSamplerTextData();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerText::BeginDestroy()
{
	if (m_Data != null)
	{
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

void	UPopcornFXAttributeSamplerText::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.Property != NULL)
	{
		if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerText, Text))
			m_Data->m_NeedsReload = true;
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerText::_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	const PopcornFX::CResourceDescriptor_Text	*defaultTextSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Text>(defaultSampler);
	if (!PK_VERIFY(defaultTextSampler != null))
		return null;
	if (m_Data->m_Desc == null)
	{
		m_Data->m_Desc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_Text_Default());
		if (!PK_VERIFY(m_Data->m_Desc != null))
			return null;
		m_Data->m_NeedsReload = true;
	}
	if (m_Data->m_NeedsReload)
	{
		// @TODO kerning
		PopcornFX::CFontMetrics		*fontMetrics = null;
		bool						useKerning = false;
		if (!PK_VERIFY(m_Data->m_Desc->_Setup(TCHAR_TO_ANSI(*Text), fontMetrics, useKerning)))
			return null;
		m_Data->m_NeedsReload = false;
	}
	return m_Data->m_Desc.Get();
}

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSamplerImage
//
//----------------------------------------------------------------------------

struct FAttributeSamplerImageData
{
	PopcornFX::PParticleSamplerDescriptor_Image_Default			m_Desc;
	PopcornFX::CImageSampler									*m_ImageSampler = null;
	PopcornFX::TResourcePtr<PopcornFX::CImage>					m_TextureResource;
	PopcornFX::TResourcePtr<PopcornFX::CRectangleList>			m_TextureAtlasResource;
#if (PK_GPU_D3D12 != 0)
	PopcornFX::TResourcePtr<PopcornFX::CImageGPU_D3D12>			m_TextureResource_D3D12;
	PopcornFX::TResourcePtr<PopcornFX::CRectangleListGPU_D3D12>	m_TextureAtlasResource_D3D12;
#endif // (PK_GPU_D3D12 != 0)
	PopcornFX::SDensitySamplerData								*m_DensityData = null;

	bool		m_ReloadTexture = true;
	bool		m_ReloadTextureAtlas = true;
	bool		m_RebuildPDF = true;

	void		Clear()
	{
		m_Desc = null;
		m_TextureResource.Clear();
#if (PK_GPU_D3D12 != 0)
		m_TextureResource_D3D12.Clear();
		m_TextureAtlasResource_D3D12.Clear();
#endif // (PK_GPU_D3D12 != 0)
		m_TextureAtlasResource.Clear();
		if (m_DensityData != null)
			m_DensityData->Clear();
		if (m_Desc != null)
			m_Desc->ClearDensity();
	}

	FAttributeSamplerImageData()
	{ }

	~FAttributeSamplerImageData()
	{
		if (m_DensityData != null)
			PK_SAFE_DELETE(m_DensityData);
		if (m_ImageSampler != null)
			PK_SAFE_DELETE(m_ImageSampler);
	}

};

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerImage::SetTexture(class UTexture *InTexture)
{
	Texture = InTexture;
	m_Data->m_ReloadTexture = true;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerImage::UPopcornFXAttributeSamplerImage(const FObjectInitializer &PCIP)
	: Super(PCIP)
{
	bAutoActivate = true;

	bAllowTextureConvertionAtRuntime = false;

	SamplingMode = EPopcornFXImageSamplingMode::Regular;
	DensitySource = EPopcornFXImageDensitySource::RGBA_Average;
	DensityPower = 1.0f;

	Texture = null;
	TextureAtlas = null;
	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Image;

	m_Data = new FAttributeSamplerImageData();
	check(m_Data != null);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerImage::OnUnregister()
{
	if (m_Data != null)
	{
#if (PK_GPU_D3D12 != 0)//(PK_HAS_GPU != 0)
		// Unregister the component during OnUnregister instead of BeginDestroy.
		// In editor mode, BeginDestroy is only called when saving a level:
		// Components ReregisterComponent() do not have a matching BeginDestroy call in editor
		if (m_Data->m_Desc != null)
			m_Data->m_Desc = null;
#endif // (PK_HAS_GPU != 0)
	}
	Super::OnUnregister();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerImage::BeginDestroy()
{
	if (m_Data != null)
	{
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

void	UPopcornFXAttributeSamplerImage::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.Property != NULL)
	{
		const FString	propertyName = propertyChangedEvent.Property->GetName();

		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerImage, Texture) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerImage, bAllowTextureConvertionAtRuntime))
		{
			m_Data->m_ReloadTexture = true;
			m_Data->m_RebuildPDF = true;
		}
		else if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerImage, TextureAtlas))
		{
			m_Data->m_ReloadTextureAtlas = true;
			m_Data->m_RebuildPDF = true;
		}
		else if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerImage, SamplingMode) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerImage, DensitySource) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerImage, DensityPower))
		{
			m_Data->m_RebuildPDF = true;
		}
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerImage::_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	const PopcornFX::CResourceDescriptor_Image	*defaultImageSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Image>(defaultSampler);
	if (!PK_VERIFY(defaultImageSampler != null))
		return null;
	if (!/*PK_VERIFY*/(RebuildImageSampler()))
	{
		const FString	imageName = Texture != null ? Texture->GetName() : FString(TEXT("null"));
		const FString	atlasName = TextureAtlas != null ? TextureAtlas->GetName() : FString(TEXT("null"));
		UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("AttrSamplerImage: Failed to setup texture '%s' with atlas '%s' in '%s'"), *imageName, *atlasName, *GetPathName());
		return null;
	}
	return m_Data->m_Desc.Get();
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerImage::RebuildImageSampler()
{
	if (!_RebuildImageSampler())
	{
		m_Data->Clear();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerImage::_RebuildImageSampler()
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerImage::Build image sampler", POPCORNFX_UE_PROFILER_COLOR);

	if (Texture == null)
		return false;

#if (PK_HAS_GPU != 0)
	bool	updateGPUTextureBinding = false;
#endif // (PK_HAS_GPU != 0)

	const bool	rebuildImage = m_Data->m_ReloadTexture;
	if (rebuildImage)
	{
		const PopcornFX::CString	fullPath = TCHAR_TO_ANSI(*Texture->GetPathName());
		bool						success = false;
		m_Data->m_TextureResource = PopcornFX::Resource::DefaultManager()->Load<PopcornFX::CImage>(fullPath, true);
		success |= (m_Data->m_TextureResource != null && !m_Data->m_TextureResource->Empty());
		if (success)
		{
			if (!PK_VERIFY(!m_Data->m_TextureResource->m_Frames.Empty()) ||
				!PK_VERIFY(!m_Data->m_TextureResource->m_Frames[0].m_Mipmaps.Empty()))
				return false;
		}

		// Try loading the image with GPU sim resource handlers:
		// There is no way currently to know if the current sampler descriptor will be used by the CPU or GPU sim, so we'll have to load both, if possible
		// GPU sim image load is trivial: it will just grab the ref to the native resource
#if (PK_GPU_D3D12 != 0)
		if (g_PopcornFXRHIAPI == SUERenderContext::D3D12)
		{
			m_Data->m_TextureResource_D3D12 = PopcornFX::Resource::DefaultManager()->Load<PopcornFX::CImageGPU_D3D12>(fullPath, true);
			success |= (m_Data->m_TextureResource_D3D12 != null && !m_Data->m_TextureResource_D3D12->Empty());
		}
#endif // (PK_GPU_D3D12 != 0)

		if (!success) // Couldn't load any of the resources (CPU/GPU)
		{
			UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("AttrSamplerImage: couldn't load texture '%s' for CPU/GPU sim sampling"), fullPath.Data());
			return false;
		}
		m_Data->m_ReloadTexture = false;
	}
	const bool	reloadImageAtlas = m_Data->m_ReloadTextureAtlas;
	if (reloadImageAtlas)
	{
		if (TextureAtlas != null)
		{
			bool						success = false;
			const PopcornFX::CString	fullPath = TCHAR_TO_ANSI(*TextureAtlas->GetPathName());
			m_Data->m_TextureAtlasResource = PopcornFX::Resource::DefaultManager()->Load<PopcornFX::CRectangleList>(fullPath, true);
			success |= (m_Data->m_TextureAtlasResource != null && !m_Data->m_TextureAtlasResource->Empty());

			// Try loading the image atlas with GPU sim resource handlers:
			// There is no way currently to know if the current sampler descriptor will be used by the CPU or GPU sim, so we'll have to load both, if possible
#if (PK_GPU_D3D12 != 0)
			if (g_PopcornFXRHIAPI == SUERenderContext::D3D12)
			{
				m_Data->m_TextureAtlasResource_D3D12 = PopcornFX::Resource::DefaultManager()->Load<PopcornFX::CRectangleListGPU_D3D12>(fullPath, true);
				success |= (m_Data->m_TextureAtlasResource_D3D12 != null && !m_Data->m_TextureAtlasResource_D3D12->Empty());
			}
#endif // (PK_GPU_D3D12 != 0)

			if (!success) // Couldn't load any of the resources (CPU/GPU)
			{
				UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("AttrSamplerImage: couldn't load texture atlas '%s' for CPU/GPU sim sampling"), fullPath.Data());
				return false;
			}
		}
		else
			m_Data->m_TextureAtlasResource.Clear();

		m_Data->m_ReloadTextureAtlas = false;
	}
	if (m_Data->m_Desc == null)
	{
		m_Data->m_Desc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_Image_Default()); // Full override
		if (!PK_VERIFY(m_Data->m_Desc != null))
			return false;
	}

#if (PK_GPU_D3D12 != 0)
	if (rebuildImage || reloadImageAtlas) // We only want to reload bindings when m_Data->m_ReloadTexture is true (or if m_Data->m_ReloadTextureAtlas is true)
	{
		if (m_Data->m_TextureResource_D3D12 != null && !m_Data->m_TextureResource_D3D12->Empty()) // Don't bind the atlas by itself, clear everything if the texture is invalid
		{
			ID3D12Resource	*imageResource = m_Data->m_TextureResource_D3D12->Texture();

			ID3D12Resource	*imageAtlasResource = null;
			u32				atlasRectCount = 0;
			if (m_Data->m_TextureAtlasResource != null)
			{
				if (m_Data->m_TextureAtlasResource_D3D12 != null && !m_Data->m_TextureAtlasResource_D3D12->Empty())
				{
					imageAtlasResource = m_Data->m_TextureAtlasResource_D3D12->Buffer();
					atlasRectCount = m_Data->m_TextureAtlasResource->AtlasRectCount();
				}
			}
			const bool		sRGB = PopcornFX::CImage::IsFormatGammaCorrected(m_Data->m_TextureResource_D3D12->StorageFormat());

			PK_ASSERT(imageResource != null);

			m_Data->m_Desc->SetupD3D12Resources(imageResource,
												m_Data->m_TextureResource_D3D12->Dimensions(),
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
												UE::DXGIUtilities::FindShaderResourceFormat(imageResource->GetDesc().Format, sRGB),
#else
												FindShaderResourceDXGIFormat(imageResource->GetDesc().Format, sRGB),
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
												imageAtlasResource,
												atlasRectCount);
		}
		else
			m_Data->m_Desc->ClearD3D12Resources();
	}
#endif // (PK_GPU_D3D12 != 0)

	if (m_Data->m_TextureResource != null) // CPU image
	{
		PopcornFX::CImageSurface	dstSurface(m_Data->m_TextureResource->m_Frames[0].m_Mipmaps[0], m_Data->m_TextureResource->m_Format);

		const bool	both = SamplingMode == EPopcornFXImageSamplingMode::Both;
		if (SamplingMode == EPopcornFXImageSamplingMode::Density || both)
		{
			if (m_Data->m_DensityData == null)
			{
				m_Data->m_DensityData = PK_NEW(PopcornFX::SDensitySamplerData);
				if (!PK_VERIFY(m_Data->m_DensityData != null))
					return false;
			}
			if (!_BuildPDFs(dstSurface))
				return false;
			if (!PK_VERIFY(m_Data->m_Desc->SetupDensity(m_Data->m_DensityData)))
				return false;
			if (!both)
				m_Data->m_Desc->m_Sampler = null;
		}

		if (SamplingMode == EPopcornFXImageSamplingMode::Regular || both)
		{
			if (!_BuildRegularImage(dstSurface, rebuildImage))
				return false;
			m_Data->m_Desc->m_Sampler = m_Data->m_ImageSampler;
			if (!both && m_Data->m_DensityData != null)
			{
				m_Data->m_DensityData->Clear();
				m_Data->m_Desc->ClearDensity();
			}
		}
	}

	if (m_Data->m_TextureAtlasResource != null)
		m_Data->m_Desc->m_AtlasRectangleList = m_Data->m_TextureAtlasResource->m_RectsFp32;
	else
		m_Data->m_Desc->m_AtlasRectangleList.Clear();
	PK_ASSERT(m_Data->m_Desc->m_Sampler == m_Data->m_ImageSampler);
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerImage::_BuildPDFs(PopcornFX::CImageSurface &dstSurface)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerImage::Build PDF", POPCORNFX_UE_PROFILER_COLOR);
	if (!m_Data->m_RebuildPDF)
		return true;
	m_Data->m_RebuildPDF = false;

	PK_STATIC_ASSERT(EPopcornFXImageDensitySource::Red			== (u32)PopcornFX::SImageConvertSettings::LumMode_Red);
	PK_STATIC_ASSERT(EPopcornFXImageDensitySource::Green		== (u32)PopcornFX::SImageConvertSettings::LumMode_Green);
	PK_STATIC_ASSERT(EPopcornFXImageDensitySource::Blue			== (u32)PopcornFX::SImageConvertSettings::LumMode_Blue);
	PK_STATIC_ASSERT(EPopcornFXImageDensitySource::Alpha		== (u32)PopcornFX::SImageConvertSettings::LumMode_Alpha);
	PK_STATIC_ASSERT(EPopcornFXImageDensitySource::RGBA_Average	== (u32)PopcornFX::SImageConvertSettings::LumMode_RGBA_Avg);

	PopcornFX::SDensitySamplerBuildSettings	settings;
	settings.m_DensityPower = DensityPower;
	settings.m_RGBAToLumMode = static_cast<PopcornFX::SImageConvertSettings::ELumMode>(DensitySource.GetValue());
	//settings.m_SampleRawValues = ;
	if (m_Data->m_TextureAtlasResource != null)
		settings.m_AtlasRectangleList = m_Data->m_TextureAtlasResource->m_RectsFp32;
	m_Data->m_DensityData->Clear();
	if (!PK_VERIFY(m_Data->m_DensityData->Build(dstSurface, settings)))
		return false;
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerImage::_BuildRegularImage(PopcornFX::CImageSurface &dstSurface, bool rebuild)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerImage::Build Image", POPCORNFX_UE_PROFILER_COLOR);

	if ((m_Data->m_ImageSampler != null) &&
		!rebuild)
		return true;
	if (m_Data->m_ImageSampler != null)
		PK_SAFE_DELETE(m_Data->m_ImageSampler);

	m_Data->m_ImageSampler = PK_NEW(PopcornFX::CImageSamplerBilinear);
	if (!PK_VERIFY(m_Data->m_ImageSampler != null))
		return false;

	if (!/*PK_VERIFY*/(m_Data->m_ImageSampler->SetupFromSurface(dstSurface)))
	{
		if (bAllowTextureConvertionAtRuntime)
		{
			const PopcornFX::CImage::EFormat		dstFormat = PopcornFX::CImage::Format_BGRA8;
			UE_LOG(LogPopcornFXAttributeSampler, Log,
				TEXT("AttrSamplerImage: texture '%s' format %s not supported for sampling, converting to %s (because AllowTextureConvertionAtRuntime) in '%s'"),
				*Texture->GetName(),
				ANSI_TO_TCHAR(PopcornFX::CImage::GetFormatName(m_Data->m_TextureResource->m_Format)),
				ANSI_TO_TCHAR(PopcornFX::CImage::GetFormatName(dstFormat)),
				*GetPathName());

			PopcornFX::CImageSurface	newSurface;
			newSurface.m_Format = dstFormat;
			if (!newSurface.CopyAndConvertIFN(dstSurface))
			{
				UE_LOG(LogPopcornFXAttributeSampler, Warning,
					TEXT("AttrSamplerImage: could not convert texture '%s' from %s to %s in %s"),
					*Texture->GetName(),
					ANSI_TO_TCHAR(PopcornFX::CImage::GetFormatName(m_Data->m_TextureResource->m_Format)),
					ANSI_TO_TCHAR(PopcornFX::CImage::GetFormatName(dstFormat)),
					*GetPathName());
				return false;
			}
			if (!PK_VERIFY(m_Data->m_ImageSampler->SetupFromSurface(newSurface)))
			{
				return false;
			}
		}
		else
		{
			UE_LOG(LogPopcornFXAttributeSampler, Warning,
				TEXT("AttrSamplerImage: texture '%s' format %s not supported for sampling (and AllowTextureConvertionAtRuntime not enabled) in %s"),
				*Texture->GetName(),
				ANSI_TO_TCHAR(PopcornFX::CImage::GetFormatName(m_Data->m_TextureResource->m_Format)),
				*GetPathName());
			return false;
		}
	}
	return true;
}


//----------------------------------------------------------------------------
//
// FAttributeSamplerVectorFieldData
//
//----------------------------------------------------------------------------

struct FAttributeSamplerVectorFieldData
{
	PopcornFX::PParticleSamplerDescriptor_VectorField_Grid	m_Desc; // We only allow static vector field override for attribute sampler

	bool		m_NeedsReload = true;
#if WITH_EDITOR
	FVector3f	m_RealExtentUnscaled = FVector3f::ZeroVector;
#endif // WITH_EDITOR
};

UPopcornFXAttributeSamplerVectorField::UPopcornFXAttributeSamplerVectorField(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	bAutoActivate = true;
#if WITH_EDITOR
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	m_IndirectSelectedThisTick = false;
#if 0
	bDrawCells = false;
#endif // 0
#endif // WITH_EDITOR

	//	default values
	Intensity = 1.f;
	VectorField = null;
	VolumeDimensions = FVector(100.f, 100.f, 100.f);
	SamplingMode = EPopcornFXVectorFieldSamplingMode::Trilinear;
	WrapMode = EPopcornFXVectorFieldWrapMode::Wrap;

	// Guess here, turbulence sampler probably used by physics evolver or script evolver
	bUseRelativeTransform = false;

	m_TimeAccumulation = 0.f;
	m_SamplerType = EPopcornFXAttributeSamplerType::Turbulence;
	m_Data = new FAttributeSamplerVectorFieldData();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::BeginDestroy()
{
	if (m_Data != null)
	{
		if (m_Data->m_Desc != null)
		{
			m_Data->m_Desc->Clear();
			m_Data->m_Desc = null;
		}
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::_SetBounds()
{
	PK_ASSERT(VectorField != null);

	if (BoundsSource == EPopcornFXVectorFieldBounds::Source)
	{
		const PopcornFX::CAABB	bounds = PopcornFX::CAABB::FromMinMax(ToPk(VectorField->Bounds.Min), ToPk(VectorField->Bounds.Max));
		m_Data->m_Desc->SetBounds(bounds);
#if WITH_EDITOR
		m_Data->m_RealExtentUnscaled = ToUE(bounds.Extent() * FPopcornFXPlugin::GlobalScale());
#endif // WITH_EDITOR
	}
	else
	{
		PK_ASSERT(BoundsSource == EPopcornFXVectorFieldBounds::Custom);

		PopcornFX::CAABB	bounds;
		bounds.SetupFromHalfExtent(ToPk(VolumeDimensions) * 0.5f * FPopcornFXPlugin::GlobalScaleRcp());
		m_Data->m_Desc->SetBounds(bounds);

#if WITH_EDITOR
		m_Data->m_RealExtentUnscaled = ToUE(bounds.Extent() * FPopcornFXPlugin::GlobalScale());
#endif // WITH_EDITOR
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::_BuildVectorFieldFlags(uint32 &flags, uint32 &interpolation) const
{
	if (WrapMode == EPopcornFXVectorFieldWrapMode::Wrap)
	{
		flags |= PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Wrap_X;
		flags |= PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Wrap_Y;
		flags |= PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Wrap_Z;
	}
	PK_STATIC_ASSERT(EPopcornFXVectorFieldSamplingMode::Point == (u32)PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Interpolation_Point);
	PK_STATIC_ASSERT(EPopcornFXVectorFieldSamplingMode::Trilinear == (u32)PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Interpolation_Trilinear);

	interpolation = static_cast<PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::EInterpolation>(SamplingMode.GetValue());
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
void	UPopcornFXAttributeSamplerVectorField::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
	if (propertyChangedEvent.MemberProperty != null && m_Data->m_Desc != null)
	{
		const FString	propertyName = propertyChangedEvent.MemberProperty->GetName();

		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerVectorField, VectorField))
		{
			m_Data->m_NeedsReload = true;
			if (VectorField == null)
			{
				m_Data->m_Desc->Clear();
				// Disable debug rendering, we don't have that info
				if (BoundsSource == EPopcornFXVectorFieldBounds::Source)
					m_Data->m_RealExtentUnscaled = FVector3f::ZeroVector;
			}
		}
		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerVectorField, Intensity))
		{
			m_Data->m_Desc->SetStrength(Intensity);
		}
		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerVectorField, BoundsSource) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerVectorField, VolumeDimensions))
		{
			if (VectorField != null)
				_SetBounds();
		}
		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerVectorField, WrapMode) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerVectorField, SamplingMode))
		{
			u32	flags = 0;
			u32	interpolation = 0;

			_BuildVectorFieldFlags(flags, interpolation);
			m_Data->m_Desc->SetFlags(flags, static_cast<PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::EInterpolation>(interpolation));
		}
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::RenderVectorFieldShape(const FMatrix &transforms, const FQuat &rotation, bool isSelected)
{
	if (m_Data->m_RealExtentUnscaled == FVector3f::ZeroVector)
		return;
	const UWorld	*world = GetWorld();
	const FColor	debugColor = isSelected ? kSamplerShapesDebugColorSelected : kSamplerShapesDebugColor;

	const FVector	boxDims = FVector(m_Data->m_RealExtentUnscaled) * GetComponentTransform().GetScale3D() * 0.5f;
	DrawDebugBox(world, transforms.GetOrigin(), boxDims, rotation, debugColor);
#if 0
	if (!bDrawCells)
		return;

	const CFloat3	boxMin = ToPk(VectorField->Bounds.Min * VolumeDimensions);
	const CFloat3	boxMax = ToPk(VectorField->Bounds.Max * VolumeDimensions);
	const CFloat4	vectorFieldSize = CFloat4(VectorField->SizeX, VectorField->SizeY, VectorField->SizeZ, 1);

	const CFloat3	boxCellDim = (boxMax - boxMin) / vectorFieldSize.xyz();

	for (int32 zS = 0; zS <= vectorFieldSize.z(); ++zS)
	{
		for (int32 yS = 0; yS <= vectorFieldSize.y(); ++yS)
		{
			FVector	lineStart = FVector(boxMin.x(), boxMin.y() + boxCellDim.y() * yS, boxMin.z() + boxCellDim.z() * zS);
			FVector	lineEnd = FVector(boxMax.x(), boxMin.y() + boxCellDim.y() * yS, boxMin.z() + boxCellDim.z() * zS);

			lineStart = transforms.TransformFVector4(lineStart);
			lineEnd = transforms.TransformFVector4(lineEnd);

			DrawDebugLine(world, lineStart, lineEnd, debugColor);
		}
		for (int32 xS = 0; xS <= vectorFieldSize.x(); ++xS)
		{
			FVector	lineStart = FVector(boxMin.x() + boxCellDim.x() * xS, boxMin.y(), boxMin.z() + boxCellDim.z() * zS);
			FVector	lineEnd = FVector(boxMin.x() + boxCellDim.x() * xS, boxMax.y(), boxMin.z() + boxCellDim.z() * zS);

			lineStart = transforms.TransformFVector4(lineStart);
			lineEnd = transforms.TransformFVector4(lineEnd);

			DrawDebugLine(world, lineStart, lineEnd, debugColor);
		}
	}

	for (int32 yS = 0; yS <= vectorFieldSize.y(); ++yS)
	{
		for (int32 xS = 0; xS <= vectorFieldSize.x(); ++xS)
		{
			FVector	lineStart = FVector(boxMin.x() + boxCellDim.x() * xS, boxMin.y() + boxCellDim.y() * yS, boxMin.z());
			FVector	lineEnd = FVector(boxMin.x() + boxCellDim.x() * xS, boxMin.y() + boxCellDim.y() * yS, boxMax.z());

			lineStart = transforms.TransformFVector4(lineStart);
			lineEnd = transforms.TransformFVector4(lineEnd);

			DrawDebugLine(world, lineStart, lineEnd, debugColor);
		}
	}
#endif // 0
}
#endif //WITH_EDITOR

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerVectorField::_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerVectorField::Build Vectorfield", POPCORNFX_UE_PROFILER_COLOR);

	check(m_Data != null);
	const PopcornFX::CResourceDescriptor_VectorField	*defaultTurbulenceSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_VectorField>(defaultSampler);
	if (!PK_VERIFY(defaultTurbulenceSampler != null))
		return null;
	if (VectorField == null)
		return null;

	if (m_Data->m_Desc == null)
	{
		m_Data->m_Desc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_VectorField_Grid()); // Full override
		if (!PK_VERIFY(m_Data->m_Desc != null))
			return null;
	}
	if (m_Data->m_NeedsReload)
	{
		m_Data->m_NeedsReload = false;

		const CFloat4	dimensions = CFloat4(VectorField->SizeX, VectorField->SizeY, VectorField->SizeZ, 1);
		const u32		elementCount = dimensions.x() * dimensions.y() * dimensions.z();
		PK_ASSERT(VectorField->SourceData.GetBulkDataSize() == elementCount * sizeof(FFloat16Color));

		PopcornFX::PRefCountedMemoryBuffer	gridRawData = PopcornFX::CRefCountedMemoryBuffer::AllocAligned(sizeof(PopcornFX::f16) * 3 * elementCount, PopcornFX::Memory::CacheLineSize);
		if (!PK_VERIFY(gridRawData != null))
			return null;

		const PopcornFX::f16			*srcValues = reinterpret_cast<PopcornFX::f16*>(VectorField->SourceData.Lock(LOCK_READ_ONLY));
		if (!PK_VERIFY(srcValues != null))
			return null;
		PopcornFX::f16					*dstValues = gridRawData->Data<PopcornFX::f16>();
		const PopcornFX::f16			*endValue = dstValues + elementCount * 3;
		while (dstValues != endValue)
		{
			*dstValues++ = *srcValues++;
			*dstValues++ = *srcValues++;
			*dstValues++ = *srcValues++;
			srcValues++;
		}
		VectorField->SourceData.Unlock();

		u32	flags = 0;
		u32	interpolation = 0;
		_BuildVectorFieldFlags(flags, interpolation);

		if (!PK_VERIFY(m_Data->m_Desc->Setup(dimensions, VectorField->Intensity, CFloat4(0.0f), CFloat4(0.0f), gridRawData,
			PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::DataType_Fp16,
			Intensity, CFloat4x4::IDENTITY, flags,
			static_cast<PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::EInterpolation>(interpolation))))
			return null;

		_SetBounds();
	}

	desc.m_NeedUpdate = true;
	_AttribSampler_PreUpdate(0.f);
	return m_Data->m_Desc.Get();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::_AttribSampler_PreUpdate(float deltaTime)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerVectorField::Update transforms", POPCORNFX_UE_PROFILER_COLOR);

	FMatrix		transforms;
	if (RotationAnimation != FVector::ZeroVector)
	{
		m_TimeAccumulation += deltaTime;

		const FVector		animationRotation = RotationAnimation * m_TimeAccumulation;
		const FTransform	eulerAngles(FQuat::MakeFromEuler(animationRotation));
		if (bUseRelativeTransform)
			transforms = (eulerAngles * GetRelativeTransform()).ToMatrixWithScale();
		else
			transforms = (eulerAngles * GetComponentTransform()).ToMatrixWithScale();
	}
	else
	{
		if (bUseRelativeTransform)
			transforms = GetRelativeTransform().ToMatrixWithScale();
		else
			transforms = GetComponentTransform().ToMatrixWithScale();
	}

#if WITH_EDITOR
	PK_ASSERT(GetWorld() != null);
	if (!GetWorld()->IsGameWorld())
	{
		const USelection	*selectedAssets = GEditor->GetSelectedActors();
		PK_ASSERT(selectedAssets != null);
		const bool			isSelected = selectedAssets->IsSelected(GetOwner());
		if (m_IndirectSelectedThisTick || isSelected)
		{
			const FQuat			eulerAngles = FQuat::MakeFromEuler(RotationAnimation * m_TimeAccumulation);
			const FMatrix		worldTransforms = (FTransform(eulerAngles) * GetComponentTransform()).ToMatrixWithScale();

			RenderVectorFieldShape(worldTransforms, eulerAngles, isSelected);
			m_IndirectSelectedThisTick = false;
		}
	}
#endif // WITH_EDITOR

	const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
	if (m_Data->m_Desc != null)
	{
		transforms.M[3][0] *= invGlobalScale;
		transforms.M[3][1] *= invGlobalScale;
		transforms.M[3][2] *= invGlobalScale;

		m_Data->m_Desc->SetTransforms(ToPk(transforms));
	}
}

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSamplerAnimTrack
//
//----------------------------------------------------------------------------

namespace	PopcornFXAttributeSamplers
{
	extern VectorRegister4f _TransformVector(VectorRegister4f &v, const FMatrix44f *m);
}

// UE animtrack attribute sampler: only contains one track
// Expensive sampling as each particle individually evaluates the UE curve.
class	CParticleSamplerDescriptor_AnimTrack_UE : public PopcornFX::CParticleSamplerDescriptor_AnimTrack
{
public:
	CParticleSamplerDescriptor_AnimTrack_UE(const FMatrix44f *trackTransforms)
	:	m_TransformFlags(0)
	,	m_TrackTransforms(trackTransforms)
	{
	}

	bool	Setup(const USplineComponent *splineComponent, u32 transformFlags)
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::Setup", POPCORNFX_UE_PROFILER_COLOR);

		PK_ASSERT(splineComponent != null);

		m_Positions.Reset();
		m_Rotations.Reset();
		m_Scales.Reset();
		m_ReparamTable.Reset();

		m_TransformFlags = transformFlags;
		if ((m_TransformFlags & PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Translate) != 0)
			m_Positions = splineComponent->GetSplinePointsPosition();
		if ((m_TransformFlags & PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Rotate) != 0)
			m_Rotations = splineComponent->GetSplinePointsRotation();
		if ((m_TransformFlags & PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Scale) != 0)
			m_Scales = splineComponent->GetSplinePointsScale();

		if (m_Positions.Points.Num() > 0 || m_Rotations.Points.Num() > 0 || m_Scales.Points.Num() > 0)
		{
			m_ReparamTable = splineComponent->SplineCurves.ReparamTable;
			m_SplineLength = splineComponent->GetSplineLength();
		}
		return true;
	}

	virtual void	SamplePosition(	const TStridedMemoryView<CFloat3>		&dstValues,
									const TStridedMemoryView<const float>	&times,
									const TStridedMemoryView<const s32>		&trackIds) const override
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::SamplePosition", POPCORNFX_UE_PROFILER_COLOR);

		PK_ASSERT(dstValues.Count() == times.Count());

		const float		invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const u32		timesCount = times.Count();

		for (u32 idxTime = 0; idxTime < timesCount; ++idxTime)
		{
			const float	currentTime = m_ReparamTable.Eval(times[idxTime] * m_SplineLength, 0.0f);

			dstValues[idxTime] = ToPk(m_Positions.Eval(currentTime)) * invGlobalScale;
		}
		_TransformValues(*m_TrackTransforms, dstValues);
	}

	virtual void	SampleOrientation(	const TStridedMemoryView<CQuaternion>	&dstValues,
										const TStridedMemoryView<const float>	&times,
										const TStridedMemoryView<const s32>		&trackIds) const override
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::SampleOrientation", POPCORNFX_UE_PROFILER_COLOR);

		PK_ASSERT(dstValues.Count() == times.Count());

		const u32	timesCount = times.Count();

		for (u32 idxTime = 0; idxTime < timesCount; ++idxTime)
		{
			const float currentTime = m_ReparamTable.Eval(times[idxTime] * m_SplineLength, 0.0f);

			dstValues[idxTime] = ToPk(m_Rotations.Eval(currentTime));
		}
		_TransformValues(*m_TrackTransforms, dstValues);
	}

	virtual void	SampleScale(	const TStridedMemoryView<CFloat3>		&dstValues,
									const TStridedMemoryView<const float>	&times,
									const TStridedMemoryView<const s32>		&trackIds) const override
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::SampleScale", POPCORNFX_UE_PROFILER_COLOR);

		PK_ASSERT(dstValues.Count() == times.Count());

		const u32	timesCount = times.Count();

		for (u32 idxTime = 0; idxTime < timesCount; ++idxTime)
		{
			const float currentTime = m_ReparamTable.Eval(times[idxTime] * m_SplineLength, 0.0f);

			dstValues[idxTime] = ToPk(m_Scales.Eval(currentTime));
		}
		_TransformValues(*m_TrackTransforms, dstValues);
	}

	virtual void	Transform(	const TStridedMemoryView<CFloat3>		&outSamples,
								const TStridedMemoryView<const CFloat3>	&locations,
								const TStridedMemoryView<const float>	&times,
								const TStridedMemoryView<const s32>		&trackIds,
								u32										transformFlags) const override
	{
		_Transform<false>(outSamples, locations, times, transformFlags);
		_TransformValues(*m_TrackTransforms, outSamples);
	}

	virtual void	GetTrackCount(const TStridedMemoryView<s32> &dstValues) const override
	{
		PK_ASSERT(!dstValues.Empty());
		const u32	trackCount = 1;
		if (!dstValues.Virtual())
		{
			PK_RELEASE_ASSERT(dstValues.Contiguous());
			PopcornFX::Mem::Fill32(dstValues.Data(), trackCount, dstValues.Count());
		}
		else
			dstValues[0] = trackCount;
	}

private:
	template<bool _InverseTransforms>
	void	_Transform(const TStridedMemoryView<CFloat3>		&outSamples,
					   const TStridedMemoryView<const CFloat3>	&locations,
					   const TStridedMemoryView<const float>	&cursors,
					   u32										transformFlags) const
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::_Transform", POPCORNFX_UE_PROFILER_COLOR);

		PK_ASSERT(outSamples.Count() == locations.Count());
		PK_ASSERT(outSamples.Count() == cursors.Count());

		bool	translate = ((m_TransformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Translate) != 0);
		bool	rotate = ((m_TransformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Rotate) != 0);
		bool	scale = ((m_TransformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Scale) != 0);

		translate = translate && ((transformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Translate) != 0);
		rotate = rotate && ((transformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Rotate) != 0);
		scale = scale && ((transformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Scale) != 0);

		if (translate || rotate || scale)
		{
			const float		invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
			const u32		cursorCount = cursors.Count();
			for (u32 iCursor = 0; iCursor < cursorCount; ++iCursor)
			{
				const float		time = m_ReparamTable.Eval(cursors[iCursor] * m_SplineLength, 0.0f);

				outSamples[iCursor] = locations[iCursor];
				if (!_InverseTransforms)
				{
					if (scale)
						outSamples[iCursor] *= ToPk(m_Scales.Eval(time));
					if (rotate)
						outSamples[iCursor] = ToPk(m_Rotations.Eval(time).RotateVector(FVector(ToUE(outSamples[iCursor]))));
					if (translate)
						outSamples[iCursor] += ToPk(m_Positions.Eval(time)) * invGlobalScale;
				}
				else
				{
					if (translate)
						outSamples[iCursor] -= ToPk(m_Positions.Eval(time)) * invGlobalScale;
					if (rotate)
						outSamples[iCursor] = ToPk(m_Rotations.Eval(time).Inverse().RotateVector(FVector(ToUE(outSamples[iCursor]))));
					if (scale)
						outSamples[iCursor] /= ToPk(m_Scales.Eval(time));
				}
			}
		}
	}

	void	_TransformValues(const FMatrix44f &transforms, const TStridedMemoryView<CFloat3> &outSamples) const
	{
		if (transforms.Equals(FMatrix44f::Identity, 1.0e-6f))
			return;
		const u32	sampleCount = outSamples.Count();
		const u32	stride = outSamples.Stride();
		CFloat3		*positions = outSamples.Data();
		for (u32 iSample = 0; iSample < sampleCount; ++iSample)
		{
			VectorRegister4f	srcPos = VectorLoadFloat3(reinterpret_cast<FVector3f*>(positions));

			srcPos = PopcornFXAttributeSamplers::_TransformVector(srcPos, &transforms);
			VectorStoreFloat3(srcPos, reinterpret_cast<FVector3f*>(positions));
			positions = PopcornFX::Mem::AdvanceRawPointer(positions, stride);
		}
	}

	void	_TransformValues(const FMatrix44f &transforms, const TStridedMemoryView<CQuaternion> &outSamples) const
	{
		// TODO
	}

private:
	// Local copy of USplineComponent's curves
	FInterpCurveVector		m_Positions;
	FInterpCurveQuat		m_Rotations;
	FInterpCurveVector		m_Scales;
	FInterpCurveFloat		m_ReparamTable;

	u32						m_TransformFlags;
	float					m_SplineLength;
	const FMatrix44f		*m_TrackTransforms;
};
PK_DECLARE_REFPTRCLASS(ParticleSamplerDescriptor_AnimTrack_UE);

//----------------------------------------------------------------------------

struct	FAttributeSamplerAnimTrackData
{
	PParticleSamplerDescriptor_AnimTrack_UE		m_Desc = null;

	PopcornFX::PParticleSamplerDescriptor_AnimTrack_Default	m_DescFast = null;
	PopcornFX::CCurveDescriptor								*m_Positions = null;
	PopcornFX::CCurveDescriptor								*m_Scales = null;

	TWeakObjectPtr<USplineComponent>		m_CurrentSplineComponent = null;

	bool	m_NeedsReload = false;
};

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerAnimTrack::UPopcornFXAttributeSamplerAnimTrack(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	bAutoActivate = true;

	// By default, only translate
	bTranslate = true;
	bScale = false;
	bRotate = false;
	bFastSampler = true;
#if WITH_EDITORONLY_DATA
	bEditorRebuildEachFrame = false;
#endif // WITH_EDITORONLY_DATA

	Transforms = EPopcornFXSplineTransforms::AttrSamplerRelativeTr;

	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::AnimTrack;

	m_Data = new FAttributeSamplerAnimTrackData();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerAnimTrack::BeginDestroy()
{
	if (m_Data != null)
	{
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

USplineComponent	*UPopcornFXAttributeSamplerAnimTrack::ResolveSplineComponent(bool logErrors)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerAnimTrack::ResolveSplineComponent", POPCORNFX_UE_PROFILER_COLOR);

	AActor	*fallbackActor = GetOwner();
	if (!PK_VERIFY(fallbackActor != null))
		return null;
	const AActor		*parent = TargetActor == null ? fallbackActor : TargetActor;
	USplineComponent	*spline = null;
	if (SplineComponentName != NAME_None)
	{
		FObjectPropertyBase	*prop = FindFProperty<FObjectPropertyBase>(parent->GetClass(), SplineComponentName);

		if (prop != null)
			spline = Cast<USplineComponent>(prop->GetObjectPropertyValue_InContainer(parent));
	}
	else
	{
		spline = Cast<USplineComponent>(parent->GetRootComponent());
	}
	if (spline == null)
	{
		if (logErrors)
		{
			UE_LOG(LogPopcornFXAttributeSampler, Warning,
				TEXT("Could not find component 'USplineComponent %s.%s' for UPopcornFXAttributeSamplerAnimTrack '%s'"),
				*parent->GetName(), (SplineComponentName != NAME_None ? *SplineComponentName.ToString() : TEXT("RootComponent")),
				*GetFullName());
		}
		return null;
	}
	return spline;
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
void	UPopcornFXAttributeSamplerAnimTrack::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.Property != NULL)
	{
		if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerAnimTrack, TargetActor) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerAnimTrack, SplineComponentName) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerAnimTrack, bTranslate) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerAnimTrack, bRotate) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerAnimTrack, bScale))
		{
			m_Data->m_NeedsReload = true;
		}
	}
	Super::PostEditChangeProperty(propertyChangedEvent);
}
#endif // WITH_EDITOR

bool	UPopcornFXAttributeSamplerAnimTrack::RebuildCurvesIFN()
{
	USplineComponent	*splineComponent = ResolveSplineComponent(true);
	if (splineComponent == null)
		return false;

	m_Data->m_CurrentSplineComponent = splineComponent;

	if (bFastSampler)
	{
		PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerAnimTrack::Setup (Fast)", POPCORNFX_UE_PROFILER_COLOR);

		if (m_Data->m_DescFast == null)
			return false; // Requires effect restart
		const FInterpCurveVector	&positions = splineComponent->GetSplinePointsPosition();
		const FInterpCurveVector	&scales = splineComponent->GetSplinePointsScale();

		// Build fast sampler
		const u32					keyCount = positions.Points.Num();
		const u32					realKeyCount = PopcornFX::PKMax(keyCount, 2U);
		PopcornFX::TArray<float>	times(realKeyCount);
		float						*dstTimes = times.RawDataPointer() + 1;
		const float					splineLength = splineComponent->GetSplineLength();

		times[0] = 0.0f;
		times[realKeyCount - 1] = 1.0f;
		for (u32 iPoint = 1; iPoint < keyCount - 1; ++iPoint)
			*dstTimes++ = splineComponent->GetDistanceAlongSplineAtSplinePoint(iPoint) / splineLength;
		const float	*srcTimes = times.RawDataPointer();
		const float	invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();

		{
#		define	REBUILD_CURVE(cond, curve, count)																\
				if (cond)																						\
				{																								\
					if (curve == null)																			\
						curve = PK_NEW(PopcornFX::CCurveDescriptor());											\
					if (!PK_VERIFY(curve != null))																\
						return false;																			\
					curve->m_Order = 3;																			\
					curve->m_Interpolator = PopcornFX::CInterpolableVectorArray::Interpolator_Hermite;			\
					if (!PK_VERIFY(curve->Resize(count)))														\
						return false;																			\
					PopcornFX::Mem::Copy(curve->m_Times.RawDataPointer(), srcTimes, sizeof(float) * count);		\
				}																								\
				else if (curve != null)																			\
					PK_SAFE_DELETE(curve);

			REBUILD_CURVE(bTranslate, m_Data->m_Positions, realKeyCount);
			REBUILD_CURVE(bScale, m_Data->m_Scales, realKeyCount);
		}

		const VectorRegister	invScale = MakeVectorRegister(invGlobalScale, invGlobalScale, invGlobalScale, invGlobalScale);

		// Positions curve
		if (bTranslate)
		{
			PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerAnimTrack::Setup (Fast) - Positions", POPCORNFX_UE_PROFILER_COLOR);

			CFloat3		*dstPos = reinterpret_cast<CFloat3*>(m_Data->m_Positions->m_FloatValues.RawDataPointer());
			CFloat3		*dstTangents = reinterpret_cast<CFloat3*>(m_Data->m_Positions->m_FloatTangents.RawDataPointer());
			const FInterpCurvePoint<FVector>	*srcPoints = positions.Points.GetData();
			for (u32 iPoint = 0; iPoint < keyCount; ++iPoint)
			{
#if (ENGINE_MAJOR_VERSION == 5)
				const CFloat3	pos = ToPk(srcPoints->OutVal) * invGlobalScale;
				const CFloat3	srcArriveTan = ToPk(srcPoints->ArriveTangent) * invGlobalScale;
				const CFloat3	srcLeaveTan = ToPk(srcPoints->LeaveTangent) * invGlobalScale;

				*dstPos++ = pos;
				*dstTangents++ = srcArriveTan;
				*dstTangents++ = srcLeaveTan;
#else
				VectorRegister	srcPos = VectorLoadFloat3(&srcPoints->OutVal);
				VectorRegister	srcArriveTan = VectorLoadFloat3(&srcPoints->ArriveTangent);
				VectorRegister	srcLeaveTan = VectorLoadFloat3(&srcPoints->LeaveTangent);

				const VectorRegister	pos = VectorMultiply(srcPos, invScale);
				const VectorRegister	arriveTangent = VectorMultiply(srcArriveTan, invScale);
				const VectorRegister	leaveTangent = VectorMultiply(srcLeaveTan, invScale);

				VectorStoreFloat3(pos, dstPos++);
				VectorStoreFloat3(arriveTangent, dstTangents++);
				VectorStoreFloat3(leaveTangent, dstTangents++);
#endif // (ENGINE_MAJOR_VERSION == 5)
				++srcPoints;
			}
			// Duplicate first and last values
			if (keyCount == 1)
			{
				PopcornFX::Mem::Copy(dstPos, dstPos - 1, sizeof(float) * 3);
				PopcornFX::Mem::Copy(dstTangents, dstTangents - 2, sizeof(float) * 6);
			}

			m_Data->m_Positions->RecomputeParametricDomain();
			if (!PK_VERIFY(m_Data->m_Positions->IsCoherent()))
				return false;
		}

		// Scales curve
		if (bScale)
		{
			PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerAnimTrack::Setup (Fast) - Scales", POPCORNFX_UE_PROFILER_COLOR);

			CFloat3		*dstPos = reinterpret_cast<CFloat3*>(m_Data->m_Scales->m_FloatValues.RawDataPointer());
			CFloat3		*dstTangents = reinterpret_cast<CFloat3*>(m_Data->m_Scales->m_FloatTangents.RawDataPointer());
			const FInterpCurvePoint<FVector>	*srcPoints = scales.Points.GetData();
			for (u32 iPoint = 0; iPoint < keyCount; ++iPoint)
			{
				*dstPos++ = ToPk(srcPoints->OutVal);
				*dstTangents++ = ToPk(srcPoints->ArriveTangent);
				*dstTangents++ = ToPk(srcPoints++->LeaveTangent);
			}
			// Duplicate first and last values
			if (keyCount == 1)
			{
				PopcornFX::Mem::Copy(dstPos, dstPos - 1, sizeof(float) * 3);
				PopcornFX::Mem::Copy(dstTangents, dstTangents - 2, sizeof(float) * 6);
			}

			m_Data->m_Scales->RecomputeParametricDomain();
			if (!PK_VERIFY(m_Data->m_Scales->IsCoherent()))
				return false;
		}

		// Rotations curve
		if (bRotate)
		{
#if WITH_EDITOR
			if (!bEditorRebuildEachFrame)
#endif // WITH_EDITOR
				UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Rotations disabled for the AnimTrack fast sampler. Uncheck \"Fast sampler\" to enable exact rotations"));
		}

		// (Re)setup the descriptor
		m_Data->m_DescFast->m_Tracks.Clear();
		const PopcornFX::CGuid	id = m_Data->m_DescFast->m_Tracks.PushBack(PopcornFX::CParticleSamplerDescriptor_AnimTrack_Default::SPathDefinition(m_Data->m_Positions, null, m_Data->m_Scales));
		PK_ASSERT(id.Valid());
		return id.Valid();
	}
	if (m_Data->m_Desc == null)
		return false; // Requires effect restart

	// Exact sampler
	u32	transformFlags = 0;
	if (bTranslate)
		transformFlags |= PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Translate;
	if (bRotate)
		transformFlags |= PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Rotate;
	if (bScale)
		transformFlags |= PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Scale;
	return m_Data->m_Desc->Setup(splineComponent, transformFlags);
}

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerAnimTrack::_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	check(m_Data != null);
	const PopcornFX::CResourceDescriptor_AnimTrack	*defaultAnimTrackSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_AnimTrack>(defaultSampler);
	if (!PK_VERIFY(defaultAnimTrackSampler != null))
		return null;
	if (bFastSampler)
	{
		// Keep those around..
		// if (m_Data->m_Desc != null)
		// 	m_Data->m_Desc = null;
		if (m_Data->m_DescFast == null)
		{
			m_Data->m_DescFast = PK_NEW(PopcornFX::CParticleSamplerDescriptor_AnimTrack_Default());
			if (!PK_VERIFY(m_Data->m_DescFast != null))
				return null;
			m_Data->m_DescFast->m_TrackTransforms = reinterpret_cast<CFloat4x4*>(&(m_TrackTransforms));
			m_Data->m_DescFast->m_TrackTransformsUnscaled = reinterpret_cast<CFloat4x4*>(&(m_TrackTransformsUnscaled));
			m_Data->m_NeedsReload = true;
		}
	}
	else
	{
		// Keep those around..
		// {
		// 	PK_SAFE_DELETE(m_Data->m_Positions);
		// 	PK_SAFE_DELETE(m_Data->m_Scales);
		// 	m_Data->m_DescFast = null;
		// }
		if (m_Data->m_Desc == null)
		{
			m_Data->m_Desc = PK_NEW(CParticleSamplerDescriptor_AnimTrack_UE(&m_TrackTransforms));
			if (!PK_VERIFY(m_Data->m_Desc != null))
				return null;
			m_Data->m_NeedsReload = true;
		}
	}
	if (m_Data->m_NeedsReload)
	{
		if (!RebuildCurvesIFN())
			return null;

		m_Data->m_NeedsReload = false;
	}
	desc.m_NeedUpdate = true;
	_AttribSampler_PreUpdate(0.f);
	if (bFastSampler)
		return m_Data->m_DescFast.Get();
	return m_Data->m_Desc.Get();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerAnimTrack::_AttribSampler_PreUpdate(float deltaTime)
{
	check(m_Data != null);

	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerAnimTrack::_AttribSampler_PreUpdate", POPCORNFX_UE_PROFILER_COLOR);

#if WITH_EDITOR
	if (m_Data->m_Desc == null && m_Data->m_DescFast == null)
		return; // We shouldn't be here. This can occur in blueprints where components get registered/unregistered multiple times
#else
	if (!PK_VERIFY(m_Data->m_Desc != null || m_Data->m_DescFast != null))
		return;
#endif

	if (!bTranslate && !bRotate && !bScale)
		return;
	if (!m_Data->m_CurrentSplineComponent.IsValid())
		m_Data->m_CurrentSplineComponent = ResolveSplineComponent(false);

#if WITH_EDITOR
	if (bEditorRebuildEachFrame)
	{
		if (!RebuildCurvesIFN())
			return;
	}
#endif // WITH_EDITOR

	if (!m_Data->m_CurrentSplineComponent.IsValid() &&
		(Transforms == EPopcornFXSplineTransforms::SplineComponentRelativeTr ||
		 Transforms == EPopcornFXSplineTransforms::SplineComponentWorldTr))
	{
		// No transforms update, keep previously created transforms, if any
		return;
	}

	switch (Transforms)
	{
		case	EPopcornFXSplineTransforms::SplineComponentRelativeTr:
			if (bScale || bFastSampler)
				m_TrackTransforms = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetRelativeTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetRelativeTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSplineTransforms::SplineComponentWorldTr:
			if (bScale || bFastSampler)
				m_TrackTransforms = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetComponentTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetComponentTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSplineTransforms::AttrSamplerRelativeTr:
			if (bScale || bFastSampler)
				m_TrackTransforms = (FMatrix44f)GetRelativeTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)GetRelativeTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSplineTransforms::AttrSamplerWorldTr:
			if (bScale || bFastSampler)
				m_TrackTransforms = (FMatrix44f)GetComponentTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)GetComponentTransform().ToMatrixNoScale();
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			break;
	}

	const float	invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();

	if (!bScale && !bFastSampler)
		m_TrackTransforms = m_TrackTransformsUnscaled;

	m_TrackTransforms.M[3][0] *= invGlobalScale;
	m_TrackTransforms.M[3][1] *= invGlobalScale;
	m_TrackTransforms.M[3][2] *= invGlobalScale;
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
