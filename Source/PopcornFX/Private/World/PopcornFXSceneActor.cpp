//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXSceneActor.h"

#include "PopcornFXSceneComponent.h"
#include "PopcornFXSDK.h"

APopcornFXSceneActor::APopcornFXSceneActor(const class FObjectInitializer& PCIP)
:	Super(PCIP)
{
	PopcornFXSceneComponent = CreateDefaultSubobject<UPopcornFXSceneComponent>(TEXT("PopcornFXSceneComponent0"));
	RootComponent = PopcornFXSceneComponent;
	SetFlags(RF_Transactional); // Allow this actor's deletion
}

CParticleScene		*APopcornFXSceneActor::ParticleScene()
{
	if (PK_VERIFY(PopcornFXSceneComponent != null))
	{
		return PopcornFXSceneComponent->ParticleScene();
	}
	return null;
}

FName	APopcornFXSceneActor::GetSceneName() const
{
	return PopcornFXSceneComponent != null ? PopcornFXSceneComponent->SceneName : FName();
}
