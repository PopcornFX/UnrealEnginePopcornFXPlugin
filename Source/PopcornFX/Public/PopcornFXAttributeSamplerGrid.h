//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "PopcornFXTypes.h"
#include "PopcornFXAttributeSampler.h"

#include "PopcornFXAttributeSamplerGrid.generated.h"

FWD_PK_API_BEGIN
class	CParticleSamplerDescriptor_Grid;
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

// this forward declaration is here to help the UE Header Parser which seems to crash because of FWD_PK_API_...
class	UPopcornFXAttributeSamplerGrid;

struct	FAttributeSamplerGridData;

UENUM(BlueprintType)
enum class	EPopcornFXGridDataType : uint8
{
	R,
	RG,
	RGBA
};

/** Can override an Attribute Sampler **Grid** by a **UTexture**. */
UCLASS(EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXAttributeSamplerGrid : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler")
	void	SetRenderTarget(class UTextureRenderTarget *InRenderTarget);

	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler", meta=(UnsafeDuringActorConstruction="true"))
	void	SetAsMaterialTextureParameter(UMaterialInstanceDynamic *Material, FName ParameterName);

	/** If true, this grid attribute sampler is setup from a 2D or Volume render target asset instead of being setup through SizeX, SizeY, SizeZ & DataType. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PopcornFX AttributeSampler")
	uint32							bAssetGrid : 1;

	/**
		If true, render target has its sRGB flag force enabled (or disabled if false).
		Enable this if writes into the grid in the source effect(s) are not linear.
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PopcornFX AttributeSampler")
	uint32							bSRGB : 1;

	/**
		2D or Volume Render target asset used by the grid attribute sampler.
		Note: this is only supported by GPU simulated particles (binding this sampler on an effect with CPU simulated particles will fallback to sampling the default resource defined in the effect).
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PopcornFX AttributeSampler")
	class UTextureRenderTarget		*RenderTarget;

	/** Grid Dimensions (Width) */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PopcornFX AttributeSampler", meta=(ClampMin="1", ClampMax="4096"))
	int32							SizeX;

	/** Grid Dimensions (Height) */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PopcornFX AttributeSampler", meta=(ClampMin="1", ClampMax="4096"))
	int32							SizeY;

	/** Grid Dimensions (Depth) */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PopcornFX AttributeSampler", meta=(ClampMin="1", ClampMax="4096"))
	int32							SizeZ;

	/** Grid Data Type */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PopcornFX AttributeSampler")
	EPopcornFXGridDataType			DataType;

	// overrides
	void				OnUnregister() override;
	void				BeginDestroy() override;

	class UTexture		*GridTexture();

#if WITH_EDITOR
	void					PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;

private:
	bool			RebuildGridSampler();
	bool			_RebuildGridSampler();

	UPROPERTY(Transient)
	UTexture					*m_GridTexture;

	FAttributeSamplerGridData	*m_Data;
};
