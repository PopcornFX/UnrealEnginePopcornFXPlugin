//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"
#include "PopcornFXAttributeSampler.h"
#include "PopcornFXAttributeSamplerAnimTrack.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXAttributeSamplerGrid.h"
#include "PopcornFXAttributeSamplerImage.h"
#include "PopcornFXAttributeSamplerShape.h"
#include "PopcornFXAttributeSamplerText.h"
#include "PopcornFXAttributeSamplerVectorField.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "PopcornFXAttributeSamplersFunctions.generated.h"

class UPopcornFXEmitterComponent;

UCLASS()
class POPCORNFX_API UPopcornFXAttributeSamplersFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:

	//---------------------------------------------------------------------------
	// 
	//	Sampler properties setters
	// 
	//---------------------------------------------------------------------------
	
	UFUNCTION(BlueprintCallable, meta=(DisplayName = "Set Attribute Sampler Anim Track Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static bool							SetAttributeSamplerAnimTrackProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesAnimTrack &InProperties);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Attribute Sampler Curve Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static bool							SetAttributeSamplerCurveProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesCurve &InProperties);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Attribute Sampler Grid Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static bool							SetAttributeSamplerGridProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesGrid &InProperties);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Attribute Sampler Image Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static bool							SetAttributeSamplerImageProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesImage &InProperties);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Attribute Sampler Shape Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static bool							SetAttributeSamplerShapeProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesShape &InProperties);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Attribute Sampler Text Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static bool							SetAttributeSamplerTextProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesText &InProperties);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Attribute Sampler VectorField Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static bool							SetAttributeSamplerVectorFieldProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesVectorField &InProperties);
	
	//---------------------------------------------------------------------------
	// 
	//	Sampler properties getters
	// 
	//---------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Attribute Sampler AnimTrack Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static void							GetAttributeSamplerAnimTrackProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesAnimTrack &OutProperties);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Attribute Sampler Curve Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static void							GetAttributeSamplerCurveProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesCurve &OutProperties);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Attribute Sampler Grid Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static void							GetAttributeSamplerGridProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesGrid &OutProperties);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Attribute Sampler Image Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static void							GetAttributeSamplerImageProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesImage &OutProperties);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Attribute Sampler Shape Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static void							GetAttributeSamplerShapeProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesShape &OutProperties);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Attribute Sampler Text Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static void							GetAttributeSamplerTextProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesText &OutProperties);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Attribute Sampler VectorField Properties", DefaultToSelf = "InSelf"), Category = "PopcornFX|Attribute Samplers")
	static void							GetAttributeSamplerVectorFieldProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesVectorField &OutProperties);

	//---------------------------------------------------------------------------
	// 
	//	Image sampler functions
	// 
	//---------------------------------------------------------------------------

#if 0 // TODO: are these functions still useful?
	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Attributes", meta = (DefaultToSelf = "Emitter"))
	static void				SetImageSamplerTexture(UPopcornFXEmitterComponent *Emitter, FString ImageSamplerName, class UTexture *InTexture);

	//---------------------------------------------------------------------------
	// 
	//	Curve sampler functions
	// 
	//---------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Attributes", meta = (DefaultToSelf = "Emitter"))
	static void				SetCurveSamplerDimension(UPopcornFXEmitterComponent *Emitter, FString CurveSamplerName, TEnumAsByte<EAttributeSamplerCurveDimension::Type> InCurveDimension);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Attributes", meta = (DefaultToSelf = "Emitter"))
	static void				SetCurveSamplerCurve(UPopcornFXEmitterComponent *Emitter, FString CurveSamplerName, class UCurveBase *InCurve, bool InIsSecondCurve);

	//---------------------------------------------------------------------------
	// 
	//	Text sampler functions
	// 
	//---------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Attributes", meta = (DefaultToSelf = "Emitter"))
	static void				SetTextSamplerText(UPopcornFXEmitterComponent *Emitter, FString TextSamplerName, FString InText);

	//---------------------------------------------------------------------------
	// 
	//	Shape sampler functions
	// 
	//---------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Attributes", meta = (DefaultToSelf = "Emitter"))
	static void				SetShapeSamplerRadius(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float Radius);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Attributes", meta = (DefaultToSelf = "Emitter"))
	static void				SetShapeSamplerWeight(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float Height);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Attributes", meta = (DefaultToSelf = "Emitter"))
	static void				SetShapeSamplerBoxDimension(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, FVector BoxDimensions);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Attributes", meta = (DefaultToSelf = "Emitter"))
	static void				SetShapeSamplerInnerRadius(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float InnerRadius);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Attributes", meta = (DefaultToSelf = "Emitter"))
	static void				SetShapeSamplerHeight(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float Height);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX|Attributes", meta = (DefaultToSelf = "Emitter"))
	static void				SetShapeSamplerScale(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, FVector Scale);
#endif // 0

	//---------------------------------------------------------------------------
	// 
	//	Grid sampler read functions
	// 
	//---------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridFloatValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<float> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridFloat2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FVector2D> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridFloat3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FVector> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridFloat4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FVector4> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridIntValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<int> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridInt2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FIntPoint> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridInt3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FIntVector> &OutValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "ReadGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	ReadGridInt4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FIntVector4> &OutValues);

	//---------------------------------------------------------------------------
	// 
	//	Grid sampler write functions
	// 
	//---------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridFloatValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<float> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridFloat2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FVector2D> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridFloat3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FVector> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridFloat4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FVector4> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridIntValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<int> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridInt2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FIntPoint> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridInt3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FIntVector> &InValues);

	UFUNCTION(BlueprintCallable, Category = "PopcornFX AttributeSampler", meta = (DisplayName = "WriteGridValues", BlueprintInternalUseOnly = "true", DefaultToSelf = "InGrid"))
	static bool	WriteGridInt4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FIntVector4> &InValues);
};
