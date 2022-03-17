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
const char			*ResolveAttribSamplerShapeNodeName(EPopcornFXAttribSamplerShapeType::Type shapeType);

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
#endif // WITH_EDITORONLY_DATA

USTRUCT()
struct FPopcornFXAttributeDesc
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	FName					m_AttributeFName;

	UPROPERTY()
	FName					m_AttributeCategoryName;

	UPROPERTY()
	uint32					m_AttributeType;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TEnumAsByte<EPopcornFXAttributeSemantic::Type>	m_AttributeSemantic;

	UPROPERTY(Transient)
	uint32					m_IsExpanded : 1;

	UPROPERTY()
	FVector					m_AttributeEulerAngles;
#endif // WITH_EDITORONLY_DATA

	FPopcornFXAttributeDesc()
	:	m_AttributeFName()
	,	m_AttributeCategoryName()
	,	m_AttributeType(~0U)
#if WITH_EDITORONLY_DATA
	,	m_AttributeSemantic(EPopcornFXAttributeSemantic::AttributeSemantic_None)
	,	m_IsExpanded(false)
	, m_AttributeEulerAngles(FVector(0.f, 0.f, 0.f))
#endif // WITH_EDITORONLY_DATA
	{ }

	bool					Valid() const { return m_AttributeFName.IsValid() && !m_AttributeFName.IsNone(); }
	FName					AttributeFName() const { return m_AttributeFName; }
	FString					AttributeName() const { return (Valid() ? m_AttributeFName.ToString() : FString()); }
	bool					ValidAttributeType() const { return m_AttributeType != ~0U; }
	uint32					AttributeBaseTypeID() const { verify(ValidAttributeType());  return m_AttributeType; }

	void		Reset()
	{
		m_AttributeFName = FName();
		m_AttributeCategoryName = FName();
		m_AttributeType = ~0U;
#if WITH_EDITORONLY_DATA
		m_AttributeSemantic = EPopcornFXAttributeSemantic::AttributeSemantic_None;
		m_AttributeEulerAngles = FVector(0.f, 0.f, 0.f);
#endif // WITH_EDITORONLY_DATA
	}

	bool		ExactMatch(const FPopcornFXAttributeDesc &other) const
	{
		return Valid() && m_AttributeFName == other.m_AttributeFName && m_AttributeType == other.m_AttributeType && m_AttributeCategoryName == other.m_AttributeCategoryName;
	}
};

USTRUCT()
struct FPopcornFXSamplerDesc
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(Category="PopcornFX AttributeSampler", VisibleAnywhere)
	FName						m_SamplerFName;

	UPROPERTY(Category="PopcornFX AttributeSampler", VisibleAnywhere)
	FName						m_AttributeCategoryName;

	UPROPERTY(Category="PopcornFX AttributeSampler", VisibleAnywhere)
	TEnumAsByte<EPopcornFXAttributeSamplerType::Type>	m_SamplerType;

	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	AActor						*m_AttributeSamplerActor;

	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	FName						m_AttributeSamplerComponentProperty;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	uint32						m_IsExpanded : 1;
#endif // WITH_EDITORONLY_DATA

	bool					m_NeedUpdate;

	FPopcornFXSamplerDesc()
	:	m_SamplerFName()
	,	m_AttributeCategoryName()
	,	m_SamplerType(EPopcornFXAttributeSamplerType::None)
	,	m_AttributeSamplerActor(nullptr)
#if WITH_EDITORONLY_DATA
	,	m_IsExpanded(false)
#endif // WITH_EDITORONLY_DATA
	,	m_NeedUpdate(false)
	{ }

	bool		Valid() const { return m_SamplerFName.IsValid() && !m_SamplerFName.IsNone(); }
	FName		SamplerFName() const { return m_SamplerFName; }
	FString		SamplerName() const { return (Valid() ? m_SamplerFName.ToString() : FString()); }
	EPopcornFXAttributeSamplerType::Type	SamplerType() const { return m_SamplerType; }

	UPopcornFXAttributeSampler		*ResolveAttributeSampler(UPopcornFXEmitterComponent *emitter, UObject *enableLogForOwner) const;

	void		Reset()
	{
		m_SamplerFName = FName();
		m_AttributeCategoryName = FName();
		m_SamplerType = EPopcornFXAttributeSamplerType::None;
	}

	void		CopyValuesFrom(const FPopcornFXSamplerDesc &other)
	{
		m_AttributeSamplerActor = other.m_AttributeSamplerActor;
		m_AttributeSamplerComponentProperty = other.m_AttributeSamplerComponentProperty;
	}

	void		SwapValuesWith(FPopcornFXSamplerDesc &other)
	{
		Swap(m_AttributeSamplerActor, other.m_AttributeSamplerActor);
		Swap(m_AttributeSamplerComponentProperty, other.m_AttributeSamplerComponentProperty);
	}

	void		ResetValue() { m_AttributeSamplerActor = nullptr; m_AttributeSamplerComponentProperty = FName(); }
	bool		ValueIsEmpty() const { return m_AttributeSamplerActor == nullptr && m_AttributeSamplerComponentProperty.IsNone(); }

	bool		ExactMatch(const FPopcornFXSamplerDesc &other) const
	{
		return Valid() && m_SamplerFName == other.m_SamplerFName && m_SamplerType == other.m_SamplerType && m_AttributeCategoryName == other.m_AttributeCategoryName;
	}
};

// Dummy
struct	FPopcornFXAttributeValue
{
	uint32	m_Value[4];
};

UCLASS(MinimalAPI, EditInlineNew, DefaultToInstanced)
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

	void			ResetToDefaultValues(UPopcornFXEffect *effect);

	void			RefreshAttributeSamplers(UPopcornFXEmitterComponent *emitter, bool reload = false) { if (m_Samplers.Num() > 0) _RefreshAttributeSamplers(emitter, reload); }
	void			RefreshAttributes(UPopcornFXEmitterComponent *emitter) { if (m_Attributes.Num() > 0) _RefreshAttributes(emitter); }

	void			Scene_PreUpdate(class CParticleScene *scene, UPopcornFXEmitterComponent *emitter, float deltaTime, enum ELevelTick tickType);
#if WITH_EDITOR
	void			AttributeSamplers_IndirectSelectedThisTick(UPopcornFXEmitterComponent *emitter) const;

	float			GetColumnWidth() const { return m_ColumnWidth; }
	void			SetColumnWidth(float width) { m_ColumnWidth = width; }

	uint32			GetCategoryCount() const { return m_Categories.Num(); }
	FName			GetCategoryName(uint32 categoryId) const { return m_Categories[categoryId]; }
	bool			IsCategoryExpanded(uint32 categoryId) const;

	void			ToggleExpandedAttributeDetails(uint32 attributeId);
	void			ToggleExpandedSamplerDetails(uint32 samplerId);
	void			ToggleExpandedCategoryDetails(uint32 categoryId);
#endif

	uint32			AttributeCount() const { return m_Attributes.Num(); }
	int32			FindAttributeIndex(FName fname) const;

	uint32			SamplerCount() const { return m_Samplers.Num(); }
	int32			FindSamplerIndex(FName fname) const;

	const FPopcornFXAttributeDesc						*GetAttributeDesc(uint32 attributeId) const;
	const FPopcornFXSamplerDesc							*GetSamplerDesc(uint32 samplerId) const;

	const void											*GetAttributeDeclaration(UPopcornFXEffect *effect, uint32 attributeId) const;
#if WITH_EDITOR
	const void											*GetParticleSampler(UPopcornFXEffect *effect, uint32 samplerId) const;
#endif
	void												GetAttribute(uint32 attributeId, FPopcornFXAttributeValue &outValue) const;
	void												SetAttribute(uint32 attributeId, const FPopcornFXAttributeValue &value);

	bool												SetAttributeSampler(FName samplerName, AActor *actor, FName propertyName);

#if WITH_EDITOR
	float												GetAttributeQuaternionDim(uint32 attributeId, uint32 dim);
	void												SetAttributeQuaternionDim(uint32 attributeId, uint32 dim, float value);

	template<typename _Scalar> void						SetAttributeDim(uint32 attributeId, uint32 dim, _Scalar value);
	template<typename _Scalar> _Scalar					GetAttributeDim(uint32 attributeId, uint32 dim);
#endif // WITH_EDITOR

	uint32					FileVersionId() const { return m_FileVersionId; }
	const UPopcornFXEffect	*Effect() const { return m_Effect; }

	// overrides UObject
	virtual bool	IsSafeForRootSet() const override { return false; }
	virtual void	PostLoad() override;
	virtual void	PostInitProperties() override;
	virtual void	BeginDestroy() override;
#if (ENGINE_MAJOR_VERSION == 5)
	virtual void	PreSave(FObjectPreSaveContext SaveContext) override;
#else
	virtual void	PreSave(const class ITargetPlatform* TargetPlatform) override;
#endif // (ENGINE_MAJOR_VERSION == 5)
	virtual void	Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void	PostEditUndo() override;
	virtual void	PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif

	void			RestoreAttributesFromCachedRawData(const TArray<uint8> &rawData);

private:
	void			_RefreshAttributes(const UPopcornFXEmitterComponent *emitter);
	void			_RefreshAttributeSamplers(UPopcornFXEmitterComponent *emitter, bool reload);

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
	TArray<FName>						m_Categories; // Per effect, we don't need that info per emitter

	UPROPERTY(Transient)
	TArray<uint32>						m_CategoriesExpanded; // Per instance and effect

	UPROPERTY(Transient)
	float								m_ColumnWidth;
#endif // WITH_EDITORONLY_DATA

public:
	UPROPERTY(Category="PopcornFX Attributes", EditAnywhere, BlueprintReadOnly, EditFixedSize)
	TArray<uint8>						m_AttributesRawData;

	const class UPopcornFXEmitterComponent	*m_Owner = nullptr;

	void								CheckEmitter(const class UPopcornFXEmitterComponent *emitter);
};
