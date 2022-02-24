//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"

#include "PopcornFXFunctions.generated.h"

class UPopcornFXEmitterComponent;

UCLASS()
class POPCORNFX_API UPopcornFXFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	/** Returns the version string of the PopcornFX Plugin build */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PopcornFX", meta=(Keywords="popcornfx"))
	static const FString	&PopcornFXPluginVersion();

	/** Returns the version string of the PopcornFX Runtime build */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PopcornFX", meta=(Keywords="popcornfx"))
	static const FString	&PopcornFXRuntimeVersion();

	/** Returns the global scale applied to convert from PopcornFX to Unreal Engine space */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PopcornFX", meta=(Keywords="popcornfx"))
	static float			PopcornFXGlobalScale();

	/** Returns the global scale applied to convert from Unreal Engine to PopcornFX space */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PopcornFX", meta=(Keywords="popcornfx"))
	static float			PopcornFXGlobalScaleInv();

	/** Returns the total particle count alive now
	* Really counts all alive particles across all Particle Scenes still alive (running or not).
	*
	* ie: in the Editor, it will count all particles of the Editor instance of your level
	* and during Play it will ADD particle count of the current Game instance playing (PIE).
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX", meta=(Keywords="popcornfx"))
	static int32			PopcornFXTotalParticleCount();

	/** Should be used after changing a setting at runtime.
	* Especially for SimulationSettings and RenderSettings which needs to be resolved explicitly
	* Will broadcast m_OnSettingsChanged.
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Settings", meta=(Keywords="popcornfx"))
	void					NotifySettingChanged();

	/** Plays the specified PopcornFXEffect at the given location and rotation, fire and forget.
	* /!\ Less performant than CopyAndStartEmitterAtLocation function
	* @param WorldContextObject - Object that we can obtain a world context from
	* @param Effect - Emitter to create
	* @param SceneName - Scene in which spawn the effect
	* @param Location - location to place the effect in world space
	* @param Rotation - rotation to place the effect in world space
	* @param bAutoDestroy - Whether the component will automatically be destroyed when the particle system completes playing or whether it can be reactivated
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static class UPopcornFXEmitterComponent* SpawnEmitterAtLocation(UObject* WorldContextObject, class UPopcornFXEffect* Effect, FName SceneName = TEXT("PopcornFX_DefaultScene"), FVector Location = FVector::ZeroVector, FRotator Rotation = FRotator::ZeroRotator, bool bStartEmitter = true, bool bAutoDestroy = true);

	/** Plays the specified PopcornFXEffect attached to and following the specified component. The system will go away when the effect is complete.
	* /!\ Less performant than CopyAndStartEmitterAttached function
	* @param Effect - Emitter to create
	* @param SceneName - Scene in which spawn the effect
	* @param AttachComponent - Component to attach to.
	* @param AttachPointName - Optional named point within the AttachComponent to spawn the emitter at
	* @param Location - Depending on the value of Location Type this is either a relative offset from the attach component/point or an absolute world position that will be translated to a relative offset
	* @param Rotation - Depending on the value of LocationType this is either a relative offset from the attach component/point or an absolute world rotation that will be translated to a realative offset
	* @param LocationType - Specifies whether Location is a relative offset or an absolute world position
	* @param bAutoDestroy - Whether the component will automatically be destroyed when the particle system completes playing or whether it can be reactivated
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static class UPopcornFXEmitterComponent* SpawnEmitterAttached(class UPopcornFXEffect* Effect, class USceneComponent* AttachToComponent, FName SceneName = TEXT("PopcornFX_DefaultScene"), FName AttachPointName = NAME_None, FVector Location = FVector(ForceInit), FRotator Rotation = FRotator::ZeroRotator, EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bStartEmitter = true, bool bAutoDestroy = true);

	/** Notify PopcornFX that an UObject has changed and must be reloaded.
	* For example: use this to get PopcornFX reload a dynamic UTexture that changed
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX", meta=(Keywords="popcornfx particle asset object", WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static void NotifyObjectChanged(class UObject *object);


	/** Return whether the "virtualPath" CPU Texture is currently in use */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Textures", meta=(DisplayName="Is Texture CPU In Use", Keywords="popcornfx asset texture", WorldContext="WorldContextObject"))
	static bool		IsTextureCPUInUse(const FString &virtualPath);

	/** Return whether the "virtualPath" GPU Texture is currently in use */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Textures", meta=(DisplayName="Is Texture GPU In Use", Keywords="popcornfx asset texture", WorldContext="WorldContextObject"))
	static bool		IsTextureGPUInUse(const FString &virtualPath);

	/** Register a virtual texture override for a CPU texture
	* Will replace the texture data of the given "virtualPath" by the given raw "pixels" data
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Textures", meta=(Keywords="popcornfx asset texture", WorldContext="WorldContextObject"))
	static bool		RegisterVirtualTextureOverride_CPU_FloatR(const FString &virtualPath, int32 width, int32 height, const TArray<float> &pixels);

	/** Register a virtual texture override for a CPU texture
	* Will replace the texture data of the given "virtualPath" by the given raw "pixels" data
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Textures", meta=(Keywords="popcornfx asset texture", WorldContext="WorldContextObject"))
	static bool		RegisterVirtualTextureOverride_CPU_FloatRGBA(const FString &virtualPath, int32 width, int32 height, const TArray<FLinearColor> &pixels);

	/** Unregister a virtual texture override for a CPU texture
	* Will re-load the true asset if nessecary
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Textures", meta=(Keywords="popcornfx asset texture", WorldContext="WorldContextObject"))
	static bool		UnregisterVirtualTextureOverride_CPU(const FString &virtualPath);



	/** Register a virtual texture override for a GPU texture
	* Will replace the texture of the given "virtualPath" by GPU data of the given "texture"
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Textures", meta=(Keywords="popcornfx asset texture", WorldContext="WorldContextObject"))
	static bool		RegisterVirtualTextureOverride_GPU(const FString &virtualPath, UTexture *texture);

	/** Unregister a virtual texture override for a GPU texture
	* Will replace the texture of the given "virtualPath" by GPU data of the given "texture"
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Textures", meta=(Keywords="popcornfx asset texture", WorldContext="WorldContextObject"))
	static bool		UnregisterVirtualTextureOverride_GPU(const FString &virtualPath);


	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsFloat(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, float &OutValue, bool InApplyGlobalScale);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsFloat2(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, float &OutValueX, float &OutValueY, bool InApplyGlobalScale);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsVector2D(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, FVector2D &OutValue, bool InApplyGlobalScale);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsFloat3(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, float &OutValueX, float &OutValueY, float &OutValueZ, bool InApplyGlobalScale);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsVector(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, FVector &OutValue, bool InApplyGlobalScale);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsFloat4(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, float &OutValueX, float &OutValueY, float &OutValueZ, float &OutValueW, bool InApplyGlobalScale);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsLinearColor(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, FLinearColor &OutValue);


	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsInt(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, int32 &OutValue);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsInt2(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, int32 &OutValueX, int32 &OutValueY);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsInt3(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsInt4(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ, int32 &OutValueW);


	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsBool(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, bool &OutValue);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsBool2(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, bool &OutValueX, bool &OutValueY);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsBool3(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, bool &OutValueX, bool &OutValueY, bool &OutValueZ);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsBool4(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, bool &OutValueX, bool &OutValueY, bool &OutValueZ, bool &OutValueW);


	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Event Payload", BlueprintInternalUseOnly="true"), Category="PopcornFX|Events")
	static bool		GetEventPayloadAsRotator(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, FRotator &OutValue, bool InApplyGlobalScale);
};
