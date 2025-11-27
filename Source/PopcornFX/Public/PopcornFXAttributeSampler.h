//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "Components/SceneComponent.h"

#include "PopcornFXAttributeSampler.generated.h"

FWD_PK_API_BEGIN
class	CParticleSamplerDescriptor;
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

DECLARE_MULTICAST_DELEGATE(FPopcornFXSamplerEventSignature);

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
		Turbulence,
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
		Turbulence,
		Text,
	};
}
enum { EPopcornFXAttributeSamplerType_Max = EPopcornFXAttributeSamplerType::Text + 1 };

UENUM()
namespace EPopcornFXAttribSamplerShapeType
{
	enum	Type
	{
		Box = 0,
		Sphere,
		Ellipsoid,
		Cylinder,
		Capsule,
		Cone,
		StaticMesh,
		SkeletalMesh,
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
		Collection,
#endif
	};
}
enum { EPopcornFXAttribSamplerShapeType_Max = EPopcornFXAttribSamplerShapeType::SkeletalMesh + 1 };
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
enum { EPopcornFXAttribSamplerShapeType_Max = EPopcornFXAttribSamplerShapeType::Collection + 1 };
#endif

UENUM()
namespace EPopcornFXMeshSamplingMode
{
	enum	Type
	{
		Uniform = 0,
		Fast,
		Weighted
	};
}

UENUM()
namespace EPopcornFXColorChannel
{
	enum	Type
	{
		Red = 0,
		Green,
		Blue,
		Alpha
	};
}

/** Wrapper of a map of properties names and their error message so we can create an array of it */
USTRUCT()
struct POPCORNFX_API FIncompatibleProperties
{
	GENERATED_USTRUCT_BODY()

	/** Map of properties than are incompatible with an emitter. Key = property name. Value = error message */
	UPROPERTY(VisibleAnywhere)
	TMap<FString, FString>	m_Properties;
};

/** Base struct for UE attribute sampler properties */
USTRUCT()
struct POPCORNFX_API FPopcornFXAttributeSamplerProperties
{
	GENERATED_USTRUCT_BODY()
};

const UClass *GetSamplerClass(EPopcornFXAttributeSamplerType::Type type);

#define UE_LOG_UNSUPPORTED_SAMPLER(Category, LogLevel, SamplerType, Sampler, ErrorString) \
	UE_LOG(Category, LogLevel, \
	TEXT("Can't build " #SamplerType " attribute sampler '%s' in actor '%s'%s:\n\t%s"), \
	*Sampler->GetName(), \
	Sampler->GetAttachmentRootActor() ? *Sampler->GetAttachmentRootActor()->GetName() : TEXT("null"), \
	Sampler->bIsInline ? *FString::Printf(TEXT(" for emitter '%s'"), *Sampler->GetOuter()->GetName()) : TEXT(""), \
	*ErrorString);

#define UE_LOG_INCOMPATIBLE_SAMPLER(Category, LogLevel, SamplerType, Sampler, Emitter, ErrorString) \
	UE_LOG(Category, LogLevel, \
	TEXT("Can't build " #SamplerType " attribute sampler '%s' in actor '%s' for emitter '%s' in actor '%s':\n\t%s for effect '%s'"), \
	*Sampler->GetName(), Sampler->GetAttachmentRootActor() ? *Sampler->GetAttachmentRootActor()->GetName() : TEXT("null"), \
	*Emitter->GetName(), Emitter->GetAttachmentRootActor() ? *Emitter->GetAttachmentRootActor()->GetName() : TEXT("null"), \
	*ErrorString, *Emitter->Effect->GetName());

#define UE_LOG_WARNING_SAMPLER(Category, LogLevel, SamplerType, Sampler, Emitter, WarningString) \
	UE_LOG(Category, LogLevel, \
	TEXT("When building " #SamplerType " attribute sampler '%s' in actor '%s' for emitter '%s' in actor '%s':\n\t%s for effect '%s'"), \
	*Sampler->GetName(), Sampler->GetAttachmentRootActor() ? *Sampler->GetAttachmentRootActor()->GetName() : TEXT("null"), \
	*Emitter->GetName(), Emitter->GetAttachmentRootActor() ? *Emitter->GetAttachmentRootActor()->GetName() : TEXT("null"), \
	*WarningString, *Emitter->Effect->GetName());

/**
	Base class for UE attribute samplers
	They can be created in any actor and referenced as "external" samplers in emitters that want to use them
	They are also created inside effect assets (UPopcornFXEffect::DefaultSamplers)
	and emitters (UPopcornFXEmitterComponent::Samplers)	to allow inline editing
*/
UCLASS(Abstract)
class POPCORNFX_API UPopcornFXAttributeSampler : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:
	static UClass										*SamplerComponentClass(EPopcornFXAttributeSamplerComponentType::Type type);
	EPopcornFXAttributeSamplerType::Type				SamplerType() const { return m_SamplerType; }

	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor		*_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) { return nullptr; }
	virtual void										_AttribSampler_PreUpdate(float deltaTime) { return; }

	virtual const FPopcornFXAttributeSamplerProperties	*GetProperties() const { return nullptr; }
	virtual void										CopyPropertiesFrom(const UPopcornFXAttributeSampler *other);
	/** Checks if properties and their combinations are supported */
	virtual bool										ArePropertiesSupported() { return true; }

#if WITH_EDITOR
	virtual void										_AttribSampler_IndirectSelectedThisTick() {}
	/**
		Setup default properties of this sampler according to the given effect. Only called on inline samplers.
		If updateUnlocked is false, only "locked" properties will be updated. These are the values 
		that depend on the source effect on inline samplers and that cannot be changed there.
		If true, every property will be updated, even the ones editable.
	*/
	virtual void										SetupDefaults(UPopcornFXEffect *effect, const uint32 samplerIdx, bool updateUnlockedValues = false) { }

	void												PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent) override;
#endif

	/**
	If true, this sampler belongs to an UPopcornFXEmitterComponent::Samplers or an UPopcornFXEffect::DefaultSamplers
	Some of its properties will depend on the emitter's effect.
	If not, it's a standalone sampler, properties can be modified freely.
	*/
	bool												bIsInline = true;

#if WITH_EDITORONLY_DATA

	/** Emitter components that are using this sampler as an external one */
	UPROPERTY(VisibleAnywhere)
	TArray<UPopcornFXEmitterComponent*>					m_EmittersUsingThis;

	/** Properties of this sampler that are unsupported, i.e. we can't build a proper sampler descriptor with them in UE. Key = property name, value = error message */
	UPROPERTY(VisibleAnywhere)
	TMap<FString, FString>								m_UnsupportedProperties;

	/** Properties than are incompatible with an emitter's effect. One list for each emitter using this sampler. Key = emitter, value = properties */
	UPROPERTY(VisibleAnywhere)
	TMap<UPopcornFXEmitterComponent*, FIncompatibleProperties>	m_IncompatibleProperties;

	/** Event broadcasted when a sampler setup becomes valid or invalid */
	FPopcornFXSamplerEventSignature						OnSamplerValidStateChanged;
#endif

protected:
	EPopcornFXAttributeSamplerType::Type				m_SamplerType;
};