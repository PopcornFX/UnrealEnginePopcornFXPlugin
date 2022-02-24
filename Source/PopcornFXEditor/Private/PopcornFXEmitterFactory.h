//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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
	virtual void			PostCreateBlueprint(UObject* Asset, AActor* CDO) override;
	virtual UObject*		GetAssetFromActorInstance(AActor* ActorInstance) override;
};
