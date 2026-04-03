//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "ComponentAssetBroker.h"

class POPCORNFX_API FPopcornFXEmitterComponentAssetBroker : public IComponentAssetBroker
{
public:
	static UClass	*ComponentStaticClass();
	UClass* GetSupportedAssetClass() override;
	virtual bool AssignAssetToComponent(UActorComponent* InComponent, UObject* InAsset) override;
	virtual UObject* GetAssetFromComponent(UActorComponent* InComponent) override;
};

#endif // WITH_EDITOR
