//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXAttributeSampler.h"
#include "PopcornFXAttributeSamplerImage.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXAttributeSamplerGrid.h"
#include "PopcornFXAttributeSamplerText.h"
#include "PopcornFXAttributeSamplerVectorField.h"
#include "PopcornFXAttributeSamplerAnimTrack.h"
#include "PopcornFXAttributeSamplerShape.h"

#include "PopcornFXAttributeList.generated.h"

DECLARE_MULTICAST_DELEGATE(FPopcornFXAttributeEventSignature);

class	AActor;

class	UPopcornFXEffect;
class	UPopcornFXEmitterComponent;

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

	UPROPERTY(VisibleAnywhere)
	FString					m_AttributeName;

	UPROPERTY(VisibleAnywhere)
	FString					m_AttributeCategoryName;

	UPROPERTY(VisibleAnywhere)
	uint32					m_AttributeType;

	UPROPERTY(VisibleAnywhere)
	bool					m_IsPrivate;
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere)
	TEnumAsByte<EPopcornFXAttributeSemantic::Type>	m_AttributeSemantic;

	UPROPERTY(VisibleAnywhere)
	FVector					m_AttributeEulerAngles;

	UPROPERTY(VisibleAnywhere)
	TEnumAsByte<EPopcornFXAttributeDropDownMode::Type>	m_DropDownMode;

	UPROPERTY(VisibleAnywhere)
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
	uint32					AttributeBaseTypeID() const;

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
		return Valid() && m_AttributeName == other.m_AttributeName &&
			m_AttributeType == other.m_AttributeType &&
			m_AttributeCategoryName == other.m_AttributeCategoryName &&
#if WITH_EDITORONLY_DATA
			m_EnumList == other.m_EnumList &&
#endif // WITH_EDITORONLY_DATA
			m_IsPrivate == other.m_IsPrivate;
		}
};

/** Basic description of a PopcornFX attribute sampler + Holds the actual sampling data */
USTRUCT()
struct FPopcornFXSamplerDesc
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(Category = "PopcornFX AttributeSampler", VisibleAnywhere)
	FString														m_SamplerName;

	UPROPERTY(Category = "PopcornFX AttributeSampler", VisibleAnywhere)
	FString														m_AttributeCategoryName;

	UPROPERTY(Category = "PopcornFX AttributeSampler", VisibleAnywhere)
	TEnumAsByte<EPopcornFXAttributeSamplerType::Type>			m_SamplerType;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	UPopcornFXAttributeSamplerAsset								*m_SamplerAsset = nullptr;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerPropertiesImage>		m_ImageProperties;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerPropertiesCurve>		m_CurveProperties;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerPropertiesAnimTrack>	m_AnimTrackProperties;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerPropertiesGrid>			m_GridProperties;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerPropertiesShape>		m_ShapeProperties;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerPropertiesText>			m_TextProperties;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerPropertiesVectorField>	m_VectorFieldProperties;

	/** Use an external sampler for this attribute */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	bool														m_UseSamplerAsset;

	UPROPERTY(VisibleAnywhere)
	bool														m_IsPrivate;

	FPopcornFXSamplerDesc()
	:	m_SamplerName()
	,	m_AttributeCategoryName()
	,	m_SamplerType(EPopcornFXAttributeSamplerType::None)
	,	m_UseSamplerAsset(false)
	,	m_IsPrivate(false)
	{ }

	FName		SamplerFName() const { return FName(m_SamplerName); }
	EPopcornFXAttributeSamplerType::Type	SamplerType() const { return m_SamplerType; }

	FPopcornFXAttributeSamplerProperties		*ResolveAttributeProperties();
	const FPopcornFXAttributeSamplerProperties	*ResolveAttributeProperties() const;
	void										SetProperties(const FPopcornFXAttributeSamplerProperties *newProperties);

	void		Reset()
	{
		m_SamplerName = FString();
		m_AttributeCategoryName = FString();
		m_SamplerType = EPopcornFXAttributeSamplerType::None;
	}

	/** Copy description values (ignore the samplers) */
	void		CopyValuesFrom(FPopcornFXSamplerDesc &other)
	{
		m_SamplerName = other.m_SamplerName;
		m_SamplerType = other.m_SamplerType;
		m_UseSamplerAsset = other.m_UseSamplerAsset;
		m_SamplerAsset = other.m_SamplerAsset;
		m_AttributeCategoryName = other.m_AttributeCategoryName;
		m_IsPrivate = other.m_IsPrivate;
	}

	/** Swap description values (ignore the samplers) */
	void		SwapValuesWith(FPopcornFXSamplerDesc &other)
	{
		Swap(m_SamplerName, other.m_SamplerName);
		Swap(m_AttributeCategoryName, other.m_AttributeCategoryName);
		Swap(m_SamplerType, other.m_SamplerType);
		Swap(m_UseSamplerAsset, other.m_UseSamplerAsset);
		Swap(m_SamplerAsset, other.m_SamplerAsset);
		Swap(m_IsPrivate, other.m_IsPrivate);
	}

	/** Reset description values (ignore the samplers) */
	void		ResetValue()
	{
		m_SamplerName = FString();
		m_SamplerType = EPopcornFXAttributeSamplerType::None;
		m_AttributeCategoryName = FString();
		m_UseSamplerAsset = false;
		m_SamplerAsset = nullptr;
		m_IsPrivate = false;
	}

	bool		ExactMatch(const FPopcornFXSamplerDesc &other) const
	{
		return m_SamplerName == other.m_SamplerName &&
			m_SamplerType == other.m_SamplerType &&
			m_AttributeCategoryName == other.m_AttributeCategoryName &&
			m_IsPrivate == other.m_IsPrivate;
	}
};

USTRUCT()
struct FPopcornFXSampler
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerImage>			m_SamplerImage;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerCurve>			m_SamplerCurve;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerAnimTrack>		m_SamplerAnimTrack;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerGrid>			m_SamplerGrid;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerShape>			m_SamplerShape;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerText>			m_SamplerText;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TOptional<FPopcornFXAttributeSamplerVectorField>	m_SamplerVectorField;

	FPopcornFXSampler()
	:	m_SamplerImage()
	,	m_SamplerCurve()
	,	m_SamplerAnimTrack()
	,	m_SamplerGrid()
	,	m_SamplerShape()
	,	m_SamplerText()
	,	m_SamplerVectorField()
	{ }
};

// Dummy
struct	FPopcornFXAttributeValue
{
	uint32	m_Value[4];
};

/*
* Class representing PopcornFX attributes
*/
USTRUCT()
struct FPopcornFXAttributeList
{
	GENERATED_USTRUCT_BODY()

public:
	FPopcornFXAttributeList();
	~FPopcornFXAttributeList();

	bool			CheckDataIntegrity() const;
	bool			Valid() const;
	bool			IsUpToDate(UPopcornFXEffect *effect) const;

	bool			IsEmpty() const;
	void			Clean();

	void			SetupDefault(UPopcornFXEffect *sourceEffect, bool force = false);
	bool			Prepare(UPopcornFXEffect *effect, bool force = false);
	bool			PrepareAttributes(TArray<FPopcornFXAttributeDesc> *attrs, const TArray<FPopcornFXAttributeDesc> *refAttrs, TArray<uint8> *rawData, const TArray<uint8> *refRawData);
	bool			PrepareSamplers(TArray<FPopcornFXSamplerDesc> *samplerDescs, const TArray<FPopcornFXSamplerDesc> *refSamplerDescs, TArray<FPopcornFXSampler> *samplers, const TArray<FPopcornFXSampler> *refSamplers);
	void			CopyFrom(const FPopcornFXAttributeList *other);

	FPopcornFXAttributeList		*GetDefaultAttributeList(UPopcornFXEffect *effect) const; // can be self

	void			ResetAllToDefaultValues(UPopcornFXEmitterComponent *emitter, UPopcornFXEffect *effect);
	void			ResetAttributesToDefaultValues(UPopcornFXEmitterComponent *emitter, UPopcornFXEffect *effect);

	void			RefreshAttributeSamplers(UPopcornFXEmitterComponent *emitter, bool reload = false) { if (m_SamplerDescs.Num() > 0 && m_Samplers.Num() > 0) _RefreshAttributeSamplers(emitter, reload); }
	void			RefreshAttributes(const UPopcornFXEmitterComponent *emitter) { if (m_AttributeDescs.Num() > 0) _RefreshAttributes(emitter); }

	void			Scene_PreUpdate(UPopcornFXEmitterComponent *emitter, float deltaTime);
#if WITH_EDITOR
	void			AttributeSamplers_IndirectSelectedThisTick(UPopcornFXEmitterComponent *emitter);

	// Gets & resets restart state
	bool			GetRestartEmitter() { const bool restartEmitter = m_RestartEmitter; m_RestartEmitter = false; return restartEmitter; }

	uint32			GetCategoryCount() const { return m_Categories.Num(); }
	FString			GetCategoryName(uint32 categoryId) const { return m_Categories[categoryId]; }

#endif

	uint32			AttributeDescCount() const { return m_AttributeDescs.Num(); }
	int32			FindAttributeIndex(const FString &name) const;

	uint32			SamplerDescCount() const { return m_SamplerDescs.Num(); }
	uint32			SamplerCount() const { return m_Samplers.Num(); }
	int32			FindSamplerIndex(const FString &name) const;

	const FPopcornFXAttributeDesc						*GetAttributeDesc(uint32 attributeId) const;
	FPopcornFXSamplerDesc								*GetSamplerDesc(uint32 samplerId);
	FPopcornFXAttributeSampler							*ResolveAttributeSampler(int32 samplerId);

	const void											*GetAttributeDeclaration(UPopcornFXEffect *effect, uint32 attributeId) const;
#if WITH_EDITOR
	const void											*GetParticleSampler(UPopcornFXEffect *effect, uint32 samplerId) const;
#endif
	void												GetAttribute(uint32 attributeId, FPopcornFXAttributeValue &outValue) const;
	void												SetAttribute(uint32 attributeId, const FPopcornFXAttributeValue &value, bool fromUI = false);

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

	void			RestoreAttributesFromCachedRawData(const TArray<uint8> &rawData);

private:
	void			_RefreshAttributes(const UPopcornFXEmitterComponent *emitter);
	// Emitter not const because we update its clickable sprite if any sampler setup is invalid
	void			_RefreshAttributeSamplers(UPopcornFXEmitterComponent *emitter, bool reload);

public:
	UPROPERTY(Category="PopcornFX Attributes", VisibleAnywhere)
	UPopcornFXEffect					*m_Effect;

	UPROPERTY(Category="PopcornFX Attributes", VisibleAnywhere)
	uint32								m_FileVersionId;

	/** Attribute descriptions */
	UPROPERTY(Category="PopcornFX Attributes", EditAnywhere)
	TArray<FPopcornFXAttributeDesc>		m_AttributeDescs;

	/** Attribute sampler descriptions */
	UPROPERTY(Category="PopcornFX Attributes", EditAnywhere)
	TArray<FPopcornFXSamplerDesc>		m_SamplerDescs;

	/** Actual samplers */
	UPROPERTY(Category = "PopcornFX Attributes", EditAnywhere)
	TArray<FPopcornFXSampler>			m_Samplers;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TArray<FString>						m_Categories; // Per effect, we don't need that info per emitter

	UPROPERTY()
	bool								m_RestartEmitter = false; // UPopcornFXSettingsEditor::bRestartEmitterWhenAttributesChanged

	UPROPERTY()
	bool								m_HasPendingOneShotReset = false;
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(Category = "PopcornFX Attributes", VisibleAnywhere)
	TArray<uint8>						m_AttributesRawData;

	// Must not be an UPROPERTY() to not get serialized!
	TWeakObjectPtr<const UPopcornFXEmitterComponent>	m_Owner;

	/** Event broadcasted when attribute samplers are refreshed */
	FPopcornFXAttributeEventSignature					OnSamplersRefreshed;

	void								CheckEmitter(const UPopcornFXEmitterComponent *emitter);
};
