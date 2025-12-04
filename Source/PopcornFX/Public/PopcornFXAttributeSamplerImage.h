//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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

// this forward declaration is here to help the UE Header Parser which seems to crash because of FWD_PK_API_...
class	UPopcornFXAttributeSamplerImage;

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

/** Can override an Attribute Sampler **Image** by a **UTexture**. */
UCLASS(EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXAttributeSamplerImage : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler")
	void	SetTexture(class UTexture *InTexture);

	/** Enable to allow PopcornFX to convert the texture at Runtime, if
	* the texture is not in a format directly samplable by PopcornFX.
	* /!\ It can take a significant amount of time to convert.
	*/
	UPROPERTY(EditAnywhere, Category="PopcornFX AttributeSampler")
	uint32	bAllowTextureConvertionAtRuntime : 1;

	/** The texture to be sampled (Only UTexture2D are supported for CPU simulated particles, UTexture with 2D dimension for GPU simulated particles (UTexture2D, UTextureRenderTarget2D, ..)*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PopcornFX AttributeSampler")
	class UTexture					*Texture;

	/** Texture atlas */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PopcornFX AttributeSampler")
	class UPopcornFXTextureAtlas	*TextureAtlas;

	UPROPERTY(EditAnywhere, Category="PopcornFX AttributeSampler")
	TEnumAsByte<EPopcornFXImageSamplingMode::Type>	SamplingMode;

	UPROPERTY(EditAnywhere, Category="PopcornFX AttributeSampler")
	TEnumAsByte<EPopcornFXImageDensitySource::Type>	DensitySource;

	UPROPERTY(EditAnywhere, meta=(ClampMin=0.01f, ClampMax=100.0f), Category="PopcornFX AttributeSampler")
	float											DensityPower;

	// overrides
	void			OnUnregister() override;
	void			BeginDestroy() override;

#if WITH_EDITOR
	void			PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;

private:
	bool			RebuildImageSampler();
	bool			_RebuildImageSampler();
	bool			_BuildPDFs(PopcornFX::CImageSurface &dstSurface);
	bool			_BuildRegularImage(PopcornFX::CImageSurface &dstSurface, bool rebuild);

	FAttributeSamplerImageData	*m_Data;
};
