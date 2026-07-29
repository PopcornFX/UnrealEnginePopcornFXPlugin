//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
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
class	UTextureRenderTarget;

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
	//virtual void		CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) override;
	/** Checks if properties set by the user are valid. For example, a Curve attribute sampler needs a Curve asset to be valid. */
	virtual bool		ArePropertiesSupported(UPopcornFXEmitterComponent *emitter, const FString &samplerName) override;
	/** Checks if properties set by the user are compatible with the emitter using it. For example, if an effect uses a 2D grid and the user sets a 3D grid, it's not compatible */
	virtual bool		ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const FString &samplerName, const PopcornFX::CResourceDescriptor *defaultSampler) override;

	/** Checks if properties are compatible with the given emitter */
	bool				IsRenderTargetCompatible(const UTextureRenderTarget *texture, UPopcornFXEmitterComponent *emitter, const FString &samplerName, const PopcornFX::CResourceDescriptor *defaultSampler);

public:

	/**
		If true, this grid attribute sampler is setup from a 2D or Volume render target asset instead of being setup manually.
		Note: this is only supported by GPU simulated particles (binding this sampler on an effect with CPU simulated particles will fallback to sampling the default resource defined in the effect).
		It can still be used with CPU simulated particles to use the render target's dimensions
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler", DisplayName = "Asset grid")
	uint32										bAssetGrid : 1;

	/**
		If true, render target has its sRGB flag force enabled (or disabled if false).
		Enable this if writes into the grid in the source effect(s) are not linear.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler")
	uint32										bSRGB : 1;

	/**
		2D or Volume Render target asset used by the grid attribute sampler.
		Note: this is only supported by GPU simulated particles (binding this sampler on an effect with CPU simulated particles will fallback to sampling the default resource defined in the effect).
		It can still be used with CPU simulated particles to use the render target's dimensions
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler", DisplayName = "Render target")
	class UTextureRenderTarget					*RenderTarget;

	/** Grid order (dimensions) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler")
	TEnumAsByte<EPopcornFXGridOrder>			Order;

	/** Grid width dimension */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler", meta = (ClampMin = "1", ClampMax = "4096"))
	int32										SizeX;

	/** Grid height dimension */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler", meta = (ClampMin = "1", ClampMax = "4096"))
	int32										SizeY;

	/** Grid depth dimension */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler", meta = (ClampMin = "1", ClampMax = "4096"))
	int32										SizeZ;

	/** Grid data type */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler", DisplayName = "Data type")
	TEnumAsByte<EPopcornFXGridDataType::Type>	DataType;

#if WITH_EDITOR
	virtual void						SetupDefaults(const PopcornFX::CParticleAttributeSamplerDeclaration *const decl, bool updateUnlockedValues) override;
#endif

	FPopcornFXAttributeSamplerPropertiesGrid()
	:	FPopcornFXAttributeSamplerProperties(EPopcornFXAttributeSamplerType::Grid)
	,	bAssetGrid(false)
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
USTRUCT(meta=(BlueprintSpawnableComponent))
struct POPCORNFX_API FPopcornFXAttributeSamplerGrid : public FPopcornFXAttributeSampler
{
	GENERATED_USTRUCT_BODY()

public:
	void		SetRenderTarget(class UTextureRenderTarget *InRenderTarget);

	/** Sets the D3D12 GPU sim grid as a parameter of a material */
	void		SetAsMaterialTextureParameter(class UMaterialInstanceDynamic *Material, FName ParameterName);

	/** Returns the number of cells in the grid */
	int32		GetCellCount() const;

	/** Returns the dimensions of the grid */
	FIntVector	GetDimensions() const;

	static bool	ReadGridFloatValues(FPopcornFXAttributeSamplerGrid *InGrid, TArray<float> &OutValues);

	static bool	ReadGridFloat2Values(FPopcornFXAttributeSamplerGrid *InGrid, TArray<FVector2D> &OutValues);

	static bool	ReadGridFloat3Values(FPopcornFXAttributeSamplerGrid *InGrid, TArray<FVector> &OutValues);

	static bool	ReadGridFloat4Values(FPopcornFXAttributeSamplerGrid *InGrid, TArray<FVector4> &OutValues);

	static bool	ReadGridIntValues(FPopcornFXAttributeSamplerGrid *InGrid, TArray<int> &OutValues);

	static bool	ReadGridInt2Values(FPopcornFXAttributeSamplerGrid *InGrid, TArray<FIntPoint> &OutValues);

	static bool	ReadGridInt3Values(FPopcornFXAttributeSamplerGrid *InGrid, TArray<FIntVector> &OutValues);

	static bool	ReadGridInt4Values(FPopcornFXAttributeSamplerGrid *InGrid, TArray<FIntVector4> &OutValues);

	static bool	WriteGridFloatValues(FPopcornFXAttributeSamplerGrid *InGrid, const TArray<float> &InValues);

	static bool	WriteGridFloat2Values(FPopcornFXAttributeSamplerGrid *InGrid, const TArray<FVector2D> &InValues);

	static bool	WriteGridFloat3Values(FPopcornFXAttributeSamplerGrid *InGrid, const TArray<FVector> &InValues);

	static bool	WriteGridFloat4Values(FPopcornFXAttributeSamplerGrid *InGrid, const TArray<FVector4> &InValues);

	static bool	WriteGridIntValues(FPopcornFXAttributeSamplerGrid *InGrid, const TArray<int> &InValues);

	static bool	WriteGridInt2Values(FPopcornFXAttributeSamplerGrid *InGrid, const TArray<FIntPoint> &InValues);

	static bool	WriteGridInt3Values(FPopcornFXAttributeSamplerGrid *InGrid, const TArray<FIntVector> &InValues);

	static bool	WriteGridInt4Values(FPopcornFXAttributeSamplerGrid *InGrid, const TArray<FIntVector4> &InValues);

	FPopcornFXAttributeSamplerPropertiesGrid	Properties;

	FPopcornFXAttributeSamplerGrid();

	class UTexture				*GridTexture();

#if WITH_EDITOR
	void						PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent) override;
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
	bool						RebuildGridSampler(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler);
	bool						_RebuildGridSampler(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler);
	bool						HasRenderTargetChanged() const;
	static bool					CanReadFromGrid(FPopcornFXAttributeSamplerGrid *InGrid, EPopcornFXGridDataType::Type WantedType);
	static bool					CanWriteToGrid(FPopcornFXAttributeSamplerGrid *InGrid, EPopcornFXGridDataType::Type WantedType, const int32 InValuesCount);

	/** Texture only used by the D3D12 GPU sim and to set as a material texture */
	UPROPERTY(VisibleAnywhere)
	UTexture					*m_GridTexture;

	FAttributeSamplerGridData	*m_Data;
};

UCLASS(meta = (BlueprintSpawnableComponent))
class POPCORNFX_API UPopcornFXAttributeSamplerGridAsset : public UPopcornFXAttributeSamplerAsset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FPopcornFXAttributeSamplerPropertiesGrid	Properties;

public:
	virtual const FPopcornFXAttributeSamplerProperties	*GetProperties() const override { return &Properties; }
	virtual FPopcornFXAttributeSamplerProperties		*GetProperties() override { return &Properties; }

};