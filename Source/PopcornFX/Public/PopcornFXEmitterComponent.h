//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"
#include "PopcornFXTypes.h"

#include "PopcornFXEmitter.h"
#include "PopcornFXRefPtrWrap.h"

#include "Components/SceneComponent.h"
#include "Runtime/Launch/Resources/Version.h"

#include "PopcornFXEmitterComponent.generated.h"

FWD_PK_API_BEGIN
class	CParticleEffectInstance;
typedef TRefPtr<CParticleEffectInstance>	ParticleEffectInstance_;
FWD_PK_API_END

// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

class	CParticleScene;
class	UPopcornFXEffect;
class	UPopcornFXAttributeList;
class	UPopcornFXEmitterComponent;
class	APopcornFXSceneActor;

DECLARE_DYNAMIC_DELEGATE(FPopcornFXRaiseEventSignature);

/** Instanciate and Emits and PopcornFX Effect into a PopcornFX Scene. */
UCLASS(HideCategories=(Object, LOD, Physics, Collision, Activation, "Components|Activation"), EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXEmitterComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:
	// dont access it direclty, use GetAttributeList() instead
	UPROPERTY(Category="PopcornFX Attributes", Instanced, VisibleAnywhere, BlueprintReadOnly)
	class UPopcornFXAttributeList			*AttributeList;

	UPROPERTY(Category="PopcornFX Emitter", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail="true"))
	UPopcornFXEffect						*Effect;

	/** If false, the emitter will not update the emission position neither attributes */
	UPROPERTY(Category="PopcornFX Emitter", EditAnywhere, BlueprintReadWrite)
	uint32									bEnableUpdates : 1;

	/** If true, the Emitter will be automaticaly started when spawned */
	UPROPERTY(Category="PopcornFX Emitter", EditAnywhere, BlueprintReadWrite)
	uint32									bPlayOnLoad : 1;

	/** If true, particles are killed when this emitter gets destroyed */
	UPROPERTY(Category="PopcornFX Emitter", EditAnywhere, BlueprintReadWrite)
	uint32									bKillParticlesOnDestroy : 1;

	/** If true, the Emitter will skip transformations when teleported.
	* So, continuous spawners will not spawn in-between teleportation.
	* But, it will break Localspace Evolvers !
	*/
	UPROPERTY(Category="PopcornFX Emitter", EditAnywhere, BlueprintReadWrite)
	uint32									bAllowTeleport : 1;

	/**
	* Enables effect prewarm. By default the emitter will use the parent effect's prewarm time.
	* Value needs to be set before the emitter is started.
	*/
	UPROPERTY(Category="PopcornFX Emitter", EditAnywhere, BlueprintReadWrite)
	uint32									bEnablePrewarm : 1;

	UPROPERTY(Category="PopcornFX Emitter", EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
	uint32									bOverridePrewarmTime : 1;

	UPROPERTY(Category="PopcornFX Emitter", EditAnywhere, BlueprintReadOnly, meta=(ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "20.0"), AdvancedDisplay)
	float									PrewarmTime;

	/** The name of the PopcornFXSceneComponent.SceneName to register for update and render */
	UPROPERTY(Category="PopcornFX Emitter", EditAnywhere, BlueprintReadOnly)
	FName									SceneName;

	UPROPERTY(Category="PopcornFX Emitter", BlueprintReadOnly, Transient)
	APopcornFXSceneActor					*Scene;

	/** Event called when StartEmitter() is called */
	UPROPERTY(Category="PopcornFX Events", BlueprintAssignable)
	FPopcornFXEmitterStartSignature			OnEmitterStart;

	/** Event called when StopEmitter() is called */
	UPROPERTY(Category="PopcornFX Events", BlueprintAssignable)
	FPopcornFXEmitterStopSignature			OnEmitterStop;

	/** Event called when TerminateEmitter() is called */
	UPROPERTY(Category="PopcornFX Events", BlueprintAssignable)
	FPopcornFXEmitterTerminateSignature		OnEmitterTerminate;

	/** Event called when TerminateEmitter() is called */
	UPROPERTY(Category = "PopcornFX Events", BlueprintAssignable)
	FPopcornFXEmitterKillParticlesSignature	OnEmitterKillParticles;

	/** Event called when the particle emission stops */
	UPROPERTY(Category="PopcornFX Events", BlueprintAssignable)
	FPopcornFXEmitterStopSignature			OnEmissionStops;

	uint32									bAutoDestroy : 1;
	uint32									bHasAlreadyPlayOnLoad : 1;

	~UPopcornFXEmitterComponent();

	/**
	* Terminate the current emitter, set the effect to @InEffect, reset attributes, and StartEmitter if @bStartEmitter.
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", bStartEmitter="false"))
	bool							SetEffect(UPopcornFXEffect *InEffect, bool bStartEmitter = false);

	/**
	* Sets the emitter's prewarm time (overrides the default parent effect's prewarm time).
	* This needs to be called before the emitter is started, and the emitter needs to have bEnablePrewarm enabled.
	*/
	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Emitter")
	void							SetPrewarmTime(float time);

	/**
	* Starts the emitter if not yet started
	* @return true if the emitter has been started
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", UnsafeDuringActorConstruction="true"))
	bool							StartEmitter();

	/** TerminateEmitter() then StartEmitter() */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", UnsafeDuringActorConstruction="true"))
	void							RestartEmitter(bool killParticles = false);

	/** Stops particle emission if emitting (the emitter can still be alive after that, see "IsEmitterStarted") */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", UnsafeDuringActorConstruction="true"))
	void							StopEmitter(bool killParticles = false);

	/** if @startEmitter StartEmitter(), else StopEmitter(), returns IsEmitterStarted() */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", UnsafeDuringActorConstruction="true"))
	bool							ToggleEmitter(bool startEmitter, bool killParticles = false);

	/** Stops emission and unregister particle instance (will unregister attached locations and attributes) */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", UnsafeDuringActorConstruction="true"))
	void							TerminateEmitter(bool killParticles = false);

	// Stops all layers/emitters of this instance, and kills all particles spawned by this effect instance
	// The kill will be processed during the next medium-collection update.
	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Emitter", meta = (Keywords = "popcornfx particle emitter effect system kill", UnsafeDuringActorConstruction = "true"))
	void							KillParticles();

	/** Get whether the emitter is still emitting particles or not */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", UnsafeDuringActorConstruction="true"))
	bool							IsEmitterEmitting() const;

	/**
	* Get whether the emitter is still alive or not.
	* An emitter could have stop emitting particles, but can still be "Started" ("Alive") if emitted particles still needs the emitter to be alive
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", UnsafeDuringActorConstruction="true"))
	bool							IsEmitterStarted() const;

	/** Alias of IsEmitterStarted() */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", UnsafeDuringActorConstruction="true"))
	bool							IsEmitterAlive() const;

	/** Reset all attribute to their default values */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Attributes", meta=(Keywords="popcornfx particle emitter effect system attribute"))
	void							ResetAttributesToDefault();

	/**
	* Registers an event listener.
	* @param Delegate - Delegate to call when the event is raised.
	* @param EventName - Name of the Event to listen.
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system event"))
	bool							RegisterEventListener(FPopcornFXRaiseEventSignature Delegate, FName EventName = NAME_None);

	/**
	* Unregisters an event listener
	* @param Delegate - Delegate to unregister from the event
	* @param EventName - Name of the Event to stop listening.
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system event"))
	void							UnregisterEventListener(FPopcornFXRaiseEventSignature Delegate, FName EventName = NAME_None);


	/*
	* Unregisters all events listeners
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system event"))
	void							UnregisterAllEventsListeners();

	/** Copies himself with current attributes values then
	* Plays the specified PopcornFXEffect at the given location and rotation, fire and forget.
	* The system will go away when the emitter is complete. Does not replicate.
	* @param Location - location to place the emitter in world space
	* @param Rotation - rotation to place the emitter in world space
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", bAutoDestroy="true", UnsafeDuringActorConstruction="true"))
	UPopcornFXEmitterComponent		*CopyAndStartEmitterAtLocation(FVector Location, FRotator Rotation = FRotator::ZeroRotator, bool bAutoDestroy = true);

	/** Copies himself with current attributes values then
	* Plays the specified PopcornFXEffect attached to and following the specified component.
	* The system will go away when the emitter is complete. Does not replicate.
	* @param AttachComponent - Component to attach to.
	* @param AttachPointName - Optional named point within the AttachComponent to spawn the emitter at
	* @param Location - Depending on the value of Location Type this is either a relative offset from the attach component/point or an absolute world position that will be translated to a relative offset
	* @param Rotation - Depending on the value of LocationType this is either a relative offset from the attach component/point or an absolute world rotation that will be translated to a realative offset
	* @param LocationType - Specifies whether Location is a relative offset or an absolute world position
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|Emitter", meta=(Keywords="popcornfx particle emitter effect system", bAutoDestroy="true", UnsafeDuringActorConstruction="true"))
	UPopcornFXEmitterComponent		*CopyAndStartEmitterAttached(class USceneComponent* AttachToComponent, FName AttachPointName = NAME_None, FVector Location = FVector(ForceInit), FRotator Rotation = FRotator::ZeroRotator, EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bAutoDestroy = true);

public:
	/**
	* True if we should automatically attach to AutoAttachParent when activated, and detach from our parent when completed.
	* This overrides any current attachment that may be present at the time of activation (deferring initial attachment until activation, if AutoAttachParent is null).
	* When enabled, detachment occurs regardless of whether AutoAttachParent is assigned, and the relative transform from the time of activation is restored.
	* This also disables attachment on dedicated servers, where we don't actually activate even if bAutoActivate is true.
	* @see AutoAttachParent, AutoAttachSocketName, AutoAttachLocationType
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attachment)
	uint32	bAutoManageAttachment : 1;

	/**
	 * Component we automatically attach to when activated, if bAutoManageAttachment is true.
	 * If null during registration, we assign the existing AttachParent and defer attachment until we activate.
	 * @see bAutoManageAttachment
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category=Attachment, meta=(EditCondition="bAutoManageAttachment"))
	TWeakObjectPtr<USceneComponent>		AutoAttachParent;

	/**
	 * Socket we automatically attach to on the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attachment, meta=(EditCondition="bAutoManageAttachment"))
	FName	AutoAttachSocketName;

	/**
	 * Options for how we handle our location when we attach to the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment, EAttachmentRule
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment, meta = (EditCondition = "bAutoManageAttachment"))
	EAttachmentRule	AutoAttachLocationRule;

	/**
	 * Options for how we handle our rotation when we attach to the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment, EAttachmentRule
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment, meta = (EditCondition = "bAutoManageAttachment"))
	EAttachmentRule	AutoAttachRotationRule;

	/**
	 * Options for how we handle our scale when we attach to the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment, EAttachmentRule
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment, meta = (EditCondition = "bAutoManageAttachment"))
	EAttachmentRule	AutoAttachScaleRule;

	/**
	 * Set AutoAttachParent, AutoAttachSocketName, AutoAttachLocationRule, AutoAttachRotationRule, AutoAttachScaleRule to the specified parameters. Does not change bAutoManageAttachment; that must be set separately.
	 * @param  Parent			Component to attach to. 
	 * @param  SocketName		Socket on Parent to attach to.
	 * @param  LocationRule		Option for how we handle our location when we attach to Parent.
	 * @param  RotationRule		Option for how we handle our rotation when we attach to Parent.
	 * @param  ScaleRule		Option for how we handle our scale when we attach to Parent.
	 * @see bAutoManageAttachment, AutoAttachParent, AutoAttachSocketName, AutoAttachLocationRule, AutoAttachRotationRule, AutoAttachScaleRule
	 */
	void	SetAutoAttachmentParameters(USceneComponent* Parent, FName SocketName, EAttachmentRule LocationRule, EAttachmentRule RotationRule, EAttachmentRule ScaleRule);

	void	SetUseAutoManageAttachment(bool bAutoManage) { bAutoManageAttachment = bAutoManage; }
	void	CancelAutoAttachment();

public:
	static UPopcornFXEmitterComponent	*CreateStandaloneEmitterComponent(UPopcornFXEffect* effect, APopcornFXSceneActor *scene, UWorld* world, AActor* actor, bool bAutoDestroy);

	class UPopcornFXAttributeList	*GetAttributeList(); // Updates attribute list ifn
	class UPopcornFXAttributeList	*GetAttributeListIFP() const;

	bool							GetPayloadValue(const FName &payloadName, EPopcornFXPayloadType::Type expectedFieldType, void *outValue) const;

	bool							ResolveScene(bool warnIFN);
	bool							SceneValid() const { return m_CurrentScene != nullptr; }

	//overrides
	virtual void					PostLoad() override;
	virtual void					BeginDestroy() override;
	virtual void					OnRegister() override;
	virtual void					OnUnregister() override;
	virtual void					BeginPlay() override;
	virtual void					EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FBoxSphereBounds		CalcBounds(const FTransform &localToWorld) const override;

	virtual void					OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;
	virtual void					ApplyWorldOffset(const FVector &inOffset, bool worldShift) override;

#if WITH_EDITOR
#if (ENGINE_MINOR_VERSION >= 25)
	virtual bool					CanEditChange(const FProperty* InProperty) const override;
#else
	virtual bool					CanEditChange(const UProperty* Property) const override;
#endif // (ENGINE_MINOR_VERSION >= 25)
	virtual void					PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
	virtual void					CheckForErrors() override;
	void							SpawnPreviewSceneIFN(UWorld *world);
#endif // WITH_EDITOR

	virtual const UObject			*AdditionalStatObject() const override;

	// PopcornFX Internals
	const CParticleScene				*_GetParticleScene() const { return m_CurrentScene; }
	PopcornFX::CParticleEffectInstance	*_GetEffectInstance() const;
	void								Scene_OnPreInitRegistered(CParticleScene *scene, uint32 selfIdInPreInit);
	void								Scene_OnPreInitUnregistered(CParticleScene *scene);
	void								Scene_OnRegistered(CParticleScene *scene, uint32 selfIdInScene);
	void								Scene_OnUnregistered(CParticleScene *scene);
	void								Scene_InitForUpdate(CParticleScene *scene);
	void								Scene_PreUpdate(CParticleScene *scene, float deltaTime, enum ELevelTick tickType);
	void								Scene_PostUpdate(CParticleScene *scene, float deltaTime, enum ELevelTick tickType);
	uint32								Scene_PreInitEmitterId() const { return m_Scene_PreInitEmitterId; };
	uint32								Scene_EmitterId() const { return m_Scene_EmitterId; };

private:
	void							EmitterBeginPlayIFN();
#if WITH_EDITOR
	void							_OnSceneLoaded(APopcornFXSceneActor *sceneActor);
	void							OnPopcornFXFileUnloaded(const class UPopcornFXFile *file);
	void							OnPopcornFXFileLoaded(const class UPopcornFXFile *file);
#endif
	void							_OnDeathNotifier(const PopcornFX::ParticleEffectInstance_ & instance);
	void							CheckForDead();

	bool							SelfSceneIsRegistered() const;
	bool							SelfPreInitSceneRegister();
	void							SelfPreInitSceneUnregister();
	bool							SelfSceneRegister();
	void							SelfSceneUnregister();

private:
	CParticleScene					*m_CurrentScene;
	uint32							m_Scene_PreInitEmitterId;
	uint32							m_Scene_EmitterId;

	TPopcornFXRefPtrWrap<PopcornFX::CParticleEffectInstance>	m_EffectInstancePtr;

	bool							m_Started;
	bool							m_Stopped;
	bool							m_DiedThisFrame;
	bool							m_TeleportThisFrame;

	bool							m_DidAutoAttach;
	FVector							m_SavedAutoAttachRelativeLocation;
	FRotator						m_SavedAutoAttachRelativeRotation;
	FVector							m_SavedAutoAttachRelativeScale3D;

#if WITH_EDITOR
	bool							m_ReplayAfterDead;
	FDelegateHandle					m_WaitForSceneDelegateHandle;
	FDelegateHandle					m_OnPopcornFXFileUnloadedHandle;
	FDelegateHandle					m_OnPopcornFXFileLoadedHandle;
#endif

	FMatrix							m_CurrentWorldTransforms;
	FMatrix							m_PreviousWorldTransforms;
	FVector							m_PreviousWorldPosition;
	FVector							m_CurrentWorldVelocity;
	FVector							m_PreviousWorldVelocity;

	uint64							m_LastFrameUpdate;
};
