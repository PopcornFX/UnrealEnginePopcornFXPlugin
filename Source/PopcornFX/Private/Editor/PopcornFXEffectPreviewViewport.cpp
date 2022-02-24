//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXEffectPreviewViewport.h"

#include "Assets/PopcornFXEffect.h"
#include "PopcornFXAttributeList.h"
#include "PopcornFXSceneActor.h"
#include "PopcornFXSceneComponent.h"
#include "PopcornFXEmitterComponent.h"
#include "Editor/PopcornFXEffectEditor.h"

#include "Slate/SceneViewport.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureCube.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

#include "PopcornFXSDK.h"

//----------------------------------------------------------------------------

class	SPopcornFXEffectPreviewViewportToolbar : public SCommonEditorViewportToolbarBase
{
public:
	SLATE_BEGIN_ARGS(SPopcornFXEffectPreviewViewportToolbar) {}
	SLATE_END_ARGS()

	void	Construct(const FArguments &args, TSharedPtr<class SPopcornFXEffectPreviewViewport> viewport)
	{
		SCommonEditorViewportToolbarBase::Construct(SCommonEditorViewportToolbarBase::FArguments(), viewport);
	}

	virtual TSharedRef<SWidget>	GenerateShowMenu() const override
	{
		GetInfoProvider().OnFloatingButtonClicked();

		FMenuBuilder							showMenuBuilder(true, GetInfoProvider().GetViewportWidget()->GetCommandList());
		const FPopcornFXEffectEditorCommands	&commands = FPopcornFXEffectEditorCommands::Get();

		showMenuBuilder.AddMenuEntry(commands.TogglePreviewFloor);
		showMenuBuilder.AddMenuEntry(commands.TogglePreviewCubemap);
		showMenuBuilder.AddMenuEntry(commands.TogglePreviewGrid);
		showMenuBuilder.AddMenuEntry(commands.ToggleDrawBounds);
		return showMenuBuilder.MakeWidget();
	}
};

//----------------------------------------------------------------------------

class	FPopcornFXEffectPreviewViewportClient : public FEditorViewportClient
{
public:
	FPopcornFXEffectPreviewViewportClient(TWeakPtr<FPopcornFXEffectEditor> effectEditor, FPreviewScene &previewScene, const TSharedRef<SPopcornFXEffectPreviewViewport> &effectEditorViewport)
	:	FEditorViewportClient(null, &previewScene, StaticCastSharedRef<SEditorViewport>(effectEditorViewport))
	,	m_EffectEditor(effectEditor)
	{
		// Setup defaults for the common draw helper.
		DrawHelper.bDrawPivot = false;
		DrawHelper.bDrawWorldBox = false;
		DrawHelper.bDrawKillZ = false;
		DrawHelper.bDrawGrid = false;
		DrawHelper.GridColorAxis = FColor(80, 80, 80);
		DrawHelper.GridColorMajor = FColor(72, 72, 72);
		DrawHelper.GridColorMinor = FColor(64, 64, 64);
		DrawHelper.PerspectiveGridSize = HALF_WORLD_MAX1;

		SetViewMode(VMI_Lit);

		EngineShowFlags.DisableAdvancedFeatures();
		EngineShowFlags.SetSnap(0);

		OverrideNearClipPlane(1.0f);
		bUsingOrbitCamera = true;

		// Don't want to display the widget in this viewport
		Widget->SetDefaultVisibility(false);

		UStaticMesh	*floorMesh = LoadObject<UStaticMesh>(null, TEXT("/Engine/EditorMeshes/PhAT_FloorBox.PhAT_FloorBox"), null, LOAD_None, null);
		if (PK_VERIFY(floorMesh != null))
		{
			FTransform	floorTransform(FRotator(0.0f), FVector(0, 0, -10.0f), FVector(8.0f, 8.0f, 1.0f));

			m_FloorComponent = NewObject<UStaticMeshComponent>((UObject*)GetTransientPackage());
			m_FloorComponent->SetStaticMesh(floorMesh);
			PreviewScene->AddComponent(m_FloorComponent, floorTransform);
		}

		m_SkyComponent = NewObject<UStaticMeshComponent>((UObject*)GetTransientPackage());
		PK_ASSERT(m_SkyComponent != null);

		UStaticMesh		*skySphere = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorMeshes/AssetViewer/Sphere_inversenormals.Sphere_inversenormals"), null, LOAD_None, null);
		UMaterial		*skyMaterial = LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorMaterials/AssetViewer/M_SkyBox.M_SkyBox"), null, LOAD_None, null);
		UTextureCube	*defaultTexture = LoadObject<UTextureCube>(null, TEXT("/Engine/MapTemplates/Sky/DayLightAmbientCubemap.DayLightAmbientCubemap"));
		if (PK_VERIFY(skyMaterial != null) &&
			PK_VERIFY(skySphere != null) &&
			PK_VERIFY(defaultTexture != null))
		{
			UMaterialInstanceConstant	*instancedSkyMaterial = NewObject<UMaterialInstanceConstant>((UObject*)GetTransientPackage());

			PK_ASSERT(instancedSkyMaterial != null);
			instancedSkyMaterial->Parent = skyMaterial;
			instancedSkyMaterial->SetTextureParameterValueEditorOnly(FName("SkyBox"), defaultTexture);
			instancedSkyMaterial->PostLoad();

			m_SkyComponent->SetMaterial(0, instancedSkyMaterial);
			m_SkyComponent->SetStaticMesh(skySphere);
			m_SkyComponent->SetCollisionProfileName("NoCollision");

			PreviewScene->AddComponent(m_SkyComponent, FTransform(FRotator(0.0f), FVector(0.0f), FVector(2000.0f)));
		}
	}

	virtual ~FPopcornFXEffectPreviewViewportClient()
	{
		if (m_FloorComponent != null)
		{
			m_FloorComponent->ConditionalBeginDestroy();
			m_FloorComponent = null;
		}
		if (m_SkyComponent != null)
		{
			m_SkyComponent->ConditionalBeginDestroy();
			m_SkyComponent = null;
		}
	}

	virtual FLinearColor	GetBackgroundColor() const override { return FColor(64, 64, 64); };
	virtual void			Tick(float deltaSeconds) override
	{
		PK_ASSERT(IsInGameThread());

		// hidden !?
		if (Viewport->GetSizeXY().X <= 1 || Viewport->GetSizeXY().Y <= 1)
			return;

		FEditorViewportClient::Tick(deltaSeconds);

		// Tick the preview scene world.
		if (GIntraFrameDebuggingGameThread)
			return;
		if (m_EmitterComponent != null &&
			m_EmitterComponent->Effect != null)
		{
			if (m_EmitterComponent->Effect->EditorLoopEmitter)
			{
				m_Time += deltaSeconds;
				if (m_Time >= m_EmitterComponent->Effect->EditorLoopDelay)
				{
					PK_ASSERT(m_EmitterComponent != null);

					m_EmitterComponent->RestartEmitter();
					m_Time = 0.0f;
				}
			}
			else
				m_Time = 0.0f;
		}

		PreviewScene->GetWorld()->Tick(LEVELTICK_All, deltaSeconds);
	}

	void	SetDisplayGrid(bool displayGrid) { DrawHelper.bDrawGrid = displayGrid; }
	void	SetDisplayFloor(bool displayFloor) { PK_ASSERT(m_FloorComponent != null); m_FloorComponent->SetVisibility(displayFloor); }
	void	SetDisplayCubemap(bool displayCubemap) { PK_ASSERT(m_SkyComponent != null); m_SkyComponent->SetVisibility(displayCubemap); }
	void	SetEmitterComponent(UPopcornFXEmitterComponent *emitterComponent) { m_EmitterComponent = emitterComponent; }
	void	ResetLoopTimer() { m_Time = 0.0f; }
	void	FocusViewportOnBounds(const FBoxSphereBounds bounds)
	{
		float	radius = bounds.SphereRadius * 0.3f;
		float	aspectToUse = AspectRatio;

		FIntPoint	viewportSize = Viewport->GetSizeXY();
		if (!bUseControllingActorViewInfo && viewportSize.X > 0 && viewportSize.Y > 0)
			aspectToUse = Viewport->GetDesiredAspectRatio();

		ToggleOrbitCamera(false);

		if (aspectToUse > 1.0f)
			radius *= aspectToUse;

		const float		halfFOVRadians = FMath::DegreesToRadians(ViewFOV / 2.0f);
		const float		distanceFromSphere = radius / FMath::Sin(halfFOVRadians);

		FViewportCameraTransform	&viewTransform = GetViewTransform();
		const FVector	cameraOffsetVector = viewTransform.GetRotation().Vector() * -distanceFromSphere;

		viewTransform.SetLookAt(FVector(0.0f));
		viewTransform.TransitionToLocation(cameraOffsetVector, EditorViewportWidget, false);

		Invalidate();
	}

	virtual void	SetWidgetMode(FWidget::EWidgetMode newMode) override { }
private:
	UStaticMeshComponent	*m_FloorComponent;
	UStaticMeshComponent	*m_SkyComponent;

	UPopcornFXEmitterComponent	*m_EmitterComponent = null;
	float						m_Time = 0.0f;

	TWeakPtr<FPopcornFXEffectEditor>	m_EffectEditor;
};

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::Construct(const FArguments &args)
{
	m_EffectEditor = args._EffectEditor;
	m_DisplayFloor = true;
	m_DisplayCubemap = true;
	m_DisplayGrid = false;

	// Rotate the light in the preview scene so that it faces the preview object
	m_PreviewScene.SetLightDirection(FRotator(-40.0f, 27.5f, 0.0f));

	SEditorViewport::Construct( SEditorViewport::FArguments() );

	m_EmitterComponent = null;
	SetPreviewEffect(m_EffectEditor.Pin()->Effect_Unsafe());
}

//----------------------------------------------------------------------------

SPopcornFXEffectPreviewViewport::~SPopcornFXEffectPreviewViewport()
{
	if (m_ViewportClient.IsValid())
	{
		if (m_EmitterComponent != null)
		{
			UPopcornFXSceneComponent	*sceneComponent = m_EmitterComponent->Scene->PopcornFXSceneComponent;
			if (sceneComponent != null)
				sceneComponent->ConditionalBeginDestroy();
			m_PreviewScene.RemoveComponent(m_EmitterComponent);
			m_EmitterComponent->ConditionalBeginDestroy();
			m_EmitterComponent = null;
			m_ViewportClient->SetEmitterComponent(null);
		}
		m_ViewportClient->Viewport = NULL;
	}
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::AddReferencedObjects(FReferenceCollector &collector)
{
	collector.AddReferencedObject(m_EmitterComponent);
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::SetPreviewEffect(UPopcornFXEffect *effect)
{
	PK_ASSERT(IsInGameThread());
	if (!PK_VERIFY(effect != null))
		return;
	if (m_EmitterComponent == null)
	{
		// Should automatically resolve the preview scene and mirror the real scene if possible
		m_EmitterComponent = NewObject<UPopcornFXEmitterComponent>((UObject*)GetTransientPackage(), NAME_None, RF_Transient);
		if (!PK_VERIFY(m_EmitterComponent != null))
			return;
		m_EmitterComponent->bAutoDestroy = false;
		m_EmitterComponent->bPlayOnLoad = true;
		FTransform	spawnTransform = FTransform::Identity;
		m_PreviewScene.AddComponent(m_EmitterComponent, spawnTransform);
		m_ViewportClient->SetEmitterComponent(m_EmitterComponent);
		m_ViewportClient->ResetLoopTimer();
	}
	m_EmitterComponent->SetEffect(effect, true);
	return;
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::OnAddedToTab(const TSharedRef<SDockTab> &ownerTab)
{
	m_ParentTab = ownerTab;
}

//----------------------------------------------------------------------------

bool	SPopcornFXEffectPreviewViewport::IsVisible() const
{
	return ViewportWidget.IsValid() && (!m_ParentTab.IsValid() || m_ParentTab.Pin()->IsForeground()) && SEditorViewport::IsVisible();
}

//----------------------------------------------------------------------------

UPopcornFXSceneComponent	*SPopcornFXEffectPreviewViewport::SceneComponent() const
{
	if (!PK_VERIFY(m_EmitterComponent != null))
		return null;
	if (!PK_VERIFY(m_EmitterComponent->Scene != null))
		return null;
	return m_EmitterComponent->Scene->PopcornFXSceneComponent;
}

//----------------------------------------------------------------------------

bool	SPopcornFXEffectPreviewViewport::IsToggleDrawBoundsChecked() const
{
	const UPopcornFXSceneComponent	*sceneComponent = SceneComponent();
	if (!PK_VERIFY(sceneComponent != null))
		return false;
	return sceneComponent->HeavyDebugMode != EPopcornFXHeavyDebugMode::None;
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::ToggleDrawBounds()
{
	PK_ASSERT(IsInGameThread());
	UPopcornFXSceneComponent	*sceneComponent = SceneComponent();
	if (!PK_VERIFY(sceneComponent != null))
		return;
	sceneComponent->HeavyDebugMode = sceneComponent->HeavyDebugMode != EPopcornFXHeavyDebugMode::None ? EPopcornFXHeavyDebugMode::None : EPopcornFXHeavyDebugMode::MediumsBBox;
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::ResetEmitter()
{
	PK_ASSERT(IsInGameThread());
	UPopcornFXSceneComponent	*sceneComponent = SceneComponent();
	if (!PK_VERIFY(sceneComponent != null))
		return;
	sceneComponent->Clear();
	if (m_ViewportClient.IsValid())
		m_ViewportClient->ResetLoopTimer();
	m_EmitterComponent->StartEmitter();
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::BindCommands()
{
	SEditorViewport::BindCommands();

	const FPopcornFXEffectEditorCommands	&commands = FPopcornFXEffectEditorCommands::Get();

	CommandList->MapAction(
		commands.TogglePreviewFloor,
		FExecuteAction::CreateSP(this, &SPopcornFXEffectPreviewViewport::TogglePreviewFloor),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SPopcornFXEffectPreviewViewport::IsTogglePreviewFloorChecked));

	CommandList->MapAction(
		commands.TogglePreviewCubemap,
		FExecuteAction::CreateSP(this, &SPopcornFXEffectPreviewViewport::TogglePreviewCubemap),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SPopcornFXEffectPreviewViewport::IsTogglePreviewCubemapChecked));

	CommandList->MapAction(
		commands.TogglePreviewGrid,
		FExecuteAction::CreateSP(this, &SPopcornFXEffectPreviewViewport::TogglePreviewGrid),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SPopcornFXEffectPreviewViewport::IsTogglePreviewGridChecked));

	CommandList->MapAction(
		commands.ToggleDrawBounds,
		FExecuteAction::CreateSP(this, &SPopcornFXEffectPreviewViewport::ToggleDrawBounds),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SPopcornFXEffectPreviewViewport::IsToggleDrawBoundsChecked));
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::OnFocusViewportToSelection()
{
	const UPopcornFXSceneComponent	*sceneComponent = SceneComponent();
	if (!PK_VERIFY(sceneComponent != null))
		return;
	m_ViewportClient->FocusViewportOnBounds(sceneComponent->Bounds);
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::TogglePreviewFloor()
{
	m_DisplayFloor = !m_DisplayFloor;
	m_ViewportClient->SetDisplayFloor(m_DisplayFloor);
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::TogglePreviewCubemap()
{
	m_DisplayCubemap = !m_DisplayCubemap;
	m_ViewportClient->SetDisplayCubemap(m_DisplayCubemap);
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::TogglePreviewGrid()
{
	m_DisplayGrid = !m_DisplayGrid;
	m_ViewportClient->SetDisplayGrid(m_DisplayGrid);
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::ToggleLoopEmitter(ECheckBoxState newState)
{
	PK_ASSERT(m_EmitterComponent != null);
	PK_ASSERT(m_EmitterComponent->Effect != null);
	
	if (m_ViewportClient.IsValid())
		m_ViewportClient->ResetLoopTimer();
	m_EmitterComponent->Effect->EditorLoopEmitter = (newState == ECheckBoxState::Checked) ? true : false;
	m_EmitterComponent->Effect->MarkPackageDirty();
}

//----------------------------------------------------------------------------

ECheckBoxState	SPopcornFXEffectPreviewViewport::IsToggleLoopEmitterChecked() const
{
	PK_ASSERT(m_EmitterComponent != null);
	PK_ASSERT(m_EmitterComponent->Effect != null);

	return m_EmitterComponent->Effect->EditorLoopEmitter ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::OnLoopDelayValueChanged(float newDelay)
{
	PK_ASSERT(m_EmitterComponent != null);
	PK_ASSERT(m_EmitterComponent->Effect != null);

	m_EmitterComponent->Effect->EditorLoopDelay = newDelay;
	m_EmitterComponent->Effect->MarkPackageDirty();
}

//----------------------------------------------------------------------------

float	SPopcornFXEffectPreviewViewport::OnGetLoopDelayValue() const
{
	PK_ASSERT(m_EmitterComponent != null);
	PK_ASSERT(m_EmitterComponent->Effect != null);

	return m_EmitterComponent->Effect->EditorLoopDelay;
}

//----------------------------------------------------------------------------

TSharedRef<class SEditorViewport>	SPopcornFXEffectPreviewViewport::GetViewportWidget()
{
	return SharedThis(this);
}

//----------------------------------------------------------------------------

TSharedPtr<FExtender>	SPopcornFXEffectPreviewViewport::GetExtenders() const
{
	return MakeShareable(new FExtender);
}

//----------------------------------------------------------------------------

TSharedRef<FEditorViewportClient>	SPopcornFXEffectPreviewViewport::MakeEditorViewportClient()
{
	m_ViewportClient = MakeShareable(new FPopcornFXEffectPreviewViewportClient(m_EffectEditor, m_PreviewScene, SharedThis(this)));
	m_ViewportClient->SetViewLocation(FVector::ZeroVector);
	m_ViewportClient->SetViewRotation(FRotator::ZeroRotator);
	m_ViewportClient->SetViewLocationForOrbiting(FVector::ZeroVector);
	m_ViewportClient->bSetListenerPosition = false;
	m_ViewportClient->SetRealtime(true);
	m_ViewportClient->VisibilityDelegate.BindSP(this, &SPopcornFXEffectPreviewViewport::IsVisible);
	m_ViewportClient->SetDisplayGrid(m_DisplayGrid);
	m_ViewportClient->SetDisplayFloor(m_DisplayFloor);
	m_ViewportClient->SetDisplayCubemap(m_DisplayCubemap);
	m_ViewportClient->SetEmitterComponent(m_EmitterComponent);
	return m_ViewportClient.ToSharedRef();
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::PopulateViewportOverlays(TSharedRef<class SOverlay> overlay)
{
	overlay->AddSlot()
	.VAlign(VAlign_Top)
	[
		SNew(SPopcornFXEffectPreviewViewportToolbar, SharedThis(this))
	];
}

//----------------------------------------------------------------------------

void	SPopcornFXEffectPreviewViewport::ResetEmitterAttributes()
{
	PK_ASSERT(m_EmitterComponent != null);
	PK_ASSERT(m_EmitterComponent->Effect != null);
	PK_ASSERT(m_EmitterComponent->GetAttributeList() != null);

	m_EmitterComponent->GetAttributeList()->ResetToDefaultValues(m_EmitterComponent->Effect);
}

//----------------------------------------------------------------------------

EVisibility	SPopcornFXEffectPreviewViewport::OnGetViewportContentVisibility() const
{
	EVisibility		baseVisibility = SEditorViewport::OnGetViewportContentVisibility();

	if (baseVisibility != EVisibility::Visible)
		return baseVisibility;
	return IsVisible() ? EVisibility::Visible : EVisibility::Collapsed;
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
