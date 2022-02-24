//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "PopcornFXAttributeSampler.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXAttributeSamplerCurveDynamic.generated.h"

FWD_PK_API_BEGIN
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

// this forward declaration is here to help the UE Header Parser which seems to crash because of FWD_PK_API_...
class	FPopcornFXPlugin;

struct	FAttributeSamplerCurveDynamicData;

UENUM()
namespace	ECurveDynamicInterpolator
{
	enum	Type
	{
		Linear,
		Spline
	};
}

/** Can override an Attribute Sampler **Curve** by a **TArray of Values**. */
UCLASS(EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXAttributeSamplerCurveDynamic : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:
	/** Curve dimension */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PopcornFX AttributeSampler")
	TEnumAsByte<EAttributeSamplerCurveDimension::Type>	CurveDimension;

	/** Determines the curve interpolator type */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PopcornFX AttributeSampler")
	TEnumAsByte<ECurveDynamicInterpolator::Type>	CurveInterpolator;

	/**
	*	Sets the times from a float array. Times must be in the 0-1 range.
	*	If this isn't called, times will be automatically generated from the number of values in the range 0-1 (fixed steps).
	*/
	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler")
	bool	SetTimes(const TArray<float> &Times);

	/** Sets the values from a float array. */
	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler")
	bool	SetValues1D(const TArray<float> &Values);

	/** Sets the values from a vector array. */
	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler")
	bool	SetValues3D(const TArray<FVector> &Values);

	/** Sets the values from a color array. */
	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler")
	bool	SetValues4D(const TArray<FLinearColor> &Values);

	/** Sets the tangents from a float array. */
	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler")
	bool	SetTangents1D(const TArray<float> &ArriveTangents, const TArray<float> &LeaveTangents);

	/** Sets the tangents from a vector array. */
	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler")
	bool	SetTangents3D(const TArray<FVector> &ArriveTangents, const TArray<FVector> &LeaveTangents);

	/** Sets the tangents from a color array. */
	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler")
	bool	SetTangents4D(const TArray<FLinearColor> &ArriveTangents, const TArray<FLinearColor> &LeaveTangents);

private:
	void	BeginDestroy() override;
	bool	CreateCurveIFN();

	template <class _Type>
	bool	SetValuesGeneric(const TArray<_Type> &values);

	template <class _Type>
	bool	SetTangentsGeneric(const TArray<_Type> &arriveTangents, const TArray<_Type> &leaveTangents);

	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;
	virtual void									_AttribSampler_PreUpdate(CParticleScene *scene, float deltaTime, enum ELevelTick tickType) override;

private:
	FAttributeSamplerCurveDynamicData	*m_Data;
};
