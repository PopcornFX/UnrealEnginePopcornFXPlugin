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

/** Can override an Attribute Sampler **Curve** by a **UCurve...**. */
UCLASS(EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXAttributeSamplerCurve : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:
	/** Curve dimension*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	TEnumAsByte<EAttributeSamplerCurveDimension::Type>	CurveDimension;

	/** Enables DoubleCurve sampling */
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

	/** Changes the Curve Dimension, will clear the current Curve if dimension changes */
	UFUNCTION(BlueprintCallable, Category="PopcornFX|AttributeSampler")
	void	SetCurveDimension(TEnumAsByte<EAttributeSamplerCurveDimension::Type> InCurveDimension);

	/** Set the UCurve to be sampled.
	* Must match the current dimension.
	* @return true if curve is up.
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX|AttributeSampler")
	bool	SetCurve(class UCurveBase *InCurve, bool InIsSecondCurve);

	// overrides
#if WITH_EDITOR
	void			PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR
	void			BeginDestroy() override;

	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;

private:
	bool			RebuildCurvesData();
	void			FetchCurveData(const FRichCurve *curve, PopcornFX::CCurveDescriptor *curveSampler, uint32 axis);
	bool			SetupCurve(PopcornFX::CCurveDescriptor *curveSampler, UCurveBase *curve);
	void			GetAssociatedCurves(UCurveBase *&curve0, UCurveBase *&curve1);

private:
	FAttributeSamplerCurveData	*m_Data;
};
