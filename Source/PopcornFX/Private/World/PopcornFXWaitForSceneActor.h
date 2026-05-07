//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "GameFramework/Actor.h"

#include "PopcornFXWaitForSceneActor.generated.h"

class UPopcornFXEmitterComponent;
class APopcornFXSceneActor;
class UPopcornFXSceneComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FPopcornFXOnSceneLoadedSignature, APopcornFXSceneActor* /*sceneActor*/);

UCLASS(HideCategories=(Input, Collision, Replication, Materials))
class POPCORNFX_API APopcornFXWaitForScene : public AActor
{
	GENERATED_UCLASS_BODY()
public:
	//UPROPERTY(Transient)
	FName									SceneName;

	//UPROPERTY(Transient)
	FPopcornFXOnSceneLoadedSignature		OnSceneLoadedDelegates;

	static APopcornFXWaitForScene			*GetIFP(UWorld *world, FName sceneName);
	static APopcornFXWaitForScene			*GetOrCreate(UWorld *world, FName sceneName);

	static void		NotifySceneLoaded(APopcornFXSceneActor *sceneActor);
	static void		NotifySceneLoaded(UPopcornFXSceneComponent *sceneComp);

private:
	void		_NotifySceneLoaded(APopcornFXSceneActor *scene);

};
