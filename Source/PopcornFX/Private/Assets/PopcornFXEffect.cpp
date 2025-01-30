//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "Assets/PopcornFXEffect.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXCustomVersion.h"
#include "PopcornFXAttributeList.h"
#include "Internal/ParticleScene.h"
#include "Internal/DependencyHelper.h"
#include "Internal/FileSystemController_UE.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "PopcornFXEffectPriv.h"

#include "Editor/EditorHelpers.h"

#include "Scalability.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_effect.h>
#include <pk_particles/include/ps_descriptor.h>
#include <pk_particles/include/ps_nodegraph_nodes_render.h>
#include <pk_particles/include/ps_event_map.h>
#include <pk_particles/include/ps_samplers_animtrack.h>
#include <pk_particles/include/ps_attributes.h>
#include <pk_base_object/include/hbo_helpers.h>
#include <pk_base_object/include/hbo_details.h>

#if WITH_EDITOR
#	include "Materials/MaterialInstanceConstant.h"
#	include "HardwareInfo.h"
#	include "Misc/FeedbackContext.h"
#	include "Misc/FileHelper.h"
#	include "TargetPlatform.h"
#	include "Platforms/PopcornFXPlatform.h"
#	include <PK-AssetBakerLib/AssetBaker_Cookery.h>
#	include <PK-AssetBakerLib/AssetBaker_Oven.h>
#	include <PK-AssetBakerLib/AssetBaker_Oven_HBO.h>
#	include <pk_particles/include/ps_descriptor_cache.h>
#endif // WITH_EDITOR

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXEffect, Log, All);

//----------------------------------------------------------------------------
//
// SPopcornFXEffect (private version of UPopcornFXEffect)
//
//----------------------------------------------------------------------------

bool	CPopcornFXEffect::RegisterEventCallback(const PopcornFX::FastDelegate<PopcornFX::CParticleEffect::EventCallback> &callback, const PopcornFX::CStringId &eventNameID)
{
	// Register broadcast callback only if this event never got registered
	if (PK_VERIFY(m_ParticleEffect != null))
		return const_cast<PopcornFX::CParticleEffect*>(m_ParticleEffect.Get())->RegisterEventCallback(callback, eventNameID);
	return false;
}

void	CPopcornFXEffect::UnregisterEventCallback(const PopcornFX::FastDelegate<PopcornFX::CParticleEffect::EventCallback> &callback, const PopcornFX::CStringId &eventNameID)
{
	if (m_ParticleEffect != null)
		const_cast<PopcornFX::CParticleEffect*>(m_ParticleEffect.Get())->UnregisterEventCallback(callback, eventNameID);
}

PopcornFX::PCParticleEffect	CPopcornFXEffect::ParticleEffect()
{
	if (!m_Owner->LoadEffectIFN())
		return null;
	return m_ParticleEffect;
}

PopcornFX::PCParticleAttributeList	CPopcornFXEffect::AttributeList() // LoadEffectIFN
{
	if (!m_Owner->LoadEffectIFN())
		return null;
	return m_ParticleEffect->AttributeFlatList().Get();
}

PopcornFX::PCParticleAttributeList	CPopcornFXEffect::AttributeListIFP() const
{
	return m_ParticleEffect == null ? null : m_ParticleEffect->AttributeFlatList().Get();
}

PopcornFX::PCParticleEffect	CPopcornFXEffect::ParticleEffectIFP() const
{
	return m_ParticleEffect;
}

PopcornFX::PParticleEffect	CPopcornFXEffect::Unsafe_ParticleEffect() const
{
	return const_cast<PopcornFX::CParticleEffect*>(m_ParticleEffect.Get());
}

//----------------------------------------------------------------------------
//
// UPopcornFXEffect
//
//----------------------------------------------------------------------------

UPopcornFXEffect::UPopcornFXEffect(const FObjectInitializer& PCIP)
	: Super(PCIP)
#if WITH_EDITORONLY_DATA
	, EditorLoopEmitter(false)
	, EditorLoopDelay(2.0f)
#endif // WITH_EDITORONLY_DATA
	, m_Loaded(false)
	, m_Cooked(false) // TMP: Until proper implementation of platform cached data
{
	m_Private = PK_NEW(CPopcornFXEffect(this));
	DefaultAttributeList = CreateDefaultSubobject<UPopcornFXAttributeList>(TEXT("DefaultAttributeList"));
}

//----------------------------------------------------------------------------

UPopcornFXEffect::~UPopcornFXEffect()
{
	PK_ASSERT(m_Private != null);
	PK_SAFE_DELETE(m_Private);
}

//----------------------------------------------------------------------------

bool	UPopcornFXEffect::IsLoadCompleted() const
{
	return HasAnyFlags(RF_LoadCompleted) && DefaultAttributeList != null && DefaultAttributeList->HasAnyFlags(RF_LoadCompleted);
}

//----------------------------------------------------------------------------

UPopcornFXAttributeList		*UPopcornFXEffect::GetDefaultAttributeList()
{
	LoadEffectIFN();

	if (!PK_VERIFY(DefaultAttributeList != null)) // should always be true ?
		return null;
	if (!PK_VERIFY(DefaultAttributeList->IsUpToDate(this)))
		DefaultAttributeList->SetupDefault(this);
	if (!PK_VERIFY(DefaultAttributeList->Valid()))
		return null;
	return DefaultAttributeList;
}

//----------------------------------------------------------------------------
#if WITH_EDITOR

//----------------------------------------------------------------------------

void	UPopcornFXEffect::PreReimport_Clean()
{
	ParticleRendererMaterials.Empty(ParticleRendererMaterials.Num());

	Super::PreReimport_Clean();
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 4)
void	UPopcornFXEffect::GetAssetRegistryTags(FAssetRegistryTagsContext context) const
{
	context.AddTag(FAssetRegistryTag("NumMaterials", LexToString(ParticleRendererMaterials.Num()), FAssetRegistryTag::TT_Numerical));

	Super::GetAssetRegistryTags(context);
}
#else
void	UPopcornFXEffect::GetAssetRegistryTags(TArray<FAssetRegistryTag> &outTags) const
{
	outTags.Add(FAssetRegistryTag("NumMaterials", LexToString(ParticleRendererMaterials.Num()), FAssetRegistryTag::TT_Numerical));

	Super::GetAssetRegistryTags(outTags);
}
#endif

//----------------------------------------------------------------------------

void	UPopcornFXEffect::BeginCacheForCookedPlatformData(const ITargetPlatform *targetPlatform)
{
	if (IsTemplate() || IsGarbageCollecting() || GIsSavingPackage)
		return;
	if (m_Cooked) // TMP: Until proper implementation of platform cached data
		return;

	const FString	path = GetPathName();
	if (!path.IsEmpty())
	{
		// Rebake self for target platform

		// For debugging purposes only, helps inspect content of baked effects
		if (FPopcornFXPlugin::Get().SettingsEditor()->bDebugBakedEffects)
		{
			const FString	kPopcornCookedPath = TEXT("Saved/PopcornFX/Cooked/");
			const FString	cookedFilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / kPopcornCookedPath / FileSourceVirtualPath + "_Pre");

			FFileHelper::SaveArrayToFile(m_FileData, *cookedFilePath);
		}

		FString		bakedFilePath;
		if (_BakeFile(path, bakedFilePath, false, targetPlatform->PlatformName()))
		{
			// For debugging purposes only, helps inspect content of baked effects
			if (FPopcornFXPlugin::Get().SettingsEditor()->bDebugBakedEffects)
			{
				const FString	kPopcornCookedPath = TEXT("Saved/PopcornFX/Cooked/");
				const FString	cookedFilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / kPopcornCookedPath / FileSourceVirtualPath);

				FFileHelper::SaveArrayToFile(m_FileData, *cookedFilePath);
			}

			// Make sure to re-create serialized renderer materials, if necessary:
			// Effect re-baked for a different platform can end up with different renderers, materials, textures..
			ReloadRendererMaterials();

			m_Cooked = true; // TMP: Until proper implementation of platform cached data
		}
	}

	Super::BeginCacheForCookedPlatformData(targetPlatform);
}
#endif // WITH_EDITOR

//----------------------------------------------------------------------------

bool	UPopcornFXEffect::LoadEffectIFN()
{
	if (!m_Loaded)
		LoadEffect();
	PK_ASSERT(m_Loaded == (m_Private->m_ParticleEffect != null));
	return m_Loaded;
}

//----------------------------------------------------------------------------

void	UPopcornFXEffect::ClearEffect()
{
	m_Loaded = false;
	m_Private->m_ParticleEffect = null;
}

//----------------------------------------------------------------------------

const PopcornFX::CStringView	kQualityLevelNames[] = { "low", "medium", "high", "epic", "cinematic" };

//----------------------------------------------------------------------------

bool	UPopcornFXEffect::LoadEffect(bool forceImport)
{
	ClearEffect();

	check(IPopcornFXPlugin::IsAvailable());
	if (!PK_VERIFY(IsFileValid()))
		return false;
	PopcornFX::PBaseObjectFile	boFile = FPopcornFXPlugin::Get().LoadPkFile(this);
	PK_ASSERT(boFile != null);

	{
		LLM_SCOPE(ELLMTag::Particles);

		PopcornFX::SEffectLoadCtl	effectLoadCtl = PopcornFX::SEffectLoadCtl::kDefault;
		effectLoadCtl.m_AllowedEffectFileType = PopcornFX::SEffectLoadCtl::EffectFileType_Any; // Can be text in shipping builds if debug baked effect is enabled. Keep as is

		// If custom quality level, fallback on low, do not print log or it will spam output log
		const Scalability::FQualityLevels	qualityLevels = Scalability::GetQualityLevels();
		const u32							qualityLevelEffects = PopcornFX::PKMin(qualityLevels.EffectsQuality, PK_ARRAY_COUNT(kQualityLevelNames) - 1);
		const PopcornFX::CStringView		qualityLevelName = qualityLevels.EffectsQuality == -1 ? PopcornFX::CStringView("low") : kQualityLevelNames[qualityLevelEffects];

		m_Private->m_ParticleEffect = PopcornFX::CParticleEffect::Load(boFile, effectLoadCtl, qualityLevelName);
		if (m_Private->m_ParticleEffect == null)
		{
			UE_LOG(LogPopcornFXEffect, Warning/*Error*/, TEXT("Could not load Effect from file: '%s' %d"), *GetPathName(), IsTemplate());
			return false;
		}
	}
	if (m_Private->m_ParticleEffect->EventConnectionMap() == null)
	{
		ClearEffect();
		UE_LOG(LogPopcornFXEffect, Warning, TEXT("Could not load effect connection-map: '%s'"), *GetPathName());
		return false;
	}

	m_Loaded = true;

	DefaultAttributeList->SetupDefault(this, forceImport);
	return m_Loaded;
}

//----------------------------------------------------------------------------

void	UPopcornFXEffect::BeginDestroy()
{
	Super::BeginDestroy();
	check(m_Private->m_ParticleEffect == null); // OnFileUnload() should have been called
}

//----------------------------------------------------------------------------

FString	UPopcornFXEffect::GetDesc()
{
	FString	description(TEXT("PopcornFX particle system"));
	return description;
}

//----------------------------------------------------------------------------
#if WITH_EDITOR

namespace
{
	//----------------------------------------------------------------------------
	//
	//	Platforms
	//
	//----------------------------------------------------------------------------

	static const char	*kDesktop = "desktop";
	static const char	*kWindows = "windows";
	static const char	*kLinux = "linux";
	static const char	*kMac = "macos";
	static const char	*kConsole = "console";

	static const char	*kXOne = "xboxone";
	static const char	*kXSeries = "UNKNOWN1eries";
	static const char	*kPS4 = "ps4";
	static const char	*kUNKNOWN2 = "UNKNOWN2";
	static const char	*kSwitch = "switch"; // console?

	static const char	*kMobile = "mobile";
	static const char	*kIOS = "ios";
	static const char	*kAndroid = "android";
	static const char	*kMagicLeap = "magicleap";

	static const char	*kEditor = "editor";
	static const char	*kEngineTag = "unrealengine";

	//----------------------------------------------------------------------------

	struct	SBackendAndBuildTags
	{
		u32					m_BackendTargets; // PopcornFX::EBackendTarget
		PopcornFX::CString	m_BuildTags;

		SBackendAndBuildTags()
		:	m_BackendTargets(PopcornFX::BackendTarget_CPU)
		{
		}
	};

	//----------------------------------------------------------------------------

	void	UEPlatformToPkBackendConfig(const FString &platformName, SBackendAndBuildTags &outBackendAndBuildTags)
	{
		// Keep this an array, allows to do some compile time checks if the list mismatches PopcornFX supported tags
		PopcornFX::TStaticCountedArray<PopcornFX::CString, 6>	buildTags;

		outBackendAndBuildTags.m_BackendTargets = 0;

		PK_VERIFY(buildTags.PushBack(kEngineTag).Valid());

		if (platformName == "AllDesktop")
		{
			PK_VERIFY(buildTags.PushBack(kDesktop).Valid());
			PK_VERIFY(buildTags.PushBack(kWindows).Valid());
			PK_VERIFY(buildTags.PushBack(kLinux).Valid());
			PK_VERIFY(buildTags.PushBack(kMac).Valid());

			// For PC cook, RHI can be specified by command line so we need to bake for both backends
			outBackendAndBuildTags.m_BackendTargets |= 1 << PopcornFX::BackendTarget_D3D12;
			outBackendAndBuildTags.m_BackendTargets |= 1 << PopcornFX::BackendTarget_D3D11;
		}
		else if (platformName == "Editor") // Pre-cooked
		{
			PK_VERIFY(buildTags.PushBack(kDesktop).Valid());
			PK_VERIFY(buildTags.PushBack(kWindows).Valid());
			PK_VERIFY(buildTags.PushBack(kLinux).Valid());
			PK_VERIFY(buildTags.PushBack(kMac).Valid());
			PK_VERIFY(buildTags.PushBack(kEditor).Valid());

			// Specific to editor baking, to reduce import times, we only import for a single API (unless bBuildAllDesktopBytecodes).
			// This means if the users change the preferred RHI, they will have to reimport all assets,
			// but it's better than to double import times in editor
			if (FPopcornFXPlugin::Get().SettingsEditor()->bBuildAllDesktopBytecodes)
			{
				outBackendAndBuildTags.m_BackendTargets |= 1 << PopcornFX::BackendTarget_D3D12;
				outBackendAndBuildTags.m_BackendTargets |= 1 << PopcornFX::BackendTarget_D3D11;
			}
			else
			{
			const FString	hardwareDetails = FHardwareInfo::GetHardwareDetailsString();

			if (hardwareDetails.Contains("D3D12") && FModuleManager::Get().IsModuleLoaded("D3D12RHI"))
				outBackendAndBuildTags.m_BackendTargets = 1 << PopcornFX::BackendTarget_D3D12;
			else if (hardwareDetails.Contains("D3D11") && FModuleManager::Get().IsModuleLoaded("D3D11RHI"))
				outBackendAndBuildTags.m_BackendTargets = 1 << PopcornFX::BackendTarget_D3D11;
		}
		}
		else if (platformName.Contains("Win"))
		{
			PK_VERIFY(buildTags.PushBack(kDesktop).Valid());
			PK_VERIFY(buildTags.PushBack(kWindows).Valid());

			// For PC cook, RHI can be specified by command line so we need to bake for both backends
			outBackendAndBuildTags.m_BackendTargets |= 1 << PopcornFX::BackendTarget_D3D12;
			if (platformName != "WinUNKNOWN") // UE4 only supports D3D12
				outBackendAndBuildTags.m_BackendTargets |= 1 << PopcornFX::BackendTarget_D3D11;
		}
		else if (platformName.Contains("Linux"))
		{
			PK_VERIFY(buildTags.PushBack(kDesktop).Valid());
			PK_VERIFY(buildTags.PushBack(kLinux).Valid());
		}
		else if (platformName.Contains("Mac"))
		{
			PK_VERIFY(buildTags.PushBack(kDesktop).Valid());
			PK_VERIFY(buildTags.PushBack(kMac).Valid());
		}
		else if (platformName == "PS4")
		{
			PK_VERIFY(buildTags.PushBack(kConsole).Valid());
			PK_VERIFY(buildTags.PushBack(kPS4).Valid());
		}
		else if (platformName == "UNKNOWN2")
		{
			PK_VERIFY(buildTags.PushBack(kConsole).Valid());
			PK_VERIFY(buildTags.PushBack(kUNKNOWN2).Valid());
			outBackendAndBuildTags.m_BackendTargets = 1 << PopcornFX::BackendTarget_UNKNOWN2;
		}
		else if (platformName.Contains("XboxOne"))
		{
			PK_VERIFY(buildTags.PushBack(kConsole).Valid());
			PK_VERIFY(buildTags.PushBack(kXOne).Valid());

			// Xbox One (XDK) not supported. Minimum shader model for PopcornFX GPU sim sources on D3D12 is 5.1
			if (platformName == "XboxOneUNKNOWN")
				outBackendAndBuildTags.m_BackendTargets = 1 << PopcornFX::BackendTarget_D3D12X;
		}
		else if (platformName == "UNKNOWN")
		{
			PK_VERIFY(buildTags.PushBack(kConsole).Valid());
			PK_VERIFY(buildTags.PushBack(kXSeries).Valid());
			outBackendAndBuildTags.m_BackendTargets = 1 << PopcornFX::BackendTarget_D3D12XS;
		}
		else if (platformName == "Switch")
		{
			PK_VERIFY(buildTags.PushBack(kConsole).Valid());
			PK_VERIFY(buildTags.PushBack(kSwitch).Valid());
		}
		else if (platformName.Contains("Android"))
		{
			PK_VERIFY(buildTags.PushBack(kMobile).Valid());
			PK_VERIFY(buildTags.PushBack(kAndroid).Valid());
		}
		else if (platformName.Contains("IOS"))
		{
			PK_VERIFY(buildTags.PushBack(kMobile).Valid());
			PK_VERIFY(buildTags.PushBack(kIOS).Valid());
		}
		else if (platformName.Contains("Lumin"))
		{
			PK_VERIFY(buildTags.PushBack(kMobile).Valid());
			PK_VERIFY(buildTags.PushBack(kMagicLeap).Valid());
		}

#if	defined(PK_DEBUG)
		PopcornFX::TMemoryView<const PopcornFX::SBuildVersion::SBuildTag>	staticBuildTags = PopcornFX::SBuildVersion::AllStaticPlatformBuildTags();
		for (const PopcornFX::CString &tag : buildTags)
			PK_ASSERT(staticBuildTags.Contains(tag.View()));
#endif

		for (const PopcornFX::CString &tag : buildTags)
		{
			outBackendAndBuildTags.m_BuildTags += tag;
			outBackendAndBuildTags.m_BuildTags.Append(",");
		}
	}

	//----------------------------------------------------------------------------
	//
	//	Dependencies
	//
	//----------------------------------------------------------------------------

	const EPopcornFXAssetDepType::Type		kPkToAssetDepType[] = {
		EPopcornFXAssetDepType::None,//Caracs_None = 0,
		// Resrouces/Paths:
		EPopcornFXAssetDepType::Effect,//Caracs_ResourceEffect,				// path to an effect
		EPopcornFXAssetDepType::Texture,//Caracs_ResourceTexture,			// path to a texture : display image browser and preview
		EPopcornFXAssetDepType::TextureAtlas,//Caracs_ResourceTextureAtlas,	// path to a texture atlas
		EPopcornFXAssetDepType::Font,//Caracs_ResourceFont,					// path to a font
		EPopcornFXAssetDepType::VectorField,//Caracs_VectorField			// path to a vector field
		EPopcornFXAssetDepType::Mesh,//Caracs_ResourceMesh,					// path to a mesh
		EPopcornFXAssetDepType::None,//Caracs_ResourceAlembic,				// path to an alembic
		EPopcornFXAssetDepType::Video,//Caracs_ResourceVideo,				// path to a video
		EPopcornFXAssetDepType::Sound,//Caracs_ResourceSound,				// path to an audio source
		EPopcornFXAssetDepType::SimCache,//Caracs_ResourceSimCache			// path to a simulation cache
		EPopcornFXAssetDepType::None,//Caracs_ResourceMaterial				// path to a material
		EPopcornFXAssetDepType::None,//Caracs_ResourcePackage				// path to a package
		EPopcornFXAssetDepType::None,//Caracs_Path,							// path to a random file
		EPopcornFXAssetDepType::None,//Caracs_PathDir,						// path to a random dir
		EPopcornFXAssetDepType::None,//Caracs_AbsPath,						// path to a random file (absolute path)
		EPopcornFXAssetDepType::None,//Caracs_AbsPathDir,					// path to a random dir (absolute path)
		// Others
		EPopcornFXAssetDepType::None,//Caracs_Color,						// RGB or RGBA color (sRGB-space)
		EPopcornFXAssetDepType::None,//Caracs_ColorLinear,					// RGB or RGBA color (linear-space)
		EPopcornFXAssetDepType::None,//Caracs_Distance,						// Distance values, stored as meters, displayed in the user's custom distance metric
		EPopcornFXAssetDepType::None,//Caracs_3DCoordinate,					// 3D coordinates, stored as {side, up, forward}, displayed in the user's custom coordinate-system
		EPopcornFXAssetDepType::None,//Caracs_3DCoordinateDistance,			// Caracs_3DCoordinate + Caracs_Distance
		EPopcornFXAssetDepType::None,//Caracs_3DScale,						// 3D scales, same as 3D Coordinates but ignore sign's axis
		EPopcornFXAssetDepType::None,//Caracs_Hex,							// should be displayed as Hexadecimal
		EPopcornFXAssetDepType::None,//Caracs_Slider,						// should be displayed as a slider
		EPopcornFXAssetDepType::None,//Caracs_BitMask,						// when the field is of type 'int', and has a GTT, should be displayed as a multi-selection dropdown, and serialized as an 'int', OR-ed with the GTT values themselves, not the IDs into the GTT
		EPopcornFXAssetDepType::None,//Caracs_TextBuffer,					// not just a string, should be editable with a larger text editor (ex: script)
		EPopcornFXAssetDepType::None,//Caracs_UserObjectName				// user-to-user name for the object
		EPopcornFXAssetDepType::None,//Caracs_UserObjectDescription			// user-to-user description for the object
	};
	PK_STATIC_ASSERT(PK_ARRAY_COUNT(kPkToAssetDepType) == PopcornFX::HBO::Caracs_UserObjectDescription + 1);

	//----------------------------------------------------------------------------
	
	EPopcornFXAssetDepType::Type	PkToAssetDepType(const PopcornFX::CBaseObject &object, uint32 pkFieldType)
	{
	if (PK_VERIFY(pkFieldType < PK_ARRAY_COUNT(kPkToAssetDepType)))
	{
		const EPopcornFXAssetDepType::Type	depType = kPkToAssetDepType[pkFieldType];
		if (depType != EPopcornFXAssetDepType::None)
			return depType;

		// v2 temp code: All asset dependencies should be retrieved by accessing the effects dependencies directly: trash all that code
		if (PopcornFX::HBO::CaracsIsPath(PopcornFX::HBO::FieldCaracs(pkFieldType))) // It's a path
		{
			const PopcornFX::CLayerCompileCacheRendererProperty	*rendererProperty = PopcornFX::HBO::Cast<const PopcornFX::CLayerCompileCacheRendererProperty>(&object);
			if (rendererProperty != null)
			{
				switch (rendererProperty->PropertyType())
				{
				case	PopcornFX::PropertyType_TexturePath:
					return EPopcornFXAssetDepType::Texture;
				case	PopcornFX::PropertyType_TextureAtlasPath:
					return EPopcornFXAssetDepType::TextureAtlas;
				case	PopcornFX::PropertyType_MeshPath:
					return EPopcornFXAssetDepType::Mesh;
				case	PopcornFX::PropertyType_SoundPath:
					return EPopcornFXAssetDepType::Sound;
				default:
					break;
				}
			}
			{
				// This is actually only needed when re-baking from inlined source
				const PopcornFX::CParticleNodePinIn				*sourcePin = PopcornFX::HBO::Cast<const PopcornFX::CParticleNodePinIn>(&object); // Renderer nodes
				const PopcornFX::CParticleNodeTemplateExport	*exportNode = PopcornFX::HBO::Cast<const PopcornFX::CParticleNodeTemplateExport>(&object); // Material nodes (default values on renderers)

				PopcornFX::Nodegraph::EDataType	dataType = PopcornFX::Nodegraph::__MaxDataTypes;
				if (sourcePin != null)
					dataType = sourcePin->Type();
				else if (exportNode != null)
					dataType = exportNode->ExportedType();

				switch (dataType)
				{
				case	PopcornFX::Nodegraph::DataType_DataGeometry:
					return EPopcornFXAssetDepType::Mesh;
				case	PopcornFX::Nodegraph::DataType_DataImage:
					return EPopcornFXAssetDepType::Texture;
				case	PopcornFX::Nodegraph::DataType_DataImageAtlas:
					return EPopcornFXAssetDepType::TextureAtlas;
				case	PopcornFX::Nodegraph::DataType_DataAudio:
					return EPopcornFXAssetDepType::Sound;
				case	PopcornFX::Nodegraph::DataType_DataAnimPath:
					return EPopcornFXAssetDepType::Mesh;
				default:
					break;
				}
			}
		}
	}
	return EPopcornFXAssetDepType::None;
	}

	//----------------------------------------------------------------------------

	struct SDependencyAppend
	{
		UPopcornFXEffect					*m_Self;
		TArray<UPopcornFXAssetDep*>			*m_OldAssetDeps;
		TArray<UPopcornFXAssetDep*>			*m_AssetDeps;

		void	_CbDependency(const PopcornFX::CString &filePath, const PopcornFX::CBaseObject &hbo, u32 fieldIndex)
		{
			TArray<UPopcornFXAssetDep*>		&assetDeps = *m_AssetDeps;
			TArray<UPopcornFXAssetDep*>		&oldAssetDeps = *m_OldAssetDeps;

			if (filePath.Empty())
				return;
			FString	cleanFilePath = ToUE(filePath);

			const PopcornFX::HBO::CFieldDefinition			&fieldDef = hbo.GetFieldDefinition(fieldIndex);
			const PopcornFX::HBO::CFieldAttributesBase		&fieldAttribs = fieldDef.GetBaseAttributes();
			const EPopcornFXAssetDepType::Type				type = PkToAssetDepType(hbo, fieldAttribs.m_Caracs);

			const bool			supported = (AssetDepTypeToClass(type) != null);
			if (!supported)
			{
				if (type != EPopcornFXAssetDepType::None)
				{
					UE_LOG(LogPopcornFXEffect, Warning, TEXT("Asset is not supported (yet): '%s' (in %s)"), *cleanFilePath, *(m_Self->GetPathName()));
				}
				return;
			}

			FString				basePath = cleanFilePath;
			if (basePath.EndsWith(TEXT(".pkmm")))
				basePath = basePath.LeftChop(5) + TEXT(".FBX");
			else if (basePath.EndsWith(TEXT(".pkan")))
				basePath = basePath.LeftChop(5) + TEXT(".FBX");
			else if (basePath.EndsWith(TEXT(".ttf")) || basePath.EndsWith(TEXT(".otf")))
				basePath = basePath.LeftChop(4) + TEXT(".pkfm");

			{
				// This is necessary to check if the path is an inline animtrack sampler
				const PopcornFX::CParticleNodePinIn				*pin = PopcornFX::HBO::Cast<const PopcornFX::CParticleNodePinIn>(&hbo);
				const PopcornFX::CParticleNodeTemplateExport	*exportNode = PopcornFX::HBO::Cast<const PopcornFX::CParticleNodeTemplateExport>(&hbo); // Material nodes (default values on renderers)

				const bool	isInlineAnimTrack = (pin != null && pin->Type() == PopcornFX::Nodegraph::DataType_DataAnimPath) || (exportNode != null && exportNode->Type() == PopcornFX::Nodegraph::DataType_DataAnimPath);
				if ((PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_AnimTrack>(&hbo) != null || PopcornFX::HBO::Cast<const PopcornFX::CParticleNodeSamplerData_AnimTrack>(&hbo) || isInlineAnimTrack) && basePath.EndsWith(TEXT(".fbx")))
				{
					basePath = basePath.LeftChop(4) + TEXT(".pkan");
				}
			}

			FString				sourcePathOrNot;
			FString				importPath;

			// Sanitize path
			// resolve Conflicts
			{
				FString		dir, name, ext;
				FPaths::Split(basePath, dir, name, ext);

				dir = SanitizeObjectPath(dir);
				name = SanitizeObjectPath(name);
				ext = SanitizeObjectPath(ext);

				importPath = dir / name + TEXT(".") + ext;
				UPopcornFXAssetDep::RenameIfCollision(importPath, type, assetDeps);

				// if the importPath differs from sourcePath, set sourcePathOrNot
				if (importPath != basePath)
					sourcePathOrNot = basePath;
				// leave sourcePathOrNot empty if not changed
			}

			for (int32 i = 0; i < oldAssetDeps.Num(); ++i)
			{
				if (oldAssetDeps[i] != null && oldAssetDeps[i]->Match(importPath, type))
				{
					// found the old one
					UPopcornFXAssetDep		*dep = oldAssetDeps[i];
					oldAssetDeps.RemoveAt(i);

					dep->ClearPatches();

					dep->AddFieldToPatch(hbo.UID(), fieldDef.m_Name);
					assetDeps.Add(dep);
					return;
				}
			}

			for (int32 i = 0; i < assetDeps.Num(); ++i)
			{
				if (PK_VERIFY(assetDeps[i] != null) && assetDeps[i]->Match(importPath, type))
				{
					// import path already exists
					assetDeps[i]->AddFieldToPatch(hbo.UID(), fieldDef.m_Name);
					return;
				}
			}

			UPopcornFXAssetDep		*dep = NewObject<UPopcornFXAssetDep>(m_Self);

			if (dep->Setup(m_Self, sourcePathOrNot, importPath, type))
			{
				dep->AddFieldToPatch(hbo.UID(), fieldDef.m_Name);
				assetDeps.Add(dep);
			}
			else
			{
				UE_LOG(LogPopcornFXEffect, Warning, TEXT("Asset Dependency cannot be resolved: '%s' in effect '%s'"), *importPath, *(m_Self->GetPathName()));
				dep->ConditionalBeginDestroy();
				dep = null;
			}
		}
	};

	//----------------------------------------------------------------------------

#if (PK_COMPILE_GPU != 0)
	bool	_SetupBakeBackendCompilers(	PopcornFX::POvenBakeConfig_Particle		&bakeConfig,
										const SBackendAndBuildTags				&backendAndBuildTags,
										const PopcornFX::CString				&fileSourceVirtualPath)
	{
		UPopcornFXSettingsEditor	*settings = GetMutableDefault<UPopcornFXSettingsEditor>();
		if (!PK_VERIFY(settings != null))
			return false;

		PK_ASSERT(bakeConfig != null);
		PopcornFXCbBuildBytecodeArray									backendCompilerCbCompiles;
		PopcornFX::COvenBakeConfig_Particle::_TypeOfBackendCompilers	backendCompilers;

#if (PK_COMPILE_GPU_PC != 0)
		PopcornFXPlatform_PC_SetupGPUBackendCompilers(fileSourceVirtualPath, backendAndBuildTags.m_BackendTargets, backendCompilerCbCompiles);
#endif // (PK_COMPILE_GPU_PC != 0)
#if (PK_COMPILE_GPU_XBOX_ONE != 0)
		PopcornFXPlatform_XboxOne_UNKNOWN_SetupGPUBackendCompilers(fileSourceVirtualPath, backendAndBuildTags.m_BackendTargets, backendCompilerCbCompiles);
#endif // (PK_COMPILE_GPU_XBOX_ONE != 0)
#if (PK_COMPILE_GPU_UNKNOWN != 0)
		PopcornFXPlatform_UNKNOWN1eries_SetupGPUBackendCompilers(fileSourceVirtualPath, backendAndBuildTags.m_BackendTargets, backendCompilerCbCompiles);
#endif // (PK_COMPILE_GPU_UNKNOWN != 0)
#if (PK_COMPILE_GPU_UNKNOWN2 != 0)
		PopcornFXPlatform_UNKNOWN2_SetupGPUBackendCompilers(fileSourceVirtualPath, backendCompilers);
#endif // (PK_COMPILE_GPU_UNKNOWN2 != 0)

		for (const auto &cbCompile : backendCompilerCbCompiles)
		{
			PopcornFX::POvenBakeConfig_ParticleCompiler	backendCompiler = bakeConfig->Context()->NewObject<PopcornFX::COvenBakeConfig_ParticleCompiler>(bakeConfig->File());
			if (!PK_VERIFY(backendCompiler != null) ||
				!PK_VERIFY(backendCompilers.PushBack(backendCompiler).Valid()))
				return false;
			backendCompiler->SetTarget(cbCompile.m_Target);
			backendCompiler->m_CbBuildBytecode_Windows = cbCompile.m_CbBuildBytecode;
		}
		bakeConfig->SetBackendCompilers(backendCompilers);
		return true;
	}
#endif // (PK_COMPILE_GPU != 0)

} // namespace

//----------------------------------------------------------------------------

bool	UPopcornFXEffect::_ImportFile(const FString &filePath)
{
	ClearEffect();

	if (!Super::_ImportFile(filePath))
		return false;
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXEffect::_BakeFile(const FString &srcFilePath, FString &outBakedFilePath, bool forEditor, const FString &targetPlatformName)
{
	if (FileSourceVirtualPathIsNotVirtual != 0)
	{
		PK_ASSERT_NOT_REACHED();
		// TODO: How can we bake effects without a pack ?
		return false;
	}

	const s32								qualityLevelCount = Scalability::GetQualityLevelCounts().EffectsQuality;
	PopcornFX::TArray<PopcornFX::CString>	targetBuildVersions;
	if (!PK_VERIFY(qualityLevelCount >= 0) ||
		!PK_VERIFY(targetBuildVersions.Reserve(qualityLevelCount)))
		return false;

	// Build the final build versions (one per UE runtime configuration) based on base tags
	SBackendAndBuildTags	backendAndBuildTags;
	UEPlatformToPkBackendConfig(targetPlatformName, backendAndBuildTags);
	if (!PK_VERIFY(!backendAndBuildTags.m_BuildTags.Empty()))
		return false; // unrecognized platform
	for (s32 i = 0; i < qualityLevelCount; ++i)
		PK_VERIFY(targetBuildVersions.PushBack(PopcornFX::CString::Format("%s:%s%s", kQualityLevelNames[i].Data(), backendAndBuildTags.m_BuildTags.Data(), kQualityLevelNames[i].Data())).Valid());

	const TCHAR		*targetPlatformNameForLog = forEditor ? UTF8_TO_TCHAR("UnrealEditor") : *targetPlatformName;

	UE_LOG(LogPopcornFXEffect, Display, TEXT("Begin effect baking '%s' for '%s'..."), *GetPathName(), targetPlatformNameForLog);

	FString		bakeDirPath; // dst bake dir
	FString		resolvedSrcFilePath; // src file path
	if (forEditor)
	{
		// Editor import: bake /c/Project/Particles/Toto.pkfx into /c/UEProject/Content/Toto.uasset
		// We want an intermediate text baked file in /c/UEProject/Saved/PopcornFX/Baked/Toto.pkfx
		// After _BakeFile, this gets serialized into /c/UEProject/Content/Toto.uasset
		const FString	kPopcornBakePath = TEXT("Saved/PopcornFX/Baked/");
		bakeDirPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / kPopcornBakePath);

		outBakedFilePath = bakeDirPath / FileSourceVirtualPath; // absolute path (C:/..)
		resolvedSrcFilePath = FileSourceVirtualPath;
	}
	else
	{
		// Editor cooking: bake /c/UEProject/Content/Toto.uasset into itself (for cooking)
		outBakedFilePath = srcFilePath; // UE content absolute path (/Game/..)
		bakeDirPath = FPaths::GetPath(outBakedFilePath);

		// Here srcFilePath is in the /Game/Path/Toto.Toto form. We want Path/Toto.pkfx to trick the asset baker as ovens are extensions based
		resolvedSrcFilePath = PkPath();
	}

	PopcornFX::CCookery				cookery;
	const PopcornFX::SBakeTarget	defaultTarget("UE_Generic", ToPk(bakeDirPath));

	if (FPopcornFXPlugin::Get().Settings() == null) // Will trigger a load of settings if necessary
		return false;

	PopcornFX::HBO::CContext	*context = null;
	if (forEditor)
	{
		// Import effect: here we want the bake context, loading the source pack
		context = FPopcornFXPlugin::Get().BakeContext(); // If first import, BakeContext gets lazy init
		if (!PK_VERIFY(context != null))
			return false;
	}
	else
	{
		// Baked effect while cooking: here we want the UE hbo context, using UE's file system
		context = PopcornFX::HBO::g_Context;
	}

	cookery.SetHBOContext(context); // init cookery with correct context
	if (!cookery.TurnOn())
	{
		UE_LOG(LogPopcornFXEffect, Warning, TEXT("Couldn't bake effect '%s'."), *GetPathName());
		return false;
	}

	const PopcornFX::CGuid	ovenIdParticle = cookery.RegisterOven(PK_NEW(PopcornFX::COven_Particle)); // Only effects need to be baked
	if (!ovenIdParticle.Valid())
		return false;
	cookery.MapOven("pkfx", ovenIdParticle);
	cookery.m_DstPackPaths.PushBack(defaultTarget);
	cookery.AddOvenFlags(PopcornFX::COven::Flags_BakeMemoryVersion);

	if (!forEditor)
		cookery.AddOvenFlags(PopcornFX::COven::Flags_ForcedOutputPath);

	PopcornFX::PBaseObjectFile	configFile = cookery.m_BaseConfigFile;
	if (!PK_VERIFY(configFile != null))
	{
		UE_LOG(LogPopcornFXEffect, Warning, TEXT("Couldn't bake effect '%s': invalid config file"), *GetPathName());
		return false;
	}
	configFile->DeleteAllObjects();

	// Single config object for particles
	PopcornFX::POvenBakeConfig_Particle	bakeConfig = context->NewObject<PopcornFX::COvenBakeConfig_Particle>(configFile.Get());
	if (!PK_VERIFY(bakeConfig != null))
	{
		UE_LOG(LogPopcornFXEffect, Warning, TEXT("Couldn't bake effect '%s': failed create bake configuration"), *GetPathName());
		return false;
	}

	bakeConfig->SetSourceConfig((forEditor) ? PopcornFX::Bake_StandaloneSource : PopcornFX::Bake_NoSource);
	bakeConfig->SetCompile(true);
	bakeConfig->SetRemoveEditorNodes(true);

	if (forEditor)
		bakeConfig->SetBakeMode(PopcornFX::COvenBakeConfig_HBO::Bake_SaveAsText);
	else
	{
		bakeConfig->SetBakeMode(FPopcornFXPlugin::Get().SettingsEditor()->bDebugBakedEffects ? PopcornFX::COvenBakeConfig_HBO::Bake_SaveAsText : PopcornFX::COvenBakeConfig_HBO::Bake_SaveAsBinary); // "Final" bake when cooking: store as binary
		// GOREFIX: #12621 - UnrealEngine: texture sampling does not work in packaged games
		bakeConfig->SetCompilerSwitches("--no-sam-cfl");
	}

	// build versions
	PopcornFX::COvenBakeConfig_Particle::_TypeOfBuildVersions	buildVersions;
	buildVersions = targetBuildVersions;
	bakeConfig->SetBuildVersions(buildVersions);

#if (PK_COMPILE_GPU != 0)
	if (backendAndBuildTags.m_BackendTargets > 0)
	{
		if (!_SetupBakeBackendCompilers(bakeConfig, backendAndBuildTags, ToPk(FileSourceVirtualPath)))
		{
			UE_LOG(LogPopcornFXEffect, Warning, TEXT("Couldn't bake effect '%s': failed setting up GPU backend compilers"), *GetPathName());
			return false;
		}
	}
#endif // (PK_COMPILE_GPU != 0)

	// Nothing to remap
	const PopcornFX::COvenBakeConfig_HBO::_TypeOfExtensionsRemap	extensionsRemap;
	bakeConfig->SetExtensionsRemap(extensionsRemap);

	PopcornFX::CMessageStream	bakerErrors;
	if (!cookery.BakeAsset(ToPk(resolvedSrcFilePath), configFile, bakerErrors))
	{
		UE_LOG(LogPopcornFXEffect, Warning, TEXT("Failed baking effect '%s' for '%s'%s"), *GetPathName(), targetPlatformNameForLog, !bakerErrors.Messages().Empty() ? TEXT(":") : TEXT("."));
		for (const auto &message : bakerErrors.Messages())
		{
			if (message.m_Level >= PopcornFX::CMessageStream::Warning)
			{
				const PopcornFX::CString	messageText = message.ToString();
				UE_LOG(LogPopcornFXEffect, Warning, TEXT("	- %s"), *ToUE(messageText));
			}
		}
		return false;
	}
	else if (FPopcornFXPlugin::Get().SettingsEditor()->bDebugBakedEffects)
	{
		for (const auto& message : bakerErrors.Messages())
		{
			if (message.m_Level >= PopcornFX::CMessageStream::Warning)
			{
				const PopcornFX::CString	messageText = message.ToString();
				UE_LOG(LogPopcornFXEffect, Warning, TEXT("	- %s"), *ToUE(messageText));
			}
		}
	}

	UE_LOG(LogPopcornFXEffect, Display, TEXT("Successfully baked effect '%s' for '%s'..."), *GetPathName(), targetPlatformNameForLog);
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXEffect::FinishImport()
{
	if (!LoadEffect(true))
		return false;

	TArray<UPopcornFXAssetDep*>		oldAssetDeps = AssetDependencies;
	AssetDependencies.Empty();

	// Re-patch assets paths from Saved overrided AssetDependencies

	SDependencyAppend		dep;
	dep.m_Self = this;
	dep.m_AssetDeps = &AssetDependencies;
	dep.m_OldAssetDeps = &oldAssetDeps;

	PopcornFX::PBaseObjectFile		bofile = m_Private->m_ParticleEffect->File();
	PopcornFX::GatherDependencies(PkPath(), PopcornFX::CbDependency(&dep, &SDependencyAppend::_CbDependency));
	for (int32 assetDepi = 0; assetDepi < AssetDependencies.Num(); ++assetDepi)
	{
		if (PK_VERIFY(AssetDependencies[assetDepi] != null))
		{
			AssetDependencies[assetDepi]->PatchFields(this);
			AssetDependencies[assetDepi]->SetFlags(RF_Transactional);
		}
	}

	bofile->Write();

	FPopcornFXPlugin::Get().UnloadPkFile(this); // Unload loaded effect

	ReloadRendererMaterials();

	return true;
}

//----------------------------------------------------------------------------

void	UPopcornFXEffect::OnAssetDepChanged(UPopcornFXAssetDep *assetDep, UObject* oldAsset, UObject* newAsset)
{
	if (oldAsset != null &&
		oldAsset->GetClass() == newAsset->GetClass())
	{
		// Before re-resolving dependencies & materials, refresh existing materials to avoid resetting them to default values
		for (UPopcornFXRendererMaterial* mat : ParticleRendererMaterials)
		{
			if (PK_VERIFY(mat != null))
				mat->RenameDependenciesIFN(oldAsset, newAsset);
		}
	}

	ReloadFile();

	ReloadRendererMaterials();

	ReloadFile();
}

//----------------------------------------------------------------------------

void	UPopcornFXEffect::ReloadRendererMaterials()
{
	// Gather renderers
	if (!LoadEffect())
		return;

	{
		TArray<UPopcornFXRendererMaterial*>					oldMaterials = ParticleRendererMaterials;
		ParticleRendererMaterials.Empty(oldMaterials.Num());

		PopcornFX::PCEventConnectionMap	ecMap = m_Private->m_ParticleEffect->EventConnectionMap();
		if (!PK_VERIFY(ecMap != null))
			return;
		s32				globalRendererIndex = -1;
		const u32		layerCount = ecMap->m_LayerSlots.Count();
		for (u32 iLayer = 0; iLayer < layerCount; ++iLayer)
		{
			const PopcornFX::PParticleDescriptor	desc = ecMap->m_LayerSlots[iLayer].m_ParentDescriptor;
			if (!PK_VERIFY(desc != null))
				return;
			PopcornFX::TMemoryView<const PopcornFX::PRendererDataBase>	renderers = desc->Renderers();
			const u32													rendererCount = renderers.Count();
			for (u32 iRenderer = 0; iRenderer < rendererCount; ++iRenderer)
			{
				globalRendererIndex++;

				const PopcornFX::CRendererDataBase	*rBase = renderers[iRenderer].Get();
				if (!PK_VERIFY(rBase != null))
					return;

				UPopcornFXRendererMaterial	*newMat = NewObject<UPopcornFXRendererMaterial>(this);
				if (!newMat->Setup(this, rBase, globalRendererIndex))
				{
					newMat->ConditionalBeginDestroy();
					newMat = null;
					continue;
				}

				// search existing renderer materials
				for (int32 mati = 0; mati < ParticleRendererMaterials.Num(); ++mati)
				{
					UPopcornFXRendererMaterial			*otherMat = ParticleRendererMaterials[mati];
					if (PK_VERIFY(otherMat != null) &&
						newMat->CanBeMergedWith(otherMat))
					{
						otherMat->Add(this, globalRendererIndex);
						newMat = null;
						break;
					}
				}

				if (newMat == null) // has been inserted
					continue;

				// search old renderer materials
				for (int32 oldi = 0; oldi < oldMaterials.Num(); ++oldi)
				{
					UPopcornFXRendererMaterial	*oldMat = oldMaterials[oldi];
					if (oldMat != null &&
						newMat->CanBeMergedWith(oldMat))
					{
						oldMat->Setup(this, rBase, globalRendererIndex);
						oldMaterials[oldi] = null;
						ParticleRendererMaterials.Add(oldMat);
						newMat = null;
						break;
					}
				}

				if (newMat != null)
				{
					ParticleRendererMaterials.Add(newMat);
					newMat = null;
				}
			}
		}
	}
	// Trigger everything to rebuild renderer drawers
	for (int32 mati = 0; mati < ParticleRendererMaterials.Num(); ++mati)
	{
		ParticleRendererMaterials[mati]->ReloadInstances();
		ParticleRendererMaterials[mati]->SetFlags(RF_Transactional);
	}

	// TODO: Sanity pass asserting all renderer indices are unique

	MarkPackageDirty();
	if (GetOuter())
		GetOuter()->MarkPackageDirty();

	m_OnFileChanged.Broadcast(this);
}

#endif // WITH_EDITOR
//----------------------------------------------------------------------------

uint64	UPopcornFXEffect::CurrentCacherVersion() const
{
	//
	// Effect cacher version contains:
	// - PopcornFX Runtime major minor patch (NOT revid !)
	// - EPopcornFXEffectCacherVersion
	//
	// NOT revid because we don't want to cache at every PopcornFX Runtime commit !
	// Use EPopcornFXEffectCacherVersion to outdate cache when MajMinPatch did not changed
	//

	const uint16	runtimeVer = FPopcornFXPlugin::PopornFXRuntimeMajMinPatch();
	const uint32	runtimeVerShift = (sizeof(uint64) - sizeof(runtimeVer)) * 8;

	enum EPopcornFXEffectCacherVersion
	{
		FirstVersion = 0,

		// Import was missing a rewrite of the file to fix \r\n to \n
		FixCRLFtoLF,

		// Here, add versions to outdate Effect cache

		//----
		_LastVersion_PlusOne,
		_LastVersion = _LastVersion_PlusOne - 1,
	};

	uint64	cacherVersion = 0;
	cacherVersion |= (uint64(runtimeVer) << runtimeVerShift);
	cacherVersion |= _LastVersion;
	return cacherVersion;
}

//----------------------------------------------------------------------------

bool	UPopcornFXEffect::LaunchCacher()
{
	if (!LoadEffect())
		return false;
	return true;
}

//----------------------------------------------------------------------------

void	UPopcornFXEffect::OnFileUnload()
{
	ClearEffect();
	Super::OnFileUnload();
}

//----------------------------------------------------------------------------

void	UPopcornFXEffect::OnFileLoad()
{
	Super::OnFileLoad();
	LoadEffect();
}

//----------------------------------------------------------------------------
