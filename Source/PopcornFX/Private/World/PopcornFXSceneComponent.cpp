//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXSceneComponent.h"

#include "PopcornFXPlugin.h"
#include "Render/RendererSubView.h"
#include "PopcornFXSceneProxy.h"
#include "Internal/ParticleScene.h"
#include "World/PopcornFXWaitForSceneActor.h"
#include "PopcornFXSceneActor.h"
#include "PopcornFXStats.h"
#include "Assets/PopcornFXEffectPriv.h"

#include "Engine/World.h"

#include "Engine/CollisionProfile.h"

#include "PopcornFXSDK.h"

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXSceneComponent, Log, All);

//----------------------------------------------------------------------------
//
// UPopcornFXSceneComponent
//
//----------------------------------------------------------------------------

UPopcornFXSceneComponent::UPopcornFXSceneComponent(const FObjectInitializer& PCIP)
	: Super(PCIP)
	, m_ParticleScene(null)
{
	const float			extent = 500;
	FixedRelativeBoundingBox = FBox(FVector(-extent), FVector(extent));

	PrimaryComponentTick.bCanEverTick = true;

	// Attr sampler skeletal mesh is executed:
	// - TG_PrePhysics for SkeletalMeshComponents
	// - TG_EndPhysics for DestructibleComponents

	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	PrimaryComponentTick.bRunOnAnyThread = false;
	// Needs thread register to PopcornFX and more safety on Emitters
	//PrimaryComponentTick.bRunOnAnyThread = true;

	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;

	bTickInEditor = true;
	//MaxTimeBeforeForceUpdateTransform = 5.0f;
	bAutoActivate = true;

	// If we are selectable, selecting a particle will select us (the Scene) instead of the Emitter
	bSelectable = false;

	bAllowConcurrentTick = true;

	SetGenerateOverlapEvents(false);

	bReceivesDecals = false;

	bCastVolumetricTranslucentShadow = true;
	//bCastVolumetricTranslucentShadow = false;

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	bEnableUpdate = true;
	bEnableRender = true;

	HeavyDebugMode = EPopcornFXHeavyDebugMode::None;
	bRender_FreezeBillboardingMatrix = false;

	struct FConstructorStatics
	{
		FName		DefaultSceneName;
		FConstructorStatics()
			: DefaultSceneName(TEXT("PopcornFX_DefaultScene"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;
	SceneName = ConstructorStatics.DefaultSceneName;

	//DefaultCollisionFilter = CreateDefaultSubobject<FPopcornFXCollisionFilter>(TEXT("DefaultCollisionFilter"));

	SetFlags(RF_Transactional); // Allow this actor's deletion
}

//----------------------------------------------------------------------------

// Copy paste of UPrimitiveComponent::ShouldRenderSelected as we can't rely on it, because PopcornFXSceneComponent::bSelectable is false
// We don't want to enable translucent selection of PK particles, otherwise it'll select the scene actor instead of the emitter (which isn't possible with PK)
// But, we want to enable 'render as selected' for things like bounds draw
bool	UPopcornFXSceneComponent::ShouldRenderSelected() const
{
	if (const AActor* Owner = GetOwner())
	{
		if (Owner->IsSelected())
		{
			return true;
		}
		else if (Owner->IsChildActor())
		{
			AActor* ParentActor = Owner->GetParentActor();
			while (ParentActor->IsChildActor())
			{
				ParentActor = ParentActor->GetParentActor();
			}
			return ParentActor->IsSelected();
		}
	}
	return false;
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::OnUnregister()
{
	if (m_OnSettingsChangedHandle.IsValid())
	{
		FPopcornFXPlugin::Get().m_OnSettingsChanged.Remove(m_OnSettingsChangedHandle);
		m_OnSettingsChangedHandle.Reset();
	}

	// Called each time a property is modified !
	// ! So, do not delete here !
	//CParticleScene::SafeDelete(m_ParticleScene);
	//m_ParticleScene = null;

	Super::OnUnregister();
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::BeginDestroy()
{
	CParticleScene::SafeDelete(m_ParticleScene);
	m_ParticleScene = null;
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::PostLoad()
{
	ResolveSettings();
	Super::PostLoad();
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::PostInitProperties()
{
	Super::PostInitProperties();
}

//----------------------------------------------------------------------------
#if WITH_EDITOR

void	UPopcornFXSceneComponent::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	static FName		SimulationSettingsOverrideName(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXSceneComponent, SimulationSettingsOverride));
	static FName		RenderSettingsOverrideName(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXSceneComponent, RenderSettingsOverride));
	if (propertyChangedEvent.MemberProperty != null)
	{
		const FName		memberName = propertyChangedEvent.MemberProperty->GetFName();
		if (memberName == SimulationSettingsOverrideName ||
			memberName == RenderSettingsOverrideName)
			ResolveSettings();
	}
	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::MirrorGameWorldProperties(const UPopcornFXSceneComponent *other)
{
	PK_ASSERT(other != null);
	PK_ASSERT(other->GetWorld() != null);
	PK_ASSERT(other->GetWorld()->IsGameWorld() || other->GetWorld()->WorldType == EWorldType::Editor);

	SceneName = other->SceneName;
	bEnableUpdate = other->bEnableUpdate;
	bEnableRender = other->bEnableRender;
	BoundingBoxMode = other->BoundingBoxMode;
	FixedRelativeBoundingBox = other->FixedRelativeBoundingBox;
	SimulationSettingsOverride = other->SimulationSettingsOverride;
	RenderSettingsOverride = other->RenderSettingsOverride;
}

#endif // WITH_EDITOR
//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::OnRegister()
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXSceneComponent::OnRegister", POPCORNFX_UE_PROFILER_COLOR);

	Super::OnRegister();
	ResolveSettings();

	SetTickGroup(m_ResolvedSimulationSettings.SceneUpdateTickGroup.GetValue());

	if (m_ParticleScene == null && !IsTemplate() && !IsRunningCommandlet())
	{
		m_ParticleScene = CParticleScene::CreateNew(this);
	}

#if WITH_EDITOR
	// Pull all Emitters that have been registered before us
	APopcornFXWaitForScene::NotifySceneLoaded(this);
#endif
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction *thisTickFunction)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXSceneComponent::TickComponent", POPCORNFX_UE_PROFILER_COLOR);
	SCOPE_CYCLE_COUNTER(STAT_PopcornFX_PopcornFXUpdateTime);
	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	if (m_ParticleScene == null || bEnableUpdate == 0 || deltaTime == 0)
		return;

	if (!m_OnSettingsChangedHandle.IsValid())
	{
		m_OnSettingsChangedHandle = FPopcornFXPlugin::Get().m_OnSettingsChanged.AddUObject(this, &UPopcornFXSceneComponent::_OnSettingsChanged);
	}

	PK_ASSERT(m_ParticleScene->SceneComponent() == this);

	m_ParticleScene->StartUpdate(deltaTime);

	FBoxSphereBounds			bounds;

	if (BoundingBoxMode == EPopcornFXSceneBBMode::Dynamic)
	{
		bounds = m_ParticleScene->Bounds();
	}
	else if (BoundingBoxMode == EPopcornFXSceneBBMode::DynamicPlusFixed)
	{
		bounds = Union(FBoxSphereBounds(FixedRelativeBoundingBox.ShiftBy(GetComponentLocation())), m_ParticleScene->Bounds());
	}
	else if (BoundingBoxMode == EPopcornFXSceneBBMode::Fixed)
		bounds = FixedRelativeBoundingBox.ShiftBy(GetComponentLocation());

	if (bounds.SphereRadius <= 0.001f)
	{
		FBoxSphereBounds		selfLittleBounds(GetComponentLocation(), FVector(100.f), 100.f);
		bounds = Union(bounds, selfLittleBounds);
	}

	if (bounds.Origin != Bounds.Origin || bounds.SphereRadius != Bounds.SphereRadius)
	{
		Bounds = bounds;
		UpdateBounds();
		UpdateComponentToWorld();
	}

	PK_FIXME("MarkRenderDynamicDataDirty only if needed");
	MarkRenderDynamicDataDirty();

	if (m_ParticleScene->PostUpdate_ShouldMarkRenderStateDirty())
		MarkRenderStateDirty();
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::ApplyWorldOffset(const FVector &inOffset, bool worldShift)
{
	Super::ApplyWorldOffset(inOffset, worldShift);

	// Should we ignore world offsets if worldShift is false ?
	if (m_ParticleScene == null)
		return;
	m_ParticleScene->ApplyWorldOffset(inOffset);
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::SendRenderDynamicData_Concurrent()
{
	LLM_SCOPE(ELLMTag::Particles);
	SCOPE_CYCLE_COUNTER(STAT_PopcornFX_SRDDTime);

	Super::SendRenderDynamicData_Concurrent();

	if (m_ParticleScene == null)
		return;
	m_ParticleScene->SendRenderDynamicData_Concurrent();
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::CreateRenderState_Concurrent(FRegisterComponentContext *context)
{
	Super::CreateRenderState_Concurrent(context);

	LLM_SCOPE(ELLMTag::Particles);
	SendRenderDynamicData_Concurrent();
}

//----------------------------------------------------------------------------

FBoxSphereBounds	UPopcornFXSceneComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	return Bounds;
}

//----------------------------------------------------------------------------

FPrimitiveSceneProxy	*UPopcornFXSceneComponent::CreateSceneProxy()
{
	LLM_SCOPE(ELLMTag::Particles);
	FPopcornFXSceneProxy *sceneProxy = new FPopcornFXSceneProxy(this);
	return sceneProxy;
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);
	if (m_ParticleScene == null)
		return;
	m_ParticleScene->GetUsedMaterials(OutMaterials, bGetDebugMaterials);
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::ResolveSettings()
{
	SimulationSettingsOverride.ResolveSettingsTo(m_ResolvedSimulationSettings);
	RenderSettingsOverride.ResolveSettingsTo(m_ResolvedRenderSettings);
}

//----------------------------------------------------------------------------
//
// Blueprint functions
//
//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::Clear()
{
	if (!PK_VERIFY(m_ParticleScene != null))
		return;
	m_ParticleScene->Clear();
}

//----------------------------------------------------------------------------

bool	UPopcornFXSceneComponent::InstallEffects(TArray<UPopcornFXEffect*> Effects)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXSceneComponent::InstallEffects", POPCORNFX_UE_PROFILER_COLOR);

	check(FPopcornFXPlugin::IsMainThread()); // Synchronous and on main thread. Async preload not supported yet

	if (!PK_VERIFY(m_ParticleScene != null))
		return false;
	if (!PK_VERIFY(m_ParticleScene->Unsafe_ParticleMediumCollection() != null))
	{
		UE_LOG(LogPopcornFXSceneComponent, Error, TEXT("Couldn't install effects into '%s': scene is not initialized"), *SceneName.ToString());
		return false;
	}
	bool		success = true;
	const u32	effectCount = Effects.Num();
	for (u32 iEffect = 0; iEffect < effectCount; ++iEffect)
	{
		if (Effects[iEffect] == null)
		{
			UE_LOG(LogPopcornFXSceneComponent, Warning, TEXT("Trying to install null effect into '%s'"), *SceneName.ToString());
			continue;
		}
		PopcornFX::PCParticleEffect	effect = Effects[iEffect]->Effect()->ParticleEffect(); // Will load if necessary
		if (effect == null)
		{
			UE_LOG(LogPopcornFXSceneComponent, Warning, TEXT("Trying to install null effect into '%s'"), *SceneName.ToString());
			continue;
		}
		if (!m_ParticleScene->Effect_Install(effect))
		{
			success = false;
			UE_LOG(LogPopcornFXSceneComponent, Warning, TEXT("Couldn't install '%s' into '%s'"), *Effects[iEffect]->GetPathName(), *SceneName.ToString());
		}
	}
	return success;
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::SetAudioSamplingInterface(IPopcornFXFillAudioBuffers *fillAudioBuffers)
{
	PK_ASSERT(IsInGameThread());
	if (!PK_VERIFY(m_ParticleScene != null))
		return;
	PK_ASSERT(m_ParticleScene->m_FillAudioBuffers == null);
	m_ParticleScene->m_FillAudioBuffers = fillAudioBuffers;
}

//----------------------------------------------------------------------------

void	UPopcornFXSceneComponent::SetAudioInterface(IPopcornFXAudio *audioInterface)
{
	PK_ASSERT(IsInGameThread());
	if (!PK_VERIFY(m_ParticleScene != null))
		return;
	m_ParticleScene->SetAudioInterface(audioInterface);
}

//----------------------------------------------------------------------------
