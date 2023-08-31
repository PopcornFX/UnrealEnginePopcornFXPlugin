//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXCustomizationSubRendererMaterial.h"

#include "Assets/PopcornFXRendererMaterial.h"
#include "Editor/EditorHelpers.h"

#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "PropertyCustomizationHelpers.h"
#include "AssetThumbnail.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"

#include "PopcornFXSDK.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXCustomizationSubRendererMaterial, Log, All);
#define LOCTEXT_NAMESPACE "PopcornFXCustomizationRendererMaterial"

// static
TSharedRef<IPropertyTypeCustomization> FPopcornFXCustomizationSubRendererMaterial::MakeInstance()
{
	return MakeShareable(new FPopcornFXCustomizationSubRendererMaterial);
}

struct FPopcornFXSubRendererMaterial	*FPopcornFXCustomizationSubRendererMaterial::Self() const
{
	if (!m_SelfPty->IsValidHandle())
		return null;
	TArray<void*> rawData;
	m_SelfPty->AccessRawData(rawData);
	if (!PK_VERIFY(rawData.Num() == 1))
		return null;
	FPopcornFXSubRendererMaterial	*pself = reinterpret_cast<FPopcornFXSubRendererMaterial*>(rawData[0]);
	return pself;
}

void	FPopcornFXCustomizationSubRendererMaterial::CustomizeHeader(
	TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	m_SelfPty = PropertyHandle;

	TSharedRef<IPropertyHandle>		material = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, Material)).ToSharedRef();
	check(material->IsValidHandle());
	m_MaterialPty = material;

	TSharedRef<IPropertyHandle>		materialType = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, MaterialType)).ToSharedRef();
	check(materialType->IsValidHandle());

	TSharedRef<IPropertyHandle>		noAlpha = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, NoAlpha)).ToSharedRef();
	check(noAlpha->IsValidHandle());

	TSharedRef<IPropertyHandle>		softAnimBlending = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, SoftAnimBlending)).ToSharedRef();
	check(softAnimBlending->IsValidHandle());

	TSharedRef<IPropertyHandle>		motionVectorsBlending = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, MotionVectorsBlending)).ToSharedRef();
	check(motionVectorsBlending->IsValidHandle());

	TSharedRef<IPropertyHandle>		castShadow = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, CastShadow)).ToSharedRef();
	check(castShadow->IsValidHandle());

	TSharedRef<IPropertyHandle>		staticMeshLOD = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, StaticMeshLOD)).ToSharedRef();
	check(staticMeshLOD->IsValidHandle());

	FPopcornFXSubRendererMaterial		*self = Self();
	if (!PK_VERIFY(self != null))
		return;

	m_Thumbs[Thumb_Diffuse].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, TextureDiffuse)).ToSharedRef();
	m_Thumbs[Thumb_Diffuse].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->TextureDiffuse, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_DiffuseRamp].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, TextureDiffuseRamp)).ToSharedRef();
	m_Thumbs[Thumb_DiffuseRamp].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->TextureDiffuseRamp, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_Emissive].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, TextureEmissive)).ToSharedRef();
	m_Thumbs[Thumb_Emissive].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->TextureEmissive, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_AlphaRemap].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, TextureAlphaRemapper)).ToSharedRef();
	m_Thumbs[Thumb_AlphaRemap].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->TextureAlphaRemapper, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_MotionVectors].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, TextureMotionVectors)).ToSharedRef();
	m_Thumbs[Thumb_MotionVectors].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->TextureMotionVectors, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_SixWay_RLTS].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, TextureSixWay_RLTS)).ToSharedRef();
	m_Thumbs[Thumb_SixWay_RLTS].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->TextureSixWay_RLTS, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_SixWay_BBF].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, TextureSixWay_BBF)).ToSharedRef();
	m_Thumbs[Thumb_SixWay_BBF].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->TextureSixWay_BBF, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_VATPosition].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, VATTexturePosition)).ToSharedRef();
	m_Thumbs[Thumb_VATPosition].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->VATTexturePosition, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_VATNormal].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, VATTextureNormal)).ToSharedRef();
	m_Thumbs[Thumb_VATNormal].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->VATTextureNormal, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_VATColor].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, VATTextureColor)).ToSharedRef();
	m_Thumbs[Thumb_VATColor].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->VATTextureColor, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_VATRotation].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, VATTextureRotation)).ToSharedRef();
	m_Thumbs[Thumb_VATRotation].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->VATTextureRotation, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_SkeletalAnimation].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, TextureSkeletalAnimation)).ToSharedRef();
	m_Thumbs[Thumb_SkeletalAnimation].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->TextureSkeletalAnimation, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_Normal].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, TextureNormal)).ToSharedRef();
	m_Thumbs[Thumb_Normal].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->TextureNormal, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_Normal].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, TextureNormal)).ToSharedRef();
	m_Thumbs[Thumb_Normal].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->TextureNormal, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_Specular].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, TextureSpecular)).ToSharedRef();
	m_Thumbs[Thumb_Specular].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->TextureSpecular, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_Mesh].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, StaticMesh)).ToSharedRef();
	m_Thumbs[Thumb_Mesh].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->StaticMesh, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_SkeletalMesh].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, SkeletalMesh)).ToSharedRef();
	m_Thumbs[Thumb_SkeletalMesh].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->SkeletalMesh, 64, 64, CustomizationUtils.GetThumbnailPool()));

	PropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationSubRendererMaterial::OnThumbnailPtyChange, -1));

	TSharedPtr<SHorizontalBox>	thumbnails;

	// TODO: VAT NoAlpha/PackedData?

	HeaderRow.NameContent()
		.MinDesiredWidth(125.f * 3.f)
		.MaxDesiredWidth(125.f * 4.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[materialType->CreatePropertyValueWidget()]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					[noAlpha->CreatePropertyValueWidget()]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					[softAnimBlending->CreatePropertyValueWidget()]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					[motionVectorsBlending->CreatePropertyValueWidget()]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					[castShadow->CreatePropertyValueWidget()]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					[staticMeshLOD->CreatePropertyValueWidget()]
			]
			+ SVerticalBox::Slot()
				[
					SAssignNew(thumbnails, SHorizontalBox)
				]
		]
	.ValueContent()
		.MinDesiredWidth(125.f * 3.f)
		.MaxDesiredWidth(125.f * 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				//material->CreatePropertyValueWidget()
				SNew(SObjectPropertyEntryBox)
				.PropertyHandle(material)
				.AllowedClass(UMaterialInterface::StaticClass())
				.ThumbnailPool(CustomizationUtils.GetThumbnailPool())
			]
			+ SHorizontalBox::Slot()
				.Padding(1.f)
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.OnClicked(this, &FPopcornFXCustomizationSubRendererMaterial::OnResetClicked)
					// too slow
					.Visibility(this, &FPopcornFXCustomizationSubRendererMaterial::GetResetVisibility)
					.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset to Default"))
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
#else
					.ButtonStyle(FEditorStyle::Get(), "NoBorder")
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
					.Content()
					[
						SNew(SImage)
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
						.Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
#else
						.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
					]
				]
		]
	;

	for (int32 i = 0; i < PK_ARRAY_COUNT(m_Thumbs); ++i)
	{
		m_Thumbs[i].m_Pty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationSubRendererMaterial::OnThumbnailPtyChange, i));
		thumbnails->AddSlot()
			.AutoWidth()
			[
				SNew(SBorder)
				.Padding(5.0f)
				[
					SNew(SBox)
					.Visibility(this, &FPopcornFXCustomizationSubRendererMaterial::OnGetThumbnailVisibility, i)
					.ToolTipText(this, &FPopcornFXCustomizationSubRendererMaterial::OnGetThumbnailToolTip, i)
					.WidthOverride(64)
					.HeightOverride(64)
					[
						m_Thumbs[i].m_Tumbnail->MakeThumbnailWidget()
					]
				]
			]
		;
	}

	// padding dummy
	thumbnails->AddSlot()
		.FillWidth(1.f)
		[
			SNew(SVerticalBox)
		];
}

void	FPopcornFXCustomizationSubRendererMaterial::CustomizeChildren(
	TSharedRef<class IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

FText	FPopcornFXCustomizationSubRendererMaterial::OnGetThumbnailToolTip(int32 thumbId) const
{
	check(thumbId >= 0 && thumbId < PK_ARRAY_COUNT(m_Thumbs));
	if (!m_Thumbs[thumbId].m_Pty->IsValidHandle())
		return FText::FromString(TEXT("Invalid"));
	UObject		*obj;
	m_Thumbs[thumbId].m_Pty->GetValue(obj);
	if (obj == null)
		return FText::FromString(TEXT("null"));
	return FText::FromString(obj->GetPathName());
}

void	FPopcornFXCustomizationSubRendererMaterial::OnThumbnailPtyChange(int32 thumbId)
{
	if (thumbId < 0)
	{
		for (int32 i = 0; i < PK_ARRAY_COUNT(m_Thumbs); ++i)
			OnThumbnailPtyChange(i);
		return;
	}
	check(thumbId < PK_ARRAY_COUNT(m_Thumbs));
	if (!m_Thumbs[thumbId].m_Pty->IsValidHandle())
		return;
	UObject		*obj;
	m_Thumbs[thumbId].m_Pty->GetValue(obj);
	m_Thumbs[thumbId].m_Tumbnail->SetAsset(obj);
}

EVisibility		FPopcornFXCustomizationSubRendererMaterial::OnGetThumbnailVisibility(int32 thumbId) const
{
	check(thumbId >= 0 && thumbId < PK_ARRAY_COUNT(m_Thumbs));
	if (!m_Thumbs[thumbId].m_Pty->IsValidHandle())
		return EVisibility::Collapsed;
	UObject		*obj;
	m_Thumbs[thumbId].m_Pty->GetValue(obj);
	if (obj == null)
		return EVisibility::Collapsed;
	return EVisibility::Visible;
}

FReply	FPopcornFXCustomizationSubRendererMaterial::OnResetClicked()
{
	FPopcornFXSubRendererMaterial		*self = Self();
	if (!PK_VERIFY(self != null))
		return FReply::Handled();
	self->_ResetDefaultMaterial_NoReload();
#if (ENGINE_MAJOR_VERSION == 5)
	m_MaterialPty->NotifyPostChange(EPropertyChangeType::ValueSet); // TODO: Is this correct
#else
	m_MaterialPty->NotifyPostChange(); // should reload
#endif // (ENGINE_MAJOR_VERSION == 5)
	return FReply::Handled();
}

EVisibility		FPopcornFXCustomizationSubRendererMaterial::GetResetVisibility() const
{
	FPopcornFXSubRendererMaterial		*self = Self();
	if (!PK_VERIFY(self != null))
		return EVisibility::Hidden;
	if (self->Material == null)
		return EVisibility::Visible;
	if (self->Material != self->FindDefaultMaterial())
		return EVisibility::Visible;
	return EVisibility::Hidden;
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
