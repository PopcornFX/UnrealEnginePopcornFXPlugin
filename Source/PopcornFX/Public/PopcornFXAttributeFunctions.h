//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "PopcornFXAttributeFunctions.generated.h"

class UPopcornFXEmitterComponent;

UCLASS()
class POPCORNFX_API UPopcornFXAttributeFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:

	/// Returns the index of the Attribute
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PopcornFX|Attributes", meta=(DefaultToSelf="InSelf"))
	static int32			FindAttributeIndex(const UPopcornFXEmitterComponent *InSelf, FName InAttributeName);

	/// Reset Attribute values to default
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PopcornFX|Attributes", meta=(DefaultToSelf="InSelf"))
	static bool				ResetToDefaultValue(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloat(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloat(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float &OutValue, bool InApplyGlobalScale = false);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloat2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float InValueX, float InValueY, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsVector2D(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FVector2D InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloat2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float &OutValueX, float &OutValueY, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsVector2D(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FVector2D &OutValue, bool InApplyGlobalScale = false);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloat3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float InValueX, float InValueY, float InValueZ, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsVector(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FVector InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloat3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float &OutValueX, float &OutValueY, float &OutValueZ, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsVector(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FVector &OutValue, bool InApplyGlobalScale = false);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloat4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float InValueX, float InValueY, float InValueZ, float InValueW, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsLinearColor(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FLinearColor InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloat4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float &OutValueX, float &OutValueY, float &OutValueZ, float &OutValueW, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsLinearColor(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FLinearColor &OutValue, bool InApplyGlobalScale = false);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsRotator(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FRotator InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsRotator(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FRotator &OutValue, bool InApplyGlobalScale = false);


	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsInt(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 InValue);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsInt(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 &OutValue);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsInt2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 InValueX, int32 InValueY);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsInt2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsInt3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 InValueX, int32 InValueY, int32 InValueZ);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsInt3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsInt4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 InValueX, int32 InValueY, int32 InValueZ, int32 InValueW);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsInt4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ, int32 &OutValueW);


	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBool(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool InValue);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBool(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool &OutValue);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBool2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool InValueX, bool InValueY);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBool2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBool3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool InValueX, bool InValueY, bool InValueZ);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBool3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY, bool &OutValueZ);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBool4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool InValueX, bool InValueY, bool InValueZ, bool InValueW);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="InSelf"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBool4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY, bool &OutValueZ, bool &OutValueW);
};
