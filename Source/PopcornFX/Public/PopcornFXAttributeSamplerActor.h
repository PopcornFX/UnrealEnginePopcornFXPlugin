//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "PopcornFXAttributeSampler.h"

#include "GameFramework/Actor.h"

#include "PopcornFXAttributeSamplerActor.generated.h"

/// Abstract Actor containing a PopcornFXAttributeSampler Component
UCLASS(Abstract)
class POPCORNFX_API APopcornFXAttributeSamplerActor : public AActor
{
	GENERATED_UCLASS_BODY()
public:
	UPROPERTY(Category="PopcornFX AttributeSampler", Instanced, VisibleAnywhere, BlueprintReadOnly)
	class UPopcornFXAttributeSampler		*Sampler;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UBillboardComponent				*SpriteComponent;
#endif // WITH_EDITORONLY_DATA

	//overrides
	virtual void		PostLoad() override;
	virtual void		PostActorCreated() override;

#if WITH_EDITOR
	//overrides
	virtual void		PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
	virtual void		PostRegisterAllComponents() override;
#endif

protected:
	void				_CtorRootSamplerComponent(const FObjectInitializer &PCIP, EPopcornFXAttributeSamplerComponentType::Type type);
	void				ReloadSprite();

	EPopcornFXAttributeSamplerComponentType::Type	m_SamplerComponentType;
	bool											m_IsValidSpecializedActor = false;
};

/** Can override an Attribute Sampler **Shape** by a **UStaticMesh**. */
UCLASS() class POPCORNFX_API APopcornFXAttributeSamplerShapeActor : public APopcornFXAttributeSamplerActor { GENERATED_UCLASS_BODY() };

/** Can override an Attribute Sampler **Shape** in a **USkeletalMesh**. */
UCLASS() class POPCORNFX_API APopcornFXAttributeSamplerSkinnedMeshActor : public APopcornFXAttributeSamplerActor { GENERATED_UCLASS_BODY() };

/** Can override an Attribute Sampler **Image** by a **UTexture**. */
UCLASS() class POPCORNFX_API APopcornFXAttributeSamplerImageActor : public APopcornFXAttributeSamplerActor { GENERATED_UCLASS_BODY() };

/** Can override an Attribute Sampler **Grid** by a **UTexture**. */
UCLASS() class POPCORNFX_API APopcornFXAttributeSamplerGridActor : public APopcornFXAttributeSamplerActor { GENERATED_UCLASS_BODY() };

/** Can override an Attribute Sampler **Curve** by a **UCurve...**. */
UCLASS() class POPCORNFX_API APopcornFXAttributeSamplerCurveActor : public APopcornFXAttributeSamplerActor { GENERATED_UCLASS_BODY() };

/** Can override an Attribute Sampler **AnimTrack** by a **USplineComponent...**. */
UCLASS() class POPCORNFX_API APopcornFXAttributeSamplerAnimTrackActor : public APopcornFXAttributeSamplerActor { GENERATED_UCLASS_BODY() };

/** Can override an Attribute Sampler **Text** by a **FString**. */
UCLASS() class POPCORNFX_API APopcornFXAttributeSamplerTextActor : public APopcornFXAttributeSamplerActor { GENERATED_UCLASS_BODY() };

/** Can override an Attribute Sampler **Turbulence** by a **UVectorFieldStatic**. */
UCLASS() class POPCORNFX_API APopcornFXAttributeSamplerVectorFieldActor : public APopcornFXAttributeSamplerActor { GENERATED_UCLASS_BODY() };
