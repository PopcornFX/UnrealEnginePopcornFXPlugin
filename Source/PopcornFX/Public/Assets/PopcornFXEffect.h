//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "Assets/PopcornFXFile.h"
#include "PopcornFXEffect.generated.h"

class	UPopcornFXAttributeList;
class	UPopcornFXRendererMaterial;
class	CPopcornFXEffect;

/** PopcornFX Effect Asset imported from a .pkfx file. */
UCLASS(MinimalAPI, BlueprintType)
class UPopcornFXEffect : public UPopcornFXFile
{
	GENERATED_UCLASS_BODY()

	~UPopcornFXEffect();

public:
	UPROPERTY(Category="PopcornFX Attributes", Instanced, VisibleAnywhere)
	UPopcornFXAttributeList					*DefaultAttributeList;

public:
	UPROPERTY(Category="PopcornFX RendererMaterials", Instanced, VisibleAnywhere)
	TArray<UPopcornFXRendererMaterial*>		ParticleRendererMaterials;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	uint32	EditorLoopEmitter:1;

	UPROPERTY(meta=(ClampMin="0.0", ClampMax="20.0"))
	float	EditorLoopDelay;
#endif // WITH_EDITORONLY_DATA

	bool					LoadEffectIFN();
	bool					IsLoadCompleted() const;
	bool					EffectIsLoaded() const { ensure(IsLoadCompleted()); return m_Loaded; }

	bool									IsTheDefaultAttributeList(const UPopcornFXAttributeList *list) const { return list == DefaultAttributeList; }
	UPopcornFXAttributeList					*GetDefaultAttributeList();

	CPopcornFXEffect						*Effect() { return m_Private; }

	// overrides UObject
	virtual void			BeginDestroy() override;
	virtual FString			GetDesc() override;

#if WITH_EDITOR
	// overrides UObject
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 4)
	void					GetAssetRegistryTags(FAssetRegistryTagsContext context) const override;
#else
	void					GetAssetRegistryTags(TArray<FAssetRegistryTag> &outTags) const override;
#endif

	// overrides FPopcornFXFile
	virtual void			PreReimport_Clean() override;

	virtual void			BeginCacheForCookedPlatformData(const ITargetPlatform *targetPlatform) override;
#endif

private:
	void					ClearEffect();
	bool					LoadEffect(bool forceImport = false);

protected:
#if WITH_EDITOR
	void					ReloadRendererMaterials();

	// overrides FPopcornFXFile
	virtual bool			_ImportFile(const FString &filePath) override;
	virtual bool			_BakeFile(const FString &srcFilePath, FString &outBakedFilePath, bool forEditor, const FString &targetPlatformName) override;
	virtual bool			FinishImport() override;
	virtual void			OnAssetDepChanged(UPopcornFXAssetDep *assetDep, UObject *oldAsset = nullptr, UObject *newAsset = nullptr) override;
#endif

	// overrides FPopcornFXFile
	virtual uint64			CurrentCacherVersion() const override;
	virtual bool			LaunchCacher() override;

	virtual void			OnFileUnload() override;
	virtual void			OnFileLoad() override;

private:
	CPopcornFXEffect		*m_Private;
	bool					m_Loaded;
	bool					m_Cooked; // TMP: Until proper implementation of platform cached data
};
