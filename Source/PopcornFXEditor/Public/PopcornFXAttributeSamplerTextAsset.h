//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXAttributeSamplerAsset.h"
#include "PopcornFXAttributeSamplerText.h"
#include "PopcornFXStyleEditor.h"

#include "AssetDefinitionDefault.h"
#include "CoreMinimal.h"
#include "Factories/Factory.h"

#include "PopcornFXAttributeSamplerTextAsset.generated.h"

UCLASS()
class UAssetDefinition_PopcornFXAttributeSamplerText : public UAssetDefinitionPopcornFXAttributeSampler
{
	GENERATED_BODY()

protected:
	//~ Begin UAssetDefinitionDefault Interface
	virtual FText GetAssetDisplayName() const override
	{
		return FText::FromString(TEXT("PopcornFX Attribute sampler text"));
	}
	virtual FText GetAssetDescription(const FAssetData &AssetData) const override
	{
		return FText::FromString(TEXT("A PopcornFX Text attribute sampler"));
	}
	virtual FLinearColor GetAssetColor() const override
	{
		return FLinearColor(FColor(128, 128, 64));
	}

#if WITH_EDITOR
	virtual const FSlateBrush *GetThumbnailBrush(const FAssetData &InAssetData, const FName InClassName) const override
	{
		return FPopcornFXStyleEditor::GetBrush("PopcornFX.Node.AttributeSampler_Text");
	}

	virtual const FSlateBrush *GetIconBrush(const FAssetData &InAssetData, const FName InClassName) const override
	{
		return FPopcornFXStyleEditor::GetBrush("PopcornFX.Node.AttributeSampler_Text");
	}
#endif

	virtual TSoftClassPtr<UObject> GetAssetClass() const override
	{
		return UPopcornFXAttributeSamplerTextAsset::StaticClass();
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
class UPopcornFXAttributeSamplerTextFactory : public UPopcornFXAttributeSamplerFactory
{
	GENERATED_UCLASS_BODY()

public:

	//~ Begin UFactory Interface
	virtual UObject *FactoryCreateNew(UClass *InClass, UObject *InParent, FName InName, EObjectFlags Flags, UObject *Context, FFeedbackContext *Warn, FName CallingContext) override
	{
		PK_ASSERT(InClass->IsChildOf(UPopcornFXAttributeSamplerTextAsset::StaticClass()));
		return NewObject<UPopcornFXAttributeSamplerTextAsset>(InParent, InClass, InName, Flags);
	}

	virtual FName GetNewAssetThumbnailOverride() const override
	{
		return "ClassThumbnail.PopcornFXAttributeSamplerText";
	}

	virtual FName GetNewAssetIconOverride() const override
	{
		return "ClassIcon.PopcornFXAttributeSamplerText";
	}
	//~ End UFactory Interface
};