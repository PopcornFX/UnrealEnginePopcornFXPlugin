//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXAttributeSamplerAsset.h"
#include "PopcornFXAttributeSamplerGrid.h"
#include "PopcornFXStyleEditor.h"

#include "AssetDefinitionDefault.h"
#include "CoreMinimal.h"
#include "Factories/Factory.h"

#include "PopcornFXAttributeSamplerGridAsset.generated.h"

UCLASS()
class UAssetDefinition_PopcornFXAttributeSamplerGrid : public UAssetDefinitionPopcornFXAttributeSampler
{
	GENERATED_BODY()

protected:
	//~ Begin UAssetDefinitionDefault Interface
	virtual FText GetAssetDisplayName() const override
	{
		return FText::FromString(TEXT("PopcornFX Attribute sampler grid"));
	}
	virtual FText GetAssetDescription(const FAssetData &AssetData) const override
	{
		return FText::FromString(TEXT("A PopcornFX Grid attribute sampler"));
	}
	virtual FLinearColor GetAssetColor() const override
	{
		return FLinearColor(FColor(128, 64, 64));
	}

#if WITH_EDITOR
	virtual const FSlateBrush *GetThumbnailBrush(const FAssetData &InAssetData, const FName InClassName) const override
	{
		return FPopcornFXStyleEditor::GetBrush("PopcornFX.Node.AttributeSampler_Grid");
	}

	virtual const FSlateBrush *GetIconBrush(const FAssetData &InAssetData, const FName InClassName) const override
	{
		return FPopcornFXStyleEditor::GetBrush("PopcornFX.Node.AttributeSampler_Grid");
	}
#endif

	virtual TSoftClassPtr<UObject> GetAssetClass() const override
	{
		return UPopcornFXAttributeSamplerGridAsset::StaticClass();
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
class UPopcornFXAttributeSamplerGridFactory : public UPopcornFXAttributeSamplerFactory
{
	GENERATED_UCLASS_BODY()

public:

	//~ Begin UFactory Interface
	virtual UObject *FactoryCreateNew(UClass *InClass, UObject *InParent, FName InName, EObjectFlags Flags, UObject *Context, FFeedbackContext *Warn, FName CallingContext) override
	{
		PK_ASSERT(InClass->IsChildOf(UPopcornFXAttributeSamplerGridAsset::StaticClass()));
		return NewObject<UPopcornFXAttributeSamplerGridAsset>(InParent, InClass, InName, Flags);
	}

	virtual FName GetNewAssetThumbnailOverride() const override
	{
		return "ClassThumbnail.PopcornFXAttributeSamplerGrid";
	}

	virtual FName GetNewAssetIconOverride() const override
	{
		return "ClassIcon.PopcornFXAttributeSamplerGrid";
	}
	//~ End UFactory Interface
};