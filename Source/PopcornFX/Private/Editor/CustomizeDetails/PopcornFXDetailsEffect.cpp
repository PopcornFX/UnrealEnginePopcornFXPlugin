//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsEffect.h"

#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "Editor/EditorHelpers.h"
#include "Editor/PopcornFXEffectEditor.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Input/STextComboPopup.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/SMultilineEditableText.h"
#include "AssetToolsModule.h"
#include "Materials/MaterialInterface.h"

#include "PopcornFXSDK.h"

#include <pk_particles/include/Renderers/ps_renderer_enums.h>

#define LOCTEXT_NAMESPACE "PopcornFXDetailsEffect"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXEffectEditor, Log, All);

DECLARE_DELEGATE_OneParam(FOnDrawCallUpdated, const FPropertyChangedEvent &);

using PopcornFX::CString;

namespace
{
	FString	_RendererNameFromType(const int type)
	{
		switch (type)
		{
		case PopcornFX::ERendererClass::Renderer_Billboard:
			return ("Billboard renderer");
		case PopcornFX::ERendererClass::Renderer_Decal:
			return ("Decal renderer");
		case PopcornFX::ERendererClass::Renderer_Light:
			return ("Light renderer");
		case PopcornFX::ERendererClass::Renderer_Mesh:
			return ("Mesh renderer");
		case PopcornFX::ERendererClass::Renderer_Ribbon:
			return ("Ribbon renderer");
		case PopcornFX::ERendererClass::Renderer_Sound:
			return ("Sound renderer");
		case PopcornFX::ERendererClass::Renderer_Triangle:
			return ("Triangle renderer");
		default:
			return ("Unsupported renderer");
		}
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEffect::BuildAssetDependenciesArray(class IDetailLayoutBuilder& DetailLayout, IDetailCategoryBuilder &cat, FName propertyPath, UClass *classOuter)
{
	TSharedRef<IPropertyHandle>		pty = DetailLayout.GetProperty(propertyPath, classOuter);
	DetailLayout.HideProperty(pty);

	cat.AddCustomRow(FText::FromString("Tooltip"))
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
				.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.Text(FText::FromString("Here are listed all the dependencies used by your effect.\n"
										"Use this to fix pipeline/import issues or what source asset should be used, not to \"edit\" your effect\n"
										"This is the lowest-level edition that you can do."))
		];
	uint32				count = 0;
	pty->GetNumChildren(count);
	for (uint32 i = 0; i < count; ++i)
		cat.AddProperty(pty->GetChildHandle(i));
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEffect::UpdateBatchMaterials(const FPropertyChangedEvent &event)
{
	const UPopcornFXRendererMaterial* editedMat = Cast<UPopcornFXRendererMaterial>(event.GetObjectBeingEdited(0));
	if (editedMat != null)
	{
		const UObject *outer = editedMat->GetOuter();
		UPopcornFXEffect *effect = Cast<UPopcornFXEffect>(editedMat->GetOuter());
		if (effect != null)
		{
			for (int32 i = 0; i < m_UniqueMats.Num(); i++)
			{
				if (m_UniqueMats[i].Mat == editedMat)
				{
					for (int32 j = 0; j < m_UniqueMats[i].MatIndices.Num(); j++)
					{
						const FPopcornFXRendererDesc desc = effect->ParticleRendererMaterials[m_UniqueMats[i].MatIndices[j]]->SubMaterials[0].Renderer;
						effect->ParticleRendererMaterials[m_UniqueMats[i].MatIndices[j]]->SubMaterials[0] = editedMat->SubMaterials[0];
						effect->ParticleRendererMaterials[m_UniqueMats[i].MatIndices[j]]->SubMaterials[0].Renderer = desc;
					}
				}
			}
			effect->ReloadFile();
		}
	}

	if (m_PropertyUtilities.IsValid())
	{
		m_PropertyUtilities->ForceRefresh();
	}
}

//----------------------------------------------------------------------------

struct FUniqueRenderer
{
	FString	ParentLayerName;
	FString	Name;
	int32	MatID;
};

void	FPopcornFXDetailsEffect::ListAllRenderers(UPopcornFXEffect* effect, TSharedRef<IPropertyHandle> pty, IDetailCategoryBuilder &cat)
{
	IDetailGroup& editGroup = cat.AddGroup(FName("Edit individual renderers"), FText::FromString("Edit individual renderers - ( Layer | Renderer | Current draw call )"));

	TArray<FUniqueRenderer> renderers;
	renderers.Reserve(effect->ParticleRendererMaterials.Num());

	uint32	count = effect->ParticleRendererMaterials.Num();
	for (uint32 mati = 0; mati < count; mati++)
	{
		const UPopcornFXRendererMaterial *mat = effect->ParticleRendererMaterials[mati];
		if (mat == null || mat->SubMaterialCount() == 0)
		{
			continue;
		}
		const FPopcornFXSubRendererMaterial &submat = mat->SubMaterials[0];

		renderers.Add(FUniqueRenderer(submat.Renderer.ParentLayerName, submat.Renderer.Name, mati));
	}

	// Sort layers and renderers: unnamed ones last
	renderers.Sort([](const FUniqueRenderer &A, const FUniqueRenderer &B)
		{
			if (A.ParentLayerName == B.ParentLayerName)
			{
				if (A.Name.IsEmpty())
				{
					return false;
				}
				if (B.Name.IsEmpty())
				{
					return true;
				}
				return A.Name < B.Name;
			}
			if (A.ParentLayerName.IsEmpty())
			{
				return false;
			}
			if (B.ParentLayerName.IsEmpty())
			{
				return true;
			}
			return A.ParentLayerName < B.ParentLayerName;
		});

	for (uint32 rendereri = 0; rendereri < count; rendereri++)
	{
		const UPopcornFXRendererMaterial *mat = effect->ParticleRendererMaterials[renderers[rendereri].MatID];
		if (mat == null || mat->SubMaterialCount() == 0)
		{
			continue;
		}

		FString layerName;
		FString layerPrefix = "";
		if (renderers[rendereri].ParentLayerName.IsEmpty())
		{
			layerName = "unnamed  ";
		}
		else
		{
			layerName = "" + renderers[rendereri].ParentLayerName + "  ";
		}
		FString rendererName;
		FString rendererPrefix = "  ";
		if (renderers[rendereri].Name.IsEmpty())
		{
			rendererName = "unnamed  ";
		}
		else
		{
			rendererName = "" + renderers[rendereri].Name + "  ";
		}
		int32 drawCall = -1;
		for (int32 j = 0; j < m_UniqueMats.Num(); j++)
		{
			if (m_UniqueMats[j].Mat->CanBeMergedWith(mat))
			{
				drawCall = j;
				break;
			}
		}
		IDetailGroup &rendererGroup = editGroup.AddGroup(FName(layerName + FString::FromInt(rendereri)), FText());
		rendererGroup.HeaderRow()
			.NameContent()
			[
				SNew(SSplitter)
					.PhysicalSplitterHandleSize(1.0f)
					.Style(FAppStyle::Get(), "DetailsView.Splitter")
					+ SSplitter::Slot().SizeRule(SSplitter::ESizeRule::SizeToContent)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock).Text(FText::FromString(layerPrefix))
								.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))

						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock).Text(FText::FromString(layerName))
								.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
								.ColorAndOpacity(layerName == "unnamed  " ? USlateThemeManager::Get().GetColor(EStyleColor::Hover) : USlateThemeManager::Get().GetColor(EStyleColor::Foreground))
						]
					]
					+ SSplitter::Slot().SizeRule(SSplitter::ESizeRule::SizeToContent)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock).Text(FText::FromString(rendererPrefix))
								.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock).Text(FText::FromString(rendererName))
								.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
								.ColorAndOpacity(rendererName == "unnamed  " ? USlateThemeManager::Get().GetColor(EStyleColor::Hover) : USlateThemeManager::Get().GetColor(EStyleColor::Foreground))
						]
					]
					+ SSplitter::Slot().SizeRule(SSplitter::ESizeRule::SizeToContent)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock).Text(FText::FromString("  Draw call " + FString::FromInt(drawCall)))
								.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
						]
					]
			]
		.ValueContent()
			[
				SNew(STextBlock).Text(FText::FromString(_RendererNameFromType(mat->SubMaterials[0].Renderer.Type)))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
			];
		rendererGroup.AddPropertyRow(pty->GetChildHandle(renderers[rendereri].MatID).ToSharedRef());
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEffect::BuildRendererMaterialsArray(class IDetailLayoutBuilder &DetailLayout, IDetailCategoryBuilder &cat, FName propertyPath, UClass *classOuter)
{
	TSharedRef<IPropertyHandle>		pty = DetailLayout.GetProperty(propertyPath, classOuter);
	DetailLayout.HideProperty(pty);

	cat.AddCustomRow(FText::FromString("Tooltip"))
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
				.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.Text(FText::FromString("Here are listed all the unique combinations of materials and their features used by your effect.\n"
										"Each combination is a single draw call. You can find which renderer is batched in which draw call in the drop downs below\n"
										"This is per effect. To change parameters on effect instances, use attributes and attribute samplers"))
		];

	TArray<UObject *> objects;
	pty->GetOuterObjects(objects);
	UPopcornFXEffect *effect = Cast<UPopcornFXEffect>(objects[0]);
	if (!effect)
	{
		UE_LOG(LogPopcornFXEffectEditor, Display, TEXT("Could not retrieve parent effect"));
		return;
	}
	uint32	count = effect->ParticleRendererMaterials.Num();

	m_UniqueMats.Empty();
	// Retrieve unique mats
	for (uint32 i = 0; i < count; ++i)
	{
		const UPopcornFXRendererMaterial	*mat = effect->ParticleRendererMaterials[i];
		if (mat == null || mat->SubMaterialCount() == 0)
		{
			continue;
		}
		const FPopcornFXSubRendererMaterial &submat = mat->SubMaterials[0];
		bool merged = false;
		for (int32 j = 0; j < m_UniqueMats.Num(); j++)
		{
			if (m_UniqueMats[j].Mat->CanBeMergedWith(mat))
			{
				merged = true;
				m_UniqueMats[j].MatIndices.Add(i);
				if (submat.Renderer.ParentLayerID == m_UniqueMats[j].Layers.Last().ID)
				{
					m_UniqueMats[j].Layers.Last().AddRendererName(submat.Renderer.Name);
					break;
				}
				FLayerDesc newLayerDesc;
				newLayerDesc.Name = submat.Renderer.ParentLayerName;
				newLayerDesc.ID = submat.Renderer.ParentLayerID;
				newLayerDesc.AddRendererName(submat.Renderer.Name);
				m_UniqueMats[j].Layers.Add(newLayerDesc);
				break;
			}
		}
		if (merged == false)
		{
			FLayerDesc newLayerDesc;
			newLayerDesc.Name = submat.Renderer.ParentLayerName;
			newLayerDesc.ID = submat.Renderer.ParentLayerID;
			newLayerDesc.AddRendererName(submat.Renderer.Name);
			FUniqueMat newUniqueMat{ .Mat = mat };
			newUniqueMat.MatIndices.Add(i);
			newUniqueMat.Layers.Add(newLayerDesc);
			m_UniqueMats.Add(newUniqueMat);
		}
	}

	// Ok let's draw
	for (int32 i = 0; i < m_UniqueMats.Num(); i++)
	{
		const FPopcornFXSubRendererMaterial	&submat = m_UniqueMats[i].Mat->SubMaterials[0];
		FString currDrawCall = "Draw call " + FString::FromInt(m_IGroups.Num());
		IDetailGroup* newGroup = &cat.AddGroup(FName(currDrawCall), FText::FromString(currDrawCall));
		newGroup->HeaderRow()
			.NameContent()
			[
				SNew(STextBlock).Text(FText::FromString(currDrawCall))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
			]
			.ValueContent()
			[
				SNew(STextBlock).Text(FText::FromString(_RendererNameFromType(submat.Renderer.Type) + "(s)"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
			];
		m_IGroups.Add(newGroup);

		// List layers / renderers using this unique mat
		FString currDrawLayers = "Layers " + FString::FromInt(m_IGroups.Num());
		IDetailGroup& renderersGroup = newGroup->AddGroup(FName(currDrawLayers), FText());
		renderersGroup.HeaderRow()
			.NameContent()
			[
				SNew(STextBlock).Text(FText::FromString("Layers"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
			]
			.ValueContent()
			[
				SNew(STextBlock).Text(FText::FromString("Renderers"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
			];

		// Sort layers: unnamed ones last
		m_UniqueMats[i].Layers.Sort([](const FLayerDesc& A, const FLayerDesc& B)
			{
				if (A.Name.IsEmpty())
				{
					return false;
				}
				if (B.Name.IsEmpty())
				{
					return true;
				}
				return A.Name < B.Name;
			});

		uint32 fullyUnamedLayers = 0;
		uint32 totalUnnamedRenderersInUnnamedLayers = 0;
		for (int32 j = 0; j < m_UniqueMats[i].Layers.Num(); j++)
		{
			const FLayerDesc &desc = m_UniqueMats[i].Layers[j];
			if (desc.IsFullyUnnamed())
			{
				fullyUnamedLayers++;
				totalUnnamedRenderersInUnnamedLayers += desc.UniqueRendererNames[""];
				continue;
			}
			FDetailWidgetRow &row = renderersGroup.AddWidgetRow()
				.NameContent()
				[
					SNew(STextBlock).Text(FText::FromString(desc.Name.IsEmpty() ? "unnamed" : desc.Name))
						.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(desc.Name.IsEmpty() ? USlateThemeManager::Get().GetColor(EStyleColor::Hover) : USlateThemeManager::Get().GetColor(EStyleColor::Foreground))

				];

			FString rendererNames;
			uint32 printed = 0;
			for (const TPair<FString, uint32> &rendererName : desc.UniqueRendererNames)
			{
				if (rendererName.Key.IsEmpty())
				{
					continue;
				}
				if (printed > 0)
				{
					rendererNames += ", ";
				}
				printed++;
				if (rendererName.Value > 1)
				{
					rendererNames += "+" + FString::FromInt(rendererName.Value) + " ";
				}
				rendererNames += rendererName.Key;
			}

			TSharedPtr<SHorizontalBox> hbox;
			row.ValueContent()
				[
					SAssignNew(hbox, SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock).Text(FText::FromString(rendererNames))
							.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					]
				];
			if (desc.UniqueRendererNames.Contains("") && desc.UniqueRendererNames[""] > 0)
			{
				FString unnamedString;
				if (!rendererNames.IsEmpty())
				{
					unnamedString += ", +";
				}
				unnamedString += FString::FromInt(desc.UniqueRendererNames[""]) + " unnamed";
				hbox->AddSlot()
					[
						SNew(STextBlock).Text(FText::FromString(unnamedString))
							.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
							.ColorAndOpacity(USlateThemeManager::Get().GetColor(EStyleColor::Hover))
					];
			}
		}
		if (fullyUnamedLayers > 0)
		{
			FString layersName = m_UniqueMats[i].Layers.Num() == fullyUnamedLayers ? "" : "+";
			layersName += FString::FromInt(fullyUnamedLayers) + " unnamed";
			FDetailWidgetRow &row = renderersGroup.AddWidgetRow()
				.NameContent()
				[
					SNew(STextBlock).Text(FText::FromString(layersName))
						.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(USlateThemeManager::Get().GetColor(EStyleColor::Hover))
				];
			FString renderersName = m_UniqueMats[i].Layers.Num() == fullyUnamedLayers ? "" : "+";
			renderersName += FString::FromInt(totalUnnamedRenderersInUnnamedLayers) + " unnamed";
			row.ValueContent()
				[
					SNew(STextBlock).Text(FText::FromString(renderersName))
						.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(USlateThemeManager::Get().GetColor(EStyleColor::Hover))
				];
		}
		newGroup->AddPropertyRow(pty->GetChildHandle(m_UniqueMats[i].MatIndices[0]).ToSharedRef());
	}

	ListAllRenderers(effect, pty, cat);

	FOnDrawCallUpdated test;
	test.BindSP(this, &FPopcornFXDetailsEffect::UpdateBatchMaterials);
	pty->SetOnChildPropertyValueChangedWithData(test);
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEffect::CustomizeDetails(class IDetailLayoutBuilder& DetailLayout)
{
	m_PropertyUtilities = DetailLayout.GetPropertyUtilities();
	DetailLayout.HideCategory("PopcornFX Default Attributes");

	// calling EditCategory reorder Categories in the Editor
	IDetailCategoryBuilder		&catRenderMats = DetailLayout.EditCategory("PopcornFX RendererMaterials");
	catRenderMats.SetDisplayName(FText::FromString("Unique materials"));

	FTextBlockStyle style;
	style.SetColorAndOpacity(FSlateColor::UseForeground());
	style.SetFont(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"));

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
					FPlatformProcess::LaunchURL(TEXT("https://documentation.popcornfx.com/PopcornFX/latest/plugins/ue-plugin/effects.html#_effect_materials"), NULL, NULL);
					return FReply::Handled();
				})
		];
	catRenderMats.HeaderContent(hbox.ToSharedRef());

	IDetailCategoryBuilder		&catAssetDeps = DetailLayout.EditCategory("PopcornFX AssetDependencies");
	catAssetDeps.SetDisplayName(FText::FromString("Low level asset dependency edition"));
	catAssetDeps.InitiallyCollapsed(true);
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
					FPlatformProcess::LaunchURL(TEXT("https://documentation.popcornfx.com/PopcornFX/latest/plugins/ue-plugin/effects.html#_effect_dependencies"), NULL, NULL);
					return FReply::Handled();
				})
		];
	catAssetDeps.HeaderContent(hbox.ToSharedRef());

	BuildRendererMaterialsArray(DetailLayout, catRenderMats, GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXEffect, ParticleRendererMaterials));
	BuildAssetDependenciesArray(DetailLayout, catAssetDeps, "AssetDependencies", UPopcornFXFile::StaticClass());
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
