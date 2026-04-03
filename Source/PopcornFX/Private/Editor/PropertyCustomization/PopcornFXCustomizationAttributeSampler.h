//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "PopcornFXAttributeList.h"
#include "PopcornFXEmitterComponent.h"
#include "Assets/PopcornFXEffect.h"

#include "PropertyEditorModule.h"
#include "IPropertyTypeCustomization.h"

/**
	Base class for customizing attribute sampler properties
*/
class FPopcornFXCustomizationAttributeSampler : public IPropertyTypeCustomization
{
public: 

	FPopcornFXCustomizationAttributeSampler();

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization>	MakeInstance();

	virtual void	CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void	CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

protected:

	/** Requests a force refresh of the UI */
	void			RebuildProperties();
	/** Updates the emitter of the preview viewport */
	void			UpdatePreviewEmitter();
	/**
		Adds a property with the child builder
		This property can be invalid, according to what the effect using this sampler needs
		In this case, the property will be customized to show that it causes an error and why
	*/
	void			AddErrorableProperty(TSharedPtr<IPropertyHandle> PropertyHandle, const FString &PropertyName, IDetailChildrenBuilder &ChildBuilder, bool editCondition, bool editConditionHides);

private:

	/** Retrieve parents objects that are being edited for this property */
	void			ResolveParents(TSharedRef<IPropertyHandle> PropertyHandle);

protected:

	/**
		The sampler being edited.
		It can be an external/standalone sampler inside any actor,
		an inline sampler of an emitter (UPopcornFXEmitterComponent::Samplers) or an effect (UPopcornFXEffect::DefaultSamplers)
		For the effect/asset preview window, we edit default samplers of the effect directly,
		that we then copy to the emitter of the preview viewport (the one suffixed with " preview").
		We set m_Sampler to this emitter's samplers which is not the one being edited directly but contains the invalid properties
	*/
	UPopcornFXAttributeSampler		*m_Sampler;
	UPopcornFXEmitterComponent		*m_Emitter;
	UPopcornFXEffect				*m_Effect;

	TSharedPtr<IPropertyUtilities>	m_CachedPropertyUtilities;
};

#endif // WITH_EDITOR
