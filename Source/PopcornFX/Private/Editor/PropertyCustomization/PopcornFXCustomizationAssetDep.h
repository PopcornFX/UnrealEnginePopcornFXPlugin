//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "PropertyEditorModule.h"

struct FAssetData;

class FPopcornFXCustomizationAssetDep : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	// overrides IPropertyTypeCustomization
	virtual void	CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void	CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	class UPopcornFXAssetDep		*Self() const;

	bool			OnFilterAssetPicker(const FAssetData& InAssetData) const;
	FReply			OnResetClicked();
	EVisibility		GetResetVisibility() const;
	FReply			OnImportDefaultClicked();
	FText			GetImportButtonText() const;
	FText			GetCurrentAssetPath() const;

	TSharedPtr<IPropertyHandle>		m_SelfPty;
	TSharedPtr<IPropertyHandle>		m_AssetPty;
};

#endif // WITH_EDITOR
