//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXAttributeSamplerAsset.h"
#include "PopcornFXAttributeSamplerShape.h"
#include "PopcornFXStyleEditor.h"

#include "AssetDefinitionDefault.h"
#include "CoreMinimal.h"
#include "Factories/Factory.h"

#include "PopcornFXAttributeSamplerShapeAsset.generated.h"

UCLASS()
class UAssetDefinition_PopcornFXAttributeSamplerShape : public UAssetDefinitionPopcornFXAttributeSampler
{
	GENERATED_BODY()

protected:
	//~ Begin UAssetDefinitionDefault Interface
	virtual FText GetAssetDisplayName() const override
	{
		return FText::FromString(TEXT("PopcornFX Attribute sampler shape"));
	}
	virtual FText GetAssetDescription(const FAssetData &AssetData) const override
	{
		return FText::FromString(TEXT("A PopcornFX shape attribute sampler"));
	}
	virtual FLinearColor GetAssetColor() const override
	{
		//return FLinearColor(FColor(120, 209, 248)); Medium color between static and skeletal meshes
		return FLinearColor(FColor(186, 188, 255));
	}

#if WITH_EDITOR
	virtual const FSlateBrush *GetThumbnailBrush(const FAssetData &InAssetData, const FName InClassName) const override
	{
		return FPopcornFXStyleEditor::GetBrush("PopcornFX.Node.AttributeSampler_Shape");
	}

	virtual const FSlateBrush *GetIconBrush(const FAssetData &InAssetData, const FName InClassName) const override
	{
		return FPopcornFXStyleEditor::GetBrush("PopcornFX.Node.AttributeSampler_Shape");
	}
#endif

	virtual TSoftClassPtr<UObject> GetAssetClass() const override
	{
		return UPopcornFXAttributeSamplerShapeAsset::StaticClass();
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
class UPopcornFXAttributeSamplerShapeFactory : public UPopcornFXAttributeSamplerFactory
{
	GENERATED_UCLASS_BODY()

public:

	//~ Begin UFactory Interface
	virtual UObject *FactoryCreateNew(UClass *InClass, UObject *InParent, FName InName, EObjectFlags Flags, UObject *Context, FFeedbackContext *Warn, FName CallingContext) override
	{
		PK_ASSERT(InClass->IsChildOf(UPopcornFXAttributeSamplerShapeAsset::StaticClass()));
		return NewObject<UPopcornFXAttributeSamplerShapeAsset>(InParent, InClass, InName, Flags);
	}

	virtual FName GetNewAssetThumbnailOverride() const override
	{
		return "ClassThumbnail.PopcornFXAttributeSamplerShape";
	}

	virtual FName GetNewAssetIconOverride() const override
	{
		return "ClassIcon.PopcornFXAttributeSamplerShape";
	}
	//~ End UFactory Interface
};