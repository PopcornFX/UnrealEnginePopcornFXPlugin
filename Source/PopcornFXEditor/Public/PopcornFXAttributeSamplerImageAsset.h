//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXAttributeSamplerAsset.h"
#include "PopcornFXAttributeSamplerImage.h"
#include "PopcornFXStyleEditor.h"

#include "AssetDefinitionDefault.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Factories/Factory.h"

#include "PopcornFXAttributeSamplerImageAsset.generated.h"

UCLASS()
class UAssetDefinition_PopcornFXAttributeSamplerImage : public UAssetDefinitionPopcornFXAttributeSampler
{
	GENERATED_BODY()

protected:
	//~ Begin UAssetDefinitionDefault Interface
	virtual FText GetAssetDisplayName() const override
	{
		return FText::FromString(TEXT("PopcornFX Attribute sampler image"));
	}
	virtual FText GetAssetDescription(const FAssetData &AssetData) const override
	{
		return FText::FromString(TEXT("A PopcornFX image attribute sampler"));
	}
	virtual FLinearColor GetAssetColor() const override
	{
		return FLinearColor(FColor::FromHex("#26601EFF"));
	}

#if WITH_EDITOR
	virtual const FSlateBrush *GetThumbnailBrush(const FAssetData &InAssetData, const FName InClassName) const override
	{
		return FPopcornFXStyleEditor::GetBrush("PopcornFX.Node.AttributeSampler_Image");
	}

	virtual const FSlateBrush *GetIconBrush(const FAssetData &InAssetData, const FName InClassName) const override
	{
		return FPopcornFXStyleEditor::GetBrush("PopcornFX.Node.AttributeSampler_Image");
	}
#endif

	virtual TSoftClassPtr<UObject> GetAssetClass() const override
	{
		return UPopcornFXAttributeSamplerImageAsset::StaticClass();
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
class UPopcornFXAttributeSamplerImageFactory : public UPopcornFXAttributeSamplerFactory
{
	GENERATED_UCLASS_BODY()

public:

	//~ Begin UFactory Interface
	virtual UObject *FactoryCreateNew(UClass *InClass, UObject *InParent, FName InName, EObjectFlags Flags, UObject *Context, FFeedbackContext *Warn, FName CallingContext) override
	{
		PK_ASSERT(InClass->IsChildOf(UPopcornFXAttributeSamplerImageAsset::StaticClass()));
		return NewObject<UPopcornFXAttributeSamplerImageAsset>(InParent, InClass, InName, Flags);
	}

	virtual FName GetNewAssetThumbnailOverride() const override
	{
		return "ClassThumbnail.PopcornFXAttributeSamplerImage";
	}

	virtual FName GetNewAssetIconOverride() const override
	{
		return "ClassIcon.PopcornFXAttributeSamplerImage";
	}
	//~ End UFactory Interface
};