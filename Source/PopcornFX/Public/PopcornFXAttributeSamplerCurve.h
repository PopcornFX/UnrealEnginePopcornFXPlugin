//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "PopcornFXAttributeSampler.h"

#include "PopcornFXAttributeSamplerCurve.generated.h"

FWD_PK_API_BEGIN
class CCurveDescriptor;
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

struct	FAttributeSamplerCurveData;
struct	FRichCurve;
class	UCurveFloat;
class	UCurveVector;
class	UCurveLinearColor;

UENUM()
namespace	EAttributeSamplerCurveDimension
{
	enum	Type
	{
		Float1 = 1,
		Float2 UMETA(Hidden, DisplayName = "Float2 - Unsupported"),
		Float3,
		Float4
	};
}

USTRUCT(BlueprintType)
struct POPCORNFX_API FPopcornFXAttributeSamplerPropertiesCurve : public FPopcornFXAttributeSamplerProperties
{
	GENERATED_USTRUCT_BODY()

public:
	//virtual void		CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) override;
	/** Checks if properties set by the user are valid. For example, a Curve attribute sampler needs a Curve asset to be valid. */
	virtual bool		ArePropertiesSupported(UPopcornFXEmitterComponent *emitter, const FString &samplerName) override;
	/** Checks if properties set by the user are compatible with the emitter using it. For example, if an effect uses a 2D grid and the user sets a 3D grid, it's not compatible */
	virtual bool		ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const FString &samplerName, const PopcornFX::CResourceDescriptor *defaultSampler) override;

public:
	/** Curve dimension*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PopcornFX AttributeSampler")
	TEnumAsByte<EAttributeSamplerCurveDimension::Type>	CurveDimension;

	/** Enables DoubleCurve sampling. Legacy feature from v1 that does not exist anymore */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PopcornFX AttributeSampler")
	uint32				bIsDoubleCurve : 1;

	/** The 1 Dimension UCurve to be sampled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PopcornFX AttributeSampler")
	UCurveFloat			*Curve1D;

	/* Second 1 Dimension UCurve when IsDoubleCurve */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	UCurveFloat			*SecondCurve1D;

	/** The 3 Dimensions UCurve to be sampled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PopcornFX AttributeSampler")
	UCurveVector		*Curve3D;

	/** Second 3 Dimensions UCurve when IsDoubleCurve */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	UCurveVector		*SecondCurve3D;

	/** The 4 Dimensions UCurve to be sampled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PopcornFX AttributeSampler")
	UCurveLinearColor	*Curve4D;

	/** Second 4 Dimensions UCurve when IsDoubleCurve */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	UCurveLinearColor	*SecondCurve4D;

#if WITH_EDITOR
	virtual void		SetupDefaults(const PopcornFX::CParticleAttributeSamplerDeclaration *const decl, bool updateUnlockedValues = false) override;
#endif

	FPopcornFXAttributeSamplerPropertiesCurve()
	:	FPopcornFXAttributeSamplerProperties(EPopcornFXAttributeSamplerType::Curve)
	,	CurveDimension(EAttributeSamplerCurveDimension::Float1)
	,	bIsDoubleCurve(false)
	,	Curve1D()
	,	SecondCurve1D()
	,	Curve3D()
	,	SecondCurve3D()
	,	Curve4D()
	,	SecondCurve4D()
	{ }
};

/** Can override an Attribute Sampler **Curve** by a **UCurve...**. */
USTRUCT(BlueprintType)
struct POPCORNFX_API FPopcornFXAttributeSamplerCurve : public FPopcornFXAttributeSampler
{
	GENERATED_USTRUCT_BODY()

public:
	/** Changes the Curve Dimension, will clear the current Curve if dimension changes */
	void	SetCurveDimension(TEnumAsByte<EAttributeSamplerCurveDimension::Type> InCurveDimension);

	/** Set the UCurve to be sampled.
	* Must match the current dimension.
	* @return true if curve is up.
	*/
	bool	SetCurve(class UCurveBase *InCurve, bool InIsSecondCurve);

	FPopcornFXAttributeSamplerPropertiesCurve	Properties;

	FPopcornFXAttributeSamplerCurve();

#if WITH_EDITOR
	void			PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent) override;
#endif // WITH_EDITOR

	// FPopcornFXAttributeSampler overrides
	virtual void									BeginDestroy() override;
	const FPopcornFXAttributeSamplerProperties		*GetProperties() const override { return &Properties; }
#if WITH_EDITOR
	virtual void									CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) override;
	virtual void									RefreshFromProperties(const FPopcornFXAttributeSamplerProperties *properties) override;
#endif
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, const FPopcornFXAttributeSamplerProperties *properties, const PopcornFX::CResourceDescriptor *defaultSampler) override;

private:
	bool			RebuildCurvesData();
	void			FetchCurveData(const FRichCurve *curve, PopcornFX::CCurveDescriptor *curveSampler, uint32 axis);
	bool			SetupCurve(PopcornFX::CCurveDescriptor *curveSampler, UCurveBase *curve);
	void			GetAssociatedCurves(UCurveBase *&curve0, UCurveBase *&curve1);

private:
	FAttributeSamplerCurveData	*m_Data;
};

UCLASS(meta = (BlueprintSpawnableComponent))
class POPCORNFX_API UPopcornFXAttributeSamplerCurveAsset : public UPopcornFXAttributeSamplerAsset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FPopcornFXAttributeSamplerPropertiesCurve	Properties;

public:
	virtual const FPopcornFXAttributeSamplerProperties	*GetProperties() const override { return &Properties; }
	virtual FPopcornFXAttributeSamplerProperties		*GetProperties() override { return &Properties; }

};