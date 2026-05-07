//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXAssetBroker.h"

#include "PopcornFXEmitterComponent.h"
#include "Assets/PopcornFXEffect.h"

#if WITH_EDITOR

UClass		*FPopcornFXEmitterComponentAssetBroker::ComponentStaticClass()
{
	return UPopcornFXEmitterComponent::StaticClass();
}

UClass		*FPopcornFXEmitterComponentAssetBroker::GetSupportedAssetClass()
{
	return UPopcornFXEffect::StaticClass();
}

bool		FPopcornFXEmitterComponentAssetBroker::AssignAssetToComponent(UActorComponent* InComponent, UObject* InAsset)
{
	UPopcornFXEmitterComponent* comp = Cast<UPopcornFXEmitterComponent>(InComponent);
	if (comp != nullptr)
	{
		UPopcornFXEffect	*asset = Cast<UPopcornFXEffect>(InAsset);
		if ((asset != nullptr) || (asset == nullptr))
		{
			comp->Effect = asset;
			return true;
		}
	}
	return false;
}

UObject		*FPopcornFXEmitterComponentAssetBroker::GetAssetFromComponent(UActorComponent* InComponent)
{
	UPopcornFXEmitterComponent* comp = Cast<UPopcornFXEmitterComponent>(InComponent);
	if (comp != nullptr)
	{
		if ((comp->Effect != nullptr) && (comp->Effect->IsAsset()))
			return comp->Effect;
	}
	return nullptr;
}

#endif // WITH_EDITOR
