//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
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
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PopcornFX|Attributes", meta=(DefaultToSelf="Emitter"))
	static int32			FindAttributeIndex(const UPopcornFXEmitterComponent *Emitter, FString InAttributeName);

	/// Reset Attribute values to default
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PopcornFX|Attributes", meta=(DefaultToSelf="Emitter"))
	static bool				ResetToDefaultValue(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloat(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloat(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float &OutValue, bool InApplyGlobalScale = false);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloat2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float InValueX, float InValueY, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsVector2D(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FVector2D InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloat2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float &OutValueX, float &OutValueY, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsVector2D(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FVector2D &OutValue, bool InApplyGlobalScale = false);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloat3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float InValueX, float InValueY, float InValueZ, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsVector(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FVector InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloat3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float &OutValueX, float &OutValueY, float &OutValueZ, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsVector(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FVector &OutValue, bool InApplyGlobalScale = false);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloat4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float InValueX, float InValueY, float InValueZ, float InValueW, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsLinearColor(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FLinearColor InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloat4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float &OutValueX, float &OutValueY, float &OutValueZ, float &OutValueW, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsLinearColor(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FLinearColor &OutValue, bool InApplyGlobalScale = false);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsRotator(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FRotator InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsRotator(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FRotator &OutValue, bool InApplyGlobalScale = false);


	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsInt(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 InValue);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsInt(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 &OutValue);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsInt2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 InValueX, int32 InValueY);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsInt2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsInt3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 InValueX, int32 InValueY, int32 InValueZ);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsInt3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsInt4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 InValueX, int32 InValueY, int32 InValueZ, int32 InValueW);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsInt4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ, int32 &OutValueW);


	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBool(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool InValue);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBool(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool &OutValue);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBool2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool InValueX, bool InValueY);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBool2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBool3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool InValueX, bool InValueY, bool InValueZ);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBool3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY, bool &OutValueZ);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBool4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool InValueX, bool InValueY, bool InValueZ, bool InValueW);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBool4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY, bool &OutValueZ, bool &OutValueW);

	// Same functions but finding attribute by name instead of by index

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloatByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloatByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float &OutValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloat2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float InValueX, float InValueY, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsVector2DByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FVector2D InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloat2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float &OutValueX, float &OutValueY, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsVector2DByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FVector2D &OutValue, bool InApplyGlobalScale = false);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloat3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float InValueX, float InValueY, float InValueZ, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsVectorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FVector InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloat3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float &OutValueX, float &OutValueY, float &OutValueZ, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsVectorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FVector &OutValue, bool InApplyGlobalScale = false);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsFloat4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float InValueX, float InValueY, float InValueZ, float InValueW, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsLinearColorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FLinearColor InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsFloat4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float &OutValueX, float &OutValueY, float &OutValueZ, float &OutValueW, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsLinearColorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FLinearColor &OutValue, bool InApplyGlobalScale = false);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsRotatorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FRotator InValue, bool InApplyGlobalScale = false);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsRotatorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FRotator &OutValue, bool InApplyGlobalScale = false);


	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsIntByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 InValue);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsIntByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 &OutValue);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsInt2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 InValueX, int32 InValueY);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsInt2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 &OutValueX, int32 &OutValueY);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsInt3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 InValueX, int32 InValueY, int32 InValueZ);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsInt3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsInt4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 InValueX, int32 InValueY, int32 InValueZ, int32 InValueW);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsInt4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ, int32 &OutValueW);


	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBoolByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool InValue);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBoolByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool &OutValue);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBool2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool InValueX, bool InValueY);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBool2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool &OutValueX, bool &OutValueY);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBool3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool InValueX, bool InValueY, bool InValueZ);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBool3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool &OutValueX, bool &OutValueY, bool &OutValueZ);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				SetAttributeAsBool4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool InValueX, bool InValueY, bool InValueZ, bool InValueW);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Attribute", BlueprintInternalUseOnly="true", DefaultToSelf="Emitter"), Category="PopcornFX|Attributes")
	static bool				GetAttributeAsBool4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool &OutValueX, bool &OutValueY, bool &OutValueZ, bool &OutValueW);

};