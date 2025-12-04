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

class UPopcornFXEmitterComponent;
class UPopcornFXEffect;

class FPopcornFXDetailsEmitterComponent : public IDetailCustomization
{
public:
	FPopcornFXDetailsEmitterComponent();

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization>	MakeInstance();

	virtual void		CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

	TOptional<float>	OnGetValue(float field) const;
	void				OnSetValue(float newValue, ETextCommit::Type commitInfo, float *field);
	int32				GetVectorDimension(int32 field) const;

private:
	void				GatherEmitters(TArray<UPopcornFXEmitterComponent*> &outComponents) const;
	void				GatherEffects(TArray<UPopcornFXEffect*> &outEffects);

	FReply				OnStartEmitter();
	FReply				OnStopEmitter();
	FReply				OnKillParticles();
	FReply				OnRestartEmitter();
	bool				IsStartEnabled() const;
	bool				IsStopEnabled() const;

	FReply				OnReloadEffect();
	FReply				OnReimportEffect();

	TArray< TWeakObjectPtr<UObject> >	m_SelectedObjectsList;
};

#endif // WITH_EDITOR
