//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXSDK.h"
#include "Assets/PopcornFXAssetDep.h"
#include "Interfaces/Interface_AssetUserData.h"
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 4)
#include "UObject/AssetRegistryTagsContext.h"
#endif

#include "PopcornFXFile.generated.h"

#if WITH_EDITOR
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPopcornFXFileUnloaded, const class UPopcornFXFile *);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPopcornFXFileLoaded, const class UPopcornFXFile *);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPopcornFXFileChanged, const class UPopcornFXFile *);
#endif

class UAssetUserData;
class UAssetImportData;

/** Base of any PopcornFX Asset imported a from .pk* file. */
UCLASS()
class POPCORNFX_API UPopcornFXFile : public UObject, public IInterface_AssetUserData
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UTexture2D			*ThumbnailImage;
#endif

	/** Array of user data stored with the asset */
	UPROPERTY(Category="UserDatas", EditAnywhere, AdvancedDisplay, Instanced)
	TArray<UAssetUserData*>		m_AssetUserDatas;

protected:
#if WITH_EDITORONLY_DATA
	/** Source virtual path of the imported file */
	UPROPERTY(Category="Source", VisibleAnywhere)
	FString				FileSourceVirtualPath;

	UPROPERTY(Category="Source", VisibleAnywhere)
	uint32				FileSourceVirtualPathIsNotVirtual;

public:
	UPROPERTY()
	UAssetImportData	*AssetImportData;
protected:
#endif

	/** Original extension of the file */
	UPROPERTY()
	FString				m_FileExtension;

	UPROPERTY()
	uint32				m_IsBaseObject : 1;

	/** The file content (from the baked effect) */
	UPROPERTY()
	TArray<uint8>		m_FileData;

	UPROPERTY()
	uint32				m_FileVersionId;

	UPROPERTY()
	uint64				m_LastCachedCacherVersion;

	UPROPERTY(Category="PopcornFX AssetDependencies", VisibleAnywhere, Instanced)
	TArray<UPopcornFXAssetDep*>	AssetDependencies;
public:

#if WITH_EDITOR
	bool						ImportFile(const FString &filePath);
	virtual bool				_ImportFile(const FString &filePath);
	virtual bool				_BakeFile(const FString &srcFilePath, FString &outBakedFilePath, bool forEditor, const FString &targetPlatformName) { outBakedFilePath = srcFilePath; return true; }

	FString						FileSourcePath() const;
	bool						HasSourceFile() const { return !FileSourceVirtualPath.IsEmpty(); }
	void						SetFileSourcePath(const FString &fileSourcePath);

	void						AskImportAssetDependenciesIFN();

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 4)
	void						GetAssetRegistryTags(FAssetRegistryTagsContext context) const override;
#else
	void						GetAssetRegistryTags(TArray<FAssetRegistryTag> &outTags) const override;
#endif

	virtual void				PreReimport_Clean();
#endif // WITH_EDITOR

	bool						IsBaseObject() const;

	void						ReloadFile();
	void						UnloadFile();

	uint32						FileVersionId() const { return m_FileVersionId; }
	bool						IsFileValid() const { return !m_FileExtension.IsEmpty(); }
	const char					*PkPath() const;
	const FString				&PkExtension() const { return m_FileExtension; }
	const TArray<uint8>			&FileData() const { return m_FileData; }
	TArray<uint8>				&FileDataForWriting() { return m_FileData; }

	// overrides UObject
	virtual void				Serialize(FArchive& Ar) override;
	virtual void				PostLoad() override;
	virtual void				PostInitProperties() override;
	virtual void				PostRename(UObject* OldOuter, const FName OldName) override;
	virtual void				BeginDestroy() override;

	// overrides IInterface_AssetUserData
	virtual void							AddAssetUserData(UAssetUserData* inUserData) override;
	virtual void							RemoveUserDataOfClass(TSubclassOf<UAssetUserData> inUserDataClass) override;
	virtual UAssetUserData					*GetAssetUserDataOfClass(TSubclassOf<UAssetUserData> inUserDataClass) override;
	virtual const TArray<UAssetUserData*>	*GetAssetUserDataArray() const override;

public:
#if WITH_EDITOR
	FOnPopcornFXFileUnloaded	m_OnPopcornFXFileUnloaded;
	FOnPopcornFXFileLoaded		m_OnPopcornFXFileLoaded;
	FOnPopcornFXFileChanged		m_OnFileChanged;
#endif

protected:
	void						GenPkPath() const;

	bool						HasCacher() const { return CurrentCacherVersion() != 0; }
	bool						CacheIsUpToDate() const { return m_LastCachedCacherVersion == CurrentCacherVersion();}
	bool						CacheAsset();
	virtual uint64				CurrentCacherVersion() const { return 0; }
	virtual bool				LaunchCacher() { return true; }

#if WITH_EDITOR
	virtual bool				ImportThumbnail();
	virtual bool				FinishImport() { return true; }
public: // internal
	virtual void				OnAssetDepChanged(UPopcornFXAssetDep *assetDep, UObject *oldAsset = nullptr, UObject *newAsset = nullptr) { }
protected:
#endif // WITH_EDITOR

	virtual void				OnFileUnload();
	virtual void				OnFileLoad();

private:
	mutable char				*m_PkPath; // mutable because lasy init
};
