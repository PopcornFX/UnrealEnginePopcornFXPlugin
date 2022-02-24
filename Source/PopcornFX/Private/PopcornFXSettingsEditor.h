//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXSettingsEditor.generated.h"

UENUM()
namespace EPopcornFXAssetDependenciesAutoImport
{
	enum Type
	{
		Never	UMETA(DisplayName="Never Import", ToolTip="Never try to import missing dependencies."),
		Ask		UMETA(DisplayName="Ask to Import", ToolTip="Ask before importing missing dependencies."),
		Always	UMETA(DisplayName="Always Import", ToolTip="Always import missing dependencies, without asking.")
	};
}

// Define our own FAutoReimportWildcard class because this class definition is contained in the UnrealEd module,
// not available in non-editor builds (Build fails with Clang on ORBIS)
// Ultimately, we should place the UPopcornFXSettingsEditor class in the PopcornFXEditor module instead of the runtime one.
USTRUCT()
struct FMyAutoReimportWildcard
{
	GENERATED_USTRUCT_BODY()

	/** The wildcard filter as a string. Files that match this wildcard will be included/excluded according to the bInclude member */
	UPROPERTY(EditAnywhere, config, Category=AutoReimport)
	FString	Wildcard;

	/** When true, files that match this wildcard will be included (if it doesn't fail any other filters), when false, matches will be excluded from the reimporter */
	UPROPERTY(EditAnywhere, config, Category=AutoReimport)
	bool	bInclude;
};

#if WITH_EDITORONLY_DATA
UENUM()
namespace EPopcornFXBytecodeOptimLevel
{
	enum Type
	{
		None		UMETA(DisplayName="Disable optimizations"),
		Default		UMETA(DisplayName="Default optimization level"),
		Highest		UMETA(DisplayName="Highest optimization level")
	};
}
#endif // WITH_EDITORONLY_DATA

struct FAutoReimportDirectoryConfig;

/**
* Implements the settings for the PopcornFX Plugin.
*/
UCLASS(MinimalAPI, Config=Editor)
class UPopcornFXSettingsEditor : public UObject
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
public:
	/**
	The path of the PopcornFX Editor Project to import assets from.
	Either the full project path or the project's directory.
	Can be an absolute path, or relative to the Game Directory
	*/
	UPROPERTY(Config, EditAnywhere, Category="SourcePack", DisplayName="Source PopcornFX Project path")
	FString				ImportSourcePack;

	/** Plugin internal: true if the Source PopcornFX Project is valid */
	UPROPERTY(Config, VisibleAnywhere, Transient, Category="SourcePack", DisplayName="Is Valid")
	uint32				bSourcePackFound : 1;

	/** Plugin internal: resolved project file */
	UPROPERTY(Config, VisibleAnywhere, Category="SourcePack", DisplayName="Resolved Project File path")
	FString				SourcePackProjectFile;

	/** Plugin internal: resolved root dir */
	UPROPERTY(Config, VisibleAnywhere, Category="SourcePack", DisplayName="Resolved Root directory")
	FString				SourcePackRootDir;

	/** Plugin internal: resolved library dir */
	UPROPERTY(Config, VisibleAnywhere, Category="SourcePack", DisplayName="Resolved Root directory")
	FString				SourcePackLibraryDir;

	/** Plugin internal: resolved thumbnails dir */
	UPROPERTY(Config, VisibleAnywhere, Category="SourcePack", DisplayName="Resolved Thumbnails directory")
	FString				SourcePackThumbnailsDir;

	/** Plugin internal: resolved cache dir */
	UPROPERTY(Config, VisibleAnywhere, Category="SourcePack", DisplayName="Resolved Cache directory")
	FString				SourcePackCacheDir;

	/**
	Enables auto reimport/mirroring of Source PopcornFX Project assets.

	This will register the Source PopcornFX Project to "Editor Preferences > Loading & Saving > Directories To Monitor".

	Will mirror all the files included in the source pack that matches wildcards options. (See bellow)

	Uncheck "Editor Preferences > Loading & Saving > Auto Create Assets" if you only want to reimport already added files.
	*/
	UPROPERTY(Config, EditAnywhere, Category="Import")
	uint32				bAddSourcePackToMonitoredDirectories : 1;

	/**
	Wildcards for Auto Reimport Mirror Pack.
	Will override PackMountPoint Wildcards in "Editor Preferences > Loading & Saving > Directories To Monitor".
	*/
	UPROPERTY(Config, EditAnywhere, Category="Import", meta=(EditCondition="bAddSourcePackToMonitoredDirectories"))
	TArray<FMyAutoReimportWildcard>	AutoReimportMirrorPackWildcards;

	/** How to import PopcornFX Asset Dependencies, if any. */
	UPROPERTY(Config, EditAnywhere, Category="Import")
	TEnumAsByte<EPopcornFXAssetDependenciesAutoImport::Type>	AssetDependenciesAutoImport;

	/** Enables storing baked effects as text (under Saved/PopcornFX/Patched and Saved/PopcornFX/Cooked) */
	UPROPERTY(Config, EditAnywhere, Category = "Cook")
	uint32				bDebugBakedEffects : 1;

	/** If enabled, will automatically create a PopcornFXSceneActor when drag&dropping an emitter into a level with no scene actor available */
	UPROPERTY(Config, EditAnywhere, Category="Editor")
	uint32				bAutoInsertSceneActor : 1;

	/** Check this to always render the attribute sampler shapes debug, even when they are not selected */
	UPROPERTY(Config, EditAnywhere, Category="Debug")
	uint32				bAlwaysRenderAttributeSamplerShapes : 1;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
public:
	void				UpdateSourcePack();
	//bool				ResolveSourcePackFromFilePathIFP(const FString &inFilePath);

	bool				ValidSourcePack() const { return !SourcePackRootDir.IsEmpty() && bSourcePackFound != 0; }
	bool				AskForAValidSourcePackForIFN(const FString &sourceAssetPath);

	static FString		FixAndAppendPopcornFXProjectFileName(const FString &path);

private:
	virtual void		PostLoad() override;
	virtual void		PreEditChange(FProperty *propertyAboutToChange) override;
	virtual void		PostEditChangeProperty(struct FPropertyChangedEvent& propertyChangedEvent) override;

	void				CheckEditorPathValidity();
	void				ForceUpdateAutoReimportSettings();

	bool				_HasSameWildcards(const FAutoReimportDirectoryConfig &config);
	void				_CopyWildcards(FAutoReimportDirectoryConfig &config);
private:
	FString				SourcePackRootDir_ToRemove;
#endif // WITH_EDITOR

};
