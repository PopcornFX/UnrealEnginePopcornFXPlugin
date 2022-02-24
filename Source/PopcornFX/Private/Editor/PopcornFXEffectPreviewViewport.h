//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "Widgets/Docking/SDockTab.h"
#include "PreviewScene.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "SEditorViewport.h"

class UPopcornFXEffect;

class SPopcornFXEffectPreviewViewport : public SEditorViewport, public FGCObject, public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(SPopcornFXEffectPreviewViewport) { }
		SLATE_ARGUMENT(TWeakPtr<class FPopcornFXEffectEditor>, EffectEditor)
	SLATE_END_ARGS()

	void	Construct(const FArguments &args);
	~SPopcornFXEffectPreviewViewport();

	void	SetPreviewEffect(UPopcornFXEffect *effect);
	void	OnAddedToTab(const TSharedRef<SDockTab> &ownerTab);

	virtual void	AddReferencedObjects(FReferenceCollector& Collector) override;

	virtual TSharedRef<class SEditorViewport>	GetViewportWidget() override;
	virtual TSharedPtr<FExtender>				GetExtenders() const override;
	virtual void								OnFloatingButtonClicked() override { };

	virtual TSharedRef<FEditorViewportClient>	MakeEditorViewportClient() override;
	virtual void								PopulateViewportOverlays(TSharedRef<class SOverlay> Overlay) override;
	virtual EVisibility							OnGetViewportContentVisibility() const override;
	virtual void								BindCommands() override;
	virtual void								OnFocusViewportToSelection() override;
public:
	void			ResetEmitterAttributes();
	void			ResetEmitter();

	void			ToggleLoopEmitter(ECheckBoxState newState);
	ECheckBoxState	IsToggleLoopEmitterChecked() const;
	float			OnGetLoopDelayValue() const;
	void			OnLoopDelayValueChanged(float newDelay);
private:
	void			TogglePreviewFloor();
	bool			IsTogglePreviewFloorChecked() const { return m_DisplayFloor; }
	void			TogglePreviewCubemap();
	bool			IsTogglePreviewCubemapChecked() const { return m_DisplayCubemap; }
	void			TogglePreviewGrid();
	bool			IsTogglePreviewGridChecked() const { return m_DisplayGrid; }
	void			ToggleDrawBounds();
	bool			IsToggleDrawBoundsChecked() const;

	class UPopcornFXSceneComponent	*SceneComponent() const;

	virtual bool	IsVisible() const override;
private:
	bool	m_DisplayFloor;
	bool	m_DisplayCubemap;
	bool	m_DisplayGrid;

	FPreviewScene						m_PreviewScene;
	class UPopcornFXEmitterComponent	*m_EmitterComponent;

	TWeakPtr<SDockTab>						m_ParentTab;
	TWeakPtr<class FPopcornFXEffectEditor>	m_EffectEditor;

	TSharedPtr<class FPopcornFXEffectPreviewViewportClient>	m_ViewportClient;
};

#endif // WITH_EDITOR
