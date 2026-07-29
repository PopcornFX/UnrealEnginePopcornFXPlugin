//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"
#include "PopcornFXSDK.h"

#if WITH_EDITOR
#	include "ThumbnailRendering/TextureThumbnailRenderer.h"
#	include "Editor/PopcornFXStyle.h"
#	include "Engine/Texture2D.h"
#endif

#include "Engine/DataAsset.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

#include "PopcornFXAttributeSampler.generated.h"

FWD_PK_API_BEGIN
class	CParticleSamplerDescriptor;
class	CParticleAttributeSamplerDeclaration;
class	CResourceDescriptor;
class	CMeshSurfaceSamplerStructuresRandom;
class	CMeshVolumeSamplerStructuresRandom;
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

class	CParticleScene;
class   UPopcornFXEffect;
class   UPopcornFXEmitterComponent;
struct	FPopcornFXSamplerDesc;

static const FColor		kSamplerShapesDebugColor = FLinearColor(0.1f, 0.3f, 0.15f, 1.f).ToFColor(false);
static const FColor		kSamplerShapesDebugColorSelected = FLinearColor(0.2f, 0.5f, 0.75f, 1.f).ToFColor(false);
static const int32		kSamplerShapesDebugSegmentCount = 32;

UENUM()
namespace EPopcornFXAttributeSamplerComponentType
{
	enum	Type
	{
		Shape,
		SkinnedMesh,
		Image,
		Grid,
		Curve,
		AnimTrack,
		VectorField,
		Text,
	};
}

UENUM()
namespace EPopcornFXAttributeSamplerType
{
	enum	Type
	{
		None = 0,
		Shape,
		Image,
		Grid,
		Curve,
		AnimTrack,
		VectorField,
		Text,
	};
}
enum { EPopcornFXAttributeSamplerType_Max = EPopcornFXAttributeSamplerType::Text + 1 };

/** Collection of samplers */
USTRUCT()
struct POPCORNFX_API FEmitterSamplers
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FString>	m_SamplerNames;
};

/** Base struct for UE attribute sampler properties */
USTRUCT()
struct POPCORNFX_API FPopcornFXAttributeSamplerProperties
{
	GENERATED_USTRUCT_BODY()

	virtual ~FPopcornFXAttributeSamplerProperties() {}

	//virtual void				CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other);
	/** Checks if properties set by the user are valid. For example, a Curve attribute sampler needs a Curve asset to be valid. */
	virtual bool				ArePropertiesSupported(UPopcornFXEmitterComponent *emitter, const FString &samplerName) { return true; }
	/** Checks if properties set by the user are compatible with the emitter using it. For example, if an effect uses a 2D grid and the user sets a 3D grid, it's not compatible */
	virtual bool				ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const FString &samplerName, const PopcornFX::CResourceDescriptor *defaultSampler) { return true; }
	
#if WITH_EDITORONLY_DATA

	/** Emitter components that are using these properties for one or more of their samplers */
	UPROPERTY()
	TMap<TSoftObjectPtr<UPopcornFXEmitterComponent>, FEmitterSamplers>	m_EmitterSamplersUsingThis;

	/** Properties or combinations that are unsupported, i.e. we can't build a proper sampler descriptor with them in UE. Key = property name, value = error message */
	UPROPERTY()
	TMap<FString, FString>								m_UnsupportedProperties;

#endif
	UPROPERTY()
	TEnumAsByte<EPopcornFXAttributeSamplerType::Type>	m_SamplerType;

#if WITH_EDITOR
	/**
		Setup default properties of this sampler according to the given effect. Only called on inline samplers.
		If updateUnlocked is false, only "locked" properties will be updated. These are the values
		that depend on the source effect on inline samplers and that cannot be changed there.
		If true, every property will be updated, even the ones editable.
	*/
	virtual void				SetupDefaults(const PopcornFX::CParticleAttributeSamplerDeclaration *const decl, bool updateUnlockedValues = false) {}
#endif

	FPopcornFXAttributeSamplerProperties()
	:	m_SamplerType(EPopcornFXAttributeSamplerType::None)
	{ }

	FPopcornFXAttributeSamplerProperties(EPopcornFXAttributeSamplerType::Type SamplerType)
	:	m_SamplerType(SamplerType)
	{ }
};

#define UE_LOG_UNSUPPORTED_SAMPLER(Category, LogLevel, SamplerType, Sampler, Emitter, ErrorString) \
	UE_LOG(Category, LogLevel, \
	TEXT("Can't build " #SamplerType " attribute sampler '%s' for emitter '%s' (effect '%s'):\n%s"), \
	Sampler, *Emitter->GetName(), *Emitter->Effect->GetName(), *ErrorString);

#define UE_LOG_INCOMPATIBLE_SAMPLER(Category, LogLevel, SamplerType, Sampler, Emitter, ErrorString) \
	UE_LOG(Category, LogLevel, \
	TEXT("Can't build " #SamplerType " attribute sampler '%s' for emitter '%s' (effect '%s') in actor '%s':\n%s"), \
	Sampler, *Emitter->GetName(), *Emitter->Effect->GetName(), \
	Emitter->GetAttachmentRootActor() ? *Emitter->GetAttachmentRootActor()->GetName() : TEXT("null"), \
	*ErrorString);

#define UE_LOG_WARNING_SAMPLER(Category, LogLevel, SamplerType, Sampler, Emitter, WarningString) \
	UE_LOG(Category, LogLevel, \
	TEXT("When building " #SamplerType " attribute sampler '%s' for emitter '%s' (effect '%s') in actor '%s':\n%s"), \
	Sampler, *Emitter->GetName(), *Emitter->Effect->GetName(), \
	Emitter->GetAttachmentRootActor() ? *Emitter->GetAttachmentRootActor()->GetName() : TEXT("null"), \
	*WarningString);

/**
	Base struct for UE attribute samplers
	They can be created in any actor and referenced as "external" samplers in emitters that want to use them
	They are also created inside effect assets (UPopcornFXEffect::DefaultSamplers)
	and emitters (UPopcornFXEmitterComponent::Samplers)	to allow inline editing
*/
USTRUCT(BlueprintType)
struct POPCORNFX_API FPopcornFXAttributeSampler
{
	GENERATED_USTRUCT_BODY()

public:
	FPopcornFXAttributeSampler()
	:	m_SamplerType(EPopcornFXAttributeSamplerType::Type::None)
	{ }
	FPopcornFXAttributeSampler(EPopcornFXAttributeSamplerType::Type type)
	:	m_SamplerType(type)
	{ }

	virtual ~FPopcornFXAttributeSampler() {}

	EPopcornFXAttributeSamplerType::Type				SamplerType() const { return m_SamplerType; }
	FString												SamplerName() const { return m_SamplerName; }
	void												SetType(const EPopcornFXAttributeSamplerType::Type &newType) { m_SamplerType = newType; }
	void												SetName(const FString &newName) { m_SamplerName = newName; }

	// PopcornFX Internal
	PopcornFX::CParticleSamplerDescriptor				*_AttribSampler_SetupSampler(UPopcornFXEmitterComponent *emitter, const FString &samplerName, FPopcornFXAttributeSamplerProperties *properties, const PopcornFX::CResourceDescriptor *defaultSampler);
	virtual PopcornFX::CParticleSamplerDescriptor		*_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, const FPopcornFXAttributeSamplerProperties *properties, const PopcornFX::CResourceDescriptor *defaultSampler) { return nullptr; }
	virtual void										_AttribSampler_PreUpdate(UPopcornFXEmitterComponent *owner, float deltaTime) { return; }

	virtual const FPopcornFXAttributeSamplerProperties	*GetProperties() const { return nullptr; }
	virtual void										CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) {}
	/** What to do when we're changing this samplers properties to new ones */
	virtual void										RefreshFromProperties(const FPopcornFXAttributeSamplerProperties *properties) {}
	/** Reimplements BeginDestroy for samplers */
	virtual void										BeginDestroy() {}

#if WITH_EDITOR
	virtual void										_AttribSampler_IndirectSelectedThisTick() {}
	/** Reimplements PostEditChangeProperty for samplers. UI customization will call it */
	virtual void										PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) {}
#endif

	/**
	If true, this sampler belongs to an UPopcornFXEmitterComponent::AttributeList or an UPopcornFXEffect::DefaultAttributeList
	Some of its properties will depend on the emitter's effect.
	If not, it's a standalone sampler, properties can be modified freely.
	*/
	bool												bIsInline = true;

	bool												m_NeedUpdate = false;

protected:
	EPopcornFXAttributeSamplerType::Type				m_SamplerType;
	UPROPERTY()
	FString												m_SamplerName;
};

UCLASS()
class POPCORNFX_API UPopcornFXAttributeSamplerAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual const FPopcornFXAttributeSamplerProperties	*GetProperties() const { return nullptr; }
	virtual FPopcornFXAttributeSamplerProperties		*GetProperties() { return nullptr; }
};