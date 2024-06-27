//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXEmitterFactory.h"

#include "Runtime/Launch/Resources/Version.h"
#if (ENGINE_MAJOR_VERSION == 5)
#	include "AssetRegistry/AssetData.h"
#else
#	include "AssetData.h"
#endif // (ENGINE_MAJOR_VERSION == 5)

#include "Assets/PopcornFXEffect.h"
#include "PopcornFXEmitter.h"
//#include "PopcornFXEmitterComponent.h"
#include "PopcornFXSceneActor.h"

#include "Editor/EditorEngine.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXEmitterFactory"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXEmitterFactory, Log, All);

//----------------------------------------------------------------------------

UPopcornFXEmitterFactory::UPopcornFXEmitterFactory(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	DisplayName = LOCTEXT("PopcornFXEmitterDisplayName", "PopcornFX Emitter");
	NewActorClass = APopcornFXEmitter::StaticClass();
}

bool	UPopcornFXEmitterFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (!AssetData.IsValid() || !AssetData.GetClass()->IsChildOf(UPopcornFXEffect::StaticClass()))
	{
		OutErrorMsg = NSLOCTEXT("CanCreateActor", "NoPopcornFXEffect", "A valid PopcornFX Effect must be specified.");
		return false;
	}

	return true;
}

void	UPopcornFXEmitterFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	UPopcornFXEffect* effect = CastChecked<UPopcornFXEffect>(Asset);

	FActorLabelUtilities::SetActorLabelUnique(NewActor, effect->GetName());

	// Change properties
	APopcornFXEmitter* emitter = CastChecked<APopcornFXEmitter>(NewActor);
	emitter->SetEffect(effect);
#if WITH_EDITOR
	emitter->EditorSpawnSceneIFN();
#endif // WITH_EDITOR
}

UObject	*UPopcornFXEmitterFactory::GetAssetFromActorInstance(AActor* Instance)
{
	check(Instance->IsA(NewActorClass));
	APopcornFXEmitter* emitter = CastChecked<APopcornFXEmitter>(Instance);
	return emitter->GetEffect();
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
