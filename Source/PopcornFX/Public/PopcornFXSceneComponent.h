//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"
#include "PopcornFXSettings.h"

#include "Components/PrimitiveComponent.h"

#include "PopcornFXSceneComponent.generated.h"

class	CParticleScene;
class	UPopcornFXEffect;
class	FPopcornFXSceneProxy;

UENUM()
namespace EPopcornFXSceneBBMode
{
	enum	Type
	{
		Dynamic,
		DynamicPlusFixed,
		Fixed,
	};
}

UENUM()
namespace EPopcornFXHeavyDebugMode
{
	enum	Type
	{
		/** No debug */
		None = 0,
		/** Debug Particles as points, colored by mediums */
		ParticlePoints,
		/** Debug Mediums BBox */
		MediumsBBox,
		/** Debug Particle Pages bounding boxes, colored by mediums */
		PagesBBox,
	};
}

/** Handles all the PopcornFX Particle Simulation and Rendering context.
* (All PopcornFX Emitters will actually ask a PopcornFXSceneComponent to spawn Particles)
*/
UCLASS(HideCategories=(Input, Collision, Replication, Materials), EditInlineNew, ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXSceneComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	/** The SceneName used to indentify this PopcornFXScene
	* PopcornFX Emitters will look for this SceneName when searching a PopcornFXScene to Spawn into.
	*/
	UPROPERTY(Category="PopcornFX Scene", EditAnywhere)
	FName									SceneName;

	/** Enable PopcornFX Updates for this scene. */
	UPROPERTY(Category="PopcornFX Scene", Interp, EditAnywhere)
	uint32									bEnableUpdate : 1;

	/** Enable PopcornFX Render for this scene. */
	UPROPERTY(Category="PopcornFX Scene", Interp, EditAnywhere)
	uint32									bEnableRender : 1;

	/**	Bounding Box Computing Mode. */
	UPROPERTY(Category="PopcornFX Scene", EditAnywhere)
	TEnumAsByte<EPopcornFXSceneBBMode::Type>	BoundingBoxMode;

	/**	Fixed relative bounding box used if BoundingBoxMode is DynamicPlusFixed or Fixed. */
	UPROPERTY(Category="PopcornFX Scene", EditAnywhere)
	FBox									FixedRelativeBoundingBox;

	/** Override PopcornFX Config's Simulation Settings. */
	UPROPERTY(Category="PopcornFX Scene", EditAnywhere)
	FPopcornFXSimulationSettings			SimulationSettingsOverride;

	/** Override PopcornFX Config's Render Settings. */
	UPROPERTY(Category="PopcornFX Scene", EditAnywhere)
	FPopcornFXRenderSettings				RenderSettingsOverride;

	/** DEBUG: Draw some heavy debug. */
	UPROPERTY(Category="PopcornFX SceneDebug", EditAnywhere)
	TEnumAsByte<EPopcornFXHeavyDebugMode::Type>		HeavyDebugMode;

	/** DEBUG: Freezes the main billboarding matrix. */
	UPROPERTY(Category="PopcornFX SceneDebug", EditAnywhere)
	uint32									bRender_FreezeBillboardingMatrix : 1;

	/** DEBUG: Overrides Particle Color by a per draw call color. */
	UPROPERTY(Category="PopcornFX SceneDebug", EditAnywhere)
	uint32									bRender_OverrideColorByDrawCall : 1;

	/** DEBUG: The number of colors used for Override Color debugs. */
	UPROPERTY(Category="PopcornFX SceneDebug", EditAnywhere)
	int32									Render_OverrideDebugColorCount;

	/** DEBUG: Overrides all Particles materials with this one.
	MUST be compatible for Particle Sprites AND Mesh Particles !
	(StaticSwitches are not available)
	*/
	UPROPERTY(Category="PopcornFX SceneDebug", EditAnywhere)
	UMaterialInterface						*Render_OverrideAllMaterial;


	/** Clears this scene from all existing Particles.
	Warning : Every emitters registered into this scene will stop to emit, you will need to manually restart your effects */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Scene", meta=(Keywords="popcornfx scene clear"))
	void								Clear();

	/** Installs all input particle effects into this PopcornFX Scene.
		This will ensure no runtime stalls at first instantiation of an emitter
		! Install is synchronous ! */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Scene", meta=(Keywords="popcornfx scene preload install"))
	bool								InstallEffects(TArray<UPopcornFXEffect*> Effects);

	/**
	* Set your own audio sampling class. Used by effects sampling audio spectrum/waveform.
	*/
	void								SetAudioSamplingInterface(class IPopcornFXFillAudioBuffers *fillAudioBuffers);

	/**
	* Set your own audio interface instead of the default one if you need to trigger Wwise, Fmod sounds instead of UE assets.
	* Call SetAudioInterface(null) or Clear() before destroying it
	*/
	void								SetAudioInterface(class IPopcornFXAudio *audioInterface);

	// overrides UObject
	virtual void						PostLoad() override;
	virtual void						PostInitProperties() override;
#if WITH_EDITOR
	virtual void						PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

	// overrides UActorComponent
	virtual void						OnRegister() override;
	virtual void						OnUnregister() override;
	virtual void						BeginDestroy() override;
	virtual void						TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction *thisTickFunction) override;
	virtual void						ApplyWorldOffset(const FVector &inOffset, bool worldShift) override;
	virtual void						SendRenderDynamicData_Concurrent() override;
	virtual void						CreateRenderState_Concurrent(FRegisterComponentContext *context) override;

	// overrides USceneComponent
	virtual FBoxSphereBounds			CalcBounds(const FTransform & LocalToWorld) const override;

	// overrides UPrimitiveComponent
	virtual FPrimitiveSceneProxy		*CreateSceneProxy() override;
	virtual bool						ShouldRenderSelected() const;
	virtual void						GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

	/** In Game builds, needs to be manually called to reload settings. */
	void								ResolveSettings();

	/** Actual Resolved Simulation Settings. */
	inline const FPopcornFXSimulationSettings		&ResolvedSimulationSettings() const { return m_ResolvedSimulationSettings; }
	/** Actual Resolved Render Settings. */
	inline const FPopcornFXRenderSettings			&ResolvedRenderSettings() const { return m_ResolvedRenderSettings; }

	inline CParticleScene				*ParticleScene() const { return m_ParticleScene; }
	inline CParticleScene				*ParticleSceneToRender() const { return bEnableRender != 0 ? ParticleScene() : nullptr; }

#if WITH_EDITOR
	void								MirrorGameWorldProperties(const UPopcornFXSceneComponent *other);
#endif // WITH_EDITOR

private:
	void								_OnSettingsChanged() { ResolveSettings(); }

	CParticleScene						*m_ParticleScene;
	FPopcornFXSimulationSettings		m_ResolvedSimulationSettings;
	FPopcornFXRenderSettings			m_ResolvedRenderSettings;
	FDelegateHandle						m_OnSettingsChangedHandle;
};
