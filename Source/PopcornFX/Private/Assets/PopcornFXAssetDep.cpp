//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "Assets/PopcornFXAssetDep.h"

#include "PopcornFXPlugin.h"
#include "Assets/PopcornFXFile.h"
#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXTextureAtlas.h"
#include "Assets/PopcornFXFont.h"
#include "Assets/PopcornFXAnimTrack.h"
#include "Assets/PopcornFXSimulationCache.h"

#include "Internal/FileSystemController_UE.h"

#include "Sound/SoundWave.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "VectorField/VectorFieldStatic.h"
#if WITH_EDITOR
#	include "AssetRegistryModule.h"
#	include "Editor/EditorHelpers.h"
#endif // WITH_EDITOR

#include "PopcornFXSDK.h"
#include <pk_base_object/include/hbo_helpers.h>
#include <pk_particles/include/ps_descriptor_cache.h>

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAssetDep, Log, All);

//----------------------------------------------------------------------------

#if WITH_EDITOR
#if (PK_COMPILER_BUILD_COMPILER == 0)
#	error Cannot build without PopcornFX graph compiler
#endif

namespace
{
	//
	// UE naming convention: https://wiki.unrealengine.com/Assets_Naming_Convention
	//
	const TCHAR		*kAssetDepTypePrefix[] = {
		null,					//None,
		TEXT("Pkfx_"),			//Effect,
		TEXT("T_"),				//Texture,
		TEXT("Pkat_"),			//TextureAtlas,
		TEXT("Pkfm_"),			//Font,
		TEXT("VF_"),			//VectorField
		TEXT("SM_"),			//Mesh,
		TEXT("Video_"),			//Video,
		TEXT("SW_"),			//Sound,
		TEXT("Pksc_"),			//SimCache
		null,					//File
	};
	PK_STATIC_ASSERT(PK_ARRAY_COUNT(kAssetDepTypePrefix) == EPopcornFXAssetDepType::_Count);

}

//----------------------------------------------------------------------------

UClass		*AssetDepTypeToClass(EPopcornFXAssetDepType::Type type, bool forCreation, bool fromCache)
{
	switch (type)
	{
	case EPopcornFXAssetDepType::None:
		return null;
	case EPopcornFXAssetDepType::Effect:
		return null; //UPopcornFXEffect::StaticClass();
	case EPopcornFXAssetDepType::Texture:
		return UTexture2D::StaticClass();
	case EPopcornFXAssetDepType::TextureAtlas:
		return UPopcornFXTextureAtlas::StaticClass();
	case EPopcornFXAssetDepType::Font:
		return UPopcornFXFont::StaticClass();
	case EPopcornFXAssetDepType::VectorField:
		return UVectorFieldStatic::StaticClass();
	case EPopcornFXAssetDepType::Mesh:
		if (fromCache)
			return UPopcornFXAnimTrack::StaticClass();
		return UStaticMesh::StaticClass();
	case EPopcornFXAssetDepType::Video:
		return null;// ::StaticClass();
	case EPopcornFXAssetDepType::Sound:
		if (forCreation)
			return USoundWave::StaticClass();
		return USoundBase::StaticClass();
	case EPopcornFXAssetDepType::SimCache:
		return UPopcornFXSimulationCache::StaticClass();
	case EPopcornFXAssetDepType::File:
		return UPopcornFXFile::StaticClass();
	}
	return null;
}

//----------------------------------------------------------------------------

bool	AssetDepClassIsCompatible(EPopcornFXAssetDepType::Type type, UClass *uclass)
{
	switch (type)
	{
	case EPopcornFXAssetDepType::None:
		PK_ASSERT_NOT_REACHED();
		return false;
	case EPopcornFXAssetDepType::Effect:
		PK_ASSERT_NOT_REACHED();
		return false; // uclass->IsChildOf<UPopcornFXEffect>();
	case EPopcornFXAssetDepType::Texture:
		return uclass->IsChildOf<UTexture>();
	case EPopcornFXAssetDepType::TextureAtlas:
		return uclass->IsChildOf<UPopcornFXTextureAtlas>();
	case EPopcornFXAssetDepType::Font:
		return uclass->IsChildOf<UPopcornFXFont>();
	case EPopcornFXAssetDepType::VectorField:
		return uclass->IsChildOf<UVectorFieldStatic>();
	case EPopcornFXAssetDepType::Mesh:
		return uclass->IsChildOf<UStaticMesh>() ||
			uclass->IsChildOf<USkeletalMesh>() ||
			uclass->IsChildOf<UPopcornFXAnimTrack>();
	case EPopcornFXAssetDepType::Video:
		PK_ASSERT_NOT_REACHED();
		return false;
	case EPopcornFXAssetDepType::Sound:
		return uclass->IsChildOf<USoundBase>();
	case EPopcornFXAssetDepType::SimCache:
		return uclass->IsChildOf<UPopcornFXSimulationCache>();
	case EPopcornFXAssetDepType::File:
		PK_ASSERT_NOT_REACHED();
		return uclass->IsChildOf<UPopcornFXFile>();
	}
	PK_ASSERT_NOT_REACHED();
	return false;
}
#endif // WITH_EDITOR

//----------------------------------------------------------------------------
//
// UPopcornFXAssetDep
//
//----------------------------------------------------------------------------

UPopcornFXAssetDep::UPopcornFXAssetDep(const FObjectInitializer& PCIP)
	: Super(PCIP)
#if WITH_EDITOR
	, Type(EPopcornFXAssetDepType::None)
	, AssetCurrent(null)
#endif
	, Asset(null)
{
}

//----------------------------------------------------------------------------
#if WITH_EDITOR
//----------------------------------------------------------------------------

bool	UPopcornFXAssetDep::IsCompatibleClass(UClass *uclass) const
{
	if (!PK_VERIFY(uclass != null))
		return false;
	return AssetDepClassIsCompatible(Type, uclass);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAssetDep::SetAsset(UPopcornFXFile *file, UObject *asset)
{
	check(ParentPopcornFXFile() == file);

	//if (asset == Asset) // no: in case some one already set Asset
	//	return true;

	Asset = null;
	if (asset == null)
		return false;
	if (!PK_VERIFY(IsCompatibleClass(asset->GetClass())))
		return false;
	Asset = asset;
	PatchFields(file);

	file->OnAssetDepChanged(this, AssetCurrent, Asset);

	AssetCurrent = Asset;
	return true;
}

//----------------------------------------------------------------------------

FString		UPopcornFXAssetDep::GetDefaultAssetPath() const
{
	return FPopcornFXPlugin::Get().BuildPathFromPkPath(TCHAR_TO_ANSI(*ImportPath), true);
}

//----------------------------------------------------------------------------

UObject		*UPopcornFXAssetDep::FindDefaultAsset() const
{
	return ::LoadObject<UObject>(null, *GetDefaultAssetPath());
}

//----------------------------------------------------------------------------

bool	UPopcornFXAssetDep::ImportIFNAndResetDefaultAsset(UPopcornFXFile *file, bool triggerOnAssetDepChanged)
{
	UObject			*defaultAsset = FindDefaultAsset();
	if (defaultAsset != null)
	{
		if (PK_VERIFY(IsCompatibleClass(defaultAsset->GetClass())))
		{
			Asset = defaultAsset;
			if (triggerOnAssetDepChanged)
				file->OnAssetDepChanged(this, AssetCurrent, Asset);
			AssetCurrent = Asset;
			return true;
		}
		else
		{
			// UE_LOG(LogPopcornFXAssetDep, Warning, TEXT("Incompatible Asset for %s: '%s'"), *defaultAsset->GetPathName());
		}
	}
	return ReimportAndResetDefaultAsset(file, triggerOnAssetDepChanged);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAssetDep::ReimportAndResetDefaultAsset(UPopcornFXFile *file, bool triggerOnAssetDepChanged)
{
	check(ParentPopcornFXFile() == file);

	FString	importSourcePath;
	if (bImportFromCache)
	{
		importSourcePath = FPopcornFXPlugin::Get().SettingsEditor()->SourcePackCacheDir / GetSourcePath();
		if (!FPaths::FileExists(importSourcePath))
		{
			UE_LOG(LogPopcornFXAssetDep, Error, TEXT("Source File '%s' not found. Please make sure to right click and 'Build Assets' on the source file."), *(FPopcornFXPlugin::Get().SettingsEditor()->SourcePackRootDir / GetSourcePath()));
			return false;
		}
	}
	else
	{
		importSourcePath = FPopcornFXPlugin::Get().SettingsEditor()->SourcePackRootDir / GetSourcePath();
		if (!FPaths::FileExists(importSourcePath))
		{
			UE_LOG(LogPopcornFXAssetDep, Error, TEXT("Source File '%s' not found to Import"), *importSourcePath);
			return false;
		}
	}

	const FString	dstPath = GetDefaultAssetPath();

	HelperImportFile(importSourcePath, dstPath, AssetDepTypeToClass(Type, true, bImportFromCache));

	UObject		*obj = null;
	bool		compatible = false;

	obj = LoadObject<UObject>(null, *dstPath);
	compatible = (obj != null && IsCompatibleClass(obj->GetClass()));

	if (obj == null)
	{
		UE_LOG(LogPopcornFXAssetDep, Error, TEXT("Failed to import '%s' to '%s'"), *importSourcePath, *dstPath);
		return false;
	}
	if (!PK_VERIFY(compatible))
	{
		UE_LOG(LogPopcornFXAssetDep, Warning, TEXT("Imported incompatible asset '%s'"), *importSourcePath);
		return false;
	}

	Asset = obj;
	PatchFields(file);

	MarkPackageDirty();

	if (triggerOnAssetDepChanged)
		file->OnAssetDepChanged(this, AssetCurrent, Asset);

	AssetCurrent = Asset;

	return true;
}

//----------------------------------------------------------------------------

void	UPopcornFXAssetDep::GetAssetPath(FString &outputPath) const
{
	if (Asset == null)
	{
		outputPath = ImportPath;
		return;
	}

	PopcornFX::PFilePack		pack;
	const PopcornFX::CString	assetPath = TCHAR_TO_ANSI(*Asset->GetPathName());
	PK_ASSERT(PopcornFX::File::DefaultFileSystem() != null);
	const PopcornFX::CString	virtPath = PopcornFX::File::DefaultFileSystem()->PhysicalToVirtual(assetPath, &pack);
	if (!virtPath.Empty())
	{
		PK_VERIFY(pack == FPopcornFXPlugin::Get().FilePack());
		outputPath = ANSI_TO_TCHAR(virtPath.Data());
		return; // OK
	}

	UE_LOG(LogPopcornFXAssetDep, Warning, TEXT("Asset '%s' is outside the mounted PopcornFX Pack"), ANSI_TO_TCHAR(assetPath.Data()));
	outputPath = ImportPath;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAssetDep::Setup(UPopcornFXFile *file, const FString &sourcePathOrNot, const FString &importPath, EPopcornFXAssetDepType::Type type)
{
	check(ParentPopcornFXFile() == file);

	// Clear
	PK_ASSERT(Asset == null);
	PK_ASSERT(SourcePath.IsEmpty());
	PK_ASSERT(ImportPath.IsEmpty());
	//Type = EPopcornFXAssetDepType::None;

	UClass		*assetTypeClass = AssetDepTypeToClass(type);
	if (!PK_VERIFY(assetTypeClass != null))
		return false;

	SourcePath = sourcePathOrNot;
	ImportPath = importPath;
	Type = type;

	bImportFromCache = false;
	const FString	ext = FPaths::GetExtension(ImportPath);
	if (ext == "pkfm" ||
		ext == "pkan")
		bImportFromCache = true;

	UObject		*asset = FPopcornFXPlugin::Get().LoadUObjectFromPkPath(TCHAR_TO_ANSI(*ImportPath), false);
	if (asset != null)
	{
		if (PK_VERIFY(IsCompatibleClass(asset->GetClass())))
		{
			Asset = asset;
			AssetCurrent = Asset;
		}
		else
			UE_LOG(LogPopcornFXAssetDep, Warning, TEXT("Asset '%s' was found but has incompatible type with %d"), *ImportPath, int32(Type.GetValue()));
	}

	return true;
}

//----------------------------------------------------------------------------

void	UPopcornFXAssetDep::ClearPatches()
{
	m_Patches.Empty(m_Patches.Num());
}

//----------------------------------------------------------------------------

void	UPopcornFXAssetDep::AddFieldToPatch(uint32 baseObjectUID, const char *fieldName)
{
	const FString			field = fieldName;
	for (int32 fieldi = 0; fieldi < m_Patches.Num(); ++fieldi)
	{
		if (m_Patches[fieldi].Match(baseObjectUID, field))
			return;
	}
	FPopcornFXFieldPath		*f = new (m_Patches)FPopcornFXFieldPath();
	check(f != null);
	f->BaseObjectUID = baseObjectUID;
	f->FieldName  = field;
}

//----------------------------------------------------------------------------

void	UPopcornFXAssetDep::PatchFields(UPopcornFXFile *file) const
{
	check(ParentPopcornFXFile() == file);

	PopcornFX::PBaseObjectFile		bofile = FPopcornFXPlugin::Get().FindPkFile(file);
	const bool						ownsFile = (bofile == null);
	if (bofile == null)
		bofile = FPopcornFXPlugin::Get().LoadPkFile(file, false);

	if (!PK_VERIFY(bofile != null))
		return;

	const PopcornFX::TArray<PopcornFX::PBaseObject>		&objects = bofile->ObjectList();
	if (objects.Empty())
		return;

	FString					newAssetPath;
	GetAssetPath(newAssetPath);
	PopcornFX::CString		newAssetPathPk = TCHAR_TO_ANSI(*newAssetPath); // Conv to UE, can't have PK types in public header.
	if (!PK_VERIFY(!newAssetPathPk.Empty()))
		return;

	PopcornFX::PBaseObject		lastBo;
	uint32						lastUID = 0;

	for (int32 patchi = 0; patchi < m_Patches.Num(); ++patchi)
	{
		PopcornFX::PBaseObject		bo;
		const FPopcornFXFieldPath	&patch = m_Patches[patchi];

		if (lastUID == patch.BaseObjectUID)
			bo = lastBo;
		else
		{
			const u32	baseObjectUID = patch.BaseObjectUID;
			for (u32 obji = 0; obji < objects.Count(); ++obji)
			{
				if (objects[obji] != null && objects[obji]->UID() == baseObjectUID)
				{
					bo = objects[obji];
				}
			}
		}
		lastUID = patch.BaseObjectUID;
		lastBo = bo;

		if (bo != null)
		{
			const PopcornFX::CString	fieldName = TCHAR_TO_ANSI(*(patch.FieldName));
			PopcornFX::CGuid			fieldId = bo->GetFieldIndex(PopcornFX::CStringView(fieldName));
			if (PK_VERIFY(fieldId.Valid()))
			{
				if (bo->GetField<PopcornFX::CString>(fieldId) != newAssetPathPk)
					bo->SetField(fieldId, newAssetPathPk);
			}
		}
	}

	{
		PopcornFX::PFilePack		pack = FPopcornFXPlugin::Get().FilePack();
		if (PK_VERIFY(pack != null))
		{
			PK_VERIFY(bofile->Write(PopcornFX::CFilePackPath(pack, bofile->Path())));
		}
	}

	if (ownsFile)
		bofile->Unload();

	MarkPackageDirty();
	if (GetOuter())
		GetOuter()->MarkPackageDirty();

	return;
}

//----------------------------------------------------------------------------

UPopcornFXFile	*UPopcornFXAssetDep::ParentPopcornFXFile() const
{
	UPopcornFXFile		*parentFile = Cast<UPopcornFXFile>(GetOuter());
	PK_ASSERT(parentFile != null);
	return parentFile;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAssetDep::Conflicts(const FString &importPath, EPopcornFXAssetDepType::Type type, const TArray<UPopcornFXAssetDep*> &otherAssetDeps)
{
	const FString			gamePath = FPopcornFXPlugin::Get().BuildPathFromPkPath(TCHAR_TO_ANSI(*importPath), true);
	if (!PK_VERIFY(!gamePath.IsEmpty()))
		return false;

	FAssetRegistryModule&	AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FAssetData				assetData = AssetRegistryModule.Get().GetAssetByObjectPath(*gamePath);
	if (assetData.IsValid())
	{
		// Retro compat for UPopcornFXFile atlases
		if (type == EPopcornFXAssetDepType::TextureAtlas &&
			assetData.GetClass() == UPopcornFXFile::StaticClass())
			return false;

		if (AssetDepClassIsCompatible(type, assetData.GetClass()))
			return false;

		// types differs
		return true;
	}

	for (int32 i = 0; i < otherAssetDeps.Num(); ++i)
	{
		if (otherAssetDeps[i] != null && otherAssetDeps[i]->Type != type)
		{
			const FString		&other = otherAssetDeps[i]->ImportPath;
			int32				lastPoint = -1;
			if (importPath.FindLastChar('.', lastPoint))
			{
				bool		same = true;
				// strncmp(other, importPath, lastPoint);
				int32		cmpMax = FMath::Min(lastPoint, other.Len());
				for (int32 j = 0; j <= cmpMax; ++j)
				{
					if (other[j] != importPath[j])
					{
						same = false;
						break;
					}
				}
				if (same)
					return true;
			}
			else if (other == importPath)
				return true;
		}
	}

	return false;
}

//----------------------------------------------------------------------------

//static
void	UPopcornFXAssetDep::RenameIfCollision(FString &inOutImportPath, EPopcornFXAssetDepType::Type type, const TArray<UPopcornFXAssetDep*> &otherAssetDeps)
{
	if (!PK_VERIFY(!inOutImportPath.IsEmpty()))
		return;

	if (!Conflicts(inOutImportPath, type, otherAssetDeps))
		return;

	FString		dir, filename, ext;
	FPaths::Split(inOutImportPath, dir, filename, ext);

	const TCHAR		*prefix = kAssetDepTypePrefix[type];

	// try prefixing the asset file name
	if (prefix != null)
	{
		const FString		testImportPath = dir / prefix + filename + TEXT(".") + ext;
		if (!Conflicts(testImportPath, type, otherAssetDeps))
		{
			UE_LOG(LogPopcornFXAssetDep, Log, TEXT("Will import '%s' to '%s' (prefixed)"), *inOutImportPath, *testImportPath);
			inOutImportPath = testImportPath;
			return;
		}
	}

	PK_ASSERT_MESSAGE(false, "Could not find an asset of the right type, even with a prefix or suffix");

	return;
}

//----------------------------------------------------------------------------

void	UPopcornFXAssetDep::PostLoad()
{
	AssetCurrent = Asset;
	Super::PostLoad();
}

//----------------------------------------------------------------------------

void	UPopcornFXAssetDep::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
	if (propertyChangedEvent.Property != NULL)
	{
		if (propertyChangedEvent.Property->GetName() == TEXT("Asset"))
		{
			SetAsset(ParentPopcornFXFile(), Asset);
		}
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXAssetDep::PostEditUndo()
{
	SetAsset(ParentPopcornFXFile(), Asset);
	Super::PostEditUndo();
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
