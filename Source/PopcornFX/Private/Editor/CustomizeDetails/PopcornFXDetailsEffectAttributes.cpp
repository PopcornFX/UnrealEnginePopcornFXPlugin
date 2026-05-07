//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsEffectAttributes.h"

#include "DetailLayoutBuilder.h"

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEffectAttributes::CustomizeDetails(class IDetailLayoutBuilder& DetailLayout)
{
	// calling EditCategory reorder Categories in the Editor
	DetailLayout.HideCategory("PopcornFX RendererMaterials");
	DetailLayout.HideCategory("PopcornFX AssetDependencies");
	DetailLayout.HideCategory("Source");
	DetailLayout.HideCategory("UserDatas");
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
