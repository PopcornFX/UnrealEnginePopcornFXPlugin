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
}

#define CHECK_IF_PTY_IS_DEFAULT(__pty) \
	if (self->EditableProperties.__pty == null) \
		return EVisibility::Visible; \
	if (self->EditableProperties.__pty != self->EditablePropertiesDefault.__pty) \
		return EVisibility::Visible; \

#define ADD_PTY(__ptyHandle, __ptyName) \
	TSharedPtr<IPropertyHandle>	pty = __ptyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXEditableMaterialProperties, __ptyName)); \
	IDetailPropertyRow &ptyRow = ChildBuilder.AddProperty(pty.ToSharedRef()); \
	TSharedPtr<SWidget>	nameWidget; \
	TSharedPtr<SWidget>	valueWidget; \
	TSharedPtr<SWidget>	resetToDefaultWidget; \
	ptyRow.DisplayName(FText::FromString(#__ptyName)).GetDefaultWidgets(nameWidget, valueWidget, true); \
	FDetailWidgetRow &currWidget = ptyRow.CustomWidget(); \
	currWidget.NameContent()[nameWidget.ToSharedRef()].ValueContent()[valueWidget.ToSharedRef()] \
		.ResetToDefaultContent() \
		[ \
			SNew(SButton) \
				.OnClicked_Lambda([this, pty]() \
					{ \
						FPopcornFXSubRendererMaterial *self = Self(); \
						if (!PK_VERIFY(self != null)) \
							return FReply::Handled(); \
						self->EditableProperties.__ptyName = self->EditablePropertiesDefault.__ptyName; \
						pty->NotifyPostChange(EPropertyChangeType::ValueSet); \
						return FReply::Handled(); \
					}) \
				.Visibility_Lambda([this]() \
					{ \
						FPopcornFXSubRendererMaterial *self = Self(); \
						if (!PK_VERIFY(self != null)) \
							return EVisibility::Hidden; \
							\
						CHECK_IF_PTY_IS_DEFAULT(__ptyName) \
							return EVisibility::Hidden; \
					}) \
				.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset this property to its default value")) \
				.ButtonColorAndOpacity(FSlateColor::UseForeground()) \
				.ButtonStyle(FAppStyle::Get(), "NoBorder") \
				.Content() \
				[ \
					SNew(SImage) \
						.Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault")) \
				] \
		]; \

void	FPopcornFXCustomizationSubRendererMaterial::CustomizeChildren(
	TSharedRef<class IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder &ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	m_SelfPty = PropertyHandle;

	FPopcornFXSubRendererMaterial *self = Self();
	if (!PK_VERIFY(self != null))
		return;

	{
		ADD_PTY(PropertyHandle, Material);
	}

	if (self->EditableProperties.Material == null)
		return;
	/* TODO: instead of returning if it's not default material, still show editable properties that are used in the current material for all PK default mats */
	if (self->EditableProperties.Material != self->EditablePropertiesDefault.Material)
			return;

	TSharedPtr<IPropertyHandle>	editablePropertiesHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, EditableProperties));
	if (self->bHasTextureDiffuse)
	{
		ADD_PTY(editablePropertiesHandle, TextureDiffuse);
	}
	if (self->bHasTextureDiffuseRamp)
	{
		ADD_PTY(editablePropertiesHandle, TextureDiffuseRamp);
	}
	if (self->bHasTextureEmissive)
	{
		ADD_PTY(editablePropertiesHandle, TextureEmissive);
	}
	if (self->bHasTextureEmissiveRamp)
	{
		ADD_PTY(editablePropertiesHandle, TextureEmissiveRamp);
	}
	if (self->bHasTextureNormal)
	{
		ADD_PTY(editablePropertiesHandle, TextureNormal);
	}
	if (self->bHasTextureRoughMetal)
	{
		ADD_PTY(editablePropertiesHandle, TextureRoughMetal);
	}
	if (self->bHasTextureSpecular)
	{
		ADD_PTY(editablePropertiesHandle, TextureSpecular);
	}
	if (self->bHasTextureAlphaRemapper)
	{
		ADD_PTY(editablePropertiesHandle, TextureAlphaRemapper);
	}
	if (self->bHasTextureMotionVectors)
	{
		ADD_PTY(editablePropertiesHandle, TextureMotionVectors);
	}
	if (self->bHasTextureSixWay_RLTS)
	{
		ADD_PTY(editablePropertiesHandle, TextureSixWay_RLTS);
	}
	if (self->bHasTextureSixWay_BBF)
	{
		ADD_PTY(editablePropertiesHandle, TextureSixWay_BBF);
	}
	if (self->bHasVATTexturePosition)
	{
		ADD_PTY(editablePropertiesHandle, VATTexturePosition);
	}
	if (self->bHasVATTextureNormal)
	{
		ADD_PTY(editablePropertiesHandle, VATTextureNormal);
	}
	if (self->bHasVATTextureColor)
	{
		ADD_PTY(editablePropertiesHandle, VATTextureColor);
	}
	if (self->bHasVATTextureRotation)
	{
		ADD_PTY(editablePropertiesHandle, VATTextureRotation);
	}
	if (self->bHasTextureSkeletalAnimation)
	{
		ADD_PTY(editablePropertiesHandle, TextureSkeletalAnimation);
	}
	if (self->bHasStaticMesh)
	{
		ADD_PTY(editablePropertiesHandle, StaticMesh);
	}
	if (self->bHasSkeletalMesh)
	{
		ADD_PTY(editablePropertiesHandle, SkeletalMesh);
	}
	/*if (self->bHasTextureAtlas) TODO?
	{
	}*/
}

#undef CHECK_IF_PTY_IS_DEFAULT
#undef ADD_PTY

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
