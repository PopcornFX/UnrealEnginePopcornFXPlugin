//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDependencyModuleLevelEditor.h"

#include "PopcornFXSDK.h"
#include "PopcornFXPlugin.h"

#include "LevelEditor.h"
#include "ISettingsModule.h"
#include "EditorStyleSet.h"
#include "Engine/Engine.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "PopcornFXLevelEditor"

//----------------------------------------------------------------------------

FPopcornFXDependencyModuleLevelEditor::FPopcornFXDependencyModuleLevelEditor()
:	m_Settings(null)
{
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleLevelEditor::Load()
{
	if (m_Loaded)
		return;
	if (!PK_VERIFY(FModuleManager::Get().IsModuleLoaded("LevelEditor")))
		return;
	m_Settings = GetMutableDefault<UPopcornFXSettingsEditor>();
	if (!PK_VERIFY(m_Settings != null))
		return;
	FLevelEditorModule		&levelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	TSharedPtr<FExtender>	customExtender = MakeShareable(new FExtender);
	if (!PK_VERIFY(customExtender.IsValid()))
		return;
	customExtender->AddMenuBarExtension(TEXT("Help"), EExtensionHook::After, null, FMenuBarExtensionDelegate::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::CreateMenuBarExtension));
	levelEditorModule.GetMenuExtensibilityManager()->AddExtender(customExtender);

	// Generate versions text
	FString	versionsText;
	versionsText += "Plugin version: ";
	versionsText += FPopcornFXPlugin::Get().PluginVersionString();
	versionsText += "\nRuntime SDK version: ";
	versionsText += FPopcornFXPlugin::Get().PopornFXRuntimeVersionString();
	m_Versions = FText::FromString(versionsText);

	m_Loaded = true;
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleLevelEditor::Unload()
{
	if (!m_Loaded)
		return;
	m_Loaded = false;

	if (!FModuleManager::Get().IsModuleLoaded("LevelEditor"))
		return;
}

//----------------------------------------------------------------------------

bool	FPopcornFXDependencyModuleLevelEditor::CanOpenSourcePack()
{
	PK_ASSERT(IsInGameThread());
	PK_ASSERT(m_Loaded);
	PK_ASSERT(m_Settings != null);

	return m_Settings->bSourcePackFound;
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleLevelEditor::OpenSourcePack()
{
	PK_ASSERT(IsInGameThread());
	PK_ASSERT(m_Loaded);
	PK_ASSERT(m_Settings != null);
	PK_ASSERT(m_Settings->bSourcePackFound);

	// We can now directly rely on opening the pkproj directly
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*m_Settings->SourcePackProjectFile);
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleLevelEditor::CreateMenuBarExtension(FMenuBarBuilder &menubarBuilder)
{
	menubarBuilder.AddPullDownMenu(
		LOCTEXT("MenuBarTitle", "PopcornFX"),
		LOCTEXT("MenuBarTooltip", "Open the PopcornFX menu"),
		FNewMenuDelegate::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::FillPopcornFXMenu));
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleLevelEditor::FillPopcornFXMenu(FMenuBuilder &menuBuilder)
{
	menuBuilder.BeginSection("Documentation", LOCTEXT("DocTitle", "Documentation"));
	{
		// Documentation
		menuBuilder.AddMenuEntry(
			LOCTEXT("DocumentationTitle", "Documentation"),
			LOCTEXT("DocumentationTooltip", "Opens the PopcornFX Wiki"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BrowseDocumentation"),
			FUIAction(FExecuteAction::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::OpenDocumentationURL, *FPopcornFXPlugin::DocumentationURL())));

		menuBuilder.AddMenuEntry(
			LOCTEXT("WikiTitle", "Plugin Wiki"),
			LOCTEXT("WikiTooltip", "Opens the PopcornFX UE4 plugin wiki"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BrowseDocumentation"),
			FUIAction(FExecuteAction::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::OpenDocumentationURL, *FPopcornFXPlugin::PluginWikiURL())));

		menuBuilder.AddMenuEntry(
			LOCTEXT("AnswerHubTitle", "AnswerHub"),
			LOCTEXT("AnswerHubTooltip", "Opens the PopcornFX Answerhub"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BrowseDocumentation"),
			FUIAction(FExecuteAction::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::OpenDocumentationURL, *FPopcornFXPlugin::AnswerHubURL())));
	}
	menuBuilder.EndSection();

	menuBuilder.BeginSection("Pack", LOCTEXT("PackTitle", "Source PopcornFX Project"));
	{
		FText	tooltip;
		if (!m_Settings->bSourcePackFound)
			tooltip = LOCTEXT("OpenSourcePackInvalidSourcePackTooltip", "Invalid 'Source PopcornFX Project path' in 'Project Settings > PopcornFX Editor'");
		else
			tooltip = LOCTEXT("OpenSourcePackTooltip", "Opens the 'Source PopcornFX Project path' in PopcornFX Editor");

		// Source pack actions
		menuBuilder.AddMenuEntry(
			LOCTEXT("OpenSourcePackTitle", "Open PopcornFX Editor Project"),
			tooltip,
			FSlateIcon(FEditorStyle::GetStyleSetName(), "PopcornFXLevelEditor.OpenSourcePack"),
			FUIAction(FExecuteAction::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::OpenSourcePack),
			FCanExecuteAction::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::CanOpenSourcePack)));
	}
	menuBuilder.EndSection();

	menuBuilder.BeginSection("Quick Access", LOCTEXT("QuickAccessTitle", "Quick Access"));
	{
		menuBuilder.AddMenuEntry(
			LOCTEXT("RuntimeSettingsTitle", "Runtime settings"),
			LOCTEXT("RuntimeSettingsTooltip", "Opens the project runtime settings"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "ProjectSettings.TabIcon"),
			FUIAction(FExecuteAction::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::OpenSettings, TEXT("PopcornFXSettings"))));

		menuBuilder.AddMenuEntry(
			LOCTEXT("EditorSettingsTitle", "Editor settings"),
			LOCTEXT("EditorSettingsTooltip", "Opens the project editor settings"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "ProjectSettings.TabIcon"),
			FUIAction(FExecuteAction::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::OpenSettings, TEXT("PopcornFXSettingsEditor"))));

		menuBuilder.AddMenuEntry(
			LOCTEXT("CmdStatPopcornFXTitle", "Cmd stat PopcornFX"),
			LOCTEXT("CmdStatPopcornFXTooltip", "Runs command \"stat PopcornFX\": Toggles PopcornFX Stats display."),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "Log.TabIcon"),
			FUIAction(FExecuteAction::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::RunCommand, TEXT("stat PopcornFX"))));

		menuBuilder.AddMenuEntry(
			LOCTEXT("CmdPopcornFXProfilerHUDTitle", "Cmd ToggleProfilerHUD"),
			LOCTEXT("CmdPopcornFXProfilerHUDTooltip", "Runs command \"PopcornFX.ToggleProfilerHUD\": Toggles PopcornFX Profiler HUD."),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "Log.TabIcon"),
			FUIAction(FExecuteAction::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::RunCommand, TEXT("PopcornFX.ToggleProfilerHUD"))));

		menuBuilder.AddMenuEntry(
			LOCTEXT("CmdPopcornFXDebugHUDTitle", "Cmd ToggleMemoryHUD"),
			LOCTEXT("CmdPopcornFXDebugHUDTooltip", "Runs command \"PopcornFX.ToggleMemoryHUD\": Toggles PopcornFX Memory HUD."),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "Log.TabIcon"),
			FUIAction(FExecuteAction::CreateRaw(this, &FPopcornFXDependencyModuleLevelEditor::RunCommand, TEXT("PopcornFX.ToggleMemoryHUD"))));

	}
	menuBuilder.EndSection();

	menuBuilder.BeginSection("Versions", LOCTEXT("VersionsTitle", "Versions"));
	{
		TAttribute<FText>::FGetter	dynamicVersionsGetter;
		dynamicVersionsGetter.BindRaw(this, &FPopcornFXDependencyModuleLevelEditor::GetVersions);
		TAttribute<FText>			dynamicVersions = TAttribute<FText>::Create(dynamicVersionsGetter);

		menuBuilder.AddMenuEntry(
			dynamicVersions,
			LOCTEXT("VersionsTooltip", "Current Plugin and Runtime SDK versions"),
			FSlateIcon(),
			FUIAction(null, FCanExecuteAction::CreateStatic(&FPopcornFXDependencyModuleLevelEditor::CannotExecuteAction)));
	}
	menuBuilder.EndSection();
}

//----------------------------------------------------------------------------

FText	FPopcornFXDependencyModuleLevelEditor::GetVersions()
{
	// Called each frame, return cached versions
	return m_Versions;
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleLevelEditor::OpenDocumentationURL(const TCHAR *url)
{
	PK_ASSERT(IsInGameThread());
	PK_ASSERT(m_Loaded);

	FPlatformProcess::LaunchURL(url, null, null);
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleLevelEditor::OpenSettings(const TCHAR *sectionName)
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "Plugins", sectionName);
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleLevelEditor::RunCommand(const TCHAR *command)
{
	check(GEngine);
	UWorld		*world = GWorld;
	if (!PK_VERIFY(world != null))
		return;
	GEditor->Exec(world, command);
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
