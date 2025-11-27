//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "PopcornFXTypes.h"
#include "PopcornFXAttributeSampler.h"

#include "PixelFormat.h"

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
namespace	EPopcornFXGridDataType
{
	enum	Type : uint8
	{
		Float,
		Float2,
		Float3 UMETA(Hidden, DisplayName = "Float3 - Unsupported"), // Hidden + DisplayName allows to hide when selecting but still show when set from code
		Float4,
		Int,
		Int2,
		Int3  UMETA(Hidden, DisplayName = "Int3 - Unsupported"), // Hidden + DisplayName allows to hide when selecting but still show when set from code
		Int4,
	};
}

UENUM(BlueprintType)
enum	EPopcornFXGridOrder
{
	OneD UMETA(DisplayName = "1D"),
	TwoD UMETA(DisplayName = "2D"),
	ThreeD UMETA(DisplayName = "3D"),
	FourD UMETA(Hidden, DisplayName = "4D - Unsupported"), // Hidden + DisplayName allows to hide when selecting but still show when set from code
};

USTRUCT(BlueprintType)
struct POPCORNFX_API FPopcornFXAttributeSamplerPropertiesGrid : public FPopcornFXAttributeSamplerProperties
{
	GENERATED_USTRUCT_BODY()

public:

	/**
		If true, this grid attribute sampler is setup from a 2D or Volume render target asset instead of being setup manually.
		Note: this is only supported by GPU simulated particles (binding this sampler on an effect with CPU simulated particles will fallback to sampling the default resource defined in the effect).
		It can still be used with CPU simulated particles to use the render target's dimensions
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PopcornFX AttributeSampler", DisplayName = "Asset grid")
	uint32								bAssetGrid : 1;

	/**
		If true, render target has its sRGB flag force enabled (or disabled if false).
		Enable this if writes into the grid in the source effect(s) are not linear.
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PopcornFX AttributeSampler")
	uint32								bSRGB : 1;

	/**
		2D or Volume Render target asset used by the grid attribute sampler.
		Note: this is only supported by GPU simulated particles (binding this sampler on an effect with CPU simulated particles will fallback to sampling the default resource defined in the effect).
		It can still be used with CPU simulated particles to use the render target's dimensions
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PopcornFX AttributeSampler", DisplayName = "Render target")
	class UTextureRenderTarget			*RenderTarget;

	/** Grid order (dimensions) */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PopcornFX AttributeSampler")
	TEnumAsByte<EPopcornFXGridOrder>	Order;

	/** Grid width dimension */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PopcornFX AttributeSampler", meta = (ClampMin = "1", ClampMax = "4096"))
	int32								SizeX;

	/** Grid height dimension */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PopcornFX AttributeSampler", meta = (ClampMin = "1", ClampMax = "4096"))
	int32								SizeY;

	/** Grid depth dimension */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PopcornFX AttributeSampler", meta = (ClampMin = "1", ClampMax = "4096"))
	int32								SizeZ;

	/** Grid data type */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PopcornFX AttributeSampler", DisplayName = "Data type")
	TEnumAsByte<EPopcornFXGridDataType::Type>	DataType;

	FPopcornFXAttributeSamplerPropertiesGrid()
	:	bAssetGrid(false)
	,	bSRGB(false)
	,	RenderTarget()
	,	Order(EPopcornFXGridOrder::ThreeD)
	,	SizeX(128)
	,	SizeY(128)
	,	SizeZ(1)
	,	DataType(EPopcornFXGridDataType::Float4)
	{ }
};

/** Can override an Attribute Sampler **Grid** by a **UTexture**. */
UCLASS(EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXAttributeSamplerGrid : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler")
	void		SetRenderTarget(class UTextureRenderTarget *InRenderTarget);

	/** Sets the D3D12 GPU sim grid as a parameter of a material */
	UFUNCTION(BlueprintCallable, Category="PopcornFX AttributeSampler", meta=(UnsafeDuringActorConstruction="true"))
	void		SetAsMaterialTextureParameter(UMaterialInstanceDynamic *Material, FName ParameterName);

	/** Returns the number of cells in the grid */
	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler")
	int32		GetCellCount() const;

	/** Returns the dimensions of the grid */
	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler")
	FIntVector	GetDimensions() const;

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridFloatValues(UPopcornFXAttributeSamplerGrid *InGrid, TArray<float> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridFloat2Values(UPopcornFXAttributeSamplerGrid *InGrid, TArray<FVector2D> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridFloat3Values(UPopcornFXAttributeSamplerGrid *InGrid, TArray<FVector> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridFloat4Values(UPopcornFXAttributeSamplerGrid *InGrid, TArray<FVector4> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridIntValues(UPopcornFXAttributeSamplerGrid *InGrid, TArray<int> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridInt2Values(UPopcornFXAttributeSamplerGrid *InGrid, TArray<FIntPoint> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridInt3Values(UPopcornFXAttributeSamplerGrid *InGrid, TArray<FIntVector> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridInt4Values(UPopcornFXAttributeSamplerGrid *InGrid, TArray<FIntVector4> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridFloatValues(UPopcornFXAttributeSamplerGrid *InGrid, const TArray<float> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridFloat2Values(UPopcornFXAttributeSamplerGrid *InGrid, const TArray<FVector2D> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridFloat3Values(UPopcornFXAttributeSamplerGrid *InGrid, const TArray<FVector> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridFloat4Values(UPopcornFXAttributeSamplerGrid *InGrid, const TArray<FVector4> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridIntValues(UPopcornFXAttributeSamplerGrid *InGrid, const TArray<int> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridInt2Values(UPopcornFXAttributeSamplerGrid *InGrid, const TArray<FIntPoint> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridInt3Values(UPopcornFXAttributeSamplerGrid *InGrid, const TArray<FIntVector> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridInt4Values(UPopcornFXAttributeSamplerGrid *InGrid, const TArray<FIntVector4> &InValues);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="PopcornFX AttributeSampler")
	FPopcornFXAttributeSamplerPropertiesGrid	Properties;

	// overrides
	void						OnUnregister() override;
	void						BeginDestroy() override;

	class UTexture				*GridTexture();

#if WITH_EDITOR
	void						PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

	const FPopcornFXAttributeSamplerProperties		*GetProperties() const override { return &Properties; }
#if WITH_EDITOR
	// UPopcornFXAttributeSampler overrides
	virtual void									CopyPropertiesFrom(const UPopcornFXAttributeSampler *other) override;
	virtual void									SetupDefaults(UPopcornFXEffect *effect, const uint32 samplerIdx, bool updateUnlockedValues) override;
#endif

	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;

private:
	bool						RebuildGridSampler(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler);
	bool						_RebuildGridSampler(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler);
	bool						ArePropertiesSupported() override;
	/** Checks if properties are compatible with the given emitter */
	bool						ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler);
	bool						IsRenderTargetCompatible(const UTextureRenderTarget *texture, UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler);
	bool						HasRenderTargetChanged() const;
	static bool					CanReadFromGrid(UPopcornFXAttributeSamplerGrid *Grid, EPopcornFXGridDataType::Type WantedType);
	static bool					CanWriteToGrid(UPopcornFXAttributeSamplerGrid *Grid, EPopcornFXGridDataType::Type WantedType, const int32 InValuesCount);

	/** Texture only used by the D3D12 GPU sim and to set as a material texture */
	UPROPERTY(Transient)
	UTexture					*m_GridTexture;

	FAttributeSamplerGridData	*m_Data;
};
