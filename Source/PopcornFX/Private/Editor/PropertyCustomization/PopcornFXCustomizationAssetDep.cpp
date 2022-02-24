//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXCustomizationAssetDep.h"

#include "PopcornFXPlugin.h"
#include "Assets/PopcornFXAssetDep.h"

#include "Editor/EditorHelpers.h"

#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "PropertyCustomizationHelpers.h"
#include "AssetThumbnail.h"
#include "AssetData.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "ContentBrowserDelegates.h"

#define LOCTEXT_NAMESPACE "PopcornFXCustomizationAssetDep"

// static
TSharedRef<IPropertyTypeCustomization> FPopcornFXCustomizationAssetDep::MakeInstance()
{
	return MakeShareable(new FPopcornFXCustomizationAssetDep);
}

class UPopcornFXAssetDep	*FPopcornFXCustomizationAssetDep::Self() const
{
	if (!m_SelfPty->IsValidHandle())
		return null;
	UObject				*_self = null;
	m_SelfPty->GetValue(_self);
	UPopcornFXAssetDep	*self = Cast<UPopcornFXAssetDep>(_self);
	return self;
}

void	FPopcornFXCustomizationAssetDep::CustomizeHeader(
	TSharedRef<IPropertyHandle> PropertyHandle,
	class FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	m_SelfPty = PropertyHandle;

	TSharedRef<IPropertyHandle>		importPath = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAssetDep, ImportPath)).ToSharedRef();
	check(importPath->IsValidHandle());

	TSharedRef<IPropertyHandle>		sourcePath = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAssetDep, SourcePath)).ToSharedRef();
	check(sourcePath->IsValidHandle());

	TSharedRef<IPropertyHandle>		type = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAssetDep, Type)).ToSharedRef();
	check(type->IsValidHandle());

	TSharedRef<IPropertyHandle>		asset = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAssetDep, Asset)).ToSharedRef();
	check(asset->IsValidHandle());
	m_AssetPty = asset;

	UPopcornFXAssetDep		*self = Self();
	if (!PK_VERIFY(self != null))
		return;

	EVisibility		importDefVis = self->FindDefaultAsset() == null ? EVisibility::Visible : EVisibility::Hidden;

	// Source\Editor\PropertyEditor\Private\UserInterface\PropertyEditor\PropertyEditorConstants.cpp
	static const FName		PropertyFontStyle( TEXT("PropertyWindow.NormalFont") );

	FSlateFontInfo			FontStyle = FEditorStyle::GetFontStyle(PropertyFontStyle);

	HeaderRow.NameContent()
		.MinDesiredWidth(125.f * 3.f)
		.MaxDesiredWidth(125.f * 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[type->CreatePropertyValueWidget()]
		]
	.ValueContent()
		.MinDesiredWidth(125.f * 3.f)
		.MaxDesiredWidth(125.f * 4.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SObjectPropertyEntryBox)
					.PropertyHandle(asset)
					.ThumbnailPool(CustomizationUtils.GetThumbnailPool())
					.OnShouldFilterAsset(this, &FPopcornFXCustomizationAssetDep::OnFilterAssetPicker)
				]
				+ SHorizontalBox::Slot()
				.Padding(1.f)
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.OnClicked(this, &FPopcornFXCustomizationAssetDep::OnResetClicked)
					.Visibility(this, &FPopcornFXCustomizationAssetDep::GetResetVisibility)
					.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset to Default"))
					.ButtonStyle(FEditorStyle::Get(), "NoBorder")
					.Content()
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SEditableTextBox)
				.Font(FontStyle)
				.IsReadOnly(true)
				.Text(this, &FPopcornFXCustomizationAssetDep::GetCurrentAssetPath)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &FPopcornFXCustomizationAssetDep::OnImportDefaultClicked)
					//.Visibility(importDefVis)
					.ToolTipText(LOCTEXT("ImportTooltip", "Import/Reimport default asset"))
					.Content()
					[
						SNew(STextBlock)
						.Font(FontStyle)
						//.Text(LOCTEXT("Reimport Default:", "Reimport Default:"))
						.Text(this, &FPopcornFXCustomizationAssetDep::GetImportButtonText)
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SSpacer)
				]
			]
		];
}

void	FPopcornFXCustomizationAssetDep::CustomizeChildren(
	TSharedRef<class IPropertyHandle> PropertyHandle,
	class IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

bool	FPopcornFXCustomizationAssetDep::OnFilterAssetPicker(const FAssetData& InAssetData) const
{
	UPopcornFXAssetDep		*self = Self();
	if (!PK_VERIFY(self != null))
		return false;
	UClass	*assetClass = InAssetData.GetClass();
	// return filterOut;
	return !self->IsCompatibleClass(assetClass) ||
		!InAssetData.ObjectPath.ToString().StartsWith(FPopcornFXPlugin::Get().Settings()->PackMountPoint);
}

FReply		FPopcornFXCustomizationAssetDep::OnResetClicked()
{
	UPopcornFXAssetDep		*self = Self();
	if (!PK_VERIFY(self != null))
		return FReply::Handled();
	self->SetAsset(self->ParentPopcornFXFile(), self->FindDefaultAsset());
	m_AssetPty->NotifyPostChange();
	return FReply::Handled();
}

EVisibility		FPopcornFXCustomizationAssetDep::GetResetVisibility() const
{
	UPopcornFXAssetDep		*self = Self();
	if (!PK_VERIFY(self != null))
		return EVisibility::Hidden;
	if (self->Asset == null)
		return EVisibility::Visible;
	const FString		defaultAsset = self->GetDefaultAssetPath();
	const FString		current = self->Asset->GetPathName();
	if (defaultAsset != current)
		return EVisibility::Visible;
	return EVisibility::Hidden;
}

FReply	FPopcornFXCustomizationAssetDep::OnImportDefaultClicked()
{
	UPopcornFXAssetDep		*self = Self();
	if (!PK_VERIFY(self != null))
		return FReply::Handled();

	self->ReimportAndResetDefaultAsset(self->ParentPopcornFXFile(), true);

	m_AssetPty->NotifyPostChange();

	return FReply::Handled();
}

FText		FPopcornFXCustomizationAssetDep::GetImportButtonText() const
{
	UPopcornFXAssetDep		*self = Self();
	if (!PK_VERIFY(self != null))
		return LOCTEXT("Reimport", "Reimport");

	const FString		&importPath = self->ImportPath;
	const FString		&sourcePath = self->SourcePath;

	FText		text;
	if (sourcePath.IsEmpty())
	{
		text = FText::Format(LOCTEXT("Reimport FMT", "Reimport '{0}'"), FText::FromString(importPath));
	}
	else
	{
		text = FText::Format(LOCTEXT("Reimport FMT 2", "Reimport '{0}' to '{1}'"), FText::FromString(sourcePath), FText::FromString(importPath));
	}

	return text;
}

FText	FPopcornFXCustomizationAssetDep::GetCurrentAssetPath() const
{
	UPopcornFXAssetDep		*self = Self();
	if (!PK_VERIFY(self != null) || self->Asset == null)
		return FText();
	FString		path;
	self->GetAssetPath(path);
	return FText::FromString(path);
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
