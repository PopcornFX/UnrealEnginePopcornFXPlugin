//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#if WITH_EDITOR

#include "PopcornFXDependencyModule.h"
#include "AssetTypeCategories.h"

class	FPopcornFXFileTypeActions;

class POPCORNFX_API FPopcornFXDependencyModuleAssetTools : public FPopcornFXDependencyModule
{
public:
	FPopcornFXDependencyModuleAssetTools();

	virtual void	Load() override;
	virtual void	Unload() override;
protected:
	EAssetTypeCategories::Type				m_PopcornFXAssetCategoryBit;
	TSharedPtr<FPopcornFXFileTypeActions>	m_EffectActions;
	TSharedPtr<FPopcornFXFileTypeActions>	m_FontMetricsActions;
	TSharedPtr<FPopcornFXFileTypeActions>	m_AnimTrackActions;
	TSharedPtr<FPopcornFXFileTypeActions>	m_SimulationCacheActions;
	TSharedPtr<FPopcornFXFileTypeActions>	m_TextureAtlasActions;
	TSharedPtr<FPopcornFXFileTypeActions>	m_FileActions;
};

#endif // WITH_EDITOR
