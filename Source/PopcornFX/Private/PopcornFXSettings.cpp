//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXSettings.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXCustomVersion.h"
#include "Assets/PopcornFXFile.h"
#include "Assets/PopcornFXRendererMaterial.h"

#include "Engine/EngineTypes.h"
#include "Materials/MaterialInterface.h"

#define LOCTEXT_NAMESPACE "PopcornFXSettings"

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXSettings, Log, All);

//----------------------------------------------------------------------------

#define DEFAULT_MATERIAL_PATH(__name)		"/PopcornFX/Materials/M_PK_Default_" #__name ".M_PK_Default_" #__name

static const char		* const kPluginsDefaultMaterials[] = {
#define	X_POPCORNFX_MATERIAL_TYPE(__name)	DEFAULT_MATERIAL_PATH(__name),
	EXEC_X_POPCORNFX_MATERIAL_TYPE()
#undef	X_POPCORNFX_MATERIAL_TYPE
};

static const char		* const kMaterialTypeNames[] = {
#define	X_POPCORNFX_MATERIAL_TYPE(__name)	"Material_" #__name,
	EXEC_X_POPCORNFX_MATERIAL_TYPE()
#undef	X_POPCORNFX_MATERIAL_TYPE
};

UPopcornFXSettings::UPopcornFXSettings(const FObjectInitializer& PCIP)
:	Super(PCIP)
,	PackMountPoint("/Game/")
,	GlobalScale(100.0f)
,	EnableAsserts(true)
,	TimeLimitPerEffect(2.0f)
,	CPUTimeLimitPerEffect(2.0f)
,	GPUTimeLimitPerEffect(1.0f)
,	TimeLimitTotal(3.0f)
,	CPUTimeLimitTotal(3.0f)
,	GPUTimeLimitTotal(2.0f)
,	CPUParticleCountLimitPerEffect(3000)
,	GPUParticleCountLimitPerEffect(10000)
,	CPUParticleCountLimitTotal(30000)
,	GPUParticleCountLimitTotal(50000)
,	DebugBoundsLinesThickness(2.0f)
,	DebugParticlePointSize(5.0f)
,	EffectsProfilerSortMode(EPopcornFXEffectsProfilerSortMode::SimulationCost)
,	HUD_DisplayScreenRatio(0.1f)
,	HUD_HideNodesBelowPercent(5)
,	HUD_UpdateTimeFrameCount(25)
{
	//if (!IsRunningCommandlet())
	{
#	define	X_POPCORNFX_MATERIAL_TYPE(__name)	DefaultMaterials.Material_ ## __name = FSoftObjectPath(DEFAULT_MATERIAL_PATH(__name));
			EXEC_X_POPCORNFX_MATERIAL_TYPE()
#	undef	X_POPCORNFX_MATERIAL_TYPE
	}
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
void	UPopcornFXSettings::PostEditChangeProperty(struct FPropertyChangedEvent& propertyChangedEvent)
{
	Super::PostEditChangeProperty(propertyChangedEvent);

	FPopcornFXPlugin::Get().HandleSettingsModified();
}
#endif // WITH_EDITOR

//----------------------------------------------------------------------------

void	UPopcornFXSettings::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FPopcornFXCustomVersion::GUID);
}

//----------------------------------------------------------------------------

UMaterialInterface	*UPopcornFXSettings::GetConfigDefaultMaterial(uint32 ePopcornFXMaterialType) const
{
	if (ePopcornFXMaterialType >= PK_ARRAY_COUNT(kPluginsDefaultMaterials))
	{
		PK_ASSERT_NOT_REACHED();
		ePopcornFXMaterialType = 0; // fall back to bb additive
	}
	const EPopcornFXMaterialType::Type	materialType = (EPopcornFXMaterialType::Type)ePopcornFXMaterialType;

	UObject			*obj = null;
	switch (materialType)
	{
#define	X_POPCORNFX_MATERIAL_TYPE(__name)	\
	case	EPopcornFXMaterialType:: __name: obj = DefaultMaterials.Material_ ## __name .TryLoad(); break;
		EXEC_X_POPCORNFX_MATERIAL_TYPE()
#undef	X_POPCORNFX_MATERIAL_TYPE
	default:
		PK_ASSERT_NOT_REACHED();
		return null;
	}
	UMaterialInterface		*mat = null;
	if (obj != null)
	{
		mat = Cast<UMaterialInterface>(obj);
		if (!PK_VERIFY(mat != null))
			UE_LOG(LogPopcornFXSettings, Warning, TEXT("PopcornFX Config %s is not a UMaterialInterface !"), UTF8_TO_TCHAR(kMaterialTypeNames[materialType]));
	}

	if (mat == null)
	{
		UE_LOG(LogPopcornFXSettings, Warning, TEXT("PopcornFX Config %s is invalid, falling back on PopcornFX Plugin's default one %s"), *FString(kMaterialTypeNames[materialType]), *FString(kPluginsDefaultMaterials[materialType]));
		mat = ::LoadObject<UMaterialInterface>(null, UTF8_TO_TCHAR(kPluginsDefaultMaterials[materialType]));
		PK_ASSERT(mat != null);
	}
	return mat;
}

//----------------------------------------------------------------------------
//
// Settings
//
//----------------------------------------------------------------------------

FPopcornFXSimulationSettings::FPopcornFXSimulationSettings()
	: bOverride_bEnablePhysicalMaterials(0)
	, bEnablePhysicalMaterials(true)
	, bOverride_LocalizedPagesMode(0)
	, LocalizedPagesMode(EPopcornFXLocalizedPagesMode::EnableDefaultsToOff)
	, bOverride_SceneUpdateTickGroup(0)
	, SceneUpdateTickGroup(TG_PostPhysics)
{
}


#define RESOLVE_SETTING(__name) do {								\
		if (bOverride_ ## __name != 0)								\
			outValues.__name = __name;								\
		else if (configValues.bOverride_ ## __name != 0)			\
			outValues.__name = configValues.__name;					\
		else														\
			outValues.__name = defaultValues.__name;				\
		outValues.bOverride_ ## __name = 1; /* just in case */		\
	} while (0)

void	FPopcornFXSimulationSettings::ResolveSettingsTo(FPopcornFXSimulationSettings &outValues) const
{
	static const FPopcornFXSimulationSettings		defaultValues;
	check(FPopcornFXPlugin::Get().Settings() != null);
	const FPopcornFXSimulationSettings				&configValues = FPopcornFXPlugin::Get().Settings()->SimulationSettings;

	RESOLVE_SETTING(bEnablePhysicalMaterials);
	RESOLVE_SETTING(LocalizedPagesMode);
	RESOLVE_SETTING(SceneUpdateTickGroup);
}

FPopcornFXRenderSettings::FPopcornFXRenderSettings()
	: bOverride_DrawCallSortMethod(0)
	, DrawCallSortMethod(EPopcornFXDrawCallSortMethod::PerDrawCalls)
	, bOverride_BillboardingLocation(0)
	, BillboardingLocation(EPopcornFXBillboardingLocation::GPU)
	, bOverride_bEnableEarlyFrameRelease(0)
	, bEnableEarlyFrameRelease(true)
	, bOverride_bDisableStatelessCollecting(0)
	, bDisableStatelessCollecting(true)
	, bOverride_bForceLightsLitTranslucent(0)
	, bForceLightsLitTranslucent(false)
{
}

void	FPopcornFXRenderSettings::ResolveSettingsTo(FPopcornFXRenderSettings &outValues) const
{
	static const FPopcornFXRenderSettings		defaultValues;
	check(FPopcornFXPlugin::Get().Settings() != null);
	const FPopcornFXRenderSettings				&configValues = FPopcornFXPlugin::Get().Settings()->RenderSettings;

	RESOLVE_SETTING(DrawCallSortMethod);
	RESOLVE_SETTING(BillboardingLocation);
	RESOLVE_SETTING(bEnableEarlyFrameRelease);
	RESOLVE_SETTING(bDisableStatelessCollecting);
	RESOLVE_SETTING(bForceLightsLitTranslucent);
}

#undef RESOLVE_SETTING

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
