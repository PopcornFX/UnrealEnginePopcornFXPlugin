//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSampler.h"

#include "PopcornFXAttributeList.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXAttributeSamplerActor.h"
#include "PopcornFXAttributeSamplerShape.h"
#include "PopcornFXAttributeSamplerImage.h"
#include "PopcornFXAttributeSamplerGrid.h"
#include "PopcornFXAttributeSamplerText.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXAttributeSamplerCurveDynamic.h"
#include "PopcornFXAttributeSamplerAnimTrack.h"
#include "PopcornFXAttributeSamplerVectorField.h"

#include "PopcornFXSDK.h"

#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"

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
				: ID_Effects(TEXT("Effects")) // do not change, recognized by the engine
				, NAME_Effects(NSLOCTEXT("SpriteCategory", "Effects", "Effects")) // do not change, recognized by the engine
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
		// Baad, should be replaced by the actual Actor icon!
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

#undef LOCTEXT_NAMESPACE
