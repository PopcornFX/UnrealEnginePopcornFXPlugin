//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "IDetailCustomization.h"
#include "Runtime/Launch/Resources/Version.h"
#include "PropertyEditorModule.h"

#include "IDetailCustomization.h"

class UPopcornFXEmitterComponent;
class UPopcornFXEffect;
class IDetailCategoryBuilder;

class FPopcornFXDetailsEmitterComponent : public IDetailCustomization
{
public:
	FPopcornFXDetailsEmitterComponent();

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization>	MakeInstance();

	virtual void			CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

private:
	void					GatherEmitters(TArray<UPopcornFXEmitterComponent *> &outComponents) const;
	void					GatherEffects(TArray<UPopcornFXEffect *> &outEffects);
	FReply					OnStartEmitter();
	FReply					OnStopEmitter();
	FReply					OnKillParticles();
	FReply					OnRestartEmitter();
	FReply					OnReloadEffect();
	FReply					OnReimportEffect();
	bool					IsStartEnabled() const;
	bool					IsStopEnabled() const;
	void					RebuildIFN();

protected:

	TArray<TWeakObjectPtr<UObject> >	m_BeingCustomized;
	TSharedPtr<IPropertyUtilities>		m_PropertyUtilities;
	IDetailCategoryBuilder				*m_AttributeListCategory;
};

#endif // WITH_EDITOR
