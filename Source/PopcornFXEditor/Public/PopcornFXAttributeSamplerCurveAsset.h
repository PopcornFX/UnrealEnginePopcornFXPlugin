//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXAttributeSamplerAsset.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXStyleEditor.h"

#include "AssetDefinitionDefault.h"
#include "CoreMinimal.h"
#include "Factories/Factory.h"

#include "PopcornFXAttributeSamplerCurveAsset.generated.h"

UCLASS()
class UAssetDefinition_PopcornFXAttributeSamplerCurve : public UAssetDefinitionPopcornFXAttributeSampler
{
	GENERATED_BODY()

protected:
	//~ Begin UAssetDefinitionDefault Interface
	virtual FText GetAssetDisplayName() const override
	{
		return FText::FromString(TEXT("PopcornFX Attribute sampler curve"));
	}
	virtual FText GetAssetDescription(const FAssetData &AssetData) const override
	{
		return FText::FromString(TEXT("A PopcornFX Curve attribute sampler"));
	}
	virtual FLinearColor GetAssetColor() const override
	{
		return FLinearColor(FColor(78, 40, 165));
	}

#if WITH_EDITOR
	virtual const FSlateBrush *GetThumbnailBrush(const FAssetData &InAssetData, const FName InClassName) const override
	{
		return FPopcornFXStyleEditor::GetBrush("PopcornFX.Node.AttributeSampler_Curve");
	}

	virtual const FSlateBrush *GetIconBrush(const FAssetData &InAssetData, const FName InClassName) const override
	{
		return FPopcornFXStyleEditor::GetBrush("PopcornFX.Node.AttributeSampler_Curve");
	}
#endif

	virtual TSoftClassPtr<UObject> GetAssetClass() const override
	{
		return UPopcornFXAttributeSamplerCurveAsset::StaticClass();
	}
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = {
		FAssetCategoryPath(FText::FromName("PopcornFX")),
		};
		return Categories;
	}
	//~ End UAssetDefinitionDefault Interface
};


UCLASS(HideCategories = Object)
class UPopcornFXAttributeSamplerCurveFactory : public UPopcornFXAttributeSamplerFactory
{
	GENERATED_UCLASS_BODY()

public:

	//~ Begin UFactory Interface
	virtual UObject *FactoryCreateNew(UClass *InClass, UObject *InParent, FName InName, EObjectFlags Flags, UObject *Context, FFeedbackContext *Warn, FName CallingContext) override
	{
		PK_ASSERT(InClass->IsChildOf(UPopcornFXAttributeSamplerCurveAsset::StaticClass()));
		return NewObject<UPopcornFXAttributeSamplerCurveAsset>(InParent, InClass, InName, Flags);
	}

	virtual FName GetNewAssetThumbnailOverride() const override
	{
		return "ClassThumbnail.PopcornFXAttributeSamplerCurve";
	}

	virtual FName GetNewAssetIconOverride() const override
	{
		return "ClassIcon.PopcornFXAttributeSamplerCurve";
	}
	//~ End UFactory Interface
};