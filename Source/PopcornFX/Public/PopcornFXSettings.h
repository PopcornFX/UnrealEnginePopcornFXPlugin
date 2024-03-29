//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "UObject/ObjectMacros.h"
#include "Engine/EngineBaseTypes.h"

#include "PopcornFXDefaultMaterialsSettings.h"

#include "PopcornFXSettings.generated.h"

class UMaterialInterface;

/** Where to Billboard PopcornFX Particles */
UENUM()
namespace EPopcornFXConfigPreferedBillboarding
{
	enum	Type
	{
		/** All CPU particles billboarded on CPU, keep GPU ones on GPU */
		MaxCPU UMETA(DisplayName="Maximum CPU"),
		/** Same as MaxCPU, but if some CPU and GPU particles can be batched together, Batch them and Billboard on GPU. */
		Auto UMETA(DisplayName="Auto"),
		/** Try to always billboard on GPU, even CPU particles (but, some billboarding features can still have to run on CPU) */
		MaxGPU UMETA(DisplayName="Maximum GPU"),
	};
}

/** How to Sort PopcornFX Particles Draw Calls */
UENUM()
namespace EPopcornFXDrawCallSortMethod
{
	enum	Type
	{
		/** Do NOT sort draw calls.
		*/
		None,

		/** Sort Draw Calls by their Bounding Boxes.
		*
		* - Pro: no performance overhead
		* - Con: still have sorting issues
		*/
		PerDrawCalls,

		/** Experimental ! work in progress !
		* Slice Draw Calls, then sort those slices.
		*
		* - Pro: fixes more sorting issues
		* - Con: performance overhead: sorts ALL particles (even additive ones)
		* - Con: performance overhead: increase the number of draw calls
		*/
		PerSlicedDrawCalls,

		/** Experimental ! work in progress !
		* Enables localized Particle Pages.
		* Then sort draw call per pages.
		*
		* - Pro: fixes more sorting issues
		* - Con: performance overhead: sorts ALL particles (even additive ones)
		* - Con: performance overhead: increase the number of draw calls
		*/
		PerPageDrawCalls,

	};
}

/** How to early Cull PopcornFX Particles for Rendering. */
UENUM()
namespace EPopcornFXRenderCullMethod
{
	enum	Type
	{
		/** Do NOT Cull Particle to Render. */
		None,

		/** Can cull entire Particle Draw Requests if BBox are out of Views's frustrums. */
		DrawRequestsAgainstFrustrum,

		/** Cull entire Particle Pages. if BBox are out of Views's frustrums. */
		// CullPagesAgainstFrustrum,
	};
}

/** PopcornFX Localized page mode */
UENUM()
namespace EPopcornFXLocalizedPagesMode
{
	enum	Type
	{
		/** Disabled */
		Disable,

		/** Enabled, Default will be off, needs per-layer explicit enabling. */
		EnableDefaultsToOff,

		/** Enabled, Default will be on, unless per-layer explicit disabling. */
		EnableDefaultsToOn,
	};
}

/** PopcornFX Billboarding location for billboard and ribbon particles */
UENUM()
namespace EPopcornFXBillboardingLocation
{
	enum	Type
	{
		/** Default */
		CPU,

		/** Experimental, only supports billboard particles. */
		GPU,
	};
}

UENUM()
namespace	EPopcornFXEffectsProfilerSortMode
{
	enum	Type
	{
		/** Sort effects by their simulation cost */
		SimulationCost,

		/** Sort effects by their particle count */
		ParticleCount,

		/** Sort effects by their instance count */
		InstanceCount,
	};
}

/** Overridable PopcornFX Simulation Settings. */
USTRUCT(BlueprintType)
struct FPopcornFXSimulationSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category="PopcornFX Simulation Settings")
	uint32 bOverride_bEnablePhysicalMaterials:1;

	/** Particle Collisions takes Physical Materials into account. */
	UPROPERTY(EditAnywhere, Category="PopcornFX Simulation Settings", meta=(EditCondition="bOverride_bEnablePhysicalMaterials"))
	uint32 bEnablePhysicalMaterials : 1;

	UPROPERTY(EditAnywhere, Category="PopcornFX Simulation Settings")
	uint32 bOverride_LocalizedPagesMode : 1;

	UPROPERTY(EditAnywhere, Category="PopcornFX Simulation Settings", meta=(EditCondition="bOverride_LocalizedPagesMode"))
	TEnumAsByte<EPopcornFXLocalizedPagesMode::Type> LocalizedPagesMode;

	UPROPERTY(EditAnywhere, Category="PopcornFX Simulation Settings")
	uint32 bOverride_SceneUpdateTickGroup : 1;

	/** Tick Group for the PopcornFX Scene Update.
	(Change this at your own risk.)
	(SceneUpdateTickGroup is taken into account at Component OnRegister.)
	- You should benchmark !
	- Scene with Particles with collision/intersect could have bad performance if "During Physics".
	- But Scene with Particles **without** collision/intersect could take advantage to run in parallel in "During Physics".
	- Ticking Later ("Post Update Work") can fix particle lag, scene view lag etc, but might affect performance: you should benchmark !
	*/
	UPROPERTY(EditAnywhere, Category="PopcornFX Simulation Settings", meta=(EditCondition="bOverride_SceneUpdateTickGroup"))
	TEnumAsByte<ETickingGroup> SceneUpdateTickGroup;

	FPopcornFXSimulationSettings();

	void		ResolveSettingsTo(FPopcornFXSimulationSettings &outSettings) const;
};


/** Overridable PopcornFX Render Settings */
USTRUCT(BlueprintType)
struct FPopcornFXRenderSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category="PopcornFX Render Settings")
	uint32 bOverride_DrawCallSortMethod:1;

	/** How to sort batches of particles for renderering. */
	UPROPERTY(EditAnywhere, Category="PopcornFX Render Settings", meta=(EditCondition="bOverride_DrawCallSortMethod"))
	TEnumAsByte<EPopcornFXDrawCallSortMethod::Type> DrawCallSortMethod;

	UPROPERTY(EditAnywhere, Category="PopcornFX Render Settings")
	uint32 bOverride_BillboardingLocation:1;

	/** PopcornFX Billboarding location for billboard and ribbon particles */
	UPROPERTY(EditAnywhere, Category="PopcornFX Render Settings", meta=(EditCondition="bOverride_BillboardingLocation"))
	TEnumAsByte<EPopcornFXBillboardingLocation::Type> BillboardingLocation;

	UPROPERTY(EditAnywhere, Category="PopcornFX Render Settings")
	uint32 bOverride_bEnableEarlyFrameRelease:1;

	/** Tries to early release RenderThread's particle data copy.
	* Can reduce a bit Tick time and memory usage.
	* But could have shadow flickering issues, and other unforseen side effects.
	*/
	UPROPERTY(EditAnywhere, Category="PopcornFX Render Settings", meta=(EditCondition="bOverride_bEnableEarlyFrameRelease"))
	uint32 bEnableEarlyFrameRelease : 1;

	UPROPERTY(EditAnywhere, Category="PopcornFX Render Settings")
	uint32 bOverride_bDisableStatelessCollecting : 1;

	/** If enabled, keeps track of the current rendered frame across render passes (main, shadow passes, ..)
	* this will prevent each pass from re-writing view independent data into gpu buffers
	*/
	UPROPERTY(EditAnywhere, Category="PopcornFX Render Settings", meta=(EditCondition="bOverride_bDisableStatelessCollecting"))
	uint32 bDisableStatelessCollecting : 1;

	UPROPERTY(EditAnywhere, Category="PopcornFX Render Settings")
	uint32 bOverride_bForceLightsLitTranslucent:1;

	/** Force Particle Lights to Lit Translucent Geometry (including other Lit Particles) */
	UPROPERTY(EditAnywhere, Category="PopcornFX Render Settings", meta=(EditCondition="bOverride_bForceLightsLitTranslucent"))
	uint32 bForceLightsLitTranslucent : 1;

	FPopcornFXRenderSettings();

	void		ResolveSettingsTo(FPopcornFXRenderSettings &outSettings) const;
};

/**
* PopcornFX Plugin Config
*/
UCLASS(MinimalAPI, Config=Engine, DefaultConfig)
class UPopcornFXSettings : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	/** Needs Restart Editor !
	* The Content root directory of the PopcornFX Pack.
	* All Assets used by PopcornFX must be under this PackMountPoint.
	*/
	UPROPERTY(Config, EditAnywhere, Category=Pack)
	FString				PackMountPoint;

	/** PopcornFX Particles to UnrealEngine Global Scale. ie: GlobalScale = 100 means 1 unit in PopcornFX will become 100 unit in Unreal Engine */
	UPROPERTY(Config, EditAnywhere, Category=Pack)
	float				GlobalScale;

	/** Whether PopcornFX asserts should be enabled */
	UPROPERTY(Config, EditAnywhere, Category=General)
	uint32				EnableAsserts : 1;

	/** PopcornFX Simulation Settings. */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Simulation Settings", meta=(FullyExpand="true"))
	FPopcornFXSimulationSettings		SimulationSettings;

	/** PopcornFX Render Settings. */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Render Settings", meta=(FullyExpand="true"))
	FPopcornFXRenderSettings			RenderSettings;

	/** List of default materials for the project. */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Materials")
	FPopcornFXDefaultMaterialsSettings	DefaultMaterials;

	/** Time limit per effect in miliseconds (does not affect simulated effects yet) */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Budget")
	float						TimeLimitPerEffect;

	/** CPU time limit per effect in miliseconds (does not affect simulated effects yet) */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Budget")
	float						CPUTimeLimitPerEffect;

	/** GPU time limit per effect in miliseconds (does not affect simulated effects yet) */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Budget")
	float						GPUTimeLimitPerEffect;

	/** Time limit in miliseconds (does not affect simulated effects yet) */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Budget")
	float						TimeLimitTotal;

	/** CPU time limit in miliseconds (does not affect simulated effects yet) */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Budget")
	float						CPUTimeLimitTotal;

	/** GPU time limit in miliseconds (does not affect simulated effects yet) */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Budget")
	float						GPUTimeLimitTotal;

	/** CPU particle count limit per effect (does not affect simulated effects yet) */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Budget")
	uint32						CPUParticleCountLimitPerEffect;

	/** GPU particle count limit per effect (does not affect simulated effects yet) */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Budget")
	uint32						GPUParticleCountLimitPerEffect;

	/** CPU particle count limit (does not affect simulated effects yet) */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Budget")
	uint32						CPUParticleCountLimitTotal;

	/** GPU particle count limit (does not affect simulated effects yet) */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX Budget")
	uint32						GPUParticleCountLimitTotal;

	/** Debug draw bounds lines thickness */
	UPROPERTY(Config, EditAnywhere, Category="Debug", meta=(ClampMin="0.1", ClampMax="100000.0", UIMin="0.1", UIMax="100000.0"))
	float						DebugBoundsLinesThickness;

	/** Debug draw particle point size */
	UPROPERTY(Config, EditAnywhere, Category="Debug", meta=(ClampMin="0.1", ClampMax="100000.0", UIMin="0.1", UIMax="100000.0"))
	float						DebugParticlePointSize;

	/** Profiler: Sort method for the effects list */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX HUD")
	TEnumAsByte<EPopcornFXEffectsProfilerSortMode::Type>	EffectsProfilerSortMode;

	/** HUDs: HUDs screen ratio */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX HUD", meta=(ClampMin="0", ClampMax="1", UIMin="0", UIMax="1"))
	float						HUD_DisplayScreenRatio;

	/** HUDs: Hide stat nodes below this percentage */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX HUD", meta=(ClampMin="0", ClampMax="100", UIMin="0", UIMax="100"))
	int32						HUD_HideNodesBelowPercent;

	/** Scene: Frame count to compute update time average */
	UPROPERTY(Config, EditAnywhere, Category="PopcornFX HUD", meta=(ClampMin="1", ClampMax="50", UIMin="1", UIMax="50"))
	uint32						HUD_UpdateTimeFrameCount;

	// overrides
	virtual void		Serialize(FArchive& Ar) override;

#if WITH_EDITOR
	virtual void		PostEditChangeProperty(struct FPropertyChangedEvent& propertyChangedEvent) override;
#endif

	UMaterialInterface	*GetConfigDefaultMaterial(uint32 ePopcornFXMaterialType) const;
};
