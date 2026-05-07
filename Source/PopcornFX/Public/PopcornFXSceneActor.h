//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "GameFramework/Actor.h"

#include "PopcornFXSceneActor.generated.h"

class CParticleScene;

/** Actor container for a PopcornFXSceneComponent. */
UCLASS(HideCategories=(Input, Collision, Replication, Materials))
class POPCORNFX_API APopcornFXSceneActor : public AActor
{
	GENERATED_UCLASS_BODY()

	/** The PopcornFX Scene Component */
	UPROPERTY(Category="PopcornFX Scene", VisibleAnywhere, Instanced, BlueprintReadOnly)
	class UPopcornFXSceneComponent		*PopcornFXSceneComponent;

	CParticleScene		*ParticleScene();

	FName				GetSceneName() const;
};
