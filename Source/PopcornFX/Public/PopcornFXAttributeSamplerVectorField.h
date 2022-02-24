//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"
#include "PopcornFXAttributeSampler.h"
#include "PopcornFXAttributeSamplerVectorField.generated.h"

struct FAttributeSamplerVectorFieldData;

UENUM()
namespace	EPopcornFXVectorFieldWrapMode
{
	enum	Type
	{
		Wrap,
		Clamp
	};
}

UENUM()
namespace	EPopcornFXVectorFieldSamplingMode
{
	enum	Type
	{
		Point,
		Trilinear
	};
}

UENUM()
namespace	EPopcornFXVectorFieldBounds
{
	enum	Type
	{
		// Use the source vectorfield bounds
		Source,

		// Custom bounds
		Custom
	};
}

/** Can override an Attribute Sampler **Turbulence** by a **UVectorFieldStatic**. */
UCLASS(EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXAttributeSamplerVectorField : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:
	/** Vectorfield asset. */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere, BlueprintReadOnly)
	class UVectorFieldStatic	*VectorField;

	/** Additional intensity multiplier. */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere, BlueprintReadOnly)
	float						Intensity;

	/** Rotation animation (euler angles / seconds). */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere, BlueprintReadOnly)
	FVector						RotationAnimation;

	/** Vectorfield wrap mode. */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	TEnumAsByte<EPopcornFXVectorFieldWrapMode::Type>		WrapMode;

	/** Vectorfield sampling mode. */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	TEnumAsByte<EPopcornFXVectorFieldSamplingMode::Type>	SamplingMode;

	/** Vectorfield bounds type. */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	TEnumAsByte<EPopcornFXVectorFieldBounds::Type>			BoundsSource;

	/** Vectorfield volume dimensions. */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere, BlueprintReadOnly)
	FVector						VolumeDimensions;

	/** Relative Transforms will be used if activated.
	* Enable if sampled in SpawnerScript's Eval(), so vectorfield will be sampled locally to the Emitter.
	* Disable if sampled in SpawnerScript's **Post**Eval(), in an evolver script or used by a physics evolver, so the vectorfield will be sampled world space.
	*/
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	uint32					bUseRelativeTransform : 1;

#if 0
#if WITH_EDITORONLY_DATA
	/** Enable to draw individual vectorfield cells. */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	uint32					bDrawCells : 1;
#endif // WITH_EDITORONLY_DATA
#endif

public:
	virtual void									BeginDestroy() override;
#if WITH_EDITOR
	void											PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

private:
	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;
	virtual void									_AttribSampler_PreUpdate(CParticleScene *scene, float deltaTime, enum ELevelTick tickType) override;
	void											_BuildVectorFieldFlags(uint32 &flags, uint32 &interpolation) const;
	void											_SetBounds();

#if WITH_EDITOR
	virtual void									_AttribSampler_IndirectSelectedThisTick() override { m_IndirectSelectedThisTick = true; }
	void											RenderVectorFieldShape(const FMatrix &transforms, const FQuat &rotation, bool isSelected);
#endif
private:
	FAttributeSamplerVectorFieldData				*m_Data;

	float											m_TimeAccumulation;
#if WITH_EDITOR
	bool											m_IndirectSelectedThisTick;
#endif // WITH_EDITOR
};

