//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"
#include "PopcornFXAttributeSampler.h"

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
	UFUNCTION(BlueprintCallable, meta=(DisplayName = "Set Attribute Sampler", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static bool										SetAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName, AActor* InActor, FString InComponentName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Attribute Sampler", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static UPopcornFXAttributeSampler				*GetAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Anim Track Attribute Sampler", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static UPopcornFXAttributeSamplerAnimTrack		*GetAnimTrackAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Curve Attribute Sampler", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static UPopcornFXAttributeSamplerCurve			*GetCurveAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Grid Attribute Sampler", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static UPopcornFXAttributeSamplerGrid			*GetGridAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Image Attribute Sampler", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static UPopcornFXAttributeSamplerImage			*GetImageAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Shape Attribute Sampler", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static UPopcornFXAttributeSamplerShape			*GetShapeAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Text Attribute Sampler", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static UPopcornFXAttributeSamplerText			*GetTextAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get VectorField Attribute Sampler", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static UPopcornFXAttributeSamplerVectorField	*GetVectorFieldAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Attribute Sampler Casted", DefaultToSelf = "InSelf", DeterminesOutputType = "SamplerClass"), Category = "PopcornFX|Attribute Samplers")
	static UPopcornFXAttributeSampler	*GetAttributeSamplerCasted(UPopcornFXEmitterComponent *InSelf, TSubclassOf<UPopcornFXAttributeSampler> SamplerClass, FString InAttributeSamplerName);
};
