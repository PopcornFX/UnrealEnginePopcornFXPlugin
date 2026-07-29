//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
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
		/** Use Spline Component's local transform relative to its parent actor */
		SplineComponentRelativeTr,
		/** Use Spline Component's world transform */
		SplineComponentWorldTr,
		/** Use Attribute Sampler local transform relative to its parent actor */
		AttrSamplerRelativeTr,
		/** Use Attribute Sampler world transform */
		AttrSamplerWorldTr,
	};
}

USTRUCT(BlueprintType)
struct POPCORNFX_API FPopcornFXAttributeSamplerPropertiesAnimTrack : public FPopcornFXAttributeSamplerProperties
{
	GENERATED_USTRUCT_BODY()

public:
	//virtual void			CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) override;
	/** Checks if properties set by the user are valid. For example, a Curve attribute sampler needs a Curve asset to be valid. */
	virtual bool			ArePropertiesSupported(UPopcornFXEmitterComponent *emitter, const FString &samplerName) override;
	/** Checks if properties set by the user are compatible with the emitter using it. For example, if an effect uses a 2D grid and the user sets a 3D grid, it's not compatible */
	virtual bool			ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const FString &samplerName, const PopcornFX::CResourceDescriptor *defaultSampler) override;

	class USplineComponent	*ResolveSplineComponent(UPopcornFXEmitterComponent *owner, const FString &samplerName, bool logErrors);

public:
	/** Specifies which actors contains the target SplineComponent */
	UPROPERTY(Category = "PopcornFX AttributeSampler", BlueprintReadWrite, EditAnywhere)
	class AActor									*TargetActor;

	/**
		Use this property to specify the target spline component name:
		- If TargetActor is specified, looks for a spline component that has this name in TargetActor
		- If TargetActor is specified, but no spline component has this name, fallbacks to TargetActor's RootComponent
		- If TargetActor isn't specified, looks for a spline component that has this name in this actor
		- If TargetActor isn't specified, but no spline component has this name, fallbacks to this actor's RootComponent
	*/
	UPROPERTY(Category = "PopcornFX AttributeSampler", BlueprintReadWrite, EditAnywhere)
	FName											SplineComponentName;

	/** Enable translations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PopcornFX AttributeSampler")
	uint32											bTranslate : 1;

	/**
		Enable rotations
		Please note: If you need accurate orientations, disable "FastSampler".
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PopcornFX AttributeSampler")
	uint32											bRotate : 1;

	/** Enable scale */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PopcornFX AttributeSampler")
	uint32											bScale : 1;

	/**
		Enable this to use PopcornFX curve sampling for better performance. The drawback is a lack of accuracy when sampling orientations.
		If you stick to positions or scale sampling keep this option enabled.
		Disable this option if you need accurate sampling for orientations, keep in mind this will be less efficient.
		Don't hesitate to contact support for more informations.
		Note: Restart emitters referencing this sampler when this value gets changed
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PopcornFX AttributeSampler")
	uint32											bFastSampler : 1;

	/** EDITOR ONLY: Enable this to rebuild the curve every frame. Useful to iterate quickly when building a spline component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PopcornFX AttributeSampler")
	uint32											bEditorRebuildEachFrame : 1;

	/** Determines what transforms will be used for this attribute sampler */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EPopcornFXSplineTransforms::Type>	Transforms;

#if WITH_EDITOR
	virtual void									SetupDefaults(const PopcornFX::CParticleAttributeSamplerDeclaration *const decl, bool updateUnlockedValues = false) override;
#endif

	FPopcornFXAttributeSamplerPropertiesAnimTrack()
	:	FPopcornFXAttributeSamplerProperties(EPopcornFXAttributeSamplerType::AnimTrack)
	,	TargetActor()
	,	bTranslate(true)
	,	bRotate(false)
	,	bScale(false)
	,	bFastSampler(true)
	,	bEditorRebuildEachFrame(false)
	,	Transforms(EPopcornFXSplineTransforms::AttrSamplerRelativeTr)
	{ }
};
/** Can override an Attribute Sampler **AnimTrack** by a **USplineComponent**. */
USTRUCT(meta=(BlueprintSpawnableComponent))
struct POPCORNFX_API FPopcornFXAttributeSamplerAnimTrack : public FPopcornFXAttributeSampler
{
	GENERATED_USTRUCT_BODY()

public:

	FPopcornFXAttributeSamplerPropertiesAnimTrack	Properties;

	FPopcornFXAttributeSamplerAnimTrack();

	void	BeginDestroy() override;

#if WITH_EDITOR
	void	PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent);
#endif // WITH_EDITOR

	// FPopcornFXAttributeSampler overrides
	const FPopcornFXAttributeSamplerProperties		*GetProperties() const override { return &Properties; }
#if WITH_EDITOR
	virtual void									CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) override;
	virtual void									RefreshFromProperties(const FPopcornFXAttributeSamplerProperties *properties) override;
#endif
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, const FPopcornFXAttributeSamplerProperties *properties, const PopcornFX::CResourceDescriptor *defaultSampler) override;
	virtual void									_AttribSampler_PreUpdate(UPopcornFXEmitterComponent *owner, float deltaTime) override;

	bool											RebuildCurvesIFN(UPopcornFXEmitterComponent *owner);

private:
	FAttributeSamplerAnimTrackData	*m_Data;

	FMatrix44f						m_TrackTransforms;
	FMatrix44f						m_TrackTransformsUnscaled;
};

UCLASS(meta = (BlueprintSpawnableComponent))
class POPCORNFX_API UPopcornFXAttributeSamplerAnimTrackAsset : public UPopcornFXAttributeSamplerAsset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FPopcornFXAttributeSamplerPropertiesAnimTrack	Properties;

public:
	virtual const FPopcornFXAttributeSamplerProperties	*GetProperties() const override { return &Properties; }
	virtual FPopcornFXAttributeSamplerProperties		*GetProperties() override { return &Properties; }

};