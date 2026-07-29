//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXAttributeSampler.h"

#include "AssetDefinitionDefault.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Factories/Factory.h"

#include "PopcornFXAttributeSamplerAsset.generated.h"

UCLASS()
class UAssetDefinitionPopcornFXAttributeSampler : public UAssetDefinitionDefault
{
	GENERATED_BODY()

protected:
	//~ Begin UAssetDefinitionDefault Interface
	virtual FText GetAssetDisplayName() const override
	{
		return FText::FromString(TEXT("PopcornFX Attribute sampler"));
	}
	virtual FText GetAssetDescription(const FAssetData &AssetData) const override
	{
		return FText::FromString(TEXT("A PopcornFX attribute sampler"));
	}
	virtual FLinearColor GetAssetColor() const override
	{
		return FLinearColor(FColor::FromHex("#26601EFF"));
	}

	virtual TSoftClassPtr<UObject> GetAssetClass() const override
	{
		return UPopcornFXAttributeSamplerAsset::StaticClass();
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
class UPopcornFXAttributeSamplerFactory : public UFactory
{
	GENERATED_BODY()

public:

	//~ Begin UFactory Interface
	virtual UObject *FactoryCreateNew(UClass *InClass, UObject *InParent, FName InName, EObjectFlags Flags, UObject *Context, FFeedbackContext *Warn, FName CallingContext) override
	{
		PK_ASSERT(InClass->IsChildOf(UPopcornFXAttributeSamplerAsset::StaticClass()));
		return NewObject<UPopcornFXAttributeSamplerAsset>(InParent, InClass, InName, Flags);
	}
	//~ End UFactory Interface
};