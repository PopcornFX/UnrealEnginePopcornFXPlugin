//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDependencyModuleAssetTools.h"

#include "PopcornFXPlugin.h"
#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXFont.h"
#include "Assets/PopcornFXAnimTrack.h"
#include "Assets/PopcornFXSimulationCache.h"
#include "Assets/PopcornFXTextureAtlas.h"

#include "AssetToolsModule.h"
#include "Editor/PopcornFXTypeActions.h"
#include "Editor/PopcornFXStyle.h"

#include "PopcornFXSDK.h"


#define LOCTEXT_NAMESPACE "PopcornFXAssetTools"

//----------------------------------------------------------------------------

FPopcornFXDependencyModuleAssetTools::FPopcornFXDependencyModuleAssetTools()
{
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleAssetTools::Load()
{
	if (m_Loaded)
		return;
	if (!PK_VERIFY(FModuleManager::Get().IsModuleLoaded("AssetTools")))
		return;

	IAssetTools	&assetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

	m_PopcornFXAssetCategoryBit = assetTools.RegisterAdvancedAssetCategory(FName(TEXT("PopcornFX")), LOCTEXT("PopcornFXAssetCategory", "PopcornFX"));

#if (ENGINE_MAJOR_VERSION == 5)
	FColor			pkColor = FPopcornFXPlugin::Color_PopcornFX().QuantizeRound();
#else
	FColor			pkColor = FPopcornFXPlugin::Color_PopcornFX().Quantize();
#endif // (ENGINE_MAJOR_VERSION == 5)

	m_EffectActions = MakeShareable(new FPopcornFXFileTypeActions(m_PopcornFXAssetCategoryBit));
	m_EffectActions->m_SupportedClass = UPopcornFXEffect::StaticClass();
	m_EffectActions->m_Name = NSLOCTEXT("AssetTypeActions", "PopcornFXEffectName", "PopcornFX Effect");
	m_EffectActions->m_Color = pkColor;
	assetTools.RegisterAssetTypeActions(m_EffectActions.ToSharedRef());

	m_FontMetricsActions = MakeShareable(new FPopcornFXFileTypeActions(m_PopcornFXAssetCategoryBit));
	m_FontMetricsActions->m_SupportedClass = UPopcornFXFont::StaticClass();
	m_FontMetricsActions->m_Name = NSLOCTEXT("AssetTypeActions", "PopcornFXFontName", "PopcornFX Font");
	m_FontMetricsActions->m_Color = pkColor;
	assetTools.RegisterAssetTypeActions(m_FontMetricsActions.ToSharedRef());

	m_AnimTrackActions = MakeShareable(new FPopcornFXFileTypeActions(m_PopcornFXAssetCategoryBit));
	m_AnimTrackActions->m_SupportedClass = UPopcornFXAnimTrack::StaticClass();
	m_AnimTrackActions->m_Name = NSLOCTEXT("AssetTypeActions", "PopcornFXAnimTrackName", "PopcornFX Animation Track");
	m_AnimTrackActions->m_Color = pkColor;
	assetTools.RegisterAssetTypeActions(m_AnimTrackActions.ToSharedRef());

	m_SimulationCacheActions = MakeShareable(new FPopcornFXFileTypeActions(m_PopcornFXAssetCategoryBit));
	m_SimulationCacheActions->m_SupportedClass = UPopcornFXSimulationCache::StaticClass();
	m_SimulationCacheActions->m_Name = NSLOCTEXT("AssetTypeActions", "PopcornFXSimCacheName", "PopcornFX Simulation Cache");
	m_SimulationCacheActions->m_Color = pkColor;
	assetTools.RegisterAssetTypeActions(m_SimulationCacheActions.ToSharedRef());

	m_TextureAtlasActions = MakeShareable(new FPopcornFXFileTypeActions(m_PopcornFXAssetCategoryBit));
	m_TextureAtlasActions->m_SupportedClass = UPopcornFXTextureAtlas::StaticClass();
	m_TextureAtlasActions->m_Name = NSLOCTEXT("AssetTypeActions", "PopcornFXTextureAtlasName", "PopcornFX Texture Atlas");
	m_TextureAtlasActions->m_Color = pkColor;
	assetTools.RegisterAssetTypeActions(m_TextureAtlasActions.ToSharedRef());

	m_FileActions = MakeShareable(new FPopcornFXFileTypeActions(m_PopcornFXAssetCategoryBit));
	m_FileActions->m_SupportedClass = UPopcornFXFile::StaticClass();
	m_FileActions->m_Name = NSLOCTEXT("AssetTypeActions", "PopcornFXFileName", "PopcornFX File");
	m_FileActions->m_Color = pkColor;
	assetTools.RegisterAssetTypeActions(m_FileActions.ToSharedRef());

	m_Loaded = true;
	FPopcornFXStyle::Initialize();
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleAssetTools::Unload()
{
	if (!m_Loaded)
		return;
	m_Loaded = false;

	if (/*PK_VERIFY*/(FModuleManager::Get().IsModuleLoaded("AssetTools"))) // @FIXME: we should be called when the plugin is still loaded
	{
		IAssetTools	&assetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		if (m_EffectActions.IsValid())
		{
			assetTools.UnregisterAssetTypeActions(m_EffectActions.ToSharedRef());
			m_EffectActions = null;
		}
		if (m_FontMetricsActions.IsValid())
		{
			assetTools.UnregisterAssetTypeActions(m_FontMetricsActions.ToSharedRef());
			m_FontMetricsActions = null;
		}
		if (m_AnimTrackActions.IsValid())
		{
			assetTools.UnregisterAssetTypeActions(m_AnimTrackActions.ToSharedRef());
			m_AnimTrackActions = null;
		}
		if (m_SimulationCacheActions.IsValid())
		{
			assetTools.UnregisterAssetTypeActions(m_SimulationCacheActions.ToSharedRef());
			m_SimulationCacheActions = null;
		}
		if (m_TextureAtlasActions.IsValid())
		{
			assetTools.UnregisterAssetTypeActions(m_TextureAtlasActions.ToSharedRef());
			m_TextureAtlasActions = null;
		}
		if (m_FileActions.IsValid())
		{
			assetTools.UnregisterAssetTypeActions(m_FileActions.ToSharedRef());
			m_FileActions = null;
		}
	}
	FPopcornFXStyle::Shutdown();
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
