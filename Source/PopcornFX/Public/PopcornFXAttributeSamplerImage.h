//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "PopcornFXAttributeSampler.h"

#include "PopcornFXAttributeSamplerImage.generated.h"

FWD_PK_API_BEGIN
class	CParticleSamplerDescriptor_Image;
class	CImageSurface;
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

struct	FAttributeSamplerImageData;

UENUM()
namespace	EPopcornFXImageSamplingMode
{
	enum	Type
	{
		Regular,
		Density,
		Both
	};
}

UENUM()
namespace	EPopcornFXImageDensitySource
{
	enum	Type
	{
		Red,
		Green,
		Blue,
		Alpha,
		RGBA_Average
	};
}

USTRUCT(BlueprintType)
struct POPCORNFX_API FPopcornFXAttributeSamplerPropertiesImage : public FPopcornFXAttributeSamplerProperties
{
	GENERATED_USTRUCT_BODY()

public:
	//virtual void		CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) override;
	/** Checks if properties set by the user are valid. For example, a Curve attribute sampler needs a Curve asset to be valid. */
	virtual bool		ArePropertiesSupported(UPopcornFXEmitterComponent *emitter, const FString &samplerName) override;
	/** Checks if properties set by the user are compatible with the emitter using it. For example, if an effect uses a 2D grid and the user sets a 3D grid, it's not compatible */
	virtual bool		ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const FString &samplerName, const PopcornFX::CResourceDescriptor *defaultSampler) override;

public:
	/** Enable to allow PopcornFX to convert the texture at Runtime, if
	* the texture is not in a format directly samplable by PopcornFX.
	* /!\ It can take a significant amount of time to convert.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler")
	uint32											bAllowTextureConversionAtRuntime : 1;

	/** The texture to be sampled (Only UTexture2D are supported for CPU simulated particles, UTexture with 2D dimension for GPU simulated particles (UTexture2D, UTextureRenderTarget2D, ..)*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler")
	class UTexture									*Texture;

	/** Texture atlas */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler")
	class UPopcornFXTextureAtlas					*TextureAtlas;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler")
	TEnumAsByte<EPopcornFXImageSamplingMode::Type>	SamplingMode;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler")
	TEnumAsByte<EPopcornFXImageDensitySource::Type>	DensitySource;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 0.01f, ClampMax = 100.0f), Category = "PopcornFX AttributeSampler")
	float											DensityPower;

#if WITH_EDITOR
	virtual void									SetupDefaults(const PopcornFX::CParticleAttributeSamplerDeclaration *const decl, bool updateUnlockedValues) override;
#endif

public:

	FPopcornFXAttributeSamplerPropertiesImage()
	:	FPopcornFXAttributeSamplerProperties(EPopcornFXAttributeSamplerType::Image)
	,	bAllowTextureConversionAtRuntime(false)
	,	Texture()
	,	TextureAtlas()
	,	SamplingMode(EPopcornFXImageSamplingMode::Regular)
	,	DensitySource(EPopcornFXImageDensitySource::RGBA_Average)
	,	DensityPower(1.0f)
	{ }
};

/** Can override an Attribute Sampler **Image**. */
USTRUCT(meta = (BlueprintSpawnableComponent))
struct POPCORNFX_API FPopcornFXAttributeSamplerImage : public FPopcornFXAttributeSampler
{
	GENERATED_USTRUCT_BODY()
	
public:
	void	SetTexture(class UTexture *InTexture);

	FPopcornFXAttributeSamplerPropertiesImage		Properties;

	FPopcornFXAttributeSamplerImage();
	virtual ~FPopcornFXAttributeSamplerImage() {}

#if WITH_EDITOR
	void			PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent) override;
#endif // WITH_EDITOR

	// FPopcornFXAttributeSampler overrides
	void											BeginDestroy() override;
	const FPopcornFXAttributeSamplerProperties		*GetProperties() const override { return &Properties; }
#if WITH_EDITOR
	virtual void									CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) override;
	virtual void									RefreshFromProperties(const FPopcornFXAttributeSamplerProperties *properties) override;
#endif
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, const FPopcornFXAttributeSamplerProperties *properties, const PopcornFX::CResourceDescriptor *defaultSampler) override;

private:
	bool			RebuildImageSampler();
	bool			_RebuildImageSampler();
	bool			_BuildPDFs(PopcornFX::CImageSurface &dstSurface);
	bool			_BuildRegularImage(PopcornFX::CImageSurface &dstSurface, bool rebuild);

	FAttributeSamplerImageData	*m_Data;
};

UCLASS(meta = (BlueprintSpawnableComponent))
class POPCORNFX_API UPopcornFXAttributeSamplerImageAsset : public UPopcornFXAttributeSamplerAsset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FPopcornFXAttributeSamplerPropertiesImage	Properties;

public:
	virtual const FPopcornFXAttributeSamplerProperties	*GetProperties() const override { return &Properties; }
	virtual FPopcornFXAttributeSamplerProperties		*GetProperties() override { return &Properties; }

};