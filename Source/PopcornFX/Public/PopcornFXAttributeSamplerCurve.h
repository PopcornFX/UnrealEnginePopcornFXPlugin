//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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
		Float2,
		Float3,
		Float4
	};
}

USTRUCT(BlueprintType)
struct POPCORNFX_API FPopcornFXAttributeSamplerPropertiesCurve : public FPopcornFXAttributeSamplerProperties
{
	GENERATED_USTRUCT_BODY()

public:
	/** Curve dimension*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	TEnumAsByte<EAttributeSamplerCurveDimension::Type>	CurveDimension;

	/** Enables DoubleCurve sampling. Legacy feature from v1 that does not exist anymore */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PopcornFX AttributeSampler")
	uint32				bIsDoubleCurve : 1;

	/** The 1 Dimension UCurve to be sampled */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	UCurveFloat			*Curve1D;

	/* Second 1 Dimension UCurve when IsDoubleCurve */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	UCurveFloat			*SecondCurve1D;

	/** The 3 Dimensions UCurve to be sampled */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	UCurveVector		*Curve3D;

	/** Second 3 Dimensions UCurve when IsDoubleCurve */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	UCurveVector		*SecondCurve3D;

	/** The 4 Dimensions UCurve to be sampled */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	UCurveLinearColor	*Curve4D;

	/** Second 4 Dimensions UCurve when IsDoubleCurve */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	UCurveLinearColor	*SecondCurve4D;

	FPopcornFXAttributeSamplerPropertiesCurve()
	:	CurveDimension(EAttributeSamplerCurveDimension::Float1)
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
UCLASS(EditInlineNew, meta = (BlueprintSpawnableComponent), ClassGroup = PopcornFX)
	class POPCORNFX_API UPopcornFXAttributeSamplerCurve : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:
	/** Changes the Curve Dimension, will clear the current Curve if dimension changes */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|AttributeSampler")
	void	SetCurveDimension(TEnumAsByte<EAttributeSamplerCurveDimension::Type> InCurveDimension);

	/** Set the UCurve to be sampled.
	* Must match the current dimension.
	* @return true if curve is up.
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|AttributeSampler")
	bool	SetCurve(class UCurveBase *InCurve, bool InIsSecondCurve);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="PopcornFX AttributeSampler")
	FPopcornFXAttributeSamplerPropertiesCurve	Properties;

	// overrides
#if WITH_EDITOR
	void			PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR
	void			BeginDestroy() override;

	const FPopcornFXAttributeSamplerProperties		*GetProperties() const override { return &Properties; }
#if WITH_EDITOR
	virtual void									CopyPropertiesFrom(const UPopcornFXAttributeSampler *other) override;
#endif

	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;

private:
	bool			RebuildCurvesData();
	void			FetchCurveData(const FRichCurve *curve, PopcornFX::CCurveDescriptor *curveSampler, uint32 axis);
	bool			SetupCurve(PopcornFX::CCurveDescriptor *curveSampler, UCurveBase *curve);
	void			GetAssociatedCurves(UCurveBase *&curve0, UCurveBase *&curve1);

private:
	FAttributeSamplerCurveData	*m_Data;
};
