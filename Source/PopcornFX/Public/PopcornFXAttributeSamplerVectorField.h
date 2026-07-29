//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
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


USTRUCT(BlueprintType)
struct POPCORNFX_API FPopcornFXAttributeSamplerPropertiesVectorField : public FPopcornFXAttributeSamplerProperties
{
	GENERATED_USTRUCT_BODY()

public:
	//virtual void				CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) override;
	/** Checks if properties set by the user are valid. For example, a Curve attribute sampler needs a Curve asset to be valid. */
	virtual bool				ArePropertiesSupported(UPopcornFXEmitterComponent *emitter, const FString &samplerName) override;
	/** Checks if properties set by the user are compatible with the emitter using it. For example, if an effect uses a 2D grid and the user sets a 3D grid, it's not compatible */
	virtual bool				ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const FString &samplerName, const PopcornFX::CResourceDescriptor *defaultSampler) override;

	/** Vectorfield asset. */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	class UVectorFieldStatic	*VectorField;

	/** Additional intensity multiplier. */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	float						Intensity;

	/** Rotation animation (euler angles / seconds). */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	FVector						RotationAnimation;

	/** Vectorfield wrap mode. */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EPopcornFXVectorFieldWrapMode::Type>		WrapMode;

	/** Vectorfield sampling mode. */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EPopcornFXVectorFieldSamplingMode::Type>	SamplingMode;

	/** Vectorfield bounds type. */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EPopcornFXVectorFieldBounds::Type>			BoundsSource;

	/** Vectorfield volume dimensions. */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	FVector					VolumeDimensions;

	/** Relative Transforms will be used if activated.
	* Enable if sampled in SpawnerScript's Eval(), so vectorfield will be sampled locally to the Emitter.
	* Disable if sampled in SpawnerScript's **Post**Eval(), in an evolver script or used by a physics evolver, so the vectorfield will be sampled world space.
	*/
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	uint32					bUseRelativeTransform : 1;

#if 0
#if WITH_EDITORONLY_DATA
	/** Enable to draw individual vectorfield cells. */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	uint32					bDrawCells : 1;
#endif // WITH_EDITORONLY_DATA
#endif

#if WITH_EDITOR
	virtual void									SetupDefaults(const PopcornFX::CParticleAttributeSamplerDeclaration *const decl, bool updateUnlockedValues = false) override;
#endif

	FPopcornFXAttributeSamplerPropertiesVectorField()
	:	FPopcornFXAttributeSamplerProperties(EPopcornFXAttributeSamplerType::VectorField)
	,	VectorField()
	,	Intensity(1.f)
	,	RotationAnimation()
	,	WrapMode(EPopcornFXVectorFieldWrapMode::Wrap)
	,	SamplingMode(EPopcornFXVectorFieldSamplingMode::Trilinear)
	,	BoundsSource()
	,	VolumeDimensions(100.f, 100.f, 100.f)
	,	bUseRelativeTransform(false)
	{ }
};

/** Can override an Attribute Sampler **VectorField** by a **UVectorFieldStatic**. */
USTRUCT(meta=(BlueprintSpawnableComponent))
struct POPCORNFX_API FPopcornFXAttributeSamplerVectorField : public FPopcornFXAttributeSampler
{
	GENERATED_USTRUCT_BODY()

public:

	FPopcornFXAttributeSamplerPropertiesVectorField	Properties;

	FPopcornFXAttributeSamplerVectorField();

public:
	virtual void									BeginDestroy() override;
#if WITH_EDITOR
	void											PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

	const FPopcornFXAttributeSamplerProperties		*GetProperties() const override { return &Properties; }
#if WITH_EDITOR
	virtual void									CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) override;
	virtual void									RefreshFromProperties(const FPopcornFXAttributeSamplerProperties *properties) override;
#endif
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, const FPopcornFXAttributeSamplerProperties *properties, const PopcornFX::CResourceDescriptor *defaultSampler) override;
	virtual void									_AttribSampler_PreUpdate(UPopcornFXEmitterComponent *owner, float deltaTime) override;
	
	// PopcornFX Internal
	void											_BuildVectorFieldFlags(uint32 &flags, uint32 &interpolation) const;
	void											_SetBounds();

#if WITH_EDITOR
	virtual void									_AttribSampler_IndirectSelectedThisTick() override { m_IndirectSelectedThisTick = true; }
	void											RenderVectorFieldShape(UPopcornFXEmitterComponent *owner, const FMatrix &transforms, const FQuat &rotation, bool isSelected);
#endif
private:
	FAttributeSamplerVectorFieldData				*m_Data;

	float											m_TimeAccumulation;
#if WITH_EDITOR
	bool											m_IndirectSelectedThisTick;
#endif // WITH_EDITOR
};

UCLASS(meta = (BlueprintSpawnableComponent))
class POPCORNFX_API UPopcornFXAttributeSamplerVectorFieldAsset : public UPopcornFXAttributeSamplerAsset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FPopcornFXAttributeSamplerPropertiesVectorField	Properties;

public:
	virtual const FPopcornFXAttributeSamplerProperties	*GetProperties() const override { return &Properties; }
	virtual FPopcornFXAttributeSamplerProperties		*GetProperties() override { return &Properties; }

};