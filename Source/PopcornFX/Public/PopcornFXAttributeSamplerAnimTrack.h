//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "PopcornFXAttributeSampler.h"
#include "PopcornFXAttributeSamplerAnimTrack.generated.h"

struct	FAttributeSamplerAnimTrackData;

FWD_PK_API_BEGIN
class CCurveDescriptor;
FWD_PK_API_END

UENUM()
namespace	EPopcornFXSplineTransforms
{
	enum	Type
	{
		/** Use Spline Component's local transforms relative to it's parent actor */
		SplineComponentRelativeTr,
		/** Use Spline Component's world transforms */
		SplineComponentWorldTr,
		/** Use Attribute Sampler local transforms relative to it's parent actor */
		AttrSamplerRelativeTr,
		/** Use Attribute Sampler world transforms */
		AttrSamplerWorldTr,
	};
}

/** Can override an Attribute Sampler **AnimTrack** by a **USplineComponent**. */
UCLASS(EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXAttributeSamplerAnimTrack : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:
	/** Specifies which actors contains the target SplineComponent */
	UPROPERTY(Category="PopcornFX AttributeSampler", BlueprintReadWrite, EditAnywhere)
	class AActor			*TargetActor;

	/**
		Use this property to specify the target spline component name:
		- If TargetActor is specified, looks for a spline component that has this name in TargetActor
		- If TargetActor is specified, but no spline component has this name, fallbacks to TargetActor's RootComponent
		- If TargetActor isn't specified, looks for a spline component that has this name in this actor
		- If TargetActor isn't specified, but no spline component has this name, fallbacks to this actor's RootComponent
	*/
	UPROPERTY(Category="PopcornFX AttributeSampler", BlueprintReadWrite, EditAnywhere)
	FName					SplineComponentName;

	/** Enable translations */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	uint32					bTranslate : 1;

	/**
		Enable rotations
		Please note: If you need accurate orientations, disable "FastSampler".
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	uint32					bRotate : 1;

	/** Enable scale */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	uint32					bScale : 1;

	/**
		Enable this to use PopcornFX curve sampling for better performance. The drawback is a lack of accuracy when sampling orientations.
		If you stick to positions or scale sampling keep this option enabled.
		Disable this option if you need accurate sampling for orientations, keep in mind this will be less efficient.
		Don't hesitate to contact support for more informations.
		Note: Restart emitters referencing this sampler when this value gets changed
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	uint32					bFastSampler : 1;

#if WITH_EDITORONLY_DATA
	/** EDITOR ONLY: Enable this to rebuild the curve every frame. Useful to iterate quickly when building a spline component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PopcornFX AttributeSampler")
	uint32					bEditorRebuildEachFrame : 1;
#endif // WITH_EDITORONLY_DATA

	/** Determines what transforms will be used for this attribute sampler */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	TEnumAsByte<EPopcornFXSplineTransforms::Type>	Transforms;

private:
	void	BeginDestroy() override;

#if WITH_EDITOR
	void	PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent);
#endif // WITH_EDITOR


	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;
	virtual void									_AttribSampler_PreUpdate(float deltaTime);

	class USplineComponent							*ResolveSplineComponent(bool logErrors);
	bool											RebuildCurvesIFN();
	bool											RebuildCurve(PopcornFX::CCurveDescriptor *&curve, const FInterpCurvePoint<FVector> *srcPoints, const float *srcTimes, uint32 keyCount);

private:
	FAttributeSamplerAnimTrackData	*m_Data;

	FMatrix44f						m_TrackTransforms;
	FMatrix44f						m_TrackTransformsUnscaled;
};
