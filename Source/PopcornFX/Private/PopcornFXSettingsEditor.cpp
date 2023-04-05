//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXSettingsEditor.h"

#include "PopcornFXPlugin.h"

#if WITH_EDITOR

#	include "PopcornFXSDK.h"
#	include <pk_particles/include/ps_project_settings.h>

#	include "PopcornFXCustomVersion.h"
#	include "Assets/PopcornFXFile.h"
#	include "Settings/EditorLoadingSavingSettings.h"
#	include "Misc/MessageDialog.h"
#	include "Misc/FileHelper.h"
#	include "UObject/UnrealType.h"
#endif

#define LOCTEXT_NAMESPACE "PopcornFXEditorSettings"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXEditorSettings, Log, All);

//----------------------------------------------------------------------------

UPopcornFXSettingsEditor::UPopcornFXSettingsEditor(const FObjectInitializer& PCIP)
:	Super(PCIP)
#if WITH_EDITORONLY_DATA
,	ImportSourcePack("")
,	bSourcePackFound(false)
,	bAddSourcePackToMonitoredDirectories(false)
,	AssetDependenciesAutoImport(EPopcornFXAssetDependenciesAutoImport::Always)
,	bDebugBakedEffects(false)
,	bBuildAllDesktopBytecodes(false)
,	bAutoInsertSceneActor(true)
,	bAlwaysRenderAttributeSamplerShapes(false)
,	bRestartEmitterWhenAttributesChanged(false)
#endif // WITH_EDITORONLY_DATA
{
	static const FString		kDefaultIncludes[] = {
		"*.pkfx", "*.pkat",
		"*.fbx",
		"*.dds", "*.png", "*.jpg", "*.jpeg", "*.tga", "*.exr", "*.hdr", "*.tif",
		"*.mp3", "*.wav", "*.ogg",
		"*.pksc",
		"*.pkfm"
	};
	static uint32				kDefaultIncludesCount = sizeof(kDefaultIncludes) / sizeof(*kDefaultIncludes);
	static const FString		kDefaultExcludes[] = {
		"Editor/*",
	};
	static uint32				kDefaultExcludesCount = sizeof(kDefaultExcludes) / sizeof(*kDefaultExcludes);
#if WITH_EDITORONLY_DATA
	AutoReimportMirrorPackWildcards.SetNum(kDefaultIncludesCount + kDefaultExcludesCount);
	uint32		configwci = 0;
	for (uint32 i = 0; i < kDefaultIncludesCount; ++i, ++configwci)
	{
		AutoReimportMirrorPackWildcards[configwci].Wildcard = kDefaultIncludes[i];
		AutoReimportMirrorPackWildcards[configwci].bInclude = true;
	}
	for (uint32 i = 0; i < kDefaultExcludesCount; ++i, ++configwci)
	{
		AutoReimportMirrorPackWildcards[configwci].Wildcard = kDefaultExcludes[i];
		AutoReimportMirrorPackWildcards[configwci].bInclude = false;
	}
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR

//----------------------------------------------------------------------------

void	UPopcornFXSettingsEditor::PostLoad()
{
	Super::PostLoad();

	// /!\ not actualy called when GetDefault<UPopcornFXSettingsEditor> is used
	UpdateSourcePack();
}

//----------------------------------------------------------------------------

void	UPopcornFXSettingsEditor::PreEditChange(FProperty *propertyAboutToChange)
{
	Super::PreEditChange(propertyAboutToChange);

	if (propertyAboutToChange != null)
	{
		if (propertyAboutToChange->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXSettingsEditor, ImportSourcePack))
		{
			SourcePackRootDir_ToRemove = SourcePackRootDir;
		}
		else if (propertyAboutToChange->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXSettingsEditor, bAddSourcePackToMonitoredDirectories))
		{
			SourcePackRootDir_ToRemove = SourcePackRootDir;
		}
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXSettingsEditor::PostEditChangeProperty(struct FPropertyChangedEvent& propertyChangedEvent)
{
	if (propertyChangedEvent.Property != null)
	{
		if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXSettingsEditor, ImportSourcePack))
		{
			UpdateSourcePack();
		}
		else if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXSettingsEditor, bAddSourcePackToMonitoredDirectories)
#if WITH_EDITORONLY_DATA
				|| propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXSettingsEditor, AutoReimportMirrorPackWildcards)
#endif // WITH_EDITORONLY_DATA
				)
		{
			ForceUpdateAutoReimportSettings();
		}
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

namespace
{

	void		_NotifyReimportManager()
	{
		UObject	*loadingSavingSettings = GetMutableDefault<UEditorLoadingSavingSettings>();

		for (TFieldIterator<FProperty> ptyIt(loadingSavingSettings->GetClass()); ptyIt; ++ptyIt)
		{
			FProperty	*pty = *ptyIt;

			if (pty != null && pty->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UEditorLoadingSavingSettings, AutoReimportDirectorySettings))
			{
				FPropertyChangedEvent		chevent(pty);
				loadingSavingSettings->PostEditChangeProperty(chevent);
				break;
			}
		}
	}

	bool		_RemoveAutoReimport(const FString &sourceDir)
	{
		bool		hasChanged = false;
		UEditorLoadingSavingSettings	*loadSaveSettings = GetMutableDefault<UEditorLoadingSavingSettings>();
		if (!PK_VERIFY(loadSaveSettings != null))
			return hasChanged;
		for (int32 i = 0; i < loadSaveSettings->AutoReimportDirectorySettings.Num(); ++i)
		{
			const FAutoReimportDirectoryConfig		&config = loadSaveSettings->AutoReimportDirectorySettings[i];
			if (config.SourceDirectory != sourceDir)
				continue;
			loadSaveSettings->AutoReimportDirectorySettings.RemoveAt(i);
			--i;
			hasChanged = true;
		}
		return hasChanged;
	}

	struct FAutoReimportWildcardComparator
	{
		FString		Wildcard;
		bool		bInclude;

		FAutoReimportWildcardComparator(const FString &wildcard, bool include)
		:	Wildcard(wildcard), bInclude(include) { }
	};
	bool	operator == (const FAutoReimportWildcard &wd, const FAutoReimportWildcardComparator &cmp)
	{
		return wd.Wildcard == cmp.Wildcard && wd.bInclude == cmp.bInclude;
	}

} // namespace

//----------------------------------------------------------------------------

static bool		operator == (const FAutoReimportWildcard &a, const FMyAutoReimportWildcard &b)
{
	return a.Wildcard == b.Wildcard && a.bInclude == b.bInclude;
}

void	UPopcornFXSettingsEditor::_CopyWildcards(FAutoReimportDirectoryConfig &config)
{
	const u32	wildcardCount = AutoReimportMirrorPackWildcards.Num();

	config.Wildcards.SetNum(wildcardCount);
	for (u32 iWildcard = 0; iWildcard < wildcardCount; ++iWildcard)
	{
		const FMyAutoReimportWildcard	&myWildcard = AutoReimportMirrorPackWildcards[iWildcard];
		FAutoReimportWildcard			newWildcard;

		newWildcard.Wildcard = myWildcard.Wildcard;
		newWildcard.bInclude = myWildcard.bInclude;
		config.Wildcards[iWildcard] = newWildcard;
	}
}

bool	UPopcornFXSettingsEditor::_HasSameWildcards(const FAutoReimportDirectoryConfig &config)
{
	const u32	wildcardCount = config.Wildcards.Num();
	if (wildcardCount != AutoReimportMirrorPackWildcards.Num())
		return false;
	for (u32 iWildcard = 0; iWildcard < wildcardCount; ++iWildcard)
	{
		if (!(config.Wildcards[iWildcard] == AutoReimportMirrorPackWildcards[iWildcard]))
			return false;
	}
	return true;
}

void	UPopcornFXSettingsEditor::ForceUpdateAutoReimportSettings()
{
	UEditorLoadingSavingSettings	*loadSaveSettings = GetMutableDefault<UEditorLoadingSavingSettings>();
	if (!PK_VERIFY(loadSaveSettings != null))
		return;

	bool			notify = false;

	// Remove old one IFN
	if (!SourcePackRootDir_ToRemove.IsEmpty() &&
		SourcePackRootDir_ToRemove != SourcePackRootDir)
	{
		notify |= _RemoveAutoReimport(SourcePackRootDir_ToRemove);
	}
	SourcePackRootDir_ToRemove.Empty();

	const bool		autoReimport = !SourcePackRootDir.IsEmpty() && bAddSourcePackToMonitoredDirectories;

	if (!autoReimport)
	{
		// remove everything !
		notify |= _RemoveAutoReimport(SourcePackRootDir);
	}
	else
	{
		const UPopcornFXSettings	*settings = GetMutableDefault<UPopcornFXSettings>();
		check(settings != null);

		// only change if the settings needs to be changed !
		// To avoids _NotifyReimportManager if not nessecary

		const uint32		expectedCount = 1;
		uint32				foundCount = 0;
		bool				hasChanged = false;
		for (int32 i = 0; i < loadSaveSettings->AutoReimportDirectorySettings.Num(); ++i)
		{
			const FAutoReimportDirectoryConfig		&config = loadSaveSettings->AutoReimportDirectorySettings[i];
			if (config.SourceDirectory != SourcePackRootDir)
				continue;
			++foundCount;
			if (foundCount > expectedCount)
				break;
			if (config.MountPoint.IsEmpty() && config.Wildcards.Num() == 0)
				continue;
			if (config.MountPoint == settings->PackMountPoint &&
				_HasSameWildcards(config))
			{
				continue;
			}

			hasChanged = true;
			break;
		}

		if (foundCount != expectedCount)
			hasChanged = true;

		if (hasChanged)
		{
			// remove all
			notify |= _RemoveAutoReimport(SourcePackRootDir);

			const int32		diri = loadSaveSettings->AutoReimportDirectorySettings.Add(FAutoReimportDirectoryConfig());
			check(diri >= 0);
			FAutoReimportDirectoryConfig		&config = loadSaveSettings->AutoReimportDirectorySettings[diri];
			config.SourceDirectory = SourcePackRootDir;
			config.MountPoint = settings->PackMountPoint;

			_CopyWildcards(config);

			notify = true;
		}
	}

	if (notify)
		_NotifyReimportManager();
}


//----------------------------------------------------------------------------

static const FString			kOldPopcornFXProjectFileName = "PopcornProject.xml";
static const FString			kPopcornFXProjectFileName = "PopcornProject.pkproj";

// static
FString		UPopcornFXSettingsEditor::FixAndAppendPopcornFXProjectFileName(const FString &path)
{
	// fix old path
	if (path.EndsWith(kOldPopcornFXProjectFileName))
	{
		FString			newPath = path;
		newPath.RemoveAt(newPath.Len() - kOldPopcornFXProjectFileName.Len(), kOldPopcornFXProjectFileName.Len(), false);
		newPath /= kPopcornFXProjectFileName;
		return newPath;
	}
	// append ifn
	if (!path.EndsWith(kPopcornFXProjectFileName))
		return path / kPopcornFXProjectFileName;
	return path;
}

void	UPopcornFXSettingsEditor::UpdateSourcePack()
{
	// if not in editor, do nothing, keep saved values

	SourcePackProjectFile.Empty();
	SourcePackRootDir.Empty();
	SourcePackLibraryDir.Empty();
	SourcePackCacheDir.Empty();
	SourcePackThumbnailsDir.Empty();
	bSourcePackFound = 0;

	if (!ImportSourcePack.IsEmpty())
	{
		FString				projectFile = ImportSourcePack;
		FPaths::NormalizeFilename(projectFile);
		FPaths::RemoveDuplicateSlashes(projectFile);

		projectFile = FixAndAppendPopcornFXProjectFileName(projectFile);

		if (FPaths::IsRelative(projectFile))
			projectFile = FPaths::ProjectDir() / projectFile;

		SourcePackProjectFile = projectFile;
		int32			lastSlash = projectFile.Find("/", ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (lastSlash != INDEX_NONE)
			SourcePackRootDir = projectFile.Left(lastSlash + 1);
		SourcePackLibraryDir = SourcePackRootDir / "Library";
		SourcePackThumbnailsDir = SourcePackRootDir / "Editor/Thumbnails"; // just in case, the default one
		SourcePackCacheDir = SourcePackRootDir / "Cache";

		if (FPaths::FileExists(projectFile))
		{
			bSourcePackFound = 1;

			TArray<uint8>	fileContent;
			if (FFileHelper::LoadFileToArray(fileContent, *projectFile))
			{
				PopcornFX::CConstMemoryStream	stream(fileContent.GetData(), fileContent.Num());

				PopcornFX::HBO::CContext	*localBoContext = PK_NEW(PopcornFX::HBO::CContext());
				if (PK_VERIFY(localBoContext != null))
				{
					PopcornFX::PProjectSettings	projectSettings = PopcornFX::CProjectSettings::LoadFromStream(stream, localBoContext);

					if (projectSettings != null && projectSettings->General() != null)
					{
						bSourcePackFound = 1;
						if (!projectSettings->General()->RootDir().Empty())
							SourcePackRootDir /= ANSI_TO_TCHAR(projectSettings->General()->RootDir().Data());
						if (!projectSettings->General()->LibraryDir().Empty())
							SourcePackLibraryDir = SourcePackRootDir / ANSI_TO_TCHAR(projectSettings->General()->LibraryDir().Data());
						if (!projectSettings->General()->ThumbnailsDir().Empty())
							SourcePackThumbnailsDir = SourcePackRootDir / ANSI_TO_TCHAR(projectSettings->General()->ThumbnailsDir().Data());
						if (!projectSettings->General()->EditorCacheDir().Empty())
							SourcePackCacheDir = SourcePackRootDir / ANSI_TO_TCHAR(projectSettings->General()->EditorCacheDir().Data());

						localBoContext->UnloadAllFiles();
						PK_DELETE(localBoContext);
					}
					else
					{
						UE_LOG(LogPopcornFXEditorSettings, Error, TEXT("Couldn't load PopcornFX Project: '%s'"), *projectFile);
					}
				}
			}
			else
			{
				UE_LOG(LogPopcornFXEditorSettings, Error, TEXT("Couldn't load PopcornFX Project: '%s'"), *projectFile);
			}
		}
		else
		{
			// Log error
		}
		SourcePackProjectFile = FPaths::ConvertRelativePathToFull(SourcePackProjectFile);
		SourcePackRootDir = FPaths::ConvertRelativePathToFull(SourcePackRootDir);
		SourcePackLibraryDir = FPaths::ConvertRelativePathToFull(SourcePackLibraryDir);
		SourcePackThumbnailsDir = FPaths::ConvertRelativePathToFull(SourcePackThumbnailsDir);
		SourcePackCacheDir = FPaths::ConvertRelativePathToFull(SourcePackCacheDir);
	}

	// Avoid adding invalid source pack folders
	if (bSourcePackFound)
	{
		ForceUpdateAutoReimportSettings();
	}
}

//----------------------------------------------------------------------------

bool	UPopcornFXSettingsEditor::AskForAValidSourcePackForIFN(const FString &sourceAssetPath)
{
	if (!PK_VERIFY(!sourceAssetPath.IsEmpty()))
		return false;

	bool				isValid = ValidSourcePack();
	if (isValid)
	{
		const FString	sourceAssetPathAbs = FPaths::ConvertRelativePathToFull(sourceAssetPath);
		if (!sourceAssetPathAbs.StartsWith(SourcePackRootDir))
		{
			UE_LOG(LogPopcornFXEditorSettings, Error, TEXT("Asset outside Source PopcornFX Project: '%s'"), *sourceAssetPath);
			const FText	title = LOCTEXT("PopcornFXAssetOutsidePackTitle", "PopcornFX: Asset outside Source PopcornFX Project");
			const FText	msg = FText::Format(LOCTEXT("PopcornFXAssetOutsidePackMsg",
													"The asset is outside the Source PopcornFX Project !\n"
													"\n"
													"Asset:\n"
													"{0}\n"
													"\n"
													"Project Settings > PopcornFX Editor > Source PopcornFX Project path:\n"
													"{1}\n"
													"\n"
													"Continue anyway ?\n"), FText::FromString(sourceAssetPath), FText::FromString(SourcePackRootDir));

			return FMessageDialog::Open(EAppMsgType::YesNo, msg, &title) == EAppReturnType::Yes;
		}
		return true;
	}
	const FString		oldImportSourcePath = ImportSourcePack;

	if (!sourceAssetPath.IsEmpty())
	{
		ImportSourcePack = FPaths::ConvertRelativePathToFull(sourceAssetPath);
		PK_ASSERT(!isValid);

		// GetPath will make it loop over all parent folders to find the right one IFP
		while (!isValid)
		{
			const FString	sourcePack = ImportSourcePack;
			FString			newSourcePack = FPaths::GetPath(sourcePack); // dirname !
			// sanity checks !
			if (newSourcePack.Len() < 3 || // matches drive paths and too small to be valid
				newSourcePack.Len() >= sourcePack.Len()) // GetPath does that too
				break;
			if (!FPaths::IsRelative(newSourcePack))
			{
				FString			relativePath = newSourcePack;
				const FString	projectDir = FPaths::ProjectDir();

				if (FPaths::MakePathRelativeTo(relativePath, *projectDir) &&
					relativePath.Len() > 1 && relativePath.Len() <= newSourcePack.Len()) // use it only if shorter
				{
					newSourcePack = relativePath;
				}
			}
			if (!PK_VERIFY(newSourcePack[newSourcePack.Len() - 1] != '/'))
				break;
			ImportSourcePack = newSourcePack;
			UpdateSourcePack();
			isValid = ValidSourcePack();
		}
	}

	if (isValid)
	{
		PostEditChange();

		const FText		title = LOCTEXT("PopcornFXSourcePackFoundTitle", "PopcornFX: Source PopcornFX Project found !");
		const FText		text =
			FText::Format(
				LOCTEXT("PopcornFXSourcePackFoundMsg",
				"Source PopcornFX Project found and saved.\n\nProject Settings > PopcornFX > Source PopcornFX Project path\n\nis now {0}\n"),
				FText::FromString(ImportSourcePack));
		FMessageDialog::Open(EAppMsgType::Ok, text, &title);

		SaveConfig(); // Force save
		return true;
	}
	else
	{
		// Restore old one
		if (ImportSourcePack != oldImportSourcePath)
		{
			ImportSourcePack = oldImportSourcePath;
			UpdateSourcePack();
			PostEditChange();
		}

		UE_LOG(LogPopcornFXEditorSettings, Error, TEXT("Source PopcornFX Project NOT found: '%s'"), *ImportSourcePack);
		const FText	title = LOCTEXT("PopcornFXSourcePackNOTFoundTitle", "PopcornFX: Invalid Source PopcornFX Project path");
		const FText	msg = LOCTEXT(	"PopcornFXSourcePackNOTFoundTitleMsg",
									"Invalid Source PopcornFX Project path, and could automaticaly find one.\n\
									Please setup:\n\
									\n\
									Project Settings > PopcornFX > Source PopcornFX Project path\n\
									\n\
									Continue anyway ?\n");
		return FMessageDialog::Open(EAppMsgType::YesNo, msg, &title) == EAppReturnType::Yes;
	}
	return false;
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
