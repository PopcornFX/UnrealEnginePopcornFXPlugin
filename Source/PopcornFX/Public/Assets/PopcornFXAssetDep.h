//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXAssetDep.generated.h"

class	UPopcornFXFile;

UENUM()
namespace EPopcornFXAssetDepType
{
	enum Type
	{
		None,

		// Path to an effect
		Effect,

		// Path to a texture
		Texture,

		// Path to a texture atlas
		TextureAtlas,

		// Path to a font
		Font,

		// Path to a vector field
		VectorField,

		// Path to a mesh (Static mesh or Animation track)
		Mesh,

		// Path to a video
		Video,

		// Path to an audio source
		Sound,

		// Path to a simulation cache
		SimCache,

		// Path to a random file
		File
	};
}
namespace EPopcornFXAssetDepType
{
	enum
	{
		_Count = File + 1
	};
}

UClass		*AssetDepTypeToClass(EPopcornFXAssetDepType::Type type, bool forCreation = false, bool fromCache = false);

USTRUCT()
struct FPopcornFXFieldPath
{
	GENERATED_USTRUCT_BODY();
public:
	UPROPERTY()
	uint32			BaseObjectUID;

	UPROPERTY()
	FString			FieldName;

	bool			Match(uint32 baseObjectUID, const FString &fieldName) const
	{
		return BaseObjectUID == baseObjectUID && FieldName == fieldName;
	}

	FPopcornFXFieldPath()
	:	BaseObjectUID()
	,	FieldName()
	{ }
};

/** Describe an Asset needed by a PopcornFX Asset. */
UCLASS(MinimalAPI, EditInlineNew)
class UPopcornFXAssetDep : public UObject
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITORONLY_DATA
	// can be empty if source path unchanged // use GetSourcePath()
	UPROPERTY(Category="PopcornFX AssetDependencies", VisibleAnywhere)
	FString			SourcePath;

	UPROPERTY(Category="PopcornFX AssetDependencies", VisibleAnywhere)
	FString			ImportPath;

	UPROPERTY(Category="PopcornFX AssetDependencies", VisibleAnywhere)
	TEnumAsByte<EPopcornFXAssetDepType::Type>	Type;

	// Whether this asset is imported from the source pack cache
	UPROPERTY(Category="PopcornFX AssetDependencies", VisibleAnywhere)
	uint32			bImportFromCache : 1;

	UPROPERTY()
	UObject			*AssetCurrent; // For renaming
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(Category="PopcornFX AssetDependencies", EditAnywhere, meta=(DisplayThumbnail="true"))
	UObject			*Asset; // keep in game build for referencing ?

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TArray<FPopcornFXFieldPath>		m_Patches;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR

	bool			IsCompatibleClass(UClass *uclass) const;

	bool			Setup(UPopcornFXFile *file, const FString &sourcePathOrNot, const FString &importPath, EPopcornFXAssetDepType::Type type);

	bool			SetAsset(UPopcornFXFile *file, UObject *asset);

	const FString	&GetSourcePath() const { return SourcePath.IsEmpty() ? ImportPath : SourcePath; }
	FString			GetDefaultAssetPath() const;
	UObject			*FindDefaultAsset() const;

	// Import default one IFN, then set to default.
	bool			ImportIFNAndResetDefaultAsset(UPopcornFXFile *file, bool triggerOnAssetDepChanged);

	// Reimport default one, then set to default
	bool			ReimportAndResetDefaultAsset(UPopcornFXFile *file, bool triggerOnAssetDepChanged);

	void			GetAssetPath(FString &outputPkPath) const;

	void			ClearPatches();
	void			AddFieldToPatch(uint32 baseObjectUID, const char *fieldName);
	void			PatchFields(UPopcornFXFile *file) const;

	UPopcornFXFile	*ParentPopcornFXFile() const;

	bool			Match(const FString &importPath, EPopcornFXAssetDepType::Type type) { return ImportPath == importPath && Type == type; }

	static bool		Conflicts(const FString &importPath, EPopcornFXAssetDepType::Type type, const TArray<UPopcornFXAssetDep*> &otherAssetDeps);

	static void		RenameIfCollision(FString &inOutImportPath, EPopcornFXAssetDepType::Type type, const TArray<UPopcornFXAssetDep*> &otherAssetDeps);

	// overrides UObject
	virtual void	PostLoad() override;
	virtual void	PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void	PostEditUndo() override;

#endif // WITH_EDITOR

};
