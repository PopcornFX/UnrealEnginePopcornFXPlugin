//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXFileFactory.h"

#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXTextureAtlas.h"
#include "Assets/PopcornFXFont.h"
#include "Assets/PopcornFXAnimTrack.h"
#include "Assets/PopcornFXSimulationCache.h"

#include "PopcornFXSDK.h"

#include "ObjectTools.h"
#include "PackageTools.h"
#include "AssetToolsModule.h"
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
#	include "AssetRegistry/AssetRegistryModule.h"
#else
#	include "AssetRegistryModule.h"
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
#include "EditorFramework/AssetImportData.h"
#include "Editor.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXFileFactory, Log, All);


//----------------------------------------------------------------------------

UPopcornFXFileFactory::UPopcornFXFileFactory(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	SupportedClass = UPopcornFXFile::StaticClass();
	Formats.Add(TEXT("pkfx;PopcornFX Effect"));
	// Formats.Add(TEXT("pkmm;PopcornFX mesh"));
	Formats.Add(TEXT("pkfm;PopcornFX Font"));
	Formats.Add(TEXT("pkan;PopcornFX AnimTrack"));
	Formats.Add(TEXT("pkat;PopcornFX Texture Atlas"));
	Formats.Add(TEXT("pksc;PopcornFX Simulation Cache"));

	bCreateNew = false;
	bText = false;
	bEditorImport = true;
}

//----------------------------------------------------------------------------

void	UPopcornFXFileFactory::CleanUp()
{
	// Cleanup the factory before further use
	Super::CleanUp();
}

//----------------------------------------------------------------------------

void	UPopcornFXFileFactory::PostInitProperties()
{
	Super::PostInitProperties();
	bEditorImport = true;
	bText = false;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFileFactory::DoesSupportClass(UClass * inClass)
{
	return inClass == UPopcornFXFile::StaticClass() || inClass->IsChildOf(UPopcornFXFile::StaticClass());
}

//----------------------------------------------------------------------------

UClass	*UPopcornFXFileFactory::ResolveSupportedClass()
{
	return SupportedClass;
}

//----------------------------------------------------------------------------

namespace
{
	UClass	*_BuildClassFromExt(const FString &sourceExt)
	{
		if (sourceExt == TEXT("pkfx"))
			return UPopcornFXEffect::StaticClass();
		else if (sourceExt == TEXT("pkat"))
			return UPopcornFXTextureAtlas::StaticClass();
		else if (sourceExt == TEXT("pkfm"))
			return UPopcornFXFont::StaticClass();
		else if (sourceExt == TEXT("pkan"))
			return UPopcornFXAnimTrack::StaticClass();
		else if (sourceExt == TEXT("pksc"))
			return UPopcornFXSimulationCache::StaticClass();
		return UPopcornFXFile::StaticClass();
	}
}

UObject	*UPopcornFXFileFactory::FactoryCreateBinary(UClass *inClass, UObject *inParent, FName inName, EObjectFlags flags, UObject *context, const TCHAR *type, const uint8 *&buffer, const uint8 *bufferEnd, FFeedbackContext *warn, bool &bOutOperationCanceled)
{
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, inClass, inParent, inName, type);

	FString				effectName = ObjectTools::SanitizeObjectName(inName.ToString());
	FString				usableResourcePath = UFactory::GetCurrentFilename();

	const FString		sourceExt = type;
	UClass				*buildClass = _BuildClassFromExt(sourceExt);

	UPopcornFXFile	*newFile = Cast<UPopcornFXFile>(CreateOrOverwriteAsset(buildClass, inParent, inName, flags | RF_Public));
	if (!PK_VERIFY(newFile != NULL))
	{
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, null);
		return null;
	}

	if (newFile->ImportFile(*usableResourcePath) &&
		PK_VERIFY(newFile->IsFileValid()) &&
		PK_VERIFY(newFile->PkExtension() == sourceExt))
	{
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(newFile);
		if (PK_VERIFY(newFile->AssetImportData != null))
			newFile->AssetImportData->Update(newFile->FileSourcePath());
	}
	else
	{
		UE_LOG(LogPopcornFXFileFactory, Error, TEXT("File creation failed '%s' in '%s'"), *newFile->GetPathName(), *inParent->GetPathName());
		bOutOperationCanceled = true;

		if (PK_VERIFY(newFile->GetOutermost() != null))
		{
			// Clear dirty flag on owner package, otherwise UE will try to process it next frame while it will not exist anymore
			newFile->GetOutermost()->SetDirtyFlag(false);
		}

		newFile->ConditionalBeginDestroy();
		newFile->GetOuter()->ConditionalBeginDestroy();
		newFile = null;
	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, newFile);

	return newFile;
}

//----------------------------------------------------------------------------
