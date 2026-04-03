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

class FPopcornFXDetailsAttributeSamplerShape : public IDetailCustomization
{
public:
	FPopcornFXDetailsAttributeSamplerShape();

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization>	MakeInstance();

	virtual void		CustomizeDetails(IDetailLayoutBuilder& detailLayout) override;
private:
	void	RebuildDetails();

	IDetailLayoutBuilder	*m_CachedDetailLayoutBuilder;
};

#endif // WITH_EDITOR
