//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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
