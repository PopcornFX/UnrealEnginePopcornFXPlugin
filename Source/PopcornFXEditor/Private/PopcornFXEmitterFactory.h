//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "ActorFactories/ActorFactory.h"
#include "PopcornFXEmitterFactory.generated.h"

UCLASS(MinimalAPI, Config=Editor)
class UPopcornFXEmitterFactory : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	// override UActorFactory
	virtual bool			CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void			PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject*		GetAssetFromActorInstance(AActor* ActorInstance) override;
};
