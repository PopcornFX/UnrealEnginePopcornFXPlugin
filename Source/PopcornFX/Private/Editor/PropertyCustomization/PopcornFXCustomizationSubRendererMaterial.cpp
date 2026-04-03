//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXCustomizationSubRendererMaterial.h"

#include "Assets/PopcornFXRendererMaterial.h"
#include "Editor/EditorHelpers.h"

#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
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

#include <pk_particles/include/Renderers/ps_renderer_enums.h>

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

#define EXEC_PK_RENDERER_CLASSES(op) \
	op(Renderer_Billboard) \
	op(Renderer_Ribbon) \
	op(Renderer_Mesh) \
	op(Renderer_Triangle) \
	op(Renderer_Decal) \
	op(Renderer_Light) \
	op(Renderer_Sound)

#define RENDERER_NAME_CASE_TO_STRING(_rendererType) case PopcornFX::ERendererClass::_rendererType : return FString(#_rendererType);

FString	FPopcornFXCustomizationSubRendererMaterial::_RendererNameFromType(const int type)
{
	switch (type)
	{
#define	X_RENDERER_CLASSES(__name)	Renderer_ ## __name,
		EXEC_PK_RENDERER_CLASSES(RENDERER_NAME_CASE_TO_STRING)
#undef	X_RENDERER_CLASSES
	default :
		return FString("Renderer_Invalid");
	}
}

#undef RENDERER_NAME_CASE_TO_STRING

void	FPopcornFXCustomizationSubRendererMaterial::CustomizeHeader(
	TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	m_SelfPty = PropertyHandle;

	TSharedRef<IPropertyHandle>		material = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, Material)).ToSharedRef();
	check(material->IsValidHandle());
	m_MaterialPty = material;

	TSharedRef<IPropertyHandle>		legacyMaterialType = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, LegacyMaterialType)).ToSharedRef();
	check(legacyMaterialType->IsValidHandle());

	TSharedRef<IPropertyHandle>		defaultMaterialType = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, DefaultMaterialType)).ToSharedRef();
	check(defaultMaterialType->IsValidHandle());

	TSharedRef<IPropertyHandle>		isLegacyHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, IsLegacy)).ToSharedRef();
	check(isLegacyHandle->IsValidHandle());

	bool isLegacy = false;
	isLegacyHandle.Get().GetValue(isLegacy);
	TSharedRef<IPropertyHandle>		materialType = isLegacy ? legacyMaterialType : defaultMaterialType;

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

	m_Thumbs[Thumb_Diffuse].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, TextureDiffuse)).ToSharedRef();
	m_Thumbs[Thumb_Diffuse].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.TextureDiffuse, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_DiffuseRamp].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, TextureDiffuseRamp)).ToSharedRef();
	m_Thumbs[Thumb_DiffuseRamp].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.TextureDiffuseRamp, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_Emissive].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, TextureEmissive)).ToSharedRef();
	m_Thumbs[Thumb_Emissive].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.TextureEmissive, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_AlphaRemap].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, TextureAlphaRemapper)).ToSharedRef();
	m_Thumbs[Thumb_AlphaRemap].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.TextureAlphaRemapper, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_MotionVectors].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, TextureMotionVectors)).ToSharedRef();
	m_Thumbs[Thumb_MotionVectors].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.TextureMotionVectors, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_SixWay_RLTS].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, TextureSixWay_RLTS)).ToSharedRef();
	m_Thumbs[Thumb_SixWay_RLTS].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.TextureSixWay_RLTS, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_SixWay_BBF].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, TextureSixWay_BBF)).ToSharedRef();
	m_Thumbs[Thumb_SixWay_BBF].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.TextureSixWay_BBF, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_VATPosition].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, VATTexturePosition)).ToSharedRef();
	m_Thumbs[Thumb_VATPosition].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.VATTexturePosition, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_VATNormal].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, VATTextureNormal)).ToSharedRef();
	m_Thumbs[Thumb_VATNormal].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.VATTextureNormal, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_VATColor].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, VATTextureColor)).ToSharedRef();
	m_Thumbs[Thumb_VATColor].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.VATTextureColor, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_VATRotation].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, VATTextureRotation)).ToSharedRef();
	m_Thumbs[Thumb_VATRotation].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.VATTextureRotation, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_SkeletalAnimation].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, TextureSkeletalAnimation)).ToSharedRef();
	m_Thumbs[Thumb_SkeletalAnimation].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.TextureSkeletalAnimation, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_Normal].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, TextureNormal)).ToSharedRef();
	m_Thumbs[Thumb_Normal].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.TextureNormal, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_Normal].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, TextureNormal)).ToSharedRef();
	m_Thumbs[Thumb_Normal].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.TextureNormal, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_Specular].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, TextureSpecular)).ToSharedRef();
	m_Thumbs[Thumb_Specular].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.TextureSpecular, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_Mesh].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, StaticMesh)).ToSharedRef();
	m_Thumbs[Thumb_Mesh].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.StaticMesh, 64, 64, CustomizationUtils.GetThumbnailPool()));

	m_Thumbs[Thumb_SkeletalMesh].m_Pty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, SkeletalMesh)).ToSharedRef();
	m_Thumbs[Thumb_SkeletalMesh].m_Tumbnail = MakeShareable(new FAssetThumbnail(self->EditableProperties.SkeletalMesh, 64, 64, CustomizationUtils.GetThumbnailPool()));

	TSharedPtr<SHorizontalBox>	thumbnails;
}

void	FPopcornFXCustomizationSubRendererMaterial::CustomizeChildren(
	TSharedRef<class IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder &ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	m_SelfPty = PropertyHandle;

	TSharedRef<IPropertyHandle>		material = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, Material)).ToSharedRef();
	check(material->IsValidHandle());
	m_MaterialPty = material;

	FPopcornFXSubRendererMaterial *self = Self();
	if (!PK_VERIFY(self != null))
		return;

	FString rendererName = "Renderer: ";
	if (self->Renderer.Name.IsEmpty())
	{
		rendererName += _RendererNameFromType(self->Renderer.Type) + " (no custom name)";
	}
	else
	{
		rendererName += "'" + self->Renderer.Name + "'";
	}

	TSharedPtr<SWidget>	nameWidget;
	TSharedPtr<SWidget>	valueWidget;
	TSharedPtr<SWidget>	resetToDefaultWidget;
	IDetailPropertyRow	&materialRow = ChildBuilder.AddProperty(material);
	materialRow.DisplayName(FText::FromString("Material")).GetDefaultWidgets(nameWidget, valueWidget, true);
	
	FDetailWidgetRow &customWidget = materialRow.CustomWidget();
	resetToDefaultWidget = customWidget.ResetToDefaultWidget.Widget;
	customWidget.NameContent()[nameWidget.ToSharedRef()].ValueContent()[valueWidget.ToSharedRef()]
		.ResetToDefaultContent()
		[
			SNew(SButton)
				.OnClicked(this, &FPopcornFXCustomizationSubRendererMaterial::OnResetClicked)
				.Visibility(this, &FPopcornFXCustomizationSubRendererMaterial::GetResetVisibility)
				.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset this property to its default value"))
				.ButtonColorAndOpacity(FSlateColor::UseForeground())
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.Content()
				[
					SNew(SImage)
						.Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
				]
		];

	if (self->EditableProperties.Material == null)
		return;
	if (self->IsLegacy && self->EditableProperties.Material != self->FindLegacyMaterial())
			return;
	else if (!self->IsLegacy && self->EditableProperties.Material != self->FindDefaultMaterial())
			return;

	if (self->bHasTextureDiffuse)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_Diffuse].m_Pty.ToSharedRef());
	if (self->bHasTextureDiffuseRamp)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_DiffuseRamp].m_Pty.ToSharedRef());
	if (self->bHasTextureEmissive)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_Emissive].m_Pty.ToSharedRef());
	/*if (self->bHasTextureEmissiveRamp) TODO?
		ChildBuilder.AddProperty(m_Thumbs[Thumb_EmissiveRamp].m_Pty.ToSharedRef());*/
	if (self->bHasTextureNormal)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_Normal].m_Pty.ToSharedRef());
	/*if (self->bHasTextureRoughMetal) TODO?
		ChildBuilder.AddProperty(m_Thumbs[Thumb_RoughMetal].m_Pty.ToSharedRef());*/
	if (self->bHasTextureSpecular)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_Specular].m_Pty.ToSharedRef());
	if (self->bHasTextureAlphaRemapper)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_AlphaRemap].m_Pty.ToSharedRef());
	if (self->bHasTextureMotionVectors)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_MotionVectors].m_Pty.ToSharedRef());
	if (self->bHasTextureSixWay_RLTS)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_SixWay_RLTS].m_Pty.ToSharedRef());
	if (self->bHasTextureSixWay_BBF)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_SixWay_BBF].m_Pty.ToSharedRef());
	if (self->bHasVATTexturePosition)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_VATPosition].m_Pty.ToSharedRef());
	if (self->bHasVATTextureNormal)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_VATNormal].m_Pty.ToSharedRef());
	if (self->bHasVATTextureColor)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_VATColor].m_Pty.ToSharedRef());
	if (self->bHasVATTextureRotation)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_VATRotation].m_Pty.ToSharedRef());
	if (self->bHasTextureSkeletalAnimation)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_SkeletalAnimation].m_Pty.ToSharedRef());
	if (self->bHasStaticMesh)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_Mesh].m_Pty.ToSharedRef());
	if (self->bHasSkeletalMesh)
		ChildBuilder.AddProperty(m_Thumbs[Thumb_SkeletalMesh].m_Pty.ToSharedRef());
	/*if (self->bHasTextureAtlas) TODO?
		newGroup.AddPropertyRow(m_Thumbs[Thumb_Atlas].m_Pty.ToSharedRef());*/


	// TODO: show some render features (enabled or not) without confusing the user?

	// TODO: VAT NoAlpha/PackedData?
	/*TSharedRef<IPropertyHandle>		legacyMaterialType = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, LegacyMaterialType)).ToSharedRef();
	check(legacyMaterialType->IsValidHandle());

	TSharedRef<IPropertyHandle>		defaultMaterialType = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, DefaultMaterialType)).ToSharedRef();
	check(defaultMaterialType->IsValidHandle());

	TSharedRef<IPropertyHandle>		isLegacyHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, IsLegacy)).ToSharedRef();
	check(isLegacyHandle->IsValidHandle());

	bool isLegacy = false;
	isLegacyHandle.Get().GetValue(isLegacy);
	TSharedRef<IPropertyHandle>		materialType = isLegacy ? legacyMaterialType : defaultMaterialType;
	ChildBuilder.AddProperty(materialType);

	TSharedRef<IPropertyHandle>		noAlpha = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, NoAlpha)).ToSharedRef();
	check(noAlpha->IsValidHandle());
	ChildBuilder.AddProperty(noAlpha);

	TSharedRef<IPropertyHandle>		softAnimBlending = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, SoftAnimBlending)).ToSharedRef();
	check(softAnimBlending->IsValidHandle());
	ChildBuilder.AddProperty(softAnimBlending);

	TSharedRef<IPropertyHandle>		motionVectorsBlending = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, MotionVectorsBlending)).ToSharedRef();
	check(motionVectorsBlending->IsValidHandle());
	ChildBuilder.AddProperty(motionVectorsBlending);

	TSharedRef<IPropertyHandle>		castShadow = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, CastShadow)).ToSharedRef();
	check(castShadow->IsValidHandle());
	ChildBuilder.AddProperty(castShadow);

	TSharedRef<IPropertyHandle>		staticMeshLOD = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, StaticMeshLOD)).ToSharedRef();
	ChildBuilder.AddProperty(staticMeshLOD);*/
}

FText	FPopcornFXCustomizationSubRendererMaterial::OnGetThumbnailToolTip(int32 thumbId) const
{
	check(thumbId >= 0 && thumbId < PK_ARRAY_COUNT(m_Thumbs));
	if (!m_Thumbs[thumbId].m_Pty->IsValidHandle())
		return FText::FromString(TEXT("Invalid"));
	UObject		*obj = null;
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
	m_MaterialPty->NotifyPostChange(EPropertyChangeType::ValueSet); // TODO: Is this correct
	return FReply::Handled();
}

EVisibility		FPopcornFXCustomizationSubRendererMaterial::GetResetVisibility() const
{
	FPopcornFXSubRendererMaterial		*self = Self();
	if (!PK_VERIFY(self != null))
		return EVisibility::Hidden;
	if (self->EditableProperties.Material == null)
		return EVisibility::Visible;
	if (self->IsLegacy)
	{
		if (self->EditableProperties.Material != self->FindLegacyMaterial())
			return EVisibility::Visible;
	}
	else
	{
		if (self->EditableProperties.Material != self->FindDefaultMaterial())
			return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
