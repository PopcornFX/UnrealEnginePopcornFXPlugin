//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "Assets/PopcornFXFile.h"

#include "PopcornFXCustomVersion.h"
#include "Internal/FileSystemController_UE.h"
#include "PopcornFXPlugin.h"

#include "Misc/FileHelper.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/MessageDialog.h"
#include "Engine/AssetUserData.h"
#include "Engine/Texture2D.h"
#include "UObject/LinkerLoad.h"
#if WITH_EDITOR
#	include "Factories/TextureFactory.h"
#	if (ENGINE_MAJOR_VERSION == 5)
#		include "AssetRegistry/AssetRegistryModule.h"
#	else
#		include "AssetRegistryModule.h"
#	endif // (ENGINE_MAJOR_VERSION == 5)
#	include "EditorReimportHandler.h"
#endif // WITH_EDITOR

#include "PopcornFXSDK.h"

#include <pk_particles/include/ps_effect.h>

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFile, Log, All);
#define LOCTEXT_NAMESPACE "PopcornFXFile"

//----------------------------------------------------------------------------

namespace
{
	PopcornFX::CString				fileToPkPath(const UPopcornFXFile &file)
	{
		PK_ASSERT(!file.IsUnreachable());

		PopcornFX::CString			rawFilePath = TCHAR_TO_ANSI(*file.GetPathName());
		PopcornFX::PFilePack		pack = FPopcornFXPlugin::Get().FilePack();
		if (pack == null)
		{
			UE_LOG(LogPopcornFile, Warning/*Error*/, TEXT("PopcornFX pack not loaded, cannot find '%s'"), ANSI_TO_TCHAR(rawFilePath.Data()));
			return null;
		}
		const PopcornFX::CString	&packPath = pack->Path();
		if (!rawFilePath.StartsWith(packPath.Data()))
		{
#if WITH_EDITOR
			if (!IsRunningCommandlet()) // We don't want this error to fail the packaging
			{
				FText	title = LOCTEXT("import_outside_mountpoint_title", "PopcornFX: Invalid file location");
				FText	finalText = FText::FromString(FString::Printf(TEXT("The PopcornFX file '%s' is outside the mount point of the PopcornFX pack '%s'"), ANSI_TO_TCHAR(rawFilePath.Data()), ANSI_TO_TCHAR(packPath.Data())));

				OpenMessageBox(EAppMsgType::Ok, finalText, title);
			}
#endif // WITH_EDITOR
			return null;
		}

		PK_ASSERT(rawFilePath[0] == '/'); // here, should be a valid Unreal path

		const u32	start = packPath.Length() + (packPath.EndsWith("/") ? 0 : 1);
		rawFilePath = rawFilePath.Extract(start, rawFilePath.Length());

		PK_ASSERT(rawFilePath[0] != '/'); // now, shoud be a valid PopcornFX virtual

		rawFilePath = PopcornFX::CFilePath::StripExtension(rawFilePath);
		rawFilePath += ".";
		rawFilePath += TCHAR_TO_ANSI(*file.PkExtension());
		return rawFilePath;
	}
}

//----------------------------------------------------------------------------
//
// UPopcornFXFile
//
//----------------------------------------------------------------------------

UPopcornFXFile::UPopcornFXFile(const FObjectInitializer& PCIP)
	: Super(PCIP)
#if WITH_EDITORONLY_DATA
	, ThumbnailImage(null)
	, FileSourceVirtualPathIsNotVirtual(0)
#endif
	, m_IsBaseObject(0)
	, m_FileVersionId(1)
	, m_LastCachedCacherVersion(0)
	, m_PkPath(null)
{
#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject))
		AssetImportData = CreateEditorOnlyDefaultSubobject<UAssetImportData>(TEXT("AssetImportData"));
#endif
}

//----------------------------------------------------------------------------
#if WITH_EDITOR

bool	UPopcornFXFile::ImportFile(const FString &inFilePath)
{
	UPopcornFXSettingsEditor	*settings = GetMutableDefault<UPopcornFXSettingsEditor>();
	if (!PK_VERIFY(settings != null))
		return false;
	if (!settings->AskForAValidSourcePackForIFN(inFilePath))
		return false;

	return _ImportFile(inFilePath);
}

bool	UPopcornFXFile::_ImportFile(const FString &inFilePath)
{
	Modify();

	const FString		filePath = inFilePath;

	if (IsFileValid())
		UnloadFile();

	PK_ASSERT(FPopcornFXPlugin::IsMainThread());

	SetFileSourcePath(filePath);

	GenPkPath(); // needs m_Extention etc.. from SetFileSourcePath !

	// When an effect is loaded in editor, its editor version will be loaded
	const FString		kEditorBuildVersionName = "Editor"; // TODO: Setting to disable this
	FString				bakedFilePath;
	if (!PK_VERIFY(_BakeFile(inFilePath, bakedFilePath, true, kEditorBuildVersionName)))
		return false;
	PK_ASSERT(!bakedFilePath.IsEmpty());

	// Note: For effects or other baked resources, inFileDataPath is != from inFilePath
	// the PopcornFXFile's source is the source .pkfx, but inFileDataPath is the baked effect (contained in project's Saved/PopcornFX/..)
	// this workflow ensures auto import works ok, UE assets points to the source effect
	// But the import data needs to be baked data
	const bool	ok = FFileHelper::LoadFileToArray(m_FileData, *bakedFilePath);
	if (ok)
	{
		PopcornFX::PBaseObjectFile		boFile_Cleaned;

		if (m_IsBaseObject)
			boFile_Cleaned = FPopcornFXPlugin::Get().LoadPkFile(this, true);

		// re-writing the file to get rid of invalid nodes
		if (boFile_Cleaned != null)
		{
			if (boFile_Cleaned->Binary())
			{
				// We don't handle binary baked files
				UE_LOG(LogPopcornFile, Error, TEXT("Cannot import binary baked PopcornFX file '%s'"), *filePath);
				const FText	title = LOCTEXT("PopcornFXCannotImportBakedTitle", "PopcornFX: Cannot import binary baked files");
				const FText	msg = FText::Format(
					LOCTEXT("PopcornFXCannotImportBakedText",
					"Please import the original source file of:\n\n{0}\n\nThen, PopcornFX Plugin and UE will take care of properly baking it.\n"),
					FText::FromString(filePath));
				OpenMessageBox(EAppMsgType::Ok, msg, title);
				FPopcornFXPlugin::Get().UnloadPkFile(this); // unload via this method (SetInternalUserData(null))
				return false;
			}
			boFile_Cleaned->SetBinary(false);

			// Write once and reload
			// Making sure data matches ("\r\n" to "\n")
			// EPopcornFXEffectCacherVersion::FixCRLFtoLF
			boFile_Cleaned->Write();

			{	// try a full reload
				PopcornFX::PBaseObjectFile	boFile = FPopcornFXPlugin::Get().LoadPkFile(this, true);
				if (!PK_VERIFY(boFile != null))
					return false;
			}
		}

		ImportThumbnail();

		if (!FinishImport())
		{
			UE_LOG(LogPopcornFile, Error, TEXT("Could not finish importing asset '%s'"), *filePath);
			return false;
		}

		AskImportAssetDependenciesIFN();

		// Reload the file after dependencies have been imported, and patched
		ReloadFile();

		CacheAsset();

		if (FPopcornFXPlugin::Get().SettingsEditor()->bDebugBakedEffects)
		{
			const FString	kPopcornPatchedPath = TEXT("Saved/PopcornFX/Patched/");
			const FString	cookedFilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / kPopcornPatchedPath / FileSourceVirtualPath);

			FFileHelper::SaveArrayToFile(m_FileData, *cookedFilePath);
		}
	}
	else
		UE_LOG(LogPopcornFile, Error, TEXT("Couldn't load file '%s'"), *filePath);

	++m_FileVersionId;

	MarkPackageDirty();
	if (GetOuter())
		GetOuter()->MarkPackageDirty();

	if (ok && PK_VERIFY(IsFileValid()))
	{
		ReloadFile();
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFile::ImportThumbnail()
{
	if (!PK_VERIFY(!FileSourceVirtualPath.IsEmpty()))
		return false;

	const UPopcornFXSettingsEditor	*settings = FPopcornFXPlugin::Get().SettingsEditor();
	if (FileSourceVirtualPathIsNotVirtual != 0)
	{
		UE_LOG(LogPopcornFile, Warning, TEXT("Imported file is outside Source PopcornFX Project, we cannot import its thumbnail !"));
	}
	else if (!PK_VERIFY(settings != null) || settings->SourcePackThumbnailsDir.IsEmpty())
	{
		UE_LOG(LogPopcornFile, Warning, TEXT("Source PopcornFX Project is invalid, we cannot import its thumbnail !"));
	}
	else // ok
	{
		const FString		srcPath = settings->SourcePackThumbnailsDir / FileSourceVirtualPath + TEXT(".png");
		bool				bImportWasCancelled;
		if (ThumbnailImage == null ||
			!FReimportManager::Instance()->Reimport(ThumbnailImage, false, false))
		{
			ThumbnailImage = null;
			if (FPaths::FileExists(srcPath)) // avoid import request if the file doesn't exist
			{
				// Remove useless dialog
				UTextureFactory::SuppressImportOverwriteDialog();
				ThumbnailImage = (UTexture2D*)UFactory::StaticImportObject(
					UTexture2D::StaticClass(),
					this,
					FName(TEXT("ThumbnailImage")),
					RF_Public, // Avoids external package ref, same as AttributeList & Sprite components
					bImportWasCancelled,
					*srcPath,
					null,
					null);
				PK_ASSERT(ThumbnailImage != null);
			}
		}
		if (ThumbnailImage != null)
		{
			// Force the reset for already existing thumbnails
			ThumbnailImage->ClearFlags(RF_Standalone);
		}
	}
	return ThumbnailImage != null;
}

//----------------------------------------------------------------------------

FString		UPopcornFXFile::FileSourcePath() const
{
	PK_ASSERT(!FileSourceVirtualPath.IsEmpty());
	if (FileSourceVirtualPathIsNotVirtual == 0)
		return FPopcornFXPlugin::Get().SettingsEditor()->SourcePackRootDir / FileSourceVirtualPath;
	return FileSourceVirtualPath;
}

//----------------------------------------------------------------------------

void		UPopcornFXFile::SetFileSourcePath(const FString &fileSourcePath)
{
	FileSourceVirtualPath.Empty();
	FileSourceVirtualPathIsNotVirtual = 0;
	m_FileExtension.Empty();
	m_IsBaseObject = false;

	bool					absoluteImport = true;
	const FString			packDir = FPopcornFXPlugin::Get().SettingsEditor()->SourcePackRootDir / "";

	if (packDir.Len() > 0)
	{
		PK_ASSERT(packDir[packDir.Len() - 1] == '/');
		const FString			filePath = FPaths::ConvertRelativePathToFull(fileSourcePath);
		if (filePath.StartsWith(packDir))
		{
			FileSourceVirtualPath = *(filePath.RightChop(packDir.Len()));
			FileSourceVirtualPathIsNotVirtual = 0;
			absoluteImport = false;
		}
		else
			UE_LOG(LogPopcornFile, Warning, TEXT("Imported File is outside Source PopcornFX Project, the file will be imported by absolute path"), *fileSourcePath);
	}
	else
		UE_LOG(LogPopcornFile, Warning, TEXT("Source PopcornFX Project is invalid, the file will be imported by absolute path"));

	if (absoluteImport)
	{
		FileSourceVirtualPath = fileSourcePath;
		FileSourceVirtualPathIsNotVirtual = 1;
	}

	m_FileExtension = FPaths::GetExtension(FileSourceVirtualPath, false);
	m_IsBaseObject = (m_FileExtension == "pkfx");
}

void	UPopcornFXFile::AskImportAssetDependenciesIFN()
{
	if (AssetDependencies.Num() == 0)
		return;

	bool			doAsk = false;
	bool			doImport = false;
	switch (FPopcornFXPlugin::Get().SettingsEditor()->AssetDependenciesAutoImport)
	{
		case	EPopcornFXAssetDependenciesAutoImport::Never:
			return;
		case	EPopcornFXAssetDependenciesAutoImport::Ask:
			doAsk = true;
			break;
		case	EPopcornFXAssetDependenciesAutoImport::Always:
			doImport = true;
			break;
	}

	PK_ASSERT(doAsk || doImport);

	FString				listFound;
	FString				listNotFound;

	bool				foundSome = false;

	const FString		sourcePack = FPopcornFXPlugin::Get().SettingsEditor()->SourcePackRootDir;
	const FString		sourcePackCacheDir = FPopcornFXPlugin::Get().SettingsEditor()->SourcePackCacheDir;
	for (int32 asseti = 0; asseti < AssetDependencies.Num(); ++asseti)
	{
		const UPopcornFXAssetDep	*assetDep = AssetDependencies[asseti];
		if (assetDep->Asset != null)
			continue;

		bool	fileExists = false;
		if (assetDep->bImportFromCache)
			fileExists = FPaths::FileExists(sourcePackCacheDir / assetDep->GetSourcePath());
		else
			fileExists = FPaths::FileExists(sourcePack / assetDep->GetSourcePath());
		if (fileExists)
		{
			listFound += "- " + assetDep->GetSourcePath();
			if (!assetDep->SourcePath.IsEmpty())
				listFound += " >>> " + assetDep->ImportPath;
			listFound += "\n";
			foundSome = true;
		}
		else
		{
			listNotFound += "- " + assetDep->ImportPath + "\n";
		}
	}

	if (!foundSome)
		return;

	if (doAsk)
	{
		FText		title = LOCTEXT("PopcornFX: Import missing asset dependencies", "PopcornFX: Import missing asset dependencies");
		FString		msg;
		msg += "Import Asset Dependencies from Source PopcornFX Project:\n\n" + listFound;
		if (!listNotFound.IsEmpty())
		{
			msg += "\n\nNOT FOUND:\n" + listNotFound;
		}
		doImport = false;
		if (FPopcornFXPlugin::AskImportAssetDependencies_YesAll())
			doImport = true;
		else
		{
			const EAppReturnType::Type	response = OpenMessageBox(EAppMsgType::YesNoYesAll, FText::FromString(msg), title);
			if (response == EAppReturnType::YesAll)
			{
				FPopcornFXPlugin::SetAskImportAssetDependencies_YesAll(true);
				doImport = true;
			}
			else if (response == EAppReturnType::Yes)
			{
				doImport = true;
			}
		}
	}
	if (doImport)
	{
		for (int32 asseti = 0; asseti < AssetDependencies.Num(); ++asseti)
		{
			UPopcornFXAssetDep		*assetDep = AssetDependencies[asseti];
			if (assetDep->Asset != null)
				continue;
			if (assetDep->ImportIFNAndResetDefaultAsset(this, false))
			{
				MarkPackageDirty();
				if (GetOuter())
					GetOuter()->MarkPackageDirty();
			}
		}
		OnAssetDepChanged(null);
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::PreReimport_Clean()
{
	AssetDependencies.Empty(AssetDependencies.Num());
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::GetAssetRegistryTags(TArray<FAssetRegistryTag> &outTags) const
{
	if (AssetImportData != null)
		outTags.Add( FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden) );

	Super::GetAssetRegistryTags(outTags);
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

bool	UPopcornFXFile::IsBaseObject() const
{
	return m_IsBaseObject != 0;
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::ReloadFile()
{
	OnFileUnload();

	if (!IsTemplate())
	{
		if (IsBaseObject())
			FPopcornFXPlugin::Get().LoadPkFile(this, true);
	}

	OnFileLoad();

#if WITH_EDITOR
	m_OnPopcornFXFileLoaded.Broadcast(this);
	m_OnFileChanged.Broadcast(this);
#endif
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::UnloadFile()
{
	PK_ASSERT(FPopcornFXPlugin::IsMainThread());

	if (IsUnreachable() && m_PkPath == null) // never loaded
	{
		return;
	}

	OnFileUnload();

	if (!IsTemplate())
	{
		if (IsBaseObject())
			FPopcornFXPlugin::Get().UnloadPkFile(this);
	}

	// dont clear m_PkPath here: we want PkPath() still valid for BeginDestroy IFN
}

//----------------------------------------------------------------------------

const char	*UPopcornFXFile::PkPath() const
{
	if (!/*PK_VERIFY*/(m_PkPath != null)) // PostLoad should have been called for someone to call PkPath()
	{
		GenPkPath();
	}
	return m_PkPath;
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::GenPkPath() const
{
	PK_ASSERT(!IsUnreachable());

	PK_ASSERT(FPopcornFXPlugin::IsMainThread());

	if (m_PkPath != null)
		delete[] m_PkPath;
	m_PkPath = null;
	PopcornFX::CString		path = fileToPkPath(*this);
	int32					len = path.Length();
	m_PkPath = new char[len + 1]();
	if (len > 0)
		memcpy(m_PkPath, path.Data(), len);
	m_PkPath[len] = 0;
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar.UsingCustomVersion(FPopcornFXCustomVersion::GUID);
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::PostInitProperties()
{
	Super::PostInitProperties();
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::PostLoad()
{
	Super::PostLoad();

	if (GetOutermost() == GetTransientPackage()) // UE doing arcane stuff at reimport (fbx files)
		return;

	ConditionalPostLoadSubobjects();

	GenPkPath();

	const int32		version = GetLinkerCustomVersion(FPopcornFXCustomVersion::GUID);

#if WITH_EDITOR
	if (version < FPopcornFXCustomVersion::RendererMaterials_AddTextureAtlasClass)
	{
		// we need to unref ALL texture atlases, or will fail at reimport !
		for (int32 i = 0; i < AssetDependencies.Num(); ++i)
		{
			UPopcornFXAssetDep		*dep = AssetDependencies[i];
			if (dep == null || dep->Asset == null)
				continue;
			if (dep->Type == EPopcornFXAssetDepType::TextureAtlas &&
				dep->Asset->GetClass() == UPopcornFXFile::StaticClass())
			{
				AssetDependencies.RemoveAt(i);
				--i;
			}
		}
	}

#else
	// NO WITH_EDITOR
	if (version < FPopcornFXCustomVersion::LatestVersion)
	{
		UE_LOG(LogPopcornFile, Warning, TEXT("UPopcornFXFile '%s' is outdated (%d), please re-save or re-cook asset"), *GetPathName(), version);
	}
#endif

	if (!CacheIsUpToDate())
		CacheAsset();
}

//----------------------------------------------------------------------------

#define PROFILE_CACHING		1

bool	UPopcornFXFile::CacheAsset()
{
	if (!HasCacher())
		return true;

#if PROFILE_CACHING
	UE_LOG(LogPopcornFile, Log, TEXT("Begin cache asset '%s'..."), *GetPathName());
	PopcornFX::CTimer	profileTimer;
	profileTimer.Start();
#endif

	PopcornFX::PBaseObjectFile		boFile;
	bool	fileWasUnloaded = false;

	bool	success = true;

	if (m_IsBaseObject)
	{
		boFile = FPopcornFXPlugin::Get().FindPkFile(this);
		if (boFile == null)
		{
			fileWasUnloaded = true;
			boFile = FPopcornFXPlugin::Get().LoadPkFile(this, true);
			if (boFile == null)
			{
				UE_LOG(LogPopcornFile, Warning, TEXT("UPopcornFXFile::CacheAsset could not load file '%s' !"), *GetPathName());
				success = false;
			}
		}
	}

	if (success)
	{
		success = LaunchCacher();
		if (!success)
			UE_LOG(LogPopcornFile, Warning, TEXT("UPopcornFXFile::CacheAsset '%s' cache failed !"), *GetPathName());
	}

	if (m_IsBaseObject)
	{
		if (success && PK_VERIFY(boFile != null))
			boFile->Write();
		if (fileWasUnloaded && boFile != null)
			UnloadFile();
		boFile = null;
	}

	if (success)
	{
		const uint64	cacherVersion = CurrentCacherVersion();
		m_LastCachedCacherVersion = cacherVersion;
	}

#if PROFILE_CACHING
	const double		cacheTime = profileTimer.Stop();
	UE_LOG(LogPopcornFile, Log, TEXT("...End cache asset '%s' %s in %.3f sec"), *GetPathName(), success ? TEXT("success") : TEXT("failed"), cacheTime);
#endif

	return success;
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::PostRename(UObject* OldOuter, const FName OldName)
{
	Super::PostRename(OldOuter, OldName);
	if (m_PkPath != null)
		GenPkPath(); // re-gen
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::AddAssetUserData(UAssetUserData* inUserData)
{
	if (inUserData != NULL)
	{
		UAssetUserData	*existingData = GetAssetUserDataOfClass(inUserData->GetClass());
		if (existingData != NULL)
			m_AssetUserDatas.Remove(existingData);
		m_AssetUserDatas.Add(inUserData);
	}
}

//----------------------------------------------------------------------------

UAssetUserData	*UPopcornFXFile::GetAssetUserDataOfClass(TSubclassOf<UAssetUserData> inUserDataClass)
{
	for (int32 i = 0; i < m_AssetUserDatas.Num(); i++)
	{
		UAssetUserData	*currentUserData = m_AssetUserDatas[i];

		if (currentUserData != NULL && currentUserData->IsA(inUserDataClass))
			return currentUserData;
	}
	return NULL;
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::RemoveUserDataOfClass(TSubclassOf<UAssetUserData> inUserDataClass)
{
	for (int32 i = 0; i < m_AssetUserDatas.Num(); i++)
	{
		UAssetUserData	*currentUserData = m_AssetUserDatas[i];

		if (currentUserData != NULL && currentUserData->IsA(inUserDataClass))
		{
			m_AssetUserDatas.RemoveAt(i);
			return;
		}
	}
}

//----------------------------------------------------------------------------

const TArray<UAssetUserData*>	*UPopcornFXFile::GetAssetUserDataArray() const
{
	return &m_AssetUserDatas;
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::BeginDestroy()
{
	UnloadFile();
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::OnFileUnload()
{
	// Make sure GPU sim tasks associated with this effect are properly flushed before unloading the effect
	// otherwise, we can have zombie pending tasks executed during the next rendered frame
	FlushRenderingCommands();

#if WITH_EDITOR
	m_OnPopcornFXFileUnloaded.Broadcast(this);
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------------

void	UPopcornFXFile::OnFileLoad()
{
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
