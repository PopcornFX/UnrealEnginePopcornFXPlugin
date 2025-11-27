//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXSDK.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXStats.h"
#include "PopcornFXAttributeSamplerActor.h"
#include "PopcornFXAttributeList.h"
#include "PopcornFXAttributeSampler.h"
#include "PopcornFXAttributeSamplerShape.h"
#include "PopcornFXAttributeSamplerImage.h"
#include "PopcornFXAttributeSamplerGrid.h"
#include "PopcornFXAttributeSamplerText.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXAttributeSamplerCurveDynamic.h"
#include "PopcornFXAttributeSamplerAnimTrack.h"
#include "PopcornFXAttributeSamplerVectorField.h"
#include "GPUSim/PopcornFXGPUSim.h"
#include "Assets/PopcornFXMesh.h"
#include "Assets/PopcornFXTextureAtlas.h"
#include "Internal/ResourceHandlerImage_UE.h"
#include "Internal/ParticleScene.h"
#include "Platforms/PopcornFXPlatform.h"

#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ClothingAsset.h"
#include "DrawDebugHelpers.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveLinearColor.h"
#include "Math/InterpCurve.h"
#include "VectorField/VectorFieldStatic.h"
#include "Serialization/BulkData.h"
#include "Curves/RichCurve.h"
#include "Engine/VolumeTexture.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "UObject/Package.h"

// Don't include the DestructibleComponent implementation header,
// relies on ApexDestruction plugin which isn't loaded when PopcornFX runtime
// module is loaded. Include the interface instead.
// #	include "DestructibleComponent.h"
#include "DestructibleInterface.h"

#if WITH_EDITOR
#	include "Editor.h"
#	include "Engine/Selection.h"
#endif

#include "PopcornFXSDK.h"
#include <pk_imaging/include/im_image.h>
#include <pk_particles/include/ps_descriptor.h>
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_particles/include/ps_effect.h>
#include <pk_kernel/include/kr_containers_array.h>
#include <pk_kernel/include/kr_containers_onstack.h>
#include <pk_kernel/include/kr_refcounted_buffer.h>
#include <pk_kernel/include/kr_mem_utils.h>
#include <pk_maths/include/pk_maths_interpolable.h>
#include <pk_geometrics/include/ge_rectangle_list.h>
#include <pk_geometrics/include/ge_mesh_sampler_accel.h>

#include <pk_particles_toolbox/include/pt_mesh_deformers_skin.h>

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
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplerShape, Log, All);

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
#if WITH_EDITOR
	Sampler->bIsInline = false;
#endif

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

APopcornFXAttributeSamplerImageActor::APopcornFXAttributeSamplerImageActor(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	_CtorRootSamplerComponent(PCIP, EPopcornFXAttributeSamplerComponentType::Image);
}

APopcornFXAttributeSamplerGridActor::APopcornFXAttributeSamplerGridActor(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	_CtorRootSamplerComponent(PCIP, EPopcornFXAttributeSamplerComponentType::Grid);
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
		case	EPopcornFXAttributeSamplerComponentType::Image:
			spriteName = "AttributeSampler_Image";
			break;
		case	EPopcornFXAttributeSamplerComponentType::Grid:
			spriteName = "AttributeSampler_Grid";
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

void	APopcornFXAttributeSamplerActor::PostLoad()
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

void	APopcornFXAttributeSamplerActor::PostActorCreated()
{
	Super::PostActorCreated();
#if WITH_EDITOR
	ReloadSprite();
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------------

const UClass	*GetSamplerClass(EPopcornFXAttributeSamplerType::Type type)
{
	switch (type)
	{
	case EPopcornFXAttributeSamplerType::Type::None:
		return null;
	case EPopcornFXAttributeSamplerType::Type::Shape:
		return UPopcornFXAttributeSamplerShape::StaticClass();
	case EPopcornFXAttributeSamplerType::Type::Image:
		return UPopcornFXAttributeSamplerImage::StaticClass();
	case EPopcornFXAttributeSamplerType::Type::Grid:
		return UPopcornFXAttributeSamplerGrid::StaticClass();
	case EPopcornFXAttributeSamplerType::Type::Curve:
		return UPopcornFXAttributeSamplerCurve::StaticClass();
	case EPopcornFXAttributeSamplerType::Type::AnimTrack:
		return UPopcornFXAttributeSamplerAnimTrack::StaticClass();
	case EPopcornFXAttributeSamplerType::Type::Turbulence:
		return UPopcornFXAttributeSamplerVectorField::StaticClass();
	case EPopcornFXAttributeSamplerType::Type::Text:
		return UPopcornFXAttributeSamplerText::StaticClass();
	}
	return null;
}

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSampler
//
//----------------------------------------------------------------------------

// static
UClass	*UPopcornFXAttributeSampler::SamplerComponentClass(EPopcornFXAttributeSamplerComponentType::Type type)
{
	switch (type)
	{
	case	EPopcornFXAttributeSamplerComponentType::Shape:
		return UPopcornFXAttributeSamplerShape::StaticClass();
	case	EPopcornFXAttributeSamplerComponentType::Image:
		return UPopcornFXAttributeSamplerImage::StaticClass();
	case	EPopcornFXAttributeSamplerComponentType::Grid:
		return UPopcornFXAttributeSamplerGrid::StaticClass();
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
#if WITH_EDITOR
,	bIsInline(true)
#endif
{
	SetFlags(RF_Transactional);
}

void	UPopcornFXAttributeSampler::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	m_SamplerType = other->m_SamplerType;
#if WITH_EDITOR
	bIsInline = other->bIsInline;
	// Note: It happens before samplers refresh so unsupported and incompatible properties are not updated yet
	m_UnsupportedProperties = other->m_UnsupportedProperties;
	m_IncompatibleProperties = other->m_IncompatibleProperties;
#endif
}

#if WITH_EDITOR
void	UPopcornFXAttributeSampler::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	Super::PostEditChangeProperty(propertyChangedEvent);

	// Remove deleted emitters
	m_EmittersUsingThis.RemoveAll([](UPopcornFXEmitterComponent *emitter)
		{
			if (emitter == null)
				return true;
			return false;
		}
	);

	for (UPopcornFXEmitterComponent *emitter : m_EmittersUsingThis)
	{
		const UPopcornFXAttributeList	*attr = emitter->GetAttributeListIFP();
		for (const FPopcornFXSamplerDesc &desc : attr->m_Samplers)
		{
			if (desc.m_UseExternalSampler && desc.m_RestartWhenSamplerChanges && desc.ResolveExternalAttributeSampler(emitter, null) == this)
			{
				emitter->RestartEmitter();
			}
		}
	}
}
#endif

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

	UStaticMesh												*m_StaticMesh = null;
	USkeletalMesh											*m_SkeletalMesh = null;
	PopcornFX::PMeshNew										m_Mesh;
	PopcornFX::PParticleSamplerDescriptor_Shape_Default		m_Desc;
	PopcornFX::PShapeDescriptor								m_Shape;
	PopcornFX::CMeshSurfaceSamplerStructuresRandom			m_SamplerSurface;

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

	PopcornFX::CBaseSkinningStreams *m_SkinningStreamsProxy = null;
	PopcornFX::SSkinContext						m_SkinContext;
	PopcornFX::CSkinAsyncContext				m_AsyncSkinContext;
	PopcornFX::CSkeletonView *m_SkeletonView = null;

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

#if WITH_EDITOR
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	m_IndirectSelectedThisTick = false;
#endif // WITH_EDITOR

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

bool	UPopcornFXAttributeSamplerShape::CanUpdateShapeProperties()
{
	PK_ASSERT(m_Data != null);
	if (m_Data->m_Shape == null)
		return false;
	if (!PK_VERIFY(m_Data->m_Desc != null) ||
		!PK_VERIFY(m_Data->m_Desc->m_Shape != null) ||
		!PK_VERIFY(m_Data->m_Desc->m_Shape == m_Data->m_Shape) ||
		m_Data->m_Shape->ShapeType() != ToPkShapeType(Properties.ShapeType))
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
	PK_ASSERT(m_Data->m_Shape->ShapeType() == ToPkShapeType(Properties.ShapeType));
	(*kCbUpdateShapeDescriptors[Properties.ShapeType.GetValue()])(SUpdateShapeParams{ this, m_Data->m_Shape.Get() });
	m_Data->m_Shape->m_Weight = Properties.Weight;
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
		if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bEditorBuildInitialPose))
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
		UE_LOG(LogPopcornFXAttributeSampler, Error, TEXT("New properties are null or not Shape properties"));
		return;
	}

	Super::CopyPropertiesFrom(other);

	Properties = *newShapeProperties;

	if (!CanUpdateShapeProperties())
		return;
	if (newShapeProperties->Weight != Properties.Weight ||
		newShapeProperties->BoxDimension != Properties.BoxDimension ||
		newShapeProperties->Radius != Properties.Radius ||
		newShapeProperties->InnerRadius != Properties.InnerRadius ||
		newShapeProperties->Height != Properties.Height ||
		newShapeProperties->Scale != Properties.Scale ||
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
		newShapeProperties->CollectionSamplingHeuristic != Properties.CollectionSamplingHeuristic ||
		newShapeProperties->CollectionUseShapeWeights != Properties.CollectionUseShapeWeights ||
#endif
		newShapeProperties->ShapeSamplingMode != Properties.ShapeSamplingMode ||
		newShapeProperties->DensityColorChannel != Properties.DensityColorChannel)
	{
		PK_ASSERT(m_Data->m_Shape->ShapeType() == ToPkShapeType(Properties.ShapeType));
		(*kCbUpdateShapeDescriptors[Properties.ShapeType.GetValue()])(SUpdateShapeParams{ this, m_Data->m_Shape.Get() });
		m_Data->m_Shape->m_Weight = Properties.Weight;
	}
	else
	{
		// invalidate shape
		m_Data->m_Desc = null;
		m_Data->m_Shape = null;
	}

}

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

				// @TODO : Warn the user that he'll have to unpause the skinner when rehooking an effect to this attr sampler
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

	else
	{
		PK_ASSERT(GetWorld() != null);
		if (GetWorld()->IsGameWorld())
			return;

		bool				render = FPopcornFXPlugin::Get().SettingsEditor()->bAlwaysRenderAttributeSamplerShapes;

		const USelection *selectedAssets = GEditor->GetSelectedActors();
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
}

//----------------------------------------------------------------------------

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

	m_Data->m_SamplerSurface = PopcornFX::CMeshSurfaceSamplerStructuresRandom();

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
	else if (!InitShape())
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
	if (Properties.CurveDimension == InCurveDimension)
		return;
	Properties.SecondCurve1D = null;
	//SecondCurve2D = null;
	Properties.SecondCurve3D = null;
	Properties.SecondCurve4D = null;
	Properties.Curve1D = null;
	//Curve2D = null;
	Properties.Curve3D = null;
	Properties.Curve4D = null;
	Properties.CurveDimension = InCurveDimension;
	m_Data->m_NeedsReload = true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurve::SetCurve(class UCurveBase *InCurve, bool InIsSecondCurve)
{
	if (!PK_VERIFY(InCurve != null))
		return false;
	if (InIsSecondCurve)
	{
		Properties.SecondCurve1D = null;
		//SecondCurve2D = null;
		Properties.SecondCurve3D = null;
		Properties.SecondCurve4D = null;
	}
	else
	{
		Properties.Curve1D = null;
		//Curve2D = null;
		Properties.Curve3D = null;
		Properties.Curve4D = null;
	}
	bool			ok = false;
	switch (Properties.CurveDimension)
	{
	case	EAttributeSamplerCurveDimension::Float1:
		if (InIsSecondCurve)
			ok = ((Properties.SecondCurve1D = Cast<UCurveFloat>(InCurve)) != null);
		else
			ok = ((Properties.Curve1D = Cast<UCurveFloat>(InCurve)) != null);
		break;
	case	EAttributeSamplerCurveDimension::Float2:
		PK_ASSERT_NOT_REACHED();
		ok = false;
		break;
	case	EAttributeSamplerCurveDimension::Float3:
		if (InIsSecondCurve)
			ok = ((Properties.SecondCurve3D = Cast<UCurveVector>(InCurve)) != null);
		else
			ok = ((Properties.Curve3D = Cast<UCurveVector>(InCurve)) != null);
		break;
	case	EAttributeSamplerCurveDimension::Float4:
		if (InIsSecondCurve)
			ok = ((Properties.SecondCurve4D = Cast<UCurveLinearColor>(InCurve)) != null);
		else
			ok = ((Properties.Curve4D = Cast<UCurveLinearColor>(InCurve)) != null);
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
	Properties.bIsDoubleCurve = false;
	Properties.CurveDimension = EAttributeSamplerCurveDimension::Float1;

	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Curve;

	Properties.Curve1D = null;
	Properties.Curve3D = null;
	Properties.Curve4D = null;

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

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesCurve *newCurveProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesCurve *>(other->GetProperties());
	if (!PK_VERIFY(newCurveProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSampler, Error, TEXT("New properties are null or not curve properties"));
		return;
	}

	Super::CopyPropertiesFrom(other);

	// Always rebuild for now
	m_Data->m_NeedsReload = true;

	Properties = *newCurveProperties;
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
		const int32	index = i * Properties.CurveDimension;
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
				tangents[tangentIndex + Properties.CurveDimension] = leaveTangent;
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
				tangents[tangentIndex + Properties.CurveDimension] = key.LeaveTangent * nextDelta;
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
				tangents[tangentIndex + Properties.CurveDimension] = leaveTangent;
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
				tangents[tangentIndex + Properties.CurveDimension] = ((nextSampledValue - prevSampledValue) / (2.0f * CURVE_MINIMUM_DELTA)) * nextDelta;
			}
		}
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::GetAssociatedCurves(UCurveBase *&curve0, UCurveBase *&curve1)
{
	switch (Properties.CurveDimension)
	{
		case	EAttributeSamplerCurveDimension::Float1:
		{
			curve0 = Properties.Curve1D;
			curve1 = Properties.SecondCurve1D;
			break;
		}
		case	EAttributeSamplerCurveDimension::Float2:
		{
			PK_ASSERT_NOT_REACHED();
			break;
		}
		case	EAttributeSamplerCurveDimension::Float3:
		{
			curve0 = Properties.Curve3D;
			curve1 = Properties.SecondCurve3D;
			break;
		}
		case	EAttributeSamplerCurveDimension::Float4:
		{
			curve0 = Properties.Curve4D;
			curve1 = Properties.SecondCurve4D;
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
	if (!PK_VERIFY(curves.Num() == Properties.CurveDimension))
		return false;

	curveDescriptor->m_Order = (u32)Properties.CurveDimension;
	curveDescriptor->m_Interpolator = PopcornFX::CInterpolableVectorArray::Interpolator_Hermite;

	TArray<float>	collectedTimes;
	// Fetch every independant keys
	for (int32 i = 0; i < Properties.CurveDimension; ++i)
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
	for (int32 i = 0; i < Properties.CurveDimension; ++i)
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
	if (curve0 == null || (Properties.bIsDoubleCurve && curve1 == null))
		return false;
	bool	success = true;

	success &= SetupCurve(m_Data->m_Curve0, curve0);
	success &= !Properties.bIsDoubleCurve || SetupCurve(m_Data->m_Curve1, curve1);
	return success;
}

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerCurve::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
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
		if (defaultIsDoubleCurve != Properties.bIsDoubleCurve)
			return null;
		if (m_Data->m_Curve0 == null)
			m_Data->m_Curve0 = PK_NEW(PopcornFX::CCurveDescriptor());
		if (Properties.bIsDoubleCurve)
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

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerCurveDynamic::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
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
	Properties.Text = InText;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerText::UPopcornFXAttributeSamplerText(const FObjectInitializer &PCIP)
: Super(PCIP)
{
	bAutoActivate = true;

	Properties.Text = "";
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
		if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesText, Text))
			m_Data->m_NeedsReload = true;
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerText::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesText *newTextProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesText *>(other->GetProperties());
	if (!PK_VERIFY(newTextProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSampler, Error, TEXT("New properties are null or not text properties"));
		return;
	}

	if (newTextProperties->Text != Properties.Text)
	{
		m_Data->m_NeedsReload = true;
	}

	Super::CopyPropertiesFrom(other);

	Properties = *newTextProperties;
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerText::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
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
		if (!PK_VERIFY(m_Data->m_Desc->_Setup(ToPk(Properties.Text), fontMetrics, useKerning)))
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
	Properties.Texture = InTexture;
	m_Data->m_ReloadTexture = true;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerImage::UPopcornFXAttributeSamplerImage(const FObjectInitializer &PCIP)
	: Super(PCIP)
{
	bAutoActivate = true;

	Properties.bAllowTextureConversionAtRuntime = false;

	Properties.SamplingMode = EPopcornFXImageSamplingMode::Regular;
	Properties.DensitySource = EPopcornFXImageDensitySource::RGBA_Average;
	Properties.DensityPower = 1.0f;

	Properties.Texture = null;
	Properties.TextureAtlas = null;
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
		// Unregister the component during OnUnregister instead of BeginDestroy.
		// In editor mode, BeginDestroy is only called when saving a level:
		// Components ReregisterComponent() do not have a matching BeginDestroy call in editor
		m_Data->m_Desc = null;
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

		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, Texture) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, bAllowTextureConversionAtRuntime))
		{
			m_Data->m_ReloadTexture = true;
			m_Data->m_RebuildPDF = true;
		}
		else if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, TextureAtlas))
		{
			m_Data->m_ReloadTextureAtlas = true;
			m_Data->m_RebuildPDF = true;
		}
		else if (	propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, SamplingMode) ||
					propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, DensitySource) ||
					propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, DensityPower))
		{
			m_Data->m_RebuildPDF = true;
		}
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerImage::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesImage *newImageProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesImage *>(other->GetProperties());
	if (!PK_VERIFY(newImageProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSampler, Error, TEXT("New properties are null or not image properties"));
		return;
	}

	Super::CopyPropertiesFrom(other);

	if (newImageProperties->Texture != Properties.Texture ||
		newImageProperties->bAllowTextureConversionAtRuntime != Properties.bAllowTextureConversionAtRuntime)
	{
		m_Data->m_ReloadTexture = true;
		m_Data->m_RebuildPDF = true;
	}
	else if (newImageProperties->TextureAtlas != Properties.TextureAtlas)
	{
		m_Data->m_ReloadTextureAtlas = true;
		m_Data->m_RebuildPDF = true;
	}
	else if (newImageProperties->SamplingMode != Properties.SamplingMode ||
		newImageProperties->DensitySource != Properties.DensitySource ||
		newImageProperties->DensityPower != Properties.DensityPower)
	{
		m_Data->m_RebuildPDF = true;
	}

	Properties = *newImageProperties;
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerImage::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	const PopcornFX::CResourceDescriptor_Image	*defaultImageSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Image>(defaultSampler);
	if (!PK_VERIFY(defaultImageSampler != null))
		return null;
	if (!/*PK_VERIFY*/(RebuildImageSampler()))
	{
		const FString	imageName = Properties.Texture != null ? Properties.Texture->GetName() : FString(TEXT("null"));
		const FString	atlasName = Properties.TextureAtlas != null ? Properties.TextureAtlas->GetName() : FString(TEXT("null"));
		// Do we really want to warn if the texture is just not set -> it's just going to use the default one?
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

	if (Properties.Texture == null)
		return false;

#if (PK_HAS_GPU != 0)
	bool	updateGPUTextureBinding = false;
#endif // (PK_HAS_GPU != 0)

	const bool	rebuildImage = m_Data->m_ReloadTexture;
	if (rebuildImage)
	{
		const PopcornFX::CString	fullPath = ToPk(Properties.Texture->GetPathName());
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
			UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("AttrSamplerImage: couldn't load texture '%s' for CPU/GPU sim sampling"), *ToUE(fullPath));
			return false;
		}
		m_Data->m_ReloadTexture = false;
	}
	const bool	reloadImageAtlas = m_Data->m_ReloadTextureAtlas;
	if (reloadImageAtlas)
	{
		if (Properties.TextureAtlas != null)
		{
			bool						success = false;
			const PopcornFX::CString	fullPath = ToPk(Properties.TextureAtlas->GetPathName());
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
				UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("AttrSamplerImage: couldn't load texture atlas '%s' for CPU/GPU sim sampling"), *ToUE(fullPath));
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
												UE::DXGIUtilities::FindShaderResourceFormat(imageResource->GetDesc().Format, sRGB),
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

		const bool	both = Properties.SamplingMode == EPopcornFXImageSamplingMode::Both;
		if (Properties.SamplingMode == EPopcornFXImageSamplingMode::Density || both)
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

		if (Properties.SamplingMode == EPopcornFXImageSamplingMode::Regular || both)
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
	settings.m_DensityPower = Properties.DensityPower;
	settings.m_RGBAToLumMode = static_cast<PopcornFX::SImageConvertSettings::ELumMode>(Properties.DensitySource.GetValue());
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
		if (Properties.bAllowTextureConversionAtRuntime)
		{
			const PopcornFX::CImage::EFormat		dstFormat = PopcornFX::CImage::Format_BGRA8;
			UE_LOG(LogPopcornFXAttributeSampler, Log,
				TEXT("AttrSamplerImage: texture '%s' format %s not supported for sampling, converting to %s (because AllowTextureConvertionAtRuntime) in '%s'"),
				*Properties.Texture->GetName(),
				UTF8_TO_TCHAR(PopcornFX::CImage::GetFormatName(m_Data->m_TextureResource->m_Format)),
				UTF8_TO_TCHAR(PopcornFX::CImage::GetFormatName(dstFormat)),
				*GetPathName());

			PopcornFX::CImageSurface	newSurface;
			newSurface.m_Format = dstFormat;
			if (!newSurface.CopyAndConvertIFN(dstSurface))
			{
				UE_LOG(LogPopcornFXAttributeSampler, Warning,
					TEXT("AttrSamplerImage: could not convert texture '%s' from %s to %s in %s"),
					*Properties.Texture->GetName(),
					UTF8_TO_TCHAR(PopcornFX::CImage::GetFormatName(m_Data->m_TextureResource->m_Format)),
					UTF8_TO_TCHAR(PopcornFX::CImage::GetFormatName(dstFormat)),
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
				*Properties.Texture->GetName(),
				UTF8_TO_TCHAR(PopcornFX::CImage::GetFormatName(m_Data->m_TextureResource->m_Format)),
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
	Properties.Intensity = 1.f;
	Properties.VectorField = null;
	Properties.VolumeDimensions = FVector(100.f, 100.f, 100.f);
	Properties.SamplingMode = EPopcornFXVectorFieldSamplingMode::Trilinear;
	Properties.WrapMode = EPopcornFXVectorFieldWrapMode::Wrap;

	// Guess here, turbulence sampler probably used by physics evolver or script evolver
	Properties.bUseRelativeTransform = false;

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
	PK_ASSERT(Properties.VectorField != null);

	if (Properties.BoundsSource == EPopcornFXVectorFieldBounds::Source)
	{
		const PopcornFX::CAABB	bounds = PopcornFX::CAABB::FromMinMax(ToPk(Properties.VectorField->Bounds.Min), ToPk(Properties.VectorField->Bounds.Max));
		m_Data->m_Desc->SetBounds(bounds);
#if WITH_EDITOR
		m_Data->m_RealExtentUnscaled = ToUE(bounds.Extent() * FPopcornFXPlugin::GlobalScale());
#endif // WITH_EDITOR
	}
	else
	{
		PK_ASSERT(Properties.BoundsSource == EPopcornFXVectorFieldBounds::Custom);

		PopcornFX::CAABB	bounds;
		bounds.SetupFromHalfExtent(ToPk(Properties.VolumeDimensions) * 0.5f * FPopcornFXPlugin::GlobalScaleRcp());
		m_Data->m_Desc->SetBounds(bounds);

#if WITH_EDITOR
		m_Data->m_RealExtentUnscaled = ToUE(bounds.Extent() * FPopcornFXPlugin::GlobalScale());
#endif // WITH_EDITOR
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::_BuildVectorFieldFlags(uint32 &flags, uint32 &interpolation) const
{
	if (Properties.WrapMode == EPopcornFXVectorFieldWrapMode::Wrap)
	{
		flags |= PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Wrap_X;
		flags |= PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Wrap_Y;
		flags |= PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Wrap_Z;
	}
	PK_STATIC_ASSERT(EPopcornFXVectorFieldSamplingMode::Point == (u32)PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Interpolation_Point);
	PK_STATIC_ASSERT(EPopcornFXVectorFieldSamplingMode::Trilinear == (u32)PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Interpolation_Trilinear);

	interpolation = static_cast<PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::EInterpolation>(Properties.SamplingMode.GetValue());
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
void	UPopcornFXAttributeSamplerVectorField::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
	if (propertyChangedEvent.MemberProperty != null && m_Data->m_Desc != null)
	{
		const FString	propertyName = propertyChangedEvent.MemberProperty->GetName();

		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, VectorField))
		{
			m_Data->m_NeedsReload = true;
			if (Properties.VectorField == null)
			{
				m_Data->m_Desc->Clear();
				// Disable debug rendering, we don't have that info
				if (Properties.BoundsSource == EPopcornFXVectorFieldBounds::Source)
					m_Data->m_RealExtentUnscaled = FVector3f::ZeroVector;
			}
		}
		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, Intensity))
		{
			m_Data->m_Desc->SetStrength(Properties.Intensity);
		}
		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, BoundsSource) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, VolumeDimensions))
		{
			if (Properties.VectorField != null)
				_SetBounds();
		}
		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, WrapMode) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, SamplingMode))
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

void	UPopcornFXAttributeSamplerVectorField::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesVectorField *newVectorFieldProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesVectorField *>(other->GetProperties());
	if (!PK_VERIFY(newVectorFieldProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSampler, Error, TEXT("New properties are null or not VectorField properties"));
		return;
	}

	Super::CopyPropertiesFrom(other);

	if (m_Data->m_Desc == null)
	{
		return;
	}
	if (newVectorFieldProperties->VectorField != Properties.VectorField)
	{
		m_Data->m_NeedsReload = true;
		if (Properties.VectorField == null)
		{
			m_Data->m_Desc->Clear();
			// Disable debug rendering, we don't have that info
			if (Properties.BoundsSource == EPopcornFXVectorFieldBounds::Source)
				m_Data->m_RealExtentUnscaled = FVector3f::ZeroVector;
		}
	}
	if (newVectorFieldProperties->Intensity != Properties.Intensity)
	{
		m_Data->m_Desc->SetStrength(Properties.Intensity);
	}
	if (newVectorFieldProperties->BoundsSource != Properties.BoundsSource ||
		newVectorFieldProperties->VolumeDimensions != Properties.VolumeDimensions)
	{
		if (Properties.VectorField != null)
			_SetBounds();
	}
	if (newVectorFieldProperties->WrapMode != Properties.WrapMode ||
		newVectorFieldProperties->SamplingMode != Properties.SamplingMode)
	{
		u32	flags = 0;
		u32	interpolation = 0;

		_BuildVectorFieldFlags(flags, interpolation);
		m_Data->m_Desc->SetFlags(flags, static_cast<PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::EInterpolation>(interpolation));
	}

	Properties = *newVectorFieldProperties;
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

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerVectorField::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerVectorField::Build Vectorfield", POPCORNFX_UE_PROFILER_COLOR);

	check(m_Data != null);
	const PopcornFX::CResourceDescriptor_VectorField	*defaultTurbulenceSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_VectorField>(defaultSampler);
	if (!PK_VERIFY(defaultTurbulenceSampler != null))
		return null;
	if (Properties.VectorField == null)
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

		const CFloat4	dimensions = CFloat4(Properties.VectorField->SizeX, Properties.VectorField->SizeY, Properties.VectorField->SizeZ, 1);
		const u32		elementCount = dimensions.x() * dimensions.y() * dimensions.z();
		PK_ASSERT(Properties.VectorField->SourceData.GetBulkDataSize() == elementCount * sizeof(FFloat16Color));

		PopcornFX::PRefCountedMemoryBuffer	gridRawData = PopcornFX::CRefCountedMemoryBuffer::AllocAligned(sizeof(PopcornFX::f16) * 3 * elementCount, PopcornFX::Memory::CacheLineSize);
		if (!PK_VERIFY(gridRawData != null))
			return null;

		const PopcornFX::f16			*srcValues = reinterpret_cast<PopcornFX::f16*>(Properties.VectorField->SourceData.Lock(LOCK_READ_ONLY));
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
		Properties.VectorField->SourceData.Unlock();

		u32	flags = 0;
		u32	interpolation = 0;
		_BuildVectorFieldFlags(flags, interpolation);

		if (!PK_VERIFY(m_Data->m_Desc->Setup(dimensions, Properties.VectorField->Intensity, CFloat4(0.0f), CFloat4(0.0f), gridRawData,
			PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::DataType_Fp16,
			Properties.Intensity, CFloat4x4::IDENTITY, flags,
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
	if (Properties.RotationAnimation != FVector::ZeroVector)
	{
		m_TimeAccumulation += deltaTime;

		const FVector		animationRotation = Properties.RotationAnimation * m_TimeAccumulation;
		const FTransform	eulerAngles(FQuat::MakeFromEuler(animationRotation));
		if (Properties.bUseRelativeTransform)
			transforms = (eulerAngles * GetRelativeTransform()).ToMatrixWithScale();
		else
			transforms = (eulerAngles * GetComponentTransform()).ToMatrixWithScale();
	}
	else
	{
		if (Properties.bUseRelativeTransform)
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
			const FQuat			eulerAngles = FQuat::MakeFromEuler(Properties.RotationAnimation * m_TimeAccumulation);
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

	virtual void	GetTrackDuration(const TStridedMemoryView<float> &dstValues, const TStridedMemoryView<const s32> &trackIds) const override
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::GetTrackDuration", POPCORNFX_UE_PROFILER_COLOR);
		(void)trackIds;
		PK_ASSERT(!dstValues.Empty());
		const float	broadcast = m_SplineLength;
		if (!dstValues.Virtual())
			PopcornFX::Mem::Fill32(dstValues.Data(), *reinterpret_cast<const u32*>(&broadcast), dstValues.Count());
		else
			dstValues[0] = broadcast;
	}

	virtual void	GetTrackLength(const TStridedMemoryView<float> &dstValues, const TStridedMemoryView<const s32> &trackIds) const override
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::GetTrackLength", POPCORNFX_UE_PROFILER_COLOR);
		(void)trackIds;
		PK_ASSERT(!dstValues.Empty());
		const float	broadcast = 1.0f;
		if (!dstValues.Virtual())
			PopcornFX::Mem::Fill32(dstValues.Data(), *reinterpret_cast<const u32*>(&broadcast), dstValues.Count());
		else
			dstValues[0] = broadcast;
	}

	virtual bool	SupportsSim(const PopcornFX::CParticleUpdater *updater) const override
	{
		(void)updater;
		PK_ASSERT(updater != null);
#if (PK_PARTICLES_UPDATER_USE_D3D12 != 0)
		if (updater->Manager()->UpdateClass() == PopcornFX::CParticleUpdateManager_D3D12::DefaultUpdateClass())
			return m_TrackResourceD3D12 != null;
#endif
		
		return true;
	}
#if (PK_PARTICLES_UPDATER_USE_D3D12 != 0)
	virtual void	BindAnimTrackResourceD3D12(const PopcornFX::SBindingContextD3D12 &context) const override
	{
		if (m_TrackResourceD3D12 != null)
		{
			context.m_Heap.BindTextureShaderResourceView(context.m_Device, m_TrackResourceD3D12, context.m_Location, m_TrackResourceD3D12->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D);
			context.m_Heap.KeepResourceReference(m_TrackResourceD3D12);
		}
	}
#endif

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
#if (PK_PARTICLES_UPDATER_USE_D3D12 != 0)
	ID3D12Resource			*m_TrackResourceD3D12 = null;
#endif
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
	Properties.bTranslate = true;
	Properties.bScale = false;
	Properties.bRotate = false;
	Properties.bFastSampler = true;
	Properties.bEditorRebuildEachFrame = false;

	Properties.Transforms = EPopcornFXSplineTransforms::AttrSamplerRelativeTr;

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
	const AActor		*parent = Properties.TargetActor == null ? fallbackActor : Properties.TargetActor;
	USplineComponent	*spline = null;
	if (Properties.SplineComponentName != NAME_None)
	{
		FObjectPropertyBase	*prop = FindFProperty<FObjectPropertyBase>(parent->GetClass(), Properties.SplineComponentName);

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
				*parent->GetName(), (Properties.SplineComponentName != NAME_None ? *Properties.SplineComponentName.ToString() : TEXT("RootComponent")),
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
		if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, TargetActor) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, SplineComponentName) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, bTranslate) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, bRotate) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, bScale))
		{
			m_Data->m_NeedsReload = true;
		}
	}
	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerAnimTrack::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesAnimTrack *newAnimTrackProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesAnimTrack *>(other->GetProperties());
	if (!PK_VERIFY(newAnimTrackProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSampler, Error, TEXT("New properties are null or not AnimTrack properties"));
		return;
	}

	Super::CopyPropertiesFrom(other);

	if (newAnimTrackProperties->TargetActor != Properties.TargetActor ||
		newAnimTrackProperties->SplineComponentName != Properties.SplineComponentName ||
		newAnimTrackProperties->bTranslate != Properties.bTranslate ||
		newAnimTrackProperties->bRotate != Properties.bRotate ||
		newAnimTrackProperties->bScale != Properties.bScale)
	{
		m_Data->m_NeedsReload = true;
	}

	Properties = *newAnimTrackProperties;
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerAnimTrack::RebuildCurvesIFN()
{
	USplineComponent	*splineComponent = ResolveSplineComponent(true);
	if (splineComponent == null)
		return false;

	m_Data->m_CurrentSplineComponent = splineComponent;

	if (Properties.bFastSampler)
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

			REBUILD_CURVE(Properties.bTranslate, m_Data->m_Positions, realKeyCount);
			REBUILD_CURVE(Properties.bScale, m_Data->m_Scales, realKeyCount);
		}

		const VectorRegister	invScale = MakeVectorRegister(invGlobalScale, invGlobalScale, invGlobalScale, invGlobalScale);

		// Positions curve
		if (Properties.bTranslate)
		{
			PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerAnimTrack::Setup (Fast) - Positions", POPCORNFX_UE_PROFILER_COLOR);

			CFloat3		*dstPos = reinterpret_cast<CFloat3*>(m_Data->m_Positions->m_FloatValues.RawDataPointer());
			CFloat3		*dstTangents = reinterpret_cast<CFloat3*>(m_Data->m_Positions->m_FloatTangents.RawDataPointer());
			const FInterpCurvePoint<FVector>	*srcPoints = positions.Points.GetData();
			for (u32 iPoint = 0; iPoint < keyCount; ++iPoint)
			{
				const CFloat3	pos = ToPk(srcPoints->OutVal) * invGlobalScale;
				const CFloat3	srcArriveTan = ToPk(srcPoints->ArriveTangent) * invGlobalScale;
				const CFloat3	srcLeaveTan = ToPk(srcPoints->LeaveTangent) * invGlobalScale;

				*dstPos++ = pos;
				*dstTangents++ = srcArriveTan;
				*dstTangents++ = srcLeaveTan;
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
		if (Properties.bScale)
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
		if (Properties.bRotate)
		{
#if WITH_EDITOR
			if (!Properties.bEditorRebuildEachFrame)
#endif // WITH_EDITOR
				UE_LOG(LogPopcornFXAttributeSampler, Warning, TEXT("Rotations disabled for the AnimTrack fast sampler. Uncheck \"Fast sampler\" to enable exact rotations"));
		}

		// (Re)setup the descriptor
		m_Data->m_DescFast->m_Tracks.Clear();
		const PopcornFX::CGuid	id = m_Data->m_DescFast->m_Tracks.PushBack(PopcornFX::CParticleSamplerDescriptor_AnimTrack_Default::SPathDefinition(m_Data->m_Positions, null, m_Data->m_Scales, splineLength));
		PK_ASSERT(id.Valid());
		return id.Valid();
	}
	if (m_Data->m_Desc == null)
		return false; // Requires effect restart

	// Exact sampler
	u32	transformFlags = 0;
	if (Properties.bTranslate)
		transformFlags |= PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Translate;
	if (Properties.bRotate)
		transformFlags |= PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Rotate;
	if (Properties.bScale)
		transformFlags |= PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Scale;
	return m_Data->m_Desc->Setup(splineComponent, transformFlags);
}

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerAnimTrack::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	check(m_Data != null);
	const PopcornFX::CResourceDescriptor_AnimTrack	*defaultAnimTrackSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_AnimTrack>(defaultSampler);
	if (!PK_VERIFY(defaultAnimTrackSampler != null))
		return null;
	if (Properties.bFastSampler)
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
	if (Properties.bFastSampler)
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

	if (!Properties.bTranslate && !Properties.bRotate && !Properties.bScale)
		return;
	if (!m_Data->m_CurrentSplineComponent.IsValid())
		m_Data->m_CurrentSplineComponent = ResolveSplineComponent(false);

#if WITH_EDITOR
	if (Properties.bEditorRebuildEachFrame)
	{
		if (!RebuildCurvesIFN())
			return;
	}
#endif // WITH_EDITOR

	if (!m_Data->m_CurrentSplineComponent.IsValid() &&
		(Properties.Transforms == EPopcornFXSplineTransforms::SplineComponentRelativeTr ||
			Properties.Transforms == EPopcornFXSplineTransforms::SplineComponentWorldTr))
	{
		// No transforms update, keep previously created transforms, if any
		return;
	}

	switch (Properties.Transforms)
	{
		case	EPopcornFXSplineTransforms::SplineComponentRelativeTr:
			if (Properties.bScale || Properties.bFastSampler)
				m_TrackTransforms = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetRelativeTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetRelativeTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSplineTransforms::SplineComponentWorldTr:
			if (Properties.bScale || Properties.bFastSampler)
				m_TrackTransforms = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetComponentTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetComponentTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSplineTransforms::AttrSamplerRelativeTr:
			if (Properties.bScale || Properties.bFastSampler)
				m_TrackTransforms = (FMatrix44f)GetRelativeTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)GetRelativeTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSplineTransforms::AttrSamplerWorldTr:
			if (Properties.bScale || Properties.bFastSampler)
				m_TrackTransforms = (FMatrix44f)GetComponentTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)GetComponentTransform().ToMatrixNoScale();
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			break;
	}

	const float	invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();

	if (!Properties.bScale && !Properties.bFastSampler)
		m_TrackTransforms = m_TrackTransformsUnscaled;

	m_TrackTransforms.M[3][0] *= invGlobalScale;
	m_TrackTransforms.M[3][1] *= invGlobalScale;
	m_TrackTransforms.M[3][2] *= invGlobalScale;
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
