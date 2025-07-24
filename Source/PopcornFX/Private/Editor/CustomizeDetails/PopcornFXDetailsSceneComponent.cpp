//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsSceneComponent.h"

#include "PopcornFXSceneComponent.h"
#include "PopcornFXSceneActor.h"
#include "Internal/ParticleScene.h"
#include "PopcornFXSDK.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailsView.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXDetailsSceneComponent, Log, All);

#define LOCTEXT_NAMESPACE "PopcornFXDetailsSceneComponent"

//----------------------------------------------------------------------------

FPopcornFXDetailsSceneComponent::FPopcornFXDetailsSceneComponent()
:	m_DetailLayout(null)
{

}

//----------------------------------------------------------------------------

TSharedRef<IDetailCustomization>	FPopcornFXDetailsSceneComponent::MakeInstance()
{
	return MakeShareable(new FPopcornFXDetailsSceneComponent);
}

//----------------------------------------------------------------------------

FReply		FPopcornFXDetailsSceneComponent::OnClear()
{
	if (!PK_VERIFY(m_DetailLayout != null))
		return FReply::Unhandled();

	const TArray< TWeakObjectPtr<UObject> >		&objects = m_DetailLayout->GetDetailsView()->GetSelectedObjects();
	for (int32 obji = 0; obji < objects.Num(); ++obji)
	{
		if (objects[obji].IsValid())
		{
			UPopcornFXSceneComponent	*sceneComponent = Cast<UPopcornFXSceneComponent>(objects[obji].Get());
			if (sceneComponent == null)
			{
				APopcornFXSceneActor	*actor = Cast<APopcornFXSceneActor>(objects[obji].Get());
				if (actor != null)
					sceneComponent = actor->PopcornFXSceneComponent;
			}
			if (sceneComponent != null && sceneComponent->ParticleScene() != null)
				sceneComponent->ParticleScene()->Clear();
		}
	}
	return FReply::Handled();
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsSceneComponent::RebuildDetails()
{
	if (!PK_VERIFY(m_DetailLayout != null))
		return;
	m_DetailLayout->ForceRefreshDetails();
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsSceneComponent::CustomizeDetails(IDetailLayoutBuilder &DetailLayout)
{
	m_DetailLayout = &DetailLayout;

	//const TArray< TWeakObjectPtr<UObject> > &objects = m_DetailLayout->GetDetailsView().GetSelectedObjects();

	//DetailLayout.HideProperty("AttributeList");

	FTextBlockStyle			buttonTextStyle;
	buttonTextStyle.SetFont(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"));
	buttonTextStyle.SetColorAndOpacity(FAppStyle::GetColor("Colors.AccentGray"));

	IDetailCategoryBuilder	&sceneCategory = DetailLayout.EditCategory("PopcornFX Scene");
	sceneCategory.AddCustomRow(LOCTEXT("Editor Actions", "Editor Actions"), false)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SceneActions", "Scene Actions"))
			.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
		]
	.ValueContent()
		.MinDesiredWidth(125.0f * 3.0f)
		.MaxDesiredWidth(125.0f * 4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
					.Text(LOCTEXT("Clear", "Clear"))
					.TextStyle(&buttonTextStyle)
					.OnClicked(this, &FPopcornFXDetailsSceneComponent::OnClear)
			]
		];

	{
		TSharedRef<IPropertyHandle>	simulationSettings = DetailLayout.GetProperty("SimulationSettingsOverride");
		if (!PK_VERIFY(simulationSettings->IsValidHandle()))
			return;

		DetailLayout.HideProperty("FieldListeners");
	}
	{
		TSharedRef<IPropertyHandle>	bboxMode = DetailLayout.GetProperty("BoundingBoxMode");
		if (!PK_VERIFY(bboxMode->IsValidHandle()))
			return;
		uint8	value;

		bboxMode->GetValue(value);
		switch (value)
		{
		case	EPopcornFXSceneBBMode::Dynamic:
			DetailLayout.HideProperty("FixedRelativeBoundingBox");
			break;
		default:
			break;
		}
		bboxMode->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsSceneComponent::RebuildDetails));
	}
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
