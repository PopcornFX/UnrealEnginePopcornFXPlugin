//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXEmitterComponent.h"

#include "PopcornFXPlugin.h"
#include "Assets/PopcornFXEffect.h"
#include "PopcornFXSceneComponent.h"
#include "PopcornFXSceneActor.h"
#include "PopcornFXEmitter.h"
#include "Internal/ParticleScene.h"
#include "PopcornFXAttributeList.h"
#include "Render/RendererSubView.h"
#include "World/PopcornFXWaitForSceneActor.h"
#include "Assets/PopcornFXEffectPriv.h"

#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Misc/MapErrors.h"
#include "Components/BillboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "UObject/UObjectIterator.h"
#if WITH_EDITOR
#	include "Engine/Selection.h"
#	include "Editor.h"
#endif

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_descriptor.h>
#include <pk_particles/include/ps_effect.h>

#include <pk_particles_toolbox/include/pt_helpers.h>

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FPopcornFXEmitterComponent"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXEmitterComponent, Log, All);

//----------------------------------------------------------------------------

// static
UPopcornFXEmitterComponent	*UPopcornFXEmitterComponent::CreateStandaloneEmitterComponent(UPopcornFXEffect* effect, APopcornFXSceneActor *scene, UWorld* world, AActor* actor, bool bAutoDestroy)
{
	UObject							*outer = (actor ? actor : static_cast<UObject*>(world));
	PK_ASSERT(outer != null);
	UPopcornFXEmitterComponent		*psc = NewObject<UPopcornFXEmitterComponent>(outer);
	if (psc != null)
	{
		psc->bEnableUpdates = false;
		psc->bPlayOnLoad = false;
		psc->bKillParticlesOnDestroy = false;

		psc->bAutoDestroy = bAutoDestroy;
		//psc->SecondsBeforeInactive = 0.0f;
		psc->bAutoActivate = false;
		if (scene != null)
		{
			psc->SceneName = scene->GetSceneName();
			psc->Scene = scene;
		}
		psc->Effect = effect;
		//psc->bOverrideLODMethod = false;

		psc->RegisterComponentWithWorld(world);
	}
	return psc;
}

//----------------------------------------------------------------------------

UPopcornFXEmitterComponent::UPopcornFXEmitterComponent(const FObjectInitializer& PCIP)
	: Super(PCIP)
	, m_CurrentScene(null)
	, m_Scene_PreInitEmitterId(PopcornFX::CGuid::INVALID)
	, m_Scene_EmitterId(PopcornFX::CGuid::INVALID)
	, m_Started(false)
	, m_Stopped(false)
	, m_DiedThisFrame(false)
	, m_TeleportThisFrame(false)
#if WITH_EDITOR
	, m_ReplayAfterDead(false)
#endif
{
	bAutoActivate = true;
	//SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	bPlayOnLoad = true;
	bKillParticlesOnDestroy = false;

	bEnablePrewarm = true;
	bOverridePrewarmTime = false;
	PrewarmTime = 0.0f;

	m_DidAutoAttach = false;
	m_SavedAutoAttachRelativeScale3D = FVector(1.f, 1.f, 1.f);

	bEnableUpdates = true;
	bAllowTeleport = false;

	{
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
	}
	Scene = null;

	AttributeList = CreateDefaultSubobject<UPopcornFXAttributeList>("AttributeList");

	//UE_LOG(LogPopcornFXEmitterComponent, Log, TEXT("EMITTERCOMP ctor attrlist %p"), AttributeList);

	AttributeList->SetFlags(RF_Transactional);

	AttributeList->CheckEmitter(this);

	bAutoDestroy = false;
	bHasAlreadyPlayOnLoad = false;

	SetFlags(RF_Transactional);
}

//----------------------------------------------------------------------------

UPopcornFXEmitterComponent::~UPopcornFXEmitterComponent()
{
	//PopcornFX::CLog::Log(PK_INFO, "UPopcornFXEmitterComponent::~UPopcornFXEmitterComponent %p List %p ", this, AttributeList);
}

//----------------------------------------------------------------------------

static const EWorldType::Type	kWorldTypeEditorPreview = EWorldType::EditorPreview;

bool	UPopcornFXEmitterComponent::ResolveScene(bool warnIFN)
{
	UWorld			*world = GetWorld();

	// dont warn if cooking or no active world
	const bool		disableWarn = !warnIFN || IsRunningCommandlet() || (world == null) || (world->WorldType == EWorldType::Inactive);
	const bool		enableWarn = !disableWarn;
	if (world == null)
	{
		if (enableWarn)
			UE_LOG(LogPopcornFXEmitterComponent, Warning, TEXT("NULL World: %s"), *GetPathName());
		return false;
	}

	if (!SceneName.IsValid() || SceneName.IsNone())
		Scene = null;
	else
	{
		bool		found = false;
		if (Scene != null && (SceneName != Scene->GetSceneName()))
			Scene = null;
		if (Scene == null && world != null)
		{
			for (TActorIterator<APopcornFXSceneActor> sceneIt(world); sceneIt; ++sceneIt)
			{
				if (sceneIt->GetSceneName() == SceneName)
				{
					Scene = *sceneIt;
					found = true;
					break;
				}
			}
#if WITH_EDITOR
			if (world->WorldType == kWorldTypeEditorPreview)
				SpawnPreviewSceneIFN(world);
#endif // WITH_EDITOR
		}
		//if (!found)
		//	UE_LOG(LogPopcornFXEmitterComponent, Warning, TEXT("Emitter '%s': scene '%s' not found"), *GetFullName(), *(SceneName.ToString()));
	}

	m_CurrentScene = Scene != null ? Scene->ParticleScene() : null;

	if (m_CurrentScene == null)
	{
		if (enableWarn)
			UE_LOG(LogPopcornFXEmitterComponent, Warning, TEXT("Could not find a valid APopcornFXSceneActor with a SceneName '%s': %s"), *SceneName.ToString(), *GetPathName());
		return false;
	}

	return m_CurrentScene != null;
}

//----------------------------------------------------------------------------

PopcornFX::CParticleEffectInstance	*UPopcornFXEmitterComponent::_GetEffectInstance() const
{
	return m_EffectInstancePtr.Get();
}

//----------------------------------------------------------------------------
#if WITH_EDITOR

#if (ENGINE_MINOR_VERSION >= 25)
bool	UPopcornFXEmitterComponent::CanEditChange(const FProperty* InProperty) const
#else
bool	UPopcornFXEmitterComponent::CanEditChange(const UProperty* InProperty) const
#endif // (ENGINE_MINOR_VERSION >= 25)
{
	if (InProperty)
	{
		FString	PropertyName = InProperty->GetName();
		if (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXEmitterComponent, PrewarmTime))
		{
			return bOverridePrewarmTime && bEnablePrewarm;
		}
	}
	return Super::CanEditChange(InProperty);
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
	Super::PostEditChangeProperty(propertyChangedEvent); // call internal OnRegister

	const UWorld	*world = GetWorld();
	PK_ASSERT(IsRegistered() || world == null ||
			 (world->WorldType == kWorldTypeEditorPreview || world->WorldType == EWorldType::Editor || world->WorldType == EWorldType::PIE));

	CheckForDead(); // we could have not been able to do that when terminated externally

	if (propertyChangedEvent.Property != null)
	{
		const FString	pName = propertyChangedEvent.Property->GetName();

		if (pName == TEXT("bPlayOnLoad"))
		{
			TerminateEmitter(true);
			if (bPlayOnLoad != 0)
				StartEmitter();
		}
		else if (pName == TEXT("Effect") || pName == TEXT("SceneName"))
		{
			const bool		wasEmitting = IsEmitterEmitting();
			TerminateEmitter(true);
			if (Effect == null)
				AttributeList->Clean();
			else
			{
				PK_VERIFY(AttributeList->Prepare(Effect));
				if (wasEmitting || bPlayOnLoad != 0)
					StartEmitter();
			}
		}
	}

	PK_VERIFY(AttributeList->Prepare(Effect)); // make sure everything is up to date, always
	AttributeList->CheckEmitter(this);
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::SpawnPreviewSceneIFN(UWorld *world)
{
	LLM_SCOPE(ELLMTag::Particles);
	if (Scene == null)
	{
		FActorSpawnParameters	params;

		params.ObjectFlags = RF_Public | RF_Transactional;
		// ! Do not force the name !
		// Or UE will magicaly just return the last deleted Actor with the same name !
		//params.Name = SceneName;
		params.bAllowDuringConstructionScript = true;
		Scene = world->SpawnActor<APopcornFXSceneActor>(APopcornFXSceneActor::StaticClass(), params);
		if (Scene != null && Scene->PopcornFXSceneComponent != null)
			Scene->PopcornFXSceneComponent->SceneName = SceneName;
	}

	if (!PK_VERIFY(Scene != null))
		return;
	// Mirror the "real" world scene properties if possible, gore walkthrough though
	bool					found = false;
	APopcornFXSceneActor	*refScene = null;
	for (TObjectIterator<UWorld> worldIt; worldIt && !found; ++worldIt)
	{
		UWorld	*currentWorld = *worldIt;

		if (currentWorld->IsGameWorld() || currentWorld->WorldType == EWorldType::Editor)
		{
			for (TActorIterator<APopcornFXSceneActor> sceneIt(currentWorld); sceneIt; ++sceneIt)
			{
				if (sceneIt->GetSceneName() == SceneName)
				{
					refScene = *sceneIt;
					found = true;
					break;
				}
			}
		}
	}
	if (found && refScene != null)
		Scene->PopcornFXSceneComponent->MirrorGameWorldProperties(refScene->PopcornFXSceneComponent);
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::CheckForErrors()
{
	Super::CheckForErrors();

	AActor	*owner = GetOwner();
	if (Effect == null)
		return;
	if (!Effect->IsFileValid())
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ComponentName"), FText::FromString(GetName()));
		Arguments.Add(TEXT("OwnerName"), FText::FromString(owner->GetName()));
		Arguments.Add(TEXT("EffectName"), FText::FromString(Effect->GetName()));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(owner))
			->AddToken(FTextToken::Create(FText::Format(LOCTEXT("MapCheck_Message_Invalid", "{EffectName} used by {ComponentName}::{OwnerName} is corrupted"), Arguments)))
			;
	}
	else if (Effect->Effect()->ParticleEffect() != null)
	{
		PopcornFX::PBaseObjectFile		file = Effect->Effect()->ParticleEffect()->File();
		if (file == null)
			return;
		PopcornFX::SEngineVersion	current = FPopcornFXPlugin::PopcornFXBuildVersion();
		PopcornFX::SEngineVersion	filever = file->Version();
		if (filever.Empty())
			filever = PopcornFX::SEngineVersion(1, 7, 3, 0); // bo file version did not exists

		bool		outOfDate = filever.Major() < current.Major();
		if (!outOfDate && filever.Minor() < current.Minor())
			outOfDate = true;
		if (outOfDate)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("EffectName"), FText::FromString(Effect->GetName()));
			Arguments.Add(TEXT("OldVersion"), FText::FromString(FString::Printf(TEXT("%d.%d.%d.%d"), filever.Major(), filever.Minor(), filever.Patch(), filever.RevID())));
			Arguments.Add(TEXT("NewVersion"), FText::FromString(FString::Printf(TEXT("%d.%d.%d.%d"), current.Major(), current.Minor(), current.Patch(), current.RevID())));
			FMessageLog("MapCheck").Warning()
				->AddToken(FUObjectToken::Create(Effect))
				->AddToken(FTextToken::Create(FText::Format(LOCTEXT("MapCheck_Message_Version", "is out of date ({OldVersion}) please upgrade your PopcornFX Pack to {NewVersion} then reimport the effect"), Arguments)));
		}
	}
}

#endif // WITH_EDITOR

const UObject	*UPopcornFXEmitterComponent::AdditionalStatObject() const
{
	return Effect;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::EmitterBeginPlayIFN()
{
	if ((!bPlayOnLoad || bHasAlreadyPlayOnLoad)
#if WITH_EDITOR
		&& !m_ReplayAfterDead
#endif
		)
		return;

#if WITH_EDITOR
	m_ReplayAfterDead = false;
#endif
	bHasAlreadyPlayOnLoad = true;

	RestartEmitter();
	return;
}

//----------------------------------------------------------------------------

UPopcornFXEmitterComponent	*UPopcornFXEmitterComponent::CopyAndStartEmitterAtLocation(
	FVector location,
	FRotator rotation,
	bool pbAutoDestroy) // shadow bAutoDestroy
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXEmitterComponent::CopyAndStartEmitterAtLocation", POPCORNFX_UE_PROFILER_COLOR);

	if (!ResolveScene(true))
		return null;

	if (Effect == null)
	{
		UE_LOG(LogScript, Warning, TEXT("UPopcornFXEmitterComponent::CopyAndStartEmitterAttached: there is no Effect: '%s'"), *GetPathName());
		return null;
	}

	UWorld						*world = GetWorld();
	UPopcornFXEmitterComponent	*psc = null;
	if (FApp::CanEverRender() && world != null && !world->IsNetMode(NM_DedicatedServer))
	{
		psc = CreateStandaloneEmitterComponent(Effect, Scene, world, null, pbAutoDestroy);
		if (psc != null)
		{
			psc->SetAbsolute(true, true, true);
			psc->SetWorldLocationAndRotation(location, rotation);
			psc->SetRelativeScale3D(FVector(1.f));
			psc->UpdateComponentToWorld();

			psc->AttributeList->CopyFrom(AttributeList, GetOwner());
			psc->StartEmitter();
		}

	}
	return psc;
}

//----------------------------------------------------------------------------

UPopcornFXEmitterComponent	*UPopcornFXEmitterComponent::CopyAndStartEmitterAttached(
	class USceneComponent* attachToComponent,
	FName attachPointName,
	FVector location,
	FRotator rotation,
	EAttachLocation::Type locationType,
	bool pbAutoDestroy) // shadows bAutoDestroy
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXEmitterComponent::CopyAndStartEmitterAttached", POPCORNFX_UE_PROFILER_COLOR);

	if (!ResolveScene(true))
		return null;

	if (Effect == null)
	{
		UE_LOG(LogScript, Warning, TEXT("UPopcornFXEmitterComponent::CopyAndStartEmitterAttached: there is no Effect: '%s'"), *GetPathName());
		return null;
	}
	if (attachToComponent == null)
	{
		UE_LOG(LogScript, Warning, TEXT("UPopcornFXEmitterComponent::CopyAndStartEmitterAttached: NULL AttachComponent specified!: '%s'"), *GetPathName());
		return null;
	}

	UWorld						*world = GetWorld();
	UPopcornFXEmitterComponent	*psc = null;
	if (FApp::CanEverRender() && world != null && !world->IsNetMode(NM_DedicatedServer))
	{
		psc = CreateStandaloneEmitterComponent(Effect, Scene, world, null, pbAutoDestroy);
		if (psc != null)
		{
			psc->AttachToComponent(attachToComponent, FAttachmentTransformRules::KeepRelativeTransform, attachPointName);
			if (locationType == EAttachLocation::KeepWorldPosition)
			{
				psc->SetWorldLocationAndRotation(location, rotation);
			}
			else
			{
				psc->SetRelativeLocationAndRotation(location, rotation);
			}
			psc->SetRelativeScale3D(FVector(1.f));
			psc->UpdateComponentToWorld();

			psc->bEnableUpdates = true;
			psc->AttributeList->CopyFrom(AttributeList, GetOwner());
			psc->StartEmitter();
		}
	}
	return psc;
}

//----------------------------------------------------------------------------

bool	UPopcornFXEmitterComponent::SetEffect(UPopcornFXEffect *effect, bool startEmitter)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXEmitterComponent::SetEffect", POPCORNFX_UE_PROFILER_COLOR);

	const UWorld	*world = GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		TerminateEmitter(true);

#if WITH_EDITOR
		if (Effect != null)
			Effect->m_OnPopcornFXFileUnloaded.Remove(m_OnPopcornFXFileUnloadedHandle);
#endif // WITH_EDITOR
		if (effect == null)
		{
			Effect = effect;
			AttributeList->Clean();
			return true;
		}
		if (effect != Effect)
			Effect = effect;
#if WITH_EDITOR
		m_OnPopcornFXFileUnloadedHandle = effect->m_OnPopcornFXFileUnloaded.AddUObject(this, &UPopcornFXEmitterComponent::OnPopcornFXFileUnloaded);
#endif // WITH_EDITOR
		PK_VERIFY(AttributeList->Prepare(effect, true));
		if (startEmitter)
			StartEmitter();
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXEmitterComponent::StartEmitter()
{
	const UWorld	*world = GetWorld();
	if (!FApp::CanEverRender() || (world != null && world->IsNetMode(NM_DedicatedServer)))
		return true;

	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXEmitterComponent::StartEmitter", POPCORNFX_UE_PROFILER_COLOR);

	// if already started do nothing
	if (m_Started)
		return true;

	if (!ResolveScene(true))
		return false;

	if (Effect == null)
	{
		UE_LOG(LogPopcornFXEmitterComponent, Warning/*Error*/, TEXT("Could not StartEmitter '%s': there is no Effect"), *GetFullName());
		return false;
	}
	if (Effect->Effect()->ParticleEffect() == null)
	{
		UE_LOG(LogPopcornFXEmitterComponent, Warning/*Error*/, TEXT("Could not StartEmitter '%s': invalid effect '%s'"), *GetFullName(), *Effect->GetPathName());
		return false;
	}

	// Auto attach if requested
	const bool	wasAutoAttached = m_DidAutoAttach;
	m_DidAutoAttach = false;
	if (bAutoManageAttachment && world->IsGameWorld())
	{
		USceneComponent		*newParent = AutoAttachParent.Get();
		if (newParent != null)
		{
			const bool	alreadyAttached =	GetAttachParent() != null &&
											GetAttachParent() == newParent &&
											GetAttachParent()->GetAttachChildren().Contains(this) &&
											GetAttachSocketName() == AutoAttachSocketName;
			if (!alreadyAttached)
			{
				m_DidAutoAttach = wasAutoAttached;
				CancelAutoAttachment();
				m_SavedAutoAttachRelativeLocation = GetRelativeLocation();
				m_SavedAutoAttachRelativeRotation = GetRelativeRotation();
				m_SavedAutoAttachRelativeScale3D = GetRelativeScale3D();
				AttachToComponent(newParent, FAttachmentTransformRules(AutoAttachLocationRule, AutoAttachRotationRule, AutoAttachScaleRule, false), AutoAttachSocketName);
			}

			m_DidAutoAttach = true;
		}
		else
		{
			CancelAutoAttachment();
		}
	}

	CFloat4x4		&currentTr = ToPkRef(m_CurrentWorldTransforms);
	CFloat4x4		&previousTr = ToPkRef(m_PreviousWorldTransforms);
	CFloat3			&currentVel = ToPkRef(m_CurrentWorldVelocity);
	CFloat3			&previousVel = ToPkRef(m_PreviousWorldVelocity);

	currentTr = ToPk(GetComponentTransform().ToMatrixNoScale());
	currentTr.StrippedTranslations() *= FPopcornFXPlugin::GlobalScaleRcp();
	previousTr = currentTr;
	currentVel = CFloat3::ZERO;
	previousVel = CFloat3::ZERO;

	float	prewarmTime = 0.0f;
	if (bEnablePrewarm)
	{
		prewarmTime = PrewarmTime;
		if (!bOverridePrewarmTime)
			prewarmTime = Effect->Effect()->ParticleEffectIFP()->PrewarmTime();
	}

	PopcornFX::SEffectStartCtl	effectStartCtl;
	effectStartCtl.m_SpawnTransformsPack.m_WorldTr_Current = &currentTr;
	effectStartCtl.m_SpawnTransformsPack.m_WorldTr_Previous = &previousTr;
	effectStartCtl.m_SpawnTransformsPack.m_WorldVel_Current = &currentVel;
	effectStartCtl.m_SpawnTransformsPack.m_WorldVel_Previous = &previousVel;
	effectStartCtl.m_PrewarmTime = prewarmTime;

	m_LastFrameUpdate = 0;

	CParticleScene			*particleScene = m_CurrentScene;
	PK_ASSERT(particleScene != null);

	PK_ASSERT(Effect->Effect()->ParticleEffectIFP() != null); // Shouldn't happen, lazy loaded above ( Effect->ParticleEffect() )
	m_EffectInstancePtr = Effect->Effect()->ParticleEffectIFP()->Instantiate(particleScene->Unsafe_ParticleMediumCollection()).Get();
	if (m_EffectInstancePtr == null)
	{
		UE_LOG(LogPopcornFXEmitterComponent, Warning/*Error*/, TEXT("Could not StartEmitter '%s'"), *GetFullName());
		return false;
	}

	m_Started = true;
	m_Stopped = false;
	m_EffectInstancePtr->m_DeathNotifier += PopcornFX::FastDelegate<void(const PopcornFX::PParticleEffectInstance &)>(this, &UPopcornFXEmitterComponent::_OnDeathNotifier);

	// Refresh attributes once we have instantiated the emitter
	UPopcornFXAttributeList		*attributeList = GetAttributeList();
	PK_ASSERT(attributeList != null);

	if (attributeList != null)
		attributeList->RefreshAttributes(this);
	if (attributeList != null)
		attributeList->RefreshAttributeSamplers(this, true);

	// actual spawn
	if (!m_EffectInstancePtr->Start(effectStartCtl))
	{
		m_Started = false;
		m_Stopped = true;
		return false;
	}

	CheckForDead();

	// The instance could be already null (ie: with a spawn burst who dies instantly)
	if (m_EffectInstancePtr != null)
	{
		// if the instance is still alive,
		// register me to the scene for update if bEnableUpdates
		if (bEnableUpdates != 0)
		{
			if (!SelfSceneRegister())
			{
				UE_LOG(LogPopcornFXEmitterComponent, Warning/*Error*/, TEXT("Could not StartEmitter '%s': failed to register in the scene (Effect updates will not work)"), *GetFullName());
				return true;
			}
		}
	}

	const FVector			rotation = GetComponentTransform().GetRotation().GetRotationAxis();
	OnEmitterStart.Broadcast(this, GetComponentTransform().GetLocation(), rotation);
	APopcornFXEmitter* actor = Cast<APopcornFXEmitter>(GetOwner());
	if (actor != null)
		actor->OnEmitterStart.Broadcast(this, GetComponentTransform().GetLocation(), rotation);

	return true;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::RestartEmitter(bool killParticles)
{
	const UWorld	*world = GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		LLM_SCOPE(ELLMTag::Particles);
		TerminateEmitter(killParticles);
		StartEmitter();
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::SetPrewarmTime(float time)
{
	PrewarmTime = time;
	bOverridePrewarmTime = true;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::StopEmitter(bool killParticles)
{
	const UWorld	*world = GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		LLM_SCOPE(ELLMTag::Particles);
		PK_NAMEDSCOPEDPROFILE_C("UPopcornFXEmitterComponent::StopEmitter", POPCORNFX_UE_PROFILER_COLOR);

		PK_ASSERT(IsRegistered() || world == null ||
			(world->WorldType == kWorldTypeEditorPreview || world->WorldType == EWorldType::Editor));

		bool		sendEvent = false;
		if (m_EffectInstancePtr != null && m_Started && !m_Stopped)
		{
			if (killParticles)
				KillParticles();
			else
				m_EffectInstancePtr->Stop();
			sendEvent = true;
		}
		m_Stopped = true;
		if (m_EffectInstancePtr == null) // we could have been destroy by the stop
			m_Started = false;

		// send only if not terminated
		if (sendEvent)
		{
			const FVector			rotation = GetComponentTransform().GetRotation().GetRotationAxis();
			OnEmitterStop.Broadcast(this, GetComponentTransform().GetLocation(), rotation);
			APopcornFXEmitter* actor = Cast<APopcornFXEmitter>(GetOwner());
			if (actor != null)
				actor->OnEmitterStop.Broadcast(this, GetComponentTransform().GetLocation(), rotation);
		}
	}
}

//----------------------------------------------------------------------------

bool	UPopcornFXEmitterComponent::ToggleEmitter(bool startEmitter, bool killParticles)
{
	const UWorld	*world = GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		LLM_SCOPE(ELLMTag::Particles);
		if (startEmitter && !IsEmitterStarted())
		{
			StartEmitter();
		}
		else if (!startEmitter && IsEmitterStarted())
		{
			StopEmitter(killParticles);
		}
	}
	return IsEmitterStarted();
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::TerminateEmitter(bool killParticles)
{
	const UWorld	*world = GetWorld();
	if (!FApp::CanEverRender() || (world != null && world->IsNetMode(NM_DedicatedServer)))
		return;

	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXEmitterComponent::TerminateEmitter", POPCORNFX_UE_PROFILER_COLOR);

	//PK_ASSERT(!IsTemplate());

	bool		sendEvent = false;
	if (m_EffectInstancePtr != null)
	{
		PK_ASSERT(FPopcornFXPlugin::IsMainThread()); // cannot be called async

		if (IsValid(AttributeList)) // something can go wrong when deleting stuff
			AttributeList->CheckEmitter(this);

		// keep a ref here
		PopcornFX::PParticleEffectInstance		instanceForRef = m_EffectInstancePtr.Get();

		if (killParticles)
			KillParticles();

		// Keep call to Terminate(), kill is deferred so we need to backup transforms & attrs
		if (m_EffectInstancePtr != null)
			m_EffectInstancePtr->Terminate();

		// Fail safe, Terminate should have called _OnDeathNotifier
		if (!PK_VERIFY(m_DiedThisFrame) ||
			!PK_VERIFY(m_EffectInstancePtr == null))
		{
			_OnDeathNotifier(m_EffectInstancePtr.Get());
			PK_ASSERT(m_DiedThisFrame);
			PK_ASSERT(m_EffectInstancePtr == null);
		}

		// Release ref
		instanceForRef = null;

		PK_ASSERT(m_CurrentScene != null);

		sendEvent = true;
		//m_EffectInstancePtr = null; // do not null that in case its called later

		// _OnDeathNotifier should have been called
	}

	CheckForDead(); // can call OnEmissionStops

	PK_ASSERT(!SelfSceneIsRegistered());
	PK_ASSERT(m_EffectInstancePtr == null);
	PK_ASSERT(!m_Started);

	// its shame we cant check that, but an OnEmissionStops could have restarted the emitter
	//PK_ASSERT(m_EffectInstancePtr == null);
	//PK_ASSERT(!m_Started);

	if (sendEvent)
	{
		const FVector			rotation = GetComponentTransform().GetRotation().GetRotationAxis();
		OnEmitterTerminate.Broadcast(this, GetComponentTransform().GetLocation(), rotation);
		APopcornFXEmitter* actor = Cast<APopcornFXEmitter>(GetOwner());
		if (actor != null)
			actor->OnEmitterTerminate.Broadcast(this, GetComponentTransform().GetLocation(), rotation);
	}
}


//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::KillParticles()
{
	const UWorld	*world = GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		LLM_SCOPE(ELLMTag::Particles);
		PK_NAMEDSCOPEDPROFILE_C("UPopcornFXEmitterComponent::KillParticles", POPCORNFX_UE_PROFILER_COLOR);

		if (m_EffectInstancePtr != null)
		{
			PK_ASSERT(FPopcornFXPlugin::IsMainThread()); // cannot be called async
			AttributeList->CheckEmitter(this);
			m_EffectInstancePtr->KillDeferred();
			PK_ASSERT(m_CurrentScene != null);

			const FVector			rotation = GetComponentTransform().GetRotation().GetRotationAxis();
			OnEmitterKillParticles.Broadcast(this, GetComponentTransform().GetLocation(), rotation);
			APopcornFXEmitter* actor = Cast<APopcornFXEmitter>(GetOwner());
			if (actor != null)
				actor->OnEmitterKillParticles.Broadcast(this, GetComponentTransform().GetLocation(), rotation);
		}
	}
}

//----------------------------------------------------------------------------

UPopcornFXAttributeList	*UPopcornFXEmitterComponent::GetAttributeList()
{
	if (!PK_VERIFY(AttributeList != null)) // something can go wrong when deleting stuff
		return null;
	const UWorld	*world = GetWorld();
	if (!FApp::CanEverRender() || (world != null && world->IsNetMode(NM_DedicatedServer)))
		return null;
	if (!PK_VERIFY(AttributeList->Prepare(Effect)))
		return null;
	AttributeList->CheckEmitter(this);
	PK_ASSERT(AttributeList->Valid() && AttributeList->IsUpToDate(Effect));
	return AttributeList;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeList	*UPopcornFXEmitterComponent::GetAttributeListIFP() const
{
	if (!PK_VERIFY(AttributeList != null)) // something can go wrong when deleting stuff
		return null;
	const UWorld	*world = GetWorld();
	if (!FApp::CanEverRender() || (world != null && world->IsNetMode(NM_DedicatedServer)))
		return null;
	if (!PK_VERIFY(AttributeList->Valid() && AttributeList->IsUpToDate(Effect)))
		return null;
	AttributeList->CheckEmitter(this);
	return AttributeList;
}

//----------------------------------------------------------------------------

bool	UPopcornFXEmitterComponent::GetPayloadValue(const FName &payloadName, EPopcornFXPayloadType::Type expectedFieldType, void *outValue) const
{
	if (!PK_VERIFY(m_CurrentScene != null))
		return false;

	if (payloadName.IsNone() || !payloadName.IsValid())
	{
		UE_LOG(LogPopcornFXEmitterComponent, Warning, TEXT("Get Payload Value: Invalid PayloadName"));
		return false;
	}

	return m_CurrentScene->GetPayloadValue(payloadName, expectedFieldType, outValue);
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::ResetAttributesToDefault()
{
	if (!PK_VERIFY(AttributeList != null)) // somthing can go wrong when deleting stuff
		return;
	UPopcornFXAttributeList		*attributeList = GetAttributeList();
	if (attributeList != null)
		attributeList->ResetToDefaultValues(Effect);
}

//----------------------------------------------------------------------------

bool	UPopcornFXEmitterComponent::RegisterEventListener(FPopcornFXRaiseEventSignature Delegate, FName EventName)
{
	LLM_SCOPE(ELLMTag::Particles);
	if (!PK_VERIFY(m_CurrentScene != null))
		return false;

	if (Effect == null)
	{
		UE_LOG(LogPopcornFXEmitterComponent, Warning, TEXT("Register Event Listener: Emitter Component has no effect assigned"));
		return false;
	}

	if (!EventName.IsValid() || EventName.IsNone())
	{
		UE_LOG(LogPopcornFXEmitterComponent, Warning, TEXT("Register Event Listener: Empty EventName"));
		return false;
	}
	const PopcornFX::CStringId	eventNameId = PopcornFX::CStringId(TCHAR_TO_ANSI(*EventName.ToString()));
	PK_ASSERT(!eventNameId.Empty());

	if (!Delegate.IsBound())
	{
		UE_LOG(LogPopcornFXEmitterComponent, Warning, TEXT("Register Event Listener: Invalid delegate"));
		return false;
	}

	if (m_EffectInstancePtr == null)
		return false;

	return m_CurrentScene->RegisterEventListener(Effect, m_EffectInstancePtr->EffectID(), eventNameId, Delegate);
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::UnregisterEventListener(FPopcornFXRaiseEventSignature Delegate, FName EventName)
{
	LLM_SCOPE(ELLMTag::Particles);
	if (!PK_VERIFY(m_CurrentScene != null))
		return;

	const PopcornFX::CStringId	eventNameId = PopcornFX::CStringId(TCHAR_TO_ANSI(*EventName.ToString()));
	if (eventNameId.Empty())
	{
		UE_LOG(LogPopcornFXEmitterComponent, Warning, TEXT("Unregister Event Listener: Empty EventName"));
		return;
	}

	if (!Delegate.IsBound())
	{
		UE_LOG(LogPopcornFXEmitterComponent, Warning, TEXT("Unregister Event Listener: Invalid delegate"));
		return;
	}

	if (!PK_VERIFY(Effect != null))
		return;

	if (m_EffectInstancePtr == null)
		return;

	m_CurrentScene->UnregisterEventListener(Effect, m_EffectInstancePtr->EffectID(), eventNameId, Delegate);
}

//----------------------------------------------------------------------------

void	 UPopcornFXEmitterComponent::UnregisterAllEventsListeners()
{
	LLM_SCOPE(ELLMTag::Particles);
	if (m_CurrentScene == null ||
		Effect == null ||
		m_EffectInstancePtr == null)
		return;

	m_CurrentScene->UnregisterAllEventsListeners(Effect, m_EffectInstancePtr->EffectID());
}

//----------------------------------------------------------------------------

bool	UPopcornFXEmitterComponent::IsEmitterAlive() const
{
	return m_Started;
}
//----------------------------------------------------------------------------

bool	UPopcornFXEmitterComponent::IsEmitterStarted() const
{
	return m_Started;
}

//----------------------------------------------------------------------------

bool	UPopcornFXEmitterComponent::IsEmitterEmitting() const
{
	return m_Started && !m_Stopped;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::PostLoad()
{
	LLM_SCOPE(ELLMTag::Particles);
	Super::PostLoad();
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::BeginDestroy()
{
	LLM_SCOPE(ELLMTag::Particles);

	if (m_Scene_PreInitEmitterId != PopcornFX::CGuid::INVALID)
		SelfPreInitSceneUnregister();

	PK_ASSERT(IsInGameThread());
	TerminateEmitter(bKillParticlesOnDestroy);
	//UE_LOG(LogPopcornFXEmitterComponent, Log, TEXT("EMITTERCOMP %p BeginDestroy %p"), this, AttributeList);
	//if (AttributeList != null && AttributeList->IsValidLowLevel())
	//{
	//	//UE_LOG(LogPopcornFXEmitterComponent, Log, TEXT("EMITTERCOMP %p force BeginDestroy attr list %p"), this, AttributeList);
	//	AttributeList->ConditionalBeginDestroy();
	//}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::OnRegister()
{
	LLM_SCOPE(ELLMTag::Particles);

	UWorld	*world = GetWorld();
	check(world != nullptr);

	if (bAutoManageAttachment && !IsActive())
	{
		// Detach from current parent, we are supposed to wait for activation.
		if (GetAttachParent() != null)
		{
			// If no auto attach parent override, use the current parent when we activate
			if (!AutoAttachParent.IsValid())
			{
				AutoAttachParent = GetAttachParent();
			}
			// If no auto attach socket override, use current socket when we activate
			if (AutoAttachSocketName == NAME_None)
			{
				AutoAttachSocketName = GetAttachSocketName();
			}

			// If in a game world, detach now if necessary. Activation will cause auto-attachment.
			if (world->IsGameWorld())
			{
				// Prevent attachment before Super::OnRegister() tries to attach us, since we only attach when activated.
				if (GetAttachParent()->GetAttachChildren().Contains(this))
				{
					// Only detach if we are not about to auto attach to the same target, that would be wasteful.
					if (!bPlayOnLoad ||
						(AutoAttachLocationRule != EAttachmentRule::KeepRelative && AutoAttachRotationRule != EAttachmentRule::KeepRelative && AutoAttachScaleRule != EAttachmentRule::KeepRelative) ||
						AutoAttachSocketName != GetAttachSocketName() ||
						AutoAttachParent != GetAttachParent())
					{
						DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepRelative, false));
					}
				}
				else
				{
					SetupAttachment(null, NAME_None);
				}
			}
		}

		m_SavedAutoAttachRelativeLocation = GetRelativeLocation();
		m_SavedAutoAttachRelativeRotation = GetRelativeRotation();
		m_SavedAutoAttachRelativeScale3D = GetRelativeScale3D();
	}

	// Keep Super::OnRegister in between the auto attachment system and the rest.
	Super::OnRegister();

	// Sometimes UE reflection breaks connection with the underlying AttributeList member, not sure why.
	// Returning here instead of crashing below, and UE properly re-registers the component later on..
	// Ugly but does the trick (there is probably wrong done plugin side that causes this issue)
	if (AttributeList == null)
		return;

	// Avoids Prepare call on the attribute list before the PostEditChangeProperty is called
	// This is due to "ReRegister()" called before the PostEditChange is broadcasted.
	if (Effect == null)
		return;

	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		PK_ASSERT(!IsTemplate());

		//UE_LOG(LogPopcornFXEmitterComponent, Log, TEXT("EMITTERCOMP %p post load attr list %p"), this, AttributeList);

		if (Effect != null)
		{
			Effect->ConditionalPostLoad();
			check(AttributeList != null);
			AttributeList->ConditionalPostLoad();
			AttributeList->CheckEmitter(this);
			PK_VERIFY(AttributeList->Prepare(Effect));
		}

		AttributeList->CheckEmitter(this);

		// We need to do that again (PostLoad not called when spawned on the fly)
		check(AttributeList != null);
		PK_VERIFY(AttributeList->Prepare(Effect));
	}

#if WITH_EDITOR
	Effect->m_OnPopcornFXFileUnloaded.Remove(m_OnPopcornFXFileUnloadedHandle);
	m_OnPopcornFXFileUnloadedHandle = Effect->m_OnPopcornFXFileUnloaded.AddUObject(this, &UPopcornFXEmitterComponent::OnPopcornFXFileUnloaded);
#endif // WITH_EDITOR

	if (ResolveScene(true) && m_Scene_PreInitEmitterId == PopcornFX::CGuid::INVALID)
	{
		SelfPreInitSceneRegister();
	}
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
void	UPopcornFXEmitterComponent::_OnSceneLoaded(APopcornFXSceneActor *sceneActor)
{
	EmitterBeginPlayIFN();
	PK_ASSERT(Scene == null || Scene == sceneActor);
	m_WaitForSceneDelegateHandle.Reset();
	return;
}
#endif // WITH_EDITOR

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::OnUnregister()
{
	LLM_SCOPE(ELLMTag::Particles);
	if (m_Scene_PreInitEmitterId != PopcornFX::CGuid::INVALID)
		SelfPreInitSceneUnregister();

#if WITH_EDITOR
	m_ReplayAfterDead = IsEmitterEmitting();
#endif
	TerminateEmitter(bKillParticlesOnDestroy);

	Super::OnUnregister();

	bHasAlreadyPlayOnLoad = false;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::BeginPlay()
{
	LLM_SCOPE(ELLMTag::Particles);
	if (!ResolveScene(true))
	{
		// ResolveScene has logged a warning ifn
	}

	Super::BeginPlay();

	if (m_CurrentScene != null && m_Scene_PreInitEmitterId == PopcornFX::CGuid::INVALID)
	{
		SelfPreInitSceneRegister();
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LLM_SCOPE(ELLMTag::Particles);

	// unregister everything in case the user forgot to do it
	if (Effect != null && Effect->Effect()->HasRegisteredEvents())
		UnregisterAllEventsListeners();

	TerminateEmitter(bKillParticlesOnDestroy);

	Super::EndPlay(EndPlayReason);

	bHasAlreadyPlayOnLoad = false;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::_OnDeathNotifier(const PopcornFX::PParticleEffectInstance & instance)
{
	PK_ASSERT(m_EffectInstancePtr == instance.Get());

	m_DiedThisFrame = true;

	if (m_EffectInstancePtr != null)
		m_EffectInstancePtr->m_DeathNotifier -= PopcornFX::FastDelegate<void(const PopcornFX::PParticleEffectInstance &)>(this, &UPopcornFXEmitterComponent::_OnDeathNotifier);

	m_EffectInstancePtr = null;
	m_Started = false;
	m_Stopped = false;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::CheckForDead()
{
	LLM_SCOPE(ELLMTag::Particles);
	if (m_DiedThisFrame)
	{
		m_DiedThisFrame = false;

		if (SelfSceneIsRegistered())
		{
			SelfSceneUnregister();
		}

		// if effect dies, need to unregister all events listeners
		UnregisterAllEventsListeners();

		if (bAutoDestroy != 0)
		{
			// Avoids nested DestroyComponent()
			if (IsRegistered())
				DestroyComponent();
		}
		else
		{
			if (bAutoManageAttachment)
			{
				CancelAutoAttachment();
			}
			if (Effect != null && !Effect->HasAnyFlags(RF_BeginDestroyed))
			{
#if WITH_EDITOR
				if (m_ReplayAfterDead)
				{
					Effect->m_OnPopcornFXFileLoaded.Remove(m_OnPopcornFXFileLoadedHandle);
					m_OnPopcornFXFileLoadedHandle = Effect->m_OnPopcornFXFileLoaded.AddUObject(this, &UPopcornFXEmitterComponent::OnPopcornFXFileLoaded);
				}
				else
#endif
				{
					if (OnEmissionStops.IsBound())
					{
						const FVector			rotation = GetComponentTransform().GetRotation().GetRotationAxis();
						OnEmissionStops.Broadcast(this, GetComponentTransform().GetLocation(), rotation);
					}
				}
			}
		}
	}
}


//----------------------------------------------------------------------------

// Compute particle system bounds
FBoxSphereBounds	UPopcornFXEmitterComponent::CalcBounds(const FTransform &localToWorld) const
{
	FBoxSphereBounds	bounds;

	// Arbitrary bounds
	bounds.Origin = localToWorld.GetLocation();
	bounds.BoxExtent = FVector(32.0f, 32.0f, 32.0f);
	bounds.SphereRadius = 64.0f;
	return bounds;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);

	switch (Teleport)
	{
	case ETeleportType::None:
		break;
	case ETeleportType::TeleportPhysics:
		m_TeleportThisFrame = true;
		break;
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::ApplyWorldOffset(const FVector &inOffset, bool worldShift)
{
	Super::ApplyWorldOffset(inOffset, worldShift);

	// Super::ApplyWorldOffset will modify the emitter transforms.
	// Reset prev values to avoid massive trails
	CFloat4x4			&currentTr = ToPkRef(m_CurrentWorldTransforms);
	CFloat4x4			&previousTr = ToPkRef(m_PreviousWorldTransforms);
	CFloat3				&prevPrevPos = ToPkRef(m_PreviousWorldPosition); // 2 frames of lag
	CFloat3				&currentVel = ToPkRef(m_CurrentWorldVelocity);
	CFloat3				&previousVel = ToPkRef(m_PreviousWorldVelocity);

	currentTr = ToPk(GetComponentTransform().ToMatrixWithScale());
	currentTr.StrippedTranslations() *= FPopcornFXPlugin::GlobalScaleRcp();

	prevPrevPos = currentTr.StrippedTranslations();
	previousTr = currentTr;
	currentVel = CFloat3::ZERO;
	previousVel = currentVel;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::Scene_OnPreInitRegistered(CParticleScene *scene, uint32 selfIdInPreInit)
{
	PK_ASSERT(!IsTemplate());

	PK_ASSERT(scene != null);
	PK_ASSERT(m_CurrentScene == scene);
	PK_ASSERT(selfIdInPreInit != PopcornFX::CGuid::INVALID);

	PK_ASSERT(m_Scene_PreInitEmitterId == PopcornFX::CGuid::INVALID);
	m_Scene_PreInitEmitterId = selfIdInPreInit;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::Scene_OnPreInitUnregistered(CParticleScene *scene)
{
	PK_ASSERT(!IsTemplate());

	PK_ASSERT(scene != null);
	PK_ASSERT(m_CurrentScene == scene);

	PK_ASSERT(m_Scene_PreInitEmitterId != PopcornFX::CGuid::INVALID);

	m_Scene_PreInitEmitterId = PopcornFX::CGuid::INVALID;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::Scene_OnRegistered(CParticleScene *scene, uint32 selfIdInScene)
{
	PK_ASSERT(!IsTemplate());

	PK_ASSERT(scene != null);
	PK_ASSERT(m_CurrentScene == scene);
	PK_ASSERT(selfIdInScene != PopcornFX::CGuid::INVALID);

	PK_ASSERT(m_Scene_EmitterId == PopcornFX::CGuid::INVALID);
	m_Scene_EmitterId = selfIdInScene;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::Scene_OnUnregistered(CParticleScene *scene)
{
	PK_ASSERT(!IsTemplate());

	PK_ASSERT(scene != null);
	PK_ASSERT(m_CurrentScene == scene);

	PK_ASSERT(m_Scene_EmitterId != PopcornFX::CGuid::INVALID);

	m_Scene_EmitterId = PopcornFX::CGuid::INVALID;
	m_CurrentScene = null;

	// !! make sure we are unregisterd (if not, infinite recusre)
	PK_ASSERT(!SelfSceneIsRegistered());
	if (!SelfSceneIsRegistered())
		TerminateEmitter(bKillParticlesOnDestroy);
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::Scene_InitForUpdate(CParticleScene *scene)
{
	PK_ASSERT(scene != null);
	PK_ASSERT(m_CurrentScene == scene);

	SelfPreInitSceneUnregister();
	EmitterBeginPlayIFN();
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::Scene_PreUpdate(CParticleScene *scene, float deltaTime, enum ELevelTick tickType)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_ASSERT(!IsTemplate());

	PK_ASSERT(scene != null);
	PK_ASSERT(SelfSceneIsRegistered());
	PK_ASSERT(m_CurrentScene == scene);

	if (IsPendingKill() || m_DiedThisFrame)
		return;
	// we should be ticking only if alive (kind-of optim purpose only)
	PK_ASSERT(IsEmitterAlive());

	const float				scale = FPopcornFXPlugin::GlobalScale();
	const float				rcpscale = 1.f / scale;

	// Scene_PreUpdate is executed once per Update, so Prev Curr should always be processed
	PK_ASSERT_MESSAGE(m_LastFrameUpdate != GFrameCounter, "Should not happen");
	m_LastFrameUpdate = GFrameCounter;

	CFloat4x4			&currentTr = ToPkRef(m_CurrentWorldTransforms);
	CFloat4x4			&previousTr = ToPkRef(m_PreviousWorldTransforms);
	CFloat3				&prevPrevPos = ToPkRef(m_PreviousWorldPosition); // 2 frames of lag
	CFloat3				&currentVel = ToPkRef(m_CurrentWorldVelocity);
	CFloat3				&previousVel = ToPkRef(m_PreviousWorldVelocity);

	const bool			teleport = bAllowTeleport && m_TeleportThisFrame;
	// Always reset m_TeleportThisFrame
	if (m_TeleportThisFrame)
		m_TeleportThisFrame = false;

	if (!teleport)
	{
		// Prev = Curr
		prevPrevPos = previousTr.StrippedTranslations();
		previousTr = currentTr;
		previousVel = currentVel;
	}

	// Update Position
	currentTr = ToPk(GetComponentTransform().ToMatrixWithScale());
	currentTr.StrippedTranslations() *= rcpscale;

	// Update Velocity
	if (!teleport && deltaTime > 0)
		currentVel = PopcornFX::ParticleToolbox::PredictCurrentVelocity(currentTr.StrippedTranslations(), previousTr.StrippedTranslations(), prevPrevPos, deltaTime);
	else
		currentVel = CFloat3::ZERO;

	if (teleport)
	{
		// Make previous frame same as current
		prevPrevPos = currentTr.StrippedTranslations();
		previousTr = currentTr;
		previousVel = currentVel;
	}

	if (IsValid(AttributeList))// && AttributeList->bNeedTick)
	{
		AttributeList->CheckEmitter(this);
		AttributeList->Scene_PreUpdate(scene, this, deltaTime, tickType);
	}

	AActor	*owner = GetOwner();
	bool	isVisible = IsVisible();
	float	timeScale = 1.0f;
	if (owner != null)
	{
		timeScale = owner->GetActorTimeDilation();
#if WITH_EDITOR
		if (owner->IsHiddenEd())
			isVisible = false;
#endif
	}
	if (PK_VERIFY(m_EffectInstancePtr != null))
	{
		m_EffectInstancePtr->SetVisible(isVisible);
		m_EffectInstancePtr->SetTimeScale(timeScale);
	}

#if WITH_EDITOR
	// Notify samplers they are indirectly selected this frame
	if (!GetWorld()->IsGameWorld() && IsValid(AttributeList))
	{
		const USelection	*selectedAssets = GEditor->GetSelectedActors();
		PK_ASSERT(selectedAssets != null);
		bool				isSelected = selectedAssets->IsSelected(GetOwner());
		if (isSelected)
			AttributeList->AttributeSamplers_IndirectSelectedThisTick(this);
	}
#endif
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::Scene_PostUpdate(CParticleScene *scene, float deltaTime, enum ELevelTick tickType)
{
	// Destroy component if instance was destroyed during update
	CheckForDead();
}

//----------------------------------------------------------------------------

bool	UPopcornFXEmitterComponent::SelfSceneIsRegistered() const
{
	return m_Scene_EmitterId != PopcornFX::CGuid::INVALID && m_CurrentScene != null;
}

//----------------------------------------------------------------------------

bool	UPopcornFXEmitterComponent::SelfPreInitSceneRegister()
{
	if (!SelfSceneIsRegistered()) // safeguard against UE random order of calling OnRegister/BeginPlay
	{
		PK_ASSERT(!IsTemplate());

		if (m_CurrentScene != null)
			return m_CurrentScene->Emitter_PreInitRegister(this);

		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::SelfPreInitSceneUnregister()
{
	PK_ASSERT(!IsTemplate());

	if (m_CurrentScene != null)
		m_CurrentScene->Emitter_PreInitUnregister(this);
}

//----------------------------------------------------------------------------

bool	UPopcornFXEmitterComponent::SelfSceneRegister()
{
	PK_ASSERT(!IsTemplate());

	PK_ASSERT(!SelfSceneIsRegistered());
	if (m_CurrentScene != null)
		m_CurrentScene->Emitter_Register(this);
	bool		ok = false;
	if (SelfSceneIsRegistered())
	{
		ok = true;
	}
	return ok;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::SelfSceneUnregister()
{
	PK_ASSERT(!IsTemplate());

	PK_ASSERT(SelfSceneIsRegistered());
	if (m_CurrentScene != null)
		m_CurrentScene->Emitter_Unregister(this);
	PK_ASSERT(!SelfSceneIsRegistered());
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

void	UPopcornFXEmitterComponent::OnPopcornFXFileUnloaded(const UPopcornFXFile *file)
{
	if (Effect == null)
	{
		PK_ASSERT(!IsEmitterEmitting());
		return; // Legit, emitter in level, asset is destroyed. We have PostEditChangeProperty called before unload method, so our Effect is null. Can't unregister callbacks
	}
	PK_ASSERT(!IsTemplate());
	if (!PK_VERIFY(file == Effect))
		return;
	m_ReplayAfterDead = IsEmitterEmitting();
	Effect->m_OnPopcornFXFileUnloaded.Remove(m_OnPopcornFXFileUnloadedHandle);
	Effect->m_OnPopcornFXFileLoaded.Remove(m_OnPopcornFXFileLoadedHandle);
	m_OnPopcornFXFileLoadedHandle = Effect->m_OnPopcornFXFileLoaded.AddUObject(this, &UPopcornFXEmitterComponent::OnPopcornFXFileLoaded);
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::OnPopcornFXFileLoaded(const UPopcornFXFile *file)
{
	if (Effect == null)
	{
		PK_ASSERT(!IsEmitterEmitting());
		return; // Legit, emitter in level, asset is destroyed. We have PostEditChangeProperty called before unload method, so our Effect is null. Can't unregister callbacks
	}
	PK_ASSERT(!IsTemplate());
	if (!PK_VERIFY(file == Effect))
		return;
	if (m_ReplayAfterDead)
	{
		Effect->m_OnPopcornFXFileLoaded.Remove(m_OnPopcornFXFileLoadedHandle);
		m_ReplayAfterDead = false;
		SetEffect(Effect, true);
	}
}
#endif

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::SetAutoAttachmentParameters(USceneComponent *parent, FName socketName, EAttachmentRule locationRule, EAttachmentRule rotationRule, EAttachmentRule scaleRule)
{
	AutoAttachParent = parent;
	AutoAttachSocketName = socketName;
	AutoAttachLocationRule = locationRule;
	AutoAttachRotationRule = rotationRule;
	AutoAttachScaleRule = scaleRule;
}

//----------------------------------------------------------------------------

void	UPopcornFXEmitterComponent::CancelAutoAttachment()
{
	if (bAutoManageAttachment)
	{
		if (m_DidAutoAttach)
		{
			// Restore relative transform from before attachment. Actual transform will be updated as part of DetachFromParent().
			SetRelativeLocation_Direct(m_SavedAutoAttachRelativeLocation);
			SetRelativeRotation_Direct(m_SavedAutoAttachRelativeRotation);
			SetRelativeScale3D_Direct(m_SavedAutoAttachRelativeScale3D);
			m_DidAutoAttach = false;
		}

		DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	}
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
