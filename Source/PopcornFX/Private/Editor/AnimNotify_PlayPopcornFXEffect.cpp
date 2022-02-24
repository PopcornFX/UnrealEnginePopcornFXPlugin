//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "AnimNotify_PlayPopcornFXEffect.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "PopcornFXSDK.h"
#include "PopcornFXFunctions.h"
#include "Assets/PopcornFXEffect.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAnimNotify, Log, All);

//----------------------------------------------------------------------------

UAnimNotify_PlayPopcornFXEffect::UAnimNotify_PlayPopcornFXEffect()
:	Super()
{
	bAttached = false;
	SceneName = "PopcornFX_DefaultScene";

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(255, 200, 200, 255);
#endif // WITH_EDITORONLY_DATA
}

//----------------------------------------------------------------------------

void	UAnimNotify_PlayPopcornFXEffect::Notify(class USkeletalMeshComponent *meshComp, class UAnimSequenceBase *animation)
{
	Super::Notify(meshComp, animation);
	if (Effect == null)
	{
		UE_LOG(LogPopcornFXAnimNotify, Warning, TEXT("PopcornFX Notify: Particle system is null for notify '%s' in anim: '%s'"), *GetNotifyName(), *GetPathNameSafe(animation));
		return;
	}
	if (bAttached)
	{
		UPopcornFXFunctions::SpawnEmitterAttached(Effect, meshComp, SceneName, SocketName, LocalOffset, LocalRotation);
	}
	else
	{
		const FTransform	socketTransform = meshComp->GetSocketTransform(SocketName, ERelativeTransformSpace::RTS_World);
		FVector				effectOffset = socketTransform.TransformPosition(LocalOffset);
		FRotator			effectRotation = meshComp->GetSocketRotation(SocketName);
		effectRotation += LocalRotation;
		UPopcornFXFunctions::SpawnEmitterAtLocation(meshComp, Effect, SceneName, effectOffset, effectRotation, true, true);
	}
}

//----------------------------------------------------------------------------

FString	UAnimNotify_PlayPopcornFXEffect::GetNotifyName_Implementation() const
{
	if (Effect != null)
		return Effect->GetName();
	else
		return Super::GetNotifyName_Implementation();
}

//----------------------------------------------------------------------------
