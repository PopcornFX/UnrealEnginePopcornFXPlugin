//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "PopcornFXAttributeSamplersFunctions.generated.h"

class UPopcornFXEmitterComponent;

UCLASS()
class POPCORNFX_API UPopcornFXAttributeSamplersFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	/**
	* Sets an attribute sampler on an emitter. Needs to be called before starting the emitter.
	* Will resolve the attribute sampler from the input actor:
	* Looks for a UPopcornFXAttributeSampler named ComponentName, fallbacks on the input actor's RootComponent otherwise
	*/
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute Sampler", DefaultToSelf="InSelf"), Category="PopcornFX|Attribute Samplers")
	static bool								SetAttributeSampler(UPopcornFXEmitterComponent *InSelf, FName InAttributeSamplerName, AActor* InActor, FName InComponentName);
};
