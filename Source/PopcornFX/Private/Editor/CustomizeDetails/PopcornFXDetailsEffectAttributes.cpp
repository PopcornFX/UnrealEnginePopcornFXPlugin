//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsEffectAttributes.h"

#include "Assets/PopcornFXEffect.h"
#include "PopcornFXEmitterComponent.h"
#include "Editor/EditorHelpers.h"
#include "Editor/PopcornFXStyle.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"

#include "Styling/StyleColors.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXDetailsEmitterComponent"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXDetailsEffectAttributes, Log, All);

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEffectAttributes::CustomizeDetails(class IDetailLayoutBuilder &DetailLayout)
{
	// calling EditCategory reorder Categories in the Editor
	DetailLayout.HideCategory("PopcornFX RendererMaterials");
	DetailLayout.HideCategory("PopcornFX AssetDependencies");
	DetailLayout.HideCategory("Source");
	DetailLayout.HideCategory("UserDatas");
	DetailLayout.HideProperty("DefaultSamplers");

	m_AttributeListCategory = &DetailLayout.EditCategory("PopcornFX Default Attributes");
	m_AttributeListCategory->SetDisplayName(FText::FromString("Default attribute values"));

	FTextBlockStyle style;
	style.SetColorAndOpacity(FSlateColor::UseForeground());
	style.SetFont(DetailLayout.GetDetailFont());

	TSharedPtr<SWidget> hbox;
	SAssignNew(hbox, SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
		[
			SNew(SScaleBox)
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("Icons.Help"))
				]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
		[
			SNew(SButton).Text(FText::FromString("Help")).TextStyle(&style).OnClicked_Lambda([]()
				{
					FPlatformProcess::LaunchURL(TEXT("https://documentation.popcornfx.com/PopcornFX/latest/plugins/ue-plugin/effects.html#_effect_attributes"), NULL, NULL);
					return FReply::Handled();
				})
		];
	m_AttributeListCategory->HeaderContent(hbox.ToSharedRef());

	m_AttributeListCategory->AddCustomRow(FText::FromString("Tooltip"))
		.NameContent()
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(EVerticalAlignment::VAlign_Center)
				[
					SNew(SScaleBox)
						[
							SNew(SImage).Image(FCoreStyle::Get().GetBrush("Icons.Info"))
						]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(EVerticalAlignment::VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(" Note"))
						.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
		]
		.ValueContent()
		[
			SNew(STextBlock)
				.AutoWrapText(true)
				.WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
				.Font(DetailLayout.GetDetailFont())
				.Text(FText::FromString("Here are the default attribute and attribute sampler values of your effect.\n"
										"Every future emitter using this effect will have these values by default.\n"
										"They also affect the emitter in the viewport above to preview your changes.\n"
										"This can be changed per instance by editing emitters"))
		];

}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
