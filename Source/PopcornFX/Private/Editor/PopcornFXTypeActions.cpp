//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXTypeActions.h"

#include "IPopcornFXPlugin.h"
#include "Assets/PopcornFXEffect.h"
#include "Editor/EditorHelpers.h"

#include "EditorFramework/AssetImportData.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/FileManager.h"
#include "EditorReimportHandler.h"

#include "PopcornFXSDK.h"

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXFileTypeActions, Log, All);

//----------------------------------------------------------------------------

FPopcornFXFileTypeActions::FPopcornFXFileTypeActions(EAssetTypeCategories::Type category)
	: m_Category(category)
{
}

//----------------------------------------------------------------------------

void	FPopcornFXFileTypeActions::ExecuteEdit(TArray<TWeakObjectPtr<UPopcornFXFile>> objects)
{
	for (auto objIt = objects.CreateConstIterator(); objIt; ++objIt)
	{
		UPopcornFXFile	*file = (*objIt).Get();
		if (file == null)
			continue;
		if (!file->IsFileValid())
			continue;

		// Manually create the absolute path
		FString		filePath = file->FileSourcePath();

		if (!FPaths::FileExists(filePath))
		{
			UE_LOG(LogPopcornFXFileTypeActions, Error, TEXT("%s doesn't exist, please reimport the PopcornFX source file."), *filePath);
			continue;
		}
		FPlatformProcess::LaunchFileInDefaultExternalApplication(*filePath);
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXFileTypeActions::ExecuteReimport(TArray<TWeakObjectPtr<UPopcornFXFile>> objects)
{
	for (auto objIt = objects.CreateConstIterator(); objIt; ++objIt)
	{
		UPopcornFXFile	*file = (*objIt).Get();
		if (file == null)
			continue;

		FReimportManager::Instance()->Reimport(file, true);
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXFileTypeActions::ExecuteReimportReset(TArray<TWeakObjectPtr<UPopcornFXFile>> objects)
{
	for (auto objIt = objects.CreateConstIterator(); objIt; ++objIt)
	{
		UPopcornFXFile	*file = (*objIt).Get();
		if (file == null)
			continue;

		file->PreReimport_Clean();
		FReimportManager::Instance()->Reimport(file, true);
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXFileTypeActions::ExecuteFindInExplorer(TArray<TWeakObjectPtr<UPopcornFXFile>> objects)
{
	for (auto objIt = objects.CreateConstIterator(); objIt; ++objIt)
	{
		UPopcornFXFile	*file = (*objIt).Get();
		if (file == null)
			continue;
		if (!file->IsFileValid())
			continue;

		const FString	sourceFilePath = UAssetImportData::ResolveImportFilename(file->FileSourcePath(), file->GetOutermost());

		if (sourceFilePath.Len() && IFileManager::Get().FileSize(*sourceFilePath) != INDEX_NONE)
		{
			FPlatformProcess::ExploreFolder(*FPaths::GetPath(sourceFilePath));
		}
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXFileTypeActions::GetActions(const TArray<UObject*>& inObjects, FMenuBuilder& menuBuilder)
{
	auto	files = GetTypedWeakObjectPtrs<UPopcornFXFile>(inObjects);

	bool	effectsOnly = true;
	for (auto objIt = files.CreateConstIterator(); objIt; ++objIt)
	{
		const UPopcornFXFile	*file = (*objIt).Get();
		if (file == null)
			continue;
		if (Cast<UPopcornFXEffect>(file) == null)
		{
			effectsOnly = false;
			break;
		}
	}
	if (effectsOnly)
	{
		menuBuilder.AddMenuEntry(
			NSLOCTEXT("PopcornFXFileTypeActions", "File_Edit", "Open In PopcornFX Editor"),
			NSLOCTEXT("PopcornFXFileTypeActions", "File_EditTooltip", "Opens the Effect in PopcornFX Editor if possible."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&FPopcornFXFileTypeActions::ExecuteEdit, files),
				FCanExecuteAction()
				)
			);
	}

	menuBuilder.AddMenuEntry(
		NSLOCTEXT("PopcornFXFileTypeActions", "File_Reimport", "Reimport"),
		NSLOCTEXT("PopcornFXFileTypeActions", "File_ReimportTooltip", "Reimports Assets."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateStatic(&FPopcornFXFileTypeActions::ExecuteReimport, files),
			FCanExecuteAction()
			)
		);

	if (effectsOnly)
	{
		menuBuilder.AddMenuEntry(
			NSLOCTEXT("PopcornFXFileTypeActions", "File_ResetToDefault", "Reimport and Reset"),
			NSLOCTEXT("PopcornFXFileTypeActions", "File_ResetToDefaultTooltip", "Reimports and Resets to default values."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&FPopcornFXFileTypeActions::ExecuteReimportReset, files),
				FCanExecuteAction()
				)
			);
	}

	menuBuilder.AddMenuEntry(
		NSLOCTEXT("PopcornFXFileTypeActions", "File_FindInExplorer", "Find Source"),
		NSLOCTEXT("PopcornFXFileTypeActions", "File_FindInExplorerTooltip", "Opens explorer at the location of this asset."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateStatic(&FPopcornFXFileTypeActions::ExecuteFindInExplorer, files),
			FCanExecuteAction::CreateSP(this, &FPopcornFXFileTypeActions::CanExecuteSourceCommands)
			)
		);

	m_CanExecuteFindInExplorer = false;
	for (auto objIt = files.CreateConstIterator(); objIt; ++objIt)
	{
		const UPopcornFXFile	*file = (*objIt).Get();
		if (file == null)
			continue;
		if (!file->IsFileValid())
			continue;

		const FString	sourceFilePath = UAssetImportData::ResolveImportFilename(file->FileSourcePath(), file->GetOutermost());
		if (sourceFilePath.Len() && IFileManager::Get().FileSize(*sourceFilePath) != INDEX_NONE)
		{
			m_CanExecuteFindInExplorer = true;
			break;
		}
	}
}

//----------------------------------------------------------------------------

bool	FPopcornFXFileTypeActions::CanExecuteSourceCommands() const
{
	return m_CanExecuteFindInExplorer;
}

//----------------------------------------------------------------------------

void	FPopcornFXFileTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor /*= TSharedPtr<IToolkitHost>()*/)
{
	TArray<UObject*>		otherObjects;

	EToolkitMode::Type		Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto objIt = InObjects.CreateConstIterator(); objIt; ++objIt)
	{
		UPopcornFXFile	*file = Cast<UPopcornFXFile>(*objIt);
		if (file == null)
			continue;
		if (!file->IsFileValid())
			continue;

		UPopcornFXEffect		*effect = Cast<UPopcornFXEffect>(file);
		if (effect != null)
		{
			IPopcornFXPlugin	*PopcornFXPlugin = &FModuleManager::LoadModuleChecked<IPopcornFXPlugin>("PopcornFX");
			PopcornFXPlugin->CreateEffectEditor(Mode, EditWithinLevelEditor, effect);
		}
		else
		{
			otherObjects.Add(file);
		}
	}

	if (otherObjects.Num() > 0)
		FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, otherObjects);
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
