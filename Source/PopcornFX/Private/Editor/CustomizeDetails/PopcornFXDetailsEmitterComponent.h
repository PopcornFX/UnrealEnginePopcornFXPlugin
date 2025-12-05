//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "PopcornFXDetailsAttributeList.h"
#include "Runtime/Launch/Resources/Version.h"
#include "PropertyEditorModule.h"

#include "IDetailCustomization.h"

class UPopcornFXEmitterComponent;
class UPopcornFXAttributeList;
class UPopcornFXEffect;
class IDetailGroup;
class IPropertyHandleArray;
class IDetailCategoryBuilder;
struct FPopcornFXAttributeDesc;
struct FPopcornFXSamplerDesc;

class FPopcornFXDetailsEmitterComponent : public FPopcornFXDetailsAttributeList
{
public:
	FPopcornFXDetailsEmitterComponent();

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization>	MakeInstance();

	virtual void			CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

private:
	void					BuildSampler(const FPopcornFXSamplerDesc *desc, const TSharedPtr<IPropertyHandle> samplerPty, const TSharedPtr<IPropertyHandle> samplerDescPty, const UPopcornFXAttributeList *attrList, uint32 sampleri, uint32 iCategory) override;

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
};

#endif // WITH_EDITOR
