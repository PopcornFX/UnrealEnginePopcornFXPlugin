//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplersFunctions.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXEmitterComponent.h"

#include "PopcornFXAttributeList.h"
#include "PopcornFXAttributeSamplerAnimTrack.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXAttributeSamplerGrid.h"
#include "PopcornFXAttributeSamplerImage.h"
#include "PopcornFXAttributeSamplerShape.h"
#include "PopcornFXAttributeSamplerVectorField.h"
#include "PopcornFXAttributeSamplerText.h"
#include "PopcornFXSDK.h"

#include "Engine/World.h"
#include "Misc/App.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplersFunctions"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplersFunctions, Log, All);

//----------------------------------------------------------------------------

#define FIND_ATTRIBUTE_SAMPLER(SamplerName, SamplerType, OutSampler) \
	for (int32 attri = 0; attri < Emitter->AttributeList.m_SamplerDescs.Num(); ++attri) \
	{ \
		if (Emitter->AttributeList.m_SamplerDescs[attri].m_SamplerName == SamplerName) \
		{ \
			OutSampler = Emitter->AttributeList.m_Samplers[attri].m_Sampler## SamplerType .GetPtrOrNull(); \
		} \
	} \

//----------------------------------------------------------------------------

#define REFRESH_ATTRIBUTE_SAMPLER(SamplerName, SamplerType) \
	FPopcornFXAttributeList *attrList = Emitter->GetAttributeList(); \
	if (!PK_VERIFY(attrList != null)) \
		return false; \
	const int32 samplerIdx = attrList->FindSamplerIndex(SamplerName); \
	if (samplerIdx == -1) \
	{ \
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler "#SamplerType" properties: can't find sampler '%s'"), *SamplerName); \
		return false; \
	} \
	FPopcornFXSamplerDesc *desc = attrList->GetSamplerDesc(samplerIdx); \
	if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::SamplerType) \
	{ \
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler "#SamplerType" properties: sampler '%s' is not a(n) "#SamplerType), *SamplerName); \
		return false; \
	} \
	desc->SetProperties(&InProperties); \
	FPopcornFXAttributeSampler##SamplerType *sampler = static_cast<FPopcornFXAttributeSampler##SamplerType *>(attrList->ResolveAttributeSampler(samplerIdx)); \
	if (sampler) \
		sampler->RefreshFromProperties(&InProperties); \

//----------------------------------------------------------------------------

#define FIND_ATTRIBUTE_SAMPLER_PROPERTIES(SamplerName, SamplerType, OutProperties) \
	FPopcornFXAttributeList *attrList = Emitter->GetAttributeList(); \
	if (!PK_VERIFY(attrList != null)) \
		return; \
	const int32 samplerIdx = attrList->FindSamplerIndex(SamplerName); \
	if (samplerIdx == -1) \
	{ \
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get "#SamplerType" attribute sampler properties: can't find sampler '%s'"), *SamplerName); \
		return; \
	} \
	FPopcornFXSamplerDesc *desc = attrList->GetSamplerDesc(samplerIdx); \
	if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::SamplerType) \
	{ \
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get "#SamplerType" attribute sampler properties: sampler '%s' is not a "#SamplerType), *SamplerName); \
		return; \
	} \
	const FPopcornFXAttributeSamplerProperties##SamplerType *properties = static_cast<const FPopcornFXAttributeSamplerProperties##SamplerType *>(desc->ResolveAttributeProperties()); \
	if (properties) \
		OutProperties = *properties; \

//----------------------------------------------------------------------------

#define CHECK_EMITTER(__ReturnValue) \
	if (Emitter == null) \
	{ \
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Invalid emitter")); \
		return __ReturnValue; \
	} \

#define CHECK_CAN_RENDER(__ReturnValue) \
	const UWorld *world = Emitter->GetWorld(); \
	if (!(FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))) \
		return __ReturnValue; \

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplersFunctions::UPopcornFXAttributeSamplersFunctions(class FObjectInitializer const &pcip)
:	Super(pcip)
{
}

//---------------------------------------------------------------------------
// 
//	Sampler properties setters
// 
//---------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerAnimTrackProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesAnimTrack &InProperties)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);
	REFRESH_ATTRIBUTE_SAMPLER(InAttributeSamplerName, AnimTrack);
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerCurveProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesCurve &InProperties)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);
	REFRESH_ATTRIBUTE_SAMPLER(InAttributeSamplerName, Curve);
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerGridProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesGrid &InProperties)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);
	REFRESH_ATTRIBUTE_SAMPLER(InAttributeSamplerName, Grid);
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerImageProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesImage &InProperties)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);
	REFRESH_ATTRIBUTE_SAMPLER(InAttributeSamplerName, Image);
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerShapeProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesShape &InProperties)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);
	REFRESH_ATTRIBUTE_SAMPLER(InAttributeSamplerName, Shape);
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerTextProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesText &InProperties)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);
	REFRESH_ATTRIBUTE_SAMPLER(InAttributeSamplerName, Text);
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerVectorFieldProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesVectorField &InProperties)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);
	REFRESH_ATTRIBUTE_SAMPLER(InAttributeSamplerName, VectorField);
	return true;
}

//---------------------------------------------------------------------------
// 
//	Sampler properties getters
// 
//---------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerAnimTrackProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesAnimTrack &OutProperties)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();
	FIND_ATTRIBUTE_SAMPLER_PROPERTIES(InAttributeSamplerName, AnimTrack, OutProperties);
}

//---------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerCurveProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesCurve &OutProperties)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();
	FIND_ATTRIBUTE_SAMPLER_PROPERTIES(InAttributeSamplerName, Curve, OutProperties);
}

//----------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerGridProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesGrid &OutProperties)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();
	FIND_ATTRIBUTE_SAMPLER_PROPERTIES(InAttributeSamplerName, Grid, OutProperties);
}

//----------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerImageProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesImage &OutProperties)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();
	FIND_ATTRIBUTE_SAMPLER_PROPERTIES(InAttributeSamplerName, Image, OutProperties);
}

//----------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerShapeProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesShape &OutProperties)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();
	FIND_ATTRIBUTE_SAMPLER_PROPERTIES(InAttributeSamplerName, Shape, OutProperties);
}

//----------------------------------------------------------------------------
void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerTextProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesText &OutProperties)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();
	FIND_ATTRIBUTE_SAMPLER_PROPERTIES(InAttributeSamplerName, Text, OutProperties);
}

//----------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerVectorFieldProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesVectorField &OutProperties)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();
	FIND_ATTRIBUTE_SAMPLER_PROPERTIES(InAttributeSamplerName, VectorField, OutProperties);
}

//---------------------------------------------------------------------------
// 
//	Image sampler functions
// 
//---------------------------------------------------------------------------

#if 0
void	UPopcornFXAttributeSamplersFunctions::SetImageSamplerTexture(UPopcornFXEmitterComponent *Emitter, FString ImageSamplerName, class UTexture *InTexture)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();

	FPopcornFXAttributeSamplerImage *samplerImage = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ImageSamplerName, Image, samplerImage);
	if (samplerImage != nullptr)
		samplerImage->SetTexture(InTexture);
}

//---------------------------------------------------------------------------
// 
//	Curve sampler functions
// 
//---------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetCurveSamplerDimension(UPopcornFXEmitterComponent *Emitter, FString CurveSamplerName, TEnumAsByte<EAttributeSamplerCurveDimension::Type> InCurveDimension)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();

	FPopcornFXAttributeSamplerCurve *samplerCurve = nullptr;
	FIND_ATTRIBUTE_SAMPLER(CurveSamplerName, Curve, samplerCurve);
	if (samplerCurve != nullptr)
		samplerCurve->SetCurveDimension(InCurveDimension);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetCurveSamplerCurve(UPopcornFXEmitterComponent *Emitter, FString CurveSamplerName, class UCurveBase *InCurve, bool InIsSecondCurve)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();

	FPopcornFXAttributeSamplerCurve *samplerCurve = nullptr;
	FIND_ATTRIBUTE_SAMPLER(CurveSamplerName, Curve, samplerCurve);
	if (samplerCurve != nullptr)
		samplerCurve->SetCurve(InCurve, InIsSecondCurve);
}

//---------------------------------------------------------------------------
// 
//	Text sampler functions
// 
//---------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetTextSamplerText(UPopcornFXEmitterComponent *Emitter, FString TextSamplerName, FString InText)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();

	FPopcornFXAttributeSamplerText *samplerText = nullptr;
	FIND_ATTRIBUTE_SAMPLER(TextSamplerName, Text, samplerText);
	if (samplerText != nullptr)
		samplerText->SetText(InText);
}

//---------------------------------------------------------------------------
// 
//	Shape sampler functions
// 
//---------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerRadius(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float Radius)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetRadius(Radius);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerWeight(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float Height)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetWeight(Height);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerBoxDimension(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, FVector BoxDimensions)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetBoxDimension(BoxDimensions);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerInnerRadius(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float InnerRadius)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetInnerRadius(InnerRadius);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerHeight(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float Height)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetHeight(Height);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerScale(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, FVector Scale)
{
	CHECK_EMITTER();
	CHECK_CAN_RENDER();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetScale(Scale);
}
#endif // 0

//---------------------------------------------------------------------------
// 
//	Grid Read functions
// 
//---------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::ReadGridFloatValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<float> &OutValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridFloatValues(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridFloat2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FVector2D> &OutValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridFloat2Values(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridFloat3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FVector> &OutValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridFloat3Values(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridFloat4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FVector4> &OutValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridFloat4Values(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridIntValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<int> &OutValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridIntValues(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridInt2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FIntPoint> &OutValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridInt2Values(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridInt3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FIntVector> &OutValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridInt3Values(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridInt4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FIntVector4> &OutValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridInt4Values(samplerGrid, OutValues);
	return false;
}

//---------------------------------------------------------------------------
// 
//	Grid Write functions
// 
//---------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::WriteGridFloatValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<float> &InValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridFloatValues(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridFloat2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FVector2D> &InValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridFloat2Values(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridFloat3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FVector> &InValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridFloat3Values(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridFloat4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FVector4> &InValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridFloat4Values(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridIntValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<int> &InValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridIntValues(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridInt2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FIntPoint> &InValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridInt2Values(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridInt3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FIntVector> &InValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridInt3Values(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridInt4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FIntVector4> &InValues)
{
	CHECK_EMITTER(false);
	CHECK_CAN_RENDER(true);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridInt4Values(samplerGrid, InValues);
	return false;
}

//----------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
