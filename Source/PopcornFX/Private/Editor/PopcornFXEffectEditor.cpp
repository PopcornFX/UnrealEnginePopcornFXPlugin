//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXEffectEditor.h"

#include "Assets/PopcornFXEffect.h"
#include "PopcornFXAttributeList.h"
#include "Editor/CustomizeDetails/PopcornFXDetailsEffect.h"
#include "Editor/CustomizeDetails/PopcornFXDetailsEffectAttributes.h"
#include "Editor/PopcornFXEffectPreviewViewport.h"

#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/PopcornFXStyle.h"
#include "EditorReimportHandler.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SBoxPanel.h"

#include "PopcornFXSDK.h"

#define LOCTEXT_NAMESPACE "FPopcornFXEffectEditor"

const FName			FPopcornFXEffectEditor::ToolbarTabId(TEXT("PopcornFXEffectEditor_Tab"));
const FName			FPopcornFXEffectEditor::PropertiesTabId(TEXT("PopcornFXEffectEditor_Properties"));
const FName			FPopcornFXEffectEditor::EffectPreviewTabId(TEXT("PopcornFXEffectEditor_Preview"));
const FName			FPopcornFXEffectEditor::EffectAttributesTabId(TEXT("PopcornFXEffectEditor_Attributes"));

//----------------------------------------------------------------------------

FPopcornFXEffectEditorCommands::FPopcornFXEffectEditorCommands()
:	TCommands<FPopcornFXEffectEditorCommands>("PopcornFXEffectEditor", LOCTEXT("PopcornFXEffectEditor", "Effect Editor"), NAME_None, FPopcornFXStyle::GetStyleSetName())
{

}

//----------------------------------------------------------------------------

void	FPopcornFXEffectEditorCommands::RegisterCommands()
{
	// Viewport "Show" menu
	UI_COMMAND(TogglePreviewFloor, "Floor", "Toggles the preview floor visibility", EUserInterfaceActionType::ToggleButton, FInputChord()); // No icon
	UI_COMMAND(TogglePreviewCubemap, "Cubemap", "Toggles the preview cubemap visibility", EUserInterfaceActionType::ToggleButton, FInputChord()); // No icon
	UI_COMMAND(TogglePreviewGrid, "Grid", "Toggles the preview pane's grid", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(ToggleDrawBounds, "Bounds", "Toggles the bounds rendering", EUserInterfaceActionType::ToggleButton, FInputChord());

	UI_COMMAND(ReimportEffect, "Reimport", "Reimports the source effect", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenInPopcornFXEditor, "Open source", "Opens the source effect inside the PopcornFX-Editor", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ResetEmitter, "Reset", "Resets the emitter", EUserInterfaceActionType::Button, FInputChord(EKeys::SpaceBar));
	UI_COMMAND(LoopEmitter, "Loop emitter", "Toggles lopping of this emitter", EUserInterfaceActionType::ToggleButton, FInputChord()); // No icon
}

//----------------------------------------------------------------------------

FPopcornFXEffectEditor::FPopcornFXEffectEditor()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FPopcornFXEffectEditor::OnObjectPropertyChanged);
}

//----------------------------------------------------------------------------

FPopcornFXEffectEditor::~FPopcornFXEffectEditor()
{
	if (m_Effect != null)
		m_Effect->m_OnFileChanged.Remove(m_OnFileChangedHandle);
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

//----------------------------------------------------------------------------

FName		FPopcornFXEffectEditor::GetToolkitFName() const
{
	return FName("PopcornFXEffectEditor");
}

//----------------------------------------------------------------------------

FText		FPopcornFXEffectEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "PopcornFX Effect Editor");
}

//----------------------------------------------------------------------------

FString		FPopcornFXEffectEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "PopcornFX Effect ").ToString();
}

//----------------------------------------------------------------------------

FLinearColor FPopcornFXEffectEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.2f, 0.2f, 0.7f, 0.5f);
}

//----------------------------------------------------------------------------

void		FPopcornFXEffectEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_PopcornFXEffecEditor", "PopcornFX Effect Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	InTabManager->RegisterTabSpawner(PropertiesTabId, FOnSpawnTab::CreateSP(this, &FPopcornFXEffectEditor::SpawnTab_Properties))
		.SetDisplayName(LOCTEXT("PropertiesTab", "PopcornFX Effect Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FPopcornFXStyle::GetStyleSetName(), "ClassIcon.PopcornFXEffect"));

	InTabManager->RegisterTabSpawner(EffectPreviewTabId, FOnSpawnTab::CreateSP(this, &FPopcornFXEffectEditor::SpawnTab_Preview))
		.SetDisplayName(LOCTEXT("EffectPreviewTab", "PopcornFX Effect Preview"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FPopcornFXStyle::GetStyleSetName(), "ClassIcon.PopcornFXEffect"));

	InTabManager->RegisterTabSpawner(EffectAttributesTabId, FOnSpawnTab::CreateSP(this, &FPopcornFXEffectEditor::SpawnTab_Attributes))
		.SetDisplayName(LOCTEXT("AttributesTab", "PopcornFX Effect Attributes"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FPopcornFXStyle::GetStyleSetName(), "ClassIcon.PopcornFXEffect"));
}

//----------------------------------------------------------------------------

void		FPopcornFXEffectEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(PropertiesTabId);
	InTabManager->UnregisterTabSpawner(EffectPreviewTabId);
	InTabManager->UnregisterTabSpawner(EffectAttributesTabId);
}

//----------------------------------------------------------------------------

void	FPopcornFXEffectEditor::FillEffectToolbar(FToolBarBuilder &toolbarBuilder)
{
	const FPopcornFXEffectEditorCommands	&effectCommands = FPopcornFXEffectEditorCommands::Get();

	toolbarBuilder.BeginSection("Source");
	{
		toolbarBuilder.AddToolBarButton(effectCommands.ReimportEffect);
		toolbarBuilder.AddToolBarButton(effectCommands.OpenInPopcornFXEditor);
	}
	toolbarBuilder.EndSection();
	toolbarBuilder.BeginSection("Actions");
	{
		toolbarBuilder.AddToolBarButton(effectCommands.ResetEmitter);
	}
	toolbarBuilder.EndSection();
	toolbarBuilder.BeginSection("Loop");
	{
		toolbarBuilder.AddWidget(
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SCheckBox)
				.IsChecked(PreviewViewport.Get(), &SPopcornFXEffectPreviewViewport::IsToggleLoopEmitterChecked)
				.OnCheckStateChanged(PreviewViewport.Get(), &SPopcornFXEffectPreviewViewport::ToggleLoopEmitter)
				.Padding(FMargin(6.0, 2.0))
				.ToolTipText(effectCommands.LoopEmitter->GetDescription())
				.Content()
				[
					SNew(STextBlock)
					.Text(effectCommands.LoopEmitter->GetLabel())
				]
			]
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DelayTitle", "Delay"))
					]
				]
				+SHorizontalBox::Slot()
				[
					SNew(SSpinBox<float>)
					.ToolTipText(LOCTEXT("DelayTooltip", "Time before the emitter restarts"))
					.MinValue(0.0f)
					.MaxValue(20.0f)
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
					.Font(FAppStyle::GetFontStyle(TEXT("MenuItem.Font")))
#else
					.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
					.Value(PreviewViewport.Get(), &SPopcornFXEffectPreviewViewport::OnGetLoopDelayValue)
					.OnValueChanged(PreviewViewport.Get(), &SPopcornFXEffectPreviewViewport::OnLoopDelayValueChanged)
				]
			]);
	}
	toolbarBuilder.EndSection();
}

//----------------------------------------------------------------------------

void	FPopcornFXEffectEditor::OnFileChanged(const UPopcornFXFile *file)
{
	PK_ASSERT(EffectDetailsView.IsValid());
	PK_ASSERT(EffectAttributesView.IsValid());
	PK_ASSERT(m_Effect == file);

	// Really what we need is just an update.. not SetObject
	EffectAttributesView->SetObject(m_Effect);
	EffectDetailsView->SetObject(m_Effect);
}

//----------------------------------------------------------------------------

void	FPopcornFXEffectEditor::OnObjectPropertyChanged(UObject *object, FPropertyChangedEvent &propertyChangedEvent)
{
	if (object == null ||
		m_Effect == null ||
		!PreviewViewport.IsValid())
		return;
	const UPopcornFXAttributeList	*defaultAttrList = Cast<UPopcornFXAttributeList>(object);
	if (defaultAttrList == null ||
		!m_Effect->IsTheDefaultAttributeList(defaultAttrList))
		return;
	PreviewViewport->ResetEmitterAttributes();
}

//----------------------------------------------------------------------------

void	FPopcornFXEffectEditor::InitEffectEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UPopcornFXEffect* Effect)
{
	check(Effect != null);
	Effect->LoadEffectIFN();
	m_Effect = Effect;

	PK_ASSERT(!m_OnFileChangedHandle.IsValid());
	m_OnFileChangedHandle = m_Effect->m_OnFileChanged.AddRaw(this, &FPopcornFXEffectEditor::OnFileChanged);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	FDetailsViewArgs	DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	EffectDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	EffectAttributesView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	FOnGetDetailCustomizationInstance LayoutCustomEffectProperties = FOnGetDetailCustomizationInstance::CreateSP(this, &FPopcornFXEffectEditor::MakeEffectDetails);
	EffectDetailsView->RegisterInstancedCustomPropertyLayout(UPopcornFXEffect::StaticClass(), LayoutCustomEffectProperties);
	EffectDetailsView->SetObject(m_Effect);

	FOnGetDetailCustomizationInstance	LayoutCustomEffectAttributes = FOnGetDetailCustomizationInstance::CreateSP(this, &FPopcornFXEffectEditor::MakeEffectAttributesDetails);
	EffectAttributesView->RegisterInstancedCustomPropertyLayout(UPopcornFXEffect::StaticClass(), LayoutCustomEffectAttributes);
	EffectAttributesView->SetObject(m_Effect);

	FPopcornFXEffectEditorCommands::Register();

	PreviewViewport = SNew(SPopcornFXEffectPreviewViewport).EffectEditor(SharedThis(this));

	BindCommands();

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_PopcornFXEditor_Layout_v2")
	->AddArea
	(
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.1f)
			->SetHideTabWell(true)
			->AddTab(ToolbarTabId, ETabState::OpenedTab)
		)
		->Split
		(
			FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->SetHideTabWell(true)
					->AddTab(EffectPreviewTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.3f)
					->SetHideTabWell(true)
					->AddTab(EffectAttributesTabId, ETabState::OpenedTab)
				)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.7f)
				->SetHideTabWell(true)
				->AddTab(PropertiesTabId, ETabState::OpenedTab)
			)
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, FName(TEXT("PopcornFXEffectEditorApp")), StandaloneDefaultLayout, bCreateDefaultToolbar, bCreateDefaultStandaloneMenu, Effect);

	TSharedPtr<FExtender>	ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FPopcornFXEffectEditor::FillEffectToolbar));

	AddToolbarExtender(ToolbarExtender);

	RegenerateMenusAndToolbars();
}

//----------------------------------------------------------------------------

void	FPopcornFXEffectEditor::BindCommands()
{
	const FPopcornFXEffectEditorCommands&	commands = FPopcornFXEffectEditorCommands::Get();

	ToolkitCommands->MapAction(
		commands.ReimportEffect,
		FExecuteAction::CreateSP(this, &FPopcornFXEffectEditor::ReimportEffect),
		FCanExecuteAction::CreateSP(this, &FPopcornFXEffectEditor::DoesSourceEffectExist));

	ToolkitCommands->MapAction(
		commands.OpenInPopcornFXEditor,
		FExecuteAction::CreateSP(this, &FPopcornFXEffectEditor::OpenInPopcornFXEditor),
		FCanExecuteAction::CreateSP(this, &FPopcornFXEffectEditor::DoesSourceEffectExist));

	PK_ASSERT(PreviewViewport.IsValid());
	ToolkitCommands->MapAction(
		commands.ResetEmitter,
		FExecuteAction::CreateSP(PreviewViewport.Get(), &SPopcornFXEffectPreviewViewport::ResetEmitter));
}

//----------------------------------------------------------------------------

bool	FPopcornFXEffectEditor::DoesSourceEffectExist()
{
	PK_ASSERT(m_Effect != null);

	return FPaths::FileExists(m_Effect->FileSourcePath());
}

//----------------------------------------------------------------------------

void	FPopcornFXEffectEditor::ReimportEffect()
{
	PK_ASSERT(m_Effect != null);

	FReimportManager::Instance()->Reimport(m_Effect, true);
}

//----------------------------------------------------------------------------

void	FPopcornFXEffectEditor::OpenInPopcornFXEditor()
{
	PK_ASSERT(m_Effect != null);

	FPlatformProcess::LaunchFileInDefaultExternalApplication(*m_Effect->FileSourcePath());
}

//----------------------------------------------------------------------------

TSharedRef<class IDetailCustomization>	FPopcornFXEffectEditor::MakeEffectDetails()
{
	TSharedRef<FPopcornFXDetailsEffect> NewDetails = MakeShareable(new FPopcornFXDetailsEffect(/**this*/));
	EffectDetails = NewDetails;
	return NewDetails;
}

//----------------------------------------------------------------------------

TSharedRef<class IDetailCustomization>	FPopcornFXEffectEditor::MakeEffectAttributesDetails()
{
	TSharedRef<FPopcornFXDetailsEffectAttributes> NewDetails = MakeShareable(new FPopcornFXDetailsEffectAttributes());
	EffectAttributesDetails = NewDetails;
	return NewDetails;
}

//----------------------------------------------------------------------------

TSharedRef<SDockTab>	FPopcornFXEffectEditor::SpawnTab_Properties(const FSpawnTabArgs &args)
{
	check(args.GetTabId() == PropertiesTabId);

	return SNew(SDockTab)
		.Label(LOCTEXT("PopcornFXEffectProperties_TabTitle", "Details"))
		[
			EffectDetailsView.ToSharedRef()
		];
}

//----------------------------------------------------------------------------

TSharedRef<SDockTab>	FPopcornFXEffectEditor::SpawnTab_Preview(const FSpawnTabArgs &args)
{
	check(args.GetTabId() == EffectPreviewTabId);

	return SNew(SDockTab)
		.Label(LOCTEXT("PopcornFXEffectPreview_TabTitle", "Preview"))
		[
			PreviewViewport.ToSharedRef()
		];
}

//----------------------------------------------------------------------------

TSharedRef<SDockTab>	FPopcornFXEffectEditor::SpawnTab_Attributes(const FSpawnTabArgs &args)
{
	check(args.GetTabId() == EffectAttributesTabId);

	return SNew(SDockTab)
		.Label(LOCTEXT("PopcornFXEffectAttributes_TabTitle", "Attributes"))
		[
			EffectAttributesView.ToSharedRef()
		];
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
