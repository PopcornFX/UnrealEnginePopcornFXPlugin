//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "IDetailCustomization.h"

class UPopcornFXEffect;
class UPopcornFXRendererMaterial;
class FPopcornFXEffectEditor;
class IDetailLayoutBuilder;
class IDetailCategoryBuilder;
class IPropertyHandle;
class IDetailGroup;
class IPropertyUtilities;

class FPopcornFXDetailsEffect : public IDetailCustomization
{
	struct FLayerDesc
	{
		FString					Name;
		uint32					ID = 0;
		TMap<FString, uint32>	UniqueRendererNames; // Unique renderer name / count

		bool IsFullyUnnamed() const
		{
			return (UniqueRendererNames.Num() == 1 && UniqueRendererNames.Contains("") && Name.IsEmpty());
		}

		void AddRendererName(const FString &RendererName)
		{
			if (!UniqueRendererNames.Contains(RendererName))
			{
				UniqueRendererNames.Add(RendererName);
			}
			UniqueRendererNames[RendererName]++;
		}
	};

	struct FUniqueMat
	{
		const UPopcornFXRendererMaterial	*Mat;
		TArray<uint32>						MatIndices; // Indices of materials using this combination in UPopcornFXEffect::ParticleRendererMaterials
		TArray<FLayerDesc>					Layers;
	};

public:
	virtual void	CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;
	void			BuildAssetDependenciesArray(class IDetailLayoutBuilder &DetailLayout, IDetailCategoryBuilder &cat, FName propertyPath, UClass *classOuter = nullptr);
	void			BuildRendererMaterialsArray(class IDetailLayoutBuilder &DetailLayout, IDetailCategoryBuilder &cat, FName propertyPath, UClass *classOuter = nullptr);
	void			UpdateBatchMaterials(const FPropertyChangedEvent &event);
	void			ListAllRenderers(UPopcornFXEffect* effect, TSharedRef<IPropertyHandle> pty, IDetailCategoryBuilder &cat);

	TArray<IDetailGroup *>			m_IGroups;
	TSharedPtr<IPropertyUtilities>	m_PropertyUtilities;
	int32							m_UniqueMatToUpdate;
	TArray<FUniqueMat>				m_UniqueMats;
};

#endif // WITH_EDITOR
