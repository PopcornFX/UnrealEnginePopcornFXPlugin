//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "GameFramework/Actor.h"

#include "PopcornFXEmitter.generated.h"

class	IPopcornFXPlugin;

/** Fires when a PopcornFX Emitter is started
* @param EmitterComponent - The emitter component.
* @param Location - Location.
* @param Rotation - Rotation.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPopcornFXEmitterStartSignature, UPopcornFXEmitterComponent*, EmitterComponent, FVector, Location, FVector, Rotation);

/** Fires when a PopcornFX Emitter is stop
* Note: could happen after Terminate
* @param EmitterComponent - The emitter component.
* @param Location - Location.
* @param Rotation - Rotation.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPopcornFXEmitterStopSignature, UPopcornFXEmitterComponent*, EmitterComponent, FVector, Location, FVector, Rotation);

/** Fires when a PopcornFX Emitter is terminated
* @param EmitterComponent - The emitter component.
* @param Location - Location.
* @param Rotation - Rotation.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPopcornFXEmitterTerminateSignature, UPopcornFXEmitterComponent*, EmitterComponent, FVector, Location, FVector, Rotation);

/** Fires when a PopcornFX Emitter is deferred killed
* @param EmitterComponent - The emitter component.
* @param Location - Location.
* @param Rotation - Rotation.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPopcornFXEmitterKillParticlesSignature, UPopcornFXEmitterComponent*, EmitterComponent, FVector, Location, FVector, Rotation);

UCLASS(HideCategories=(Input, Collision, Replication))
class POPCORNFX_API APopcornFXEmitter : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(Category="PopcornFX Emitter", Instanced, VisibleAnywhere, BlueprintReadOnly)
	class UPopcornFXEmitterComponent		*PopcornFXEmitterComponent;

	UPROPERTY(Category="PopcornFX Emitter", BlueprintAssignable)
	FPopcornFXEmitterStartSignature			OnEmitterStart;

	UPROPERTY(Category="PopcornFX Emitter", BlueprintAssignable)
	FPopcornFXEmitterStopSignature			OnEmitterStop;

	UPROPERTY(Category="PopcornFX Emitter", BlueprintAssignable)
	FPopcornFXEmitterTerminateSignature		OnEmitterTerminate;

	UPROPERTY(Category = "PopcornFX Emitter", BlueprintAssignable)
	FPopcornFXEmitterKillParticlesSignature	OnEmitterKillParticles;

	/** Matinee Toggle anim track compatibility */
	UFUNCTION(BlueprintCallable, Category="Rendering", meta=(CallInEditor=true))
	void				OnInterpToggle(bool bEnable);

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UBillboardComponent				*SpriteComponent;
#endif

	bool				SetEffect(class UPopcornFXEffect *effect);
	UPopcornFXEffect	*GetEffect();

#if WITH_EDITOR
	// overrides AActor
	virtual bool		GetReferencedContentObjects(TArray<UObject*>& Objects) const override;
	//virtual void		CheckForErrors() override; // @TODO

	bool				EditorSpawnSceneIFN();
#endif // WITH_EDITOR
};
