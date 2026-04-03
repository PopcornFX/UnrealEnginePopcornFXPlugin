//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "Runtime/Launch/Resources/Version.h"
#include "PropertyEditorModule.h"

#include "IDetailCustomization.h"

class FPopcornFXDetailsSceneComponent : public IDetailCustomization
{
public:
	FPopcornFXDetailsSceneComponent();

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization>	MakeInstance();

	virtual void			CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

private:
	void					RebuildDetails();

private:
	FReply					OnClear();
	FReply					OnResetMaterials();
	FReply					OnReloadMaterials();

	/** Cached off reference to the layout builder */
	IDetailLayoutBuilder	*m_DetailLayout;
};

#endif // WITH_EDITOR
