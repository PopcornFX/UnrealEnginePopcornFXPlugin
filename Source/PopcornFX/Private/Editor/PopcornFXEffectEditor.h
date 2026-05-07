//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "IPopcornFXEffectEditor.h"

#include "Framework/Commands/Commands.h"

class UPopcornFXFile;
class UPopcornFXEffect;

class FPopcornFXEffectEditorCommands : public TCommands<FPopcornFXEffectEditorCommands>
{
public:
	FPopcornFXEffectEditorCommands();

	virtual void	RegisterCommands() override;
public:
	// Realtime viewport commands
	TSharedPtr<FUICommandInfo>	TogglePreviewFloor;
	TSharedPtr<FUICommandInfo>	TogglePreviewCubemap;
	TSharedPtr<FUICommandInfo>	TogglePreviewGrid;
	TSharedPtr<FUICommandInfo>	ToggleDrawBounds;

	// Toolbar commands
	TSharedPtr<FUICommandInfo>	ReimportEffect;
	TSharedPtr<FUICommandInfo>	OpenInPopcornFXEditor;
	TSharedPtr<FUICommandInfo>	ResetEmitter;
	TSharedPtr<FUICommandInfo>	LoopEmitter;
};

class FPopcornFXEffectEditor : public IPopcornFXEffectEditor
{
public:
	FPopcornFXEffectEditor();
	virtual ~FPopcornFXEffectEditor();

	void				InitEffectEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UPopcornFXEffect* Effect);

	// IToolkit
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	// FAssetEditorToolkit
	virtual FName		GetToolkitFName() const override;
	virtual FText		GetBaseToolkitName() const override;
	virtual FString		GetWorldCentricTabPrefix() const override;

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	const UPopcornFXEffect		*Effect() const { return m_Effect; }
	UPopcornFXEffect			*Effect_Unsafe() const { return m_Effect; }

	TSharedPtr<class SPopcornFXEffectPreviewViewport>	GetPreviewViewport() const { return PreviewViewport; }

private:
	TSharedRef<class IDetailCustomization>	MakeEffectDetails();
	TSharedRef<class IDetailCustomization>	MakeEffectAttributesDetails();

	TSharedRef<SDockTab>		SpawnTab_Properties(const FSpawnTabArgs &Args);
	TSharedRef<SDockTab>		SpawnTab_Preview(const FSpawnTabArgs &args);
	TSharedRef<SDockTab>		SpawnTab_Attributes(const FSpawnTabArgs &args);

	void		OnObjectPropertyChanged(UObject *object, FPropertyChangedEvent &propertyChangedEvent);
	void		FillEffectToolbar(FToolBarBuilder &toolbarBuilder);
	void		BindCommands();
	bool		DoesSourceEffectExist();
	void		ReimportEffect();
	void		OpenInPopcornFXEditor();
	void		OnFileChanged(const UPopcornFXFile *file);
private:
	UPopcornFXEffect	*m_Effect;

	TSharedPtr<class IDetailsView>			EffectDetailsView;
	TSharedPtr<class IDetailsView>			EffectAttributesView;

	TWeakPtr<class FPopcornFXDetailsEffect>				EffectDetails;
	TWeakPtr<class FPopcornFXDetailsEffectAttributes>	EffectAttributesDetails;

	TSharedPtr<class SPopcornFXEffectPreviewViewport>	PreviewViewport;

	FDelegateHandle		m_OnFileChangedHandle;

	/**	The tab ids for all the tabs used */
	static const FName	ToolbarTabId;
	static const FName	PropertiesTabId;
	static const FName	EffectPreviewTabId;
	static const FName	EffectAttributesTabId;
};

#endif // WITH_EDITOR
