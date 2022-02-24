//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_PlayPopcornFXEffect.generated.h"

/**
 *	Basic effect play animation notify
 */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Play PopcornFX Effect"))
class UAnimNotify_PlayPopcornFXEffect : public UAnimNotify
{
	GENERATED_BODY()
public:
	UAnimNotify_PlayPopcornFXEffect();

	virtual FString		GetNotifyName_Implementation() const override;
	virtual void		Notify(USkeletalMeshComponent *meshComp, UAnimSequenceBase *animation) override;

	// PopcornFX Effect to Spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(DisplayName="Effect"))
	class UPopcornFXEffect	*Effect;

	// PopcornFX Scene name you want to spawn the effect into
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify")
	FName					SceneName;

	// Bone name to spawn from
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify")
	FName					SocketName;

	// Location offset from the socket
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify")
	FVector					LocalOffset;

	// Rotation offset from socket
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify")
	FRotator				LocalRotation;

	// Should attach to the bone/socket
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify")
	uint32					bAttached : 1;
};
