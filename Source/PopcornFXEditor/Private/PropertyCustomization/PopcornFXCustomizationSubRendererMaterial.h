//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "PropertyEditorModule.h"
#include "AssetThumbnail.h"

class FPopcornFXCustomizationSubRendererMaterial : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	// overrides IPropertyTypeCustomization
	virtual void	CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void	CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	struct FPopcornFXSubRendererMaterial		*Self() const;

	static FString	_RendererNameFromType(const int type);

	TSharedPtr<IPropertyHandle>		m_SelfPty;
	TSharedPtr<IPropertyHandle>		m_MaterialPty;
};

#endif // WITH_EDITOR
