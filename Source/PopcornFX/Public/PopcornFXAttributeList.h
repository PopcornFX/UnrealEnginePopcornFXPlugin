//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXAttributeSampler.h"

#include "PopcornFXAttributeList.generated.h"

class	AActor;

class	UPopcornFXEffect;
class	UPopcornFXAttribSamplerInterface;
class	UPopcornFXEmitterComponent;

uint32				ToPkShapeType(EPopcornFXAttribSamplerShapeType::Type ueShapeType);

#if WITH_EDITORONLY_DATA
UENUM()
namespace EPopcornFXAttributeSemantic
{
	enum	Type
	{
		AttributeSemantic_None = 0,
		AttributeSemantic_3DCoordinate,
		AttributeSemantic_3DScale,
		AttributeSemantic_Color,
		AttributeSemantic_Count
	};
}

UENUM()
namespace EPopcornFXAttributeDropDownMode
{
	enum	Type
	{
		AttributeDropDownMode_None = 0,
		AttributeDropDownMode_SingleSelect,
		AttributeDropDownMode_MultiSelect,
		AttributeDropDownMode_Count
	};
}
#endif // WITH_EDITORONLY_DATA

USTRUCT()
struct FPopcornFXAttributeDesc
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	FString					m_AttributeName;

	UPROPERTY()
	FString					m_AttributeCategoryName;

	UPROPERTY()
	uint32					m_AttributeType;

	UPROPERTY()
	bool					m_IsPrivate;
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TEnumAsByte<EPopcornFXAttributeSemantic::Type>	m_AttributeSemantic;

	UPROPERTY()
	FVector					m_AttributeEulerAngles;

	UPROPERTY()
	TEnumAsByte<EPopcornFXAttributeDropDownMode::Type>	m_DropDownMode;

	UPROPERTY()
	TArray<FString>			m_EnumList;
#endif // WITH_EDITORONLY_DATA

	FPopcornFXAttributeDesc()
	:	m_AttributeName()
	,	m_AttributeCategoryName()
	,	m_AttributeType(~0U)
	,	m_IsPrivate(false)
#if WITH_EDITORONLY_DATA
	,	m_AttributeSemantic(EPopcornFXAttributeSemantic::AttributeSemantic_None)
	,	m_AttributeEulerAngles(FVector(0.f, 0.f, 0.f))
	,	m_DropDownMode(EPopcornFXAttributeDropDownMode::AttributeDropDownMode_None)
#endif // WITH_EDITORONLY_DATA
	{ }

	bool					Valid() const { return !m_AttributeName.IsEmpty(); }
	FName					AttributeFName() const { return FName(m_AttributeName); }
	bool					ValidAttributeType() const { return m_AttributeType != ~0U; }
	uint32					AttributeBaseTypeID() const { verify(ValidAttributeType());  return m_AttributeType; }

	void		Reset()
	{
		m_AttributeName = FString();
		m_AttributeCategoryName = FString();
		m_AttributeType = ~0U;
#if WITH_EDITORONLY_DATA
		m_AttributeSemantic = EPopcornFXAttributeSemantic::AttributeSemantic_None;
		m_AttributeEulerAngles = FVector(0.f, 0.f, 0.f);
		m_DropDownMode = EPopcornFXAttributeDropDownMode::AttributeDropDownMode_None;
		m_EnumList.Empty();
#endif // WITH_EDITORONLY_DATA
	}

	bool		ExactMatch(const FPopcornFXAttributeDesc &other) const
	{
		return Valid() && m_AttributeName == other.m_AttributeName && m_AttributeType == other.m_AttributeType && m_AttributeCategoryName == other.m_AttributeCategoryName;
	}
};

USTRUCT()
struct FPopcornFXSamplerDesc
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(Category = "PopcornFX AttributeSampler", VisibleAnywhere)
	FString												m_SamplerName;

	UPROPERTY(Category = "PopcornFX AttributeSampler", VisibleAnywhere)
	FString												m_AttributeCategoryName;

	UPROPERTY(Category = "PopcornFX AttributeSampler", VisibleAnywhere)
	TEnumAsByte<EPopcornFXAttributeSamplerType::Type>	m_SamplerType;

	/** The actor that contains a sampler component to use for this attribute */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	AActor												*m_AttributeSamplerActor;

	/** The name of the sampler component in the target actor. Will try to use the root component if none is specified */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	FString												m_AttributeSamplerComponentName;

	/**
		False if we can't find any sampler component with this name in the target actor
		Target actor can't be invalid since it falls back on the emitter's owner in not given
	*/
	UPROPERTY(VisibleAnywhere)
	mutable bool										m_IsSamplerComponentValid = false; // Useless?

	/** Use an external sampler for this attribute */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	bool												m_UseExternalSampler;

	/** Restart the emitter when this sampler changes? */
	UPROPERTY(EditAnywhere)
	bool												m_RestartWhenSamplerChanges = true;

	UPROPERTY()
	bool					m_IsPrivate;

	bool					m_NeedUpdate;

	FPopcornFXSamplerDesc()
	:	m_SamplerName()
	,	m_AttributeCategoryName()
	,	m_SamplerType(EPopcornFXAttributeSamplerType::None)
	,	m_AttributeSamplerActor(nullptr)
	,	m_AttributeSamplerComponentName()
	,	m_UseExternalSampler(false)
	,	m_IsPrivate(false)
	,	m_NeedUpdate(false)
	{ }

	bool		Valid() const { return true; }
	FName		SamplerFName() const { return FName(m_SamplerName); }
	EPopcornFXAttributeSamplerType::Type	SamplerType() const { return m_SamplerType; }

	UPopcornFXAttributeSampler		*ResolveExternalAttributeSampler(UPopcornFXEmitterComponent *emitter, const UObject *enableLogForOwner) const;
	UPopcornFXAttributeSampler		*ResolveAttributeSampler(UPopcornFXEmitterComponent *emitter, const UObject *enableLogForOwner) const;

	void		Reset()
	{
		m_SamplerName = FString();
		m_AttributeCategoryName = FString();
		m_SamplerType = EPopcornFXAttributeSamplerType::None;
	}

	void		CopyValuesFrom(const FPopcornFXSamplerDesc &other)
	{
		m_AttributeSamplerActor = other.m_AttributeSamplerActor;
		m_AttributeSamplerComponentName = other.m_AttributeSamplerComponentName;
	}

	void		SwapValuesWith(FPopcornFXSamplerDesc &other)
	{
		Swap(m_AttributeSamplerActor, other.m_AttributeSamplerActor);
		Swap(m_AttributeSamplerComponentName, other.m_AttributeSamplerComponentName);
	}

	void		ResetValue() { m_AttributeSamplerActor = nullptr; m_AttributeSamplerComponentName = FString(); }
	bool		ValueIsEmpty() const { return m_AttributeSamplerActor == nullptr && m_AttributeSamplerComponentName.IsEmpty(); }

	bool		ExactMatch(const FPopcornFXSamplerDesc &other) const
	{
		return Valid() && m_SamplerName == other.m_SamplerName && m_SamplerType == other.m_SamplerType && m_AttributeCategoryName == other.m_AttributeCategoryName;
	}
};

// Dummy
struct	FPopcornFXAttributeValue
{
	uint32	m_Value[4];
};

UCLASS(MinimalAPI)
class UPopcornFXAttributeList : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	~UPopcornFXAttributeList();

	bool			CheckDataIntegrity() const;
	bool			Valid() const;
	bool			IsUpToDate(UPopcornFXEffect *effect) const;

	bool			IsEmpty() const;
	void			Clean();

	void			SetupDefault(UPopcornFXEffect *sourceEffect, bool force = false);
	bool			Prepare(UPopcornFXEffect *effect, bool force = false);
	void			CopyFrom(const UPopcornFXAttributeList *other, AActor *patchParentActor = nullptr);

	const UPopcornFXAttributeList		*GetDefaultAttributeList(UPopcornFXEffect *effect) const; // can be self

	void			ResetToDefaultValues(UPopcornFXEmitterComponent *emitter, UPopcornFXEffect *effect);

	void			RefreshAttributeSamplers(UPopcornFXEmitterComponent *emitter, bool reload = false) { if (m_Samplers.Num() > 0) _RefreshAttributeSamplers(emitter, reload); }
	void			RefreshAttributes(const UPopcornFXEmitterComponent *emitter) { if (m_Attributes.Num() > 0) _RefreshAttributes(emitter); }

	void			Scene_PreUpdate(UPopcornFXEmitterComponent *emitter, float deltaTime);
#if WITH_EDITOR
	void			AttributeSamplers_IndirectSelectedThisTick(UPopcornFXEmitterComponent *emitter) const;

	// Gets & resets restart state
	bool			GetRestartEmitter() { const bool restartEmitter = m_RestartEmitter; m_RestartEmitter = false; return restartEmitter; }

	uint32			GetCategoryCount() const { return m_Categories.Num(); }
	FString			GetCategoryName(uint32 categoryId) const { return m_Categories[categoryId]; }

#endif

	uint32			AttributeCount() const { return m_Attributes.Num(); }
	int32			FindAttributeIndex(const FString &name) const;

	uint32			SamplerCount() const { return m_Samplers.Num(); }
	int32			FindSamplerIndex(const FString &name) const;

	const FPopcornFXAttributeDesc						*GetAttributeDesc(uint32 attributeId) const;
	const FPopcornFXSamplerDesc							*GetSamplerDesc(uint32 samplerId) const;

	const void											*GetAttributeDeclaration(UPopcornFXEffect *effect, uint32 attributeId) const;
#if WITH_EDITOR
	const void											*GetParticleSampler(UPopcornFXEffect *effect, uint32 samplerId) const;
#endif
	void												GetAttribute(uint32 attributeId, FPopcornFXAttributeValue &outValue) const;
	void												SetAttribute(uint32 attributeId, const FPopcornFXAttributeValue &value, bool fromUI = false);

	bool												SetAttributeSampler(const FString &samplerName, AActor *actor, const FString &propertyName);

#if WITH_EDITOR
	float												GetAttributeQuaternionDim(uint32 attributeId, uint32 dim);
	void												SetAttributeQuaternionDim(uint32 attributeId, uint32 dim, float value, bool fromUI = false);

	template<typename _Scalar> void						SetAttributeDim(uint32 attributeId, uint32 dim, _Scalar value, bool fromUI = false);
	template<typename _Scalar> _Scalar					GetAttributeDim(uint32 attributeId, uint32 dim);

	void												PulseBoolAttributeDim(uint32 attributeId, uint32 dim, bool fromUI = false);

	void												ResetPulsedBoolAttributesIFN();
#endif // WITH_EDITOR

	uint32					FileVersionId() const { return m_FileVersionId; }
	UPopcornFXEffect		*Effect() const { return m_Effect; }

	// overrides UObject
	virtual bool	IsSafeForRootSet() const override { return false; }
	virtual void	PostLoad() override;
	virtual void	PostInitProperties() override;
	virtual void	BeginDestroy() override;
	virtual void	PreSave(FObjectPreSaveContext SaveContext) override;
	virtual void	Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void	PostEditUndo() override;
#endif

	void			RestoreAttributesFromCachedRawData(const TArray<uint8> &rawData);

private:
	void			_RefreshAttributes(const UPopcornFXEmitterComponent *emitter);
	void			_RefreshAttributeSamplers(UPopcornFXEmitterComponent *emitter, bool reload);

public:
	UPROPERTY()
	UPopcornFXEffect					*m_Effect;

	UPROPERTY(Category="PopcornFX Attributes", VisibleAnywhere)
	uint32								m_FileVersionId;

	UPROPERTY(Category="PopcornFX Attributes", VisibleAnywhere)
	TArray<FPopcornFXAttributeDesc>		m_Attributes;

	UPROPERTY(Category="PopcornFX Attributes", EditAnywhere, EditFixedSize)
	TArray<FPopcornFXSamplerDesc>		m_Samplers;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TArray<FString>						m_Categories; // Per effect, we don't need that info per emitter

	bool								m_RestartEmitter = false; // UPopcornFXSettingsEditor::bRestartEmitterWhenAttributesChanged
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	bool								m_HasPendingOneShotReset = false;
#endif // WITH_EDITOR

	UPROPERTY(Category="PopcornFX Attributes", EditAnywhere, BlueprintReadOnly, EditFixedSize)
	TArray<uint8>						m_AttributesRawData;

	TWeakObjectPtr<const UPopcornFXEmitterComponent>	m_Owner;

	void								CheckEmitter(const UPopcornFXEmitterComponent *emitter);
};
