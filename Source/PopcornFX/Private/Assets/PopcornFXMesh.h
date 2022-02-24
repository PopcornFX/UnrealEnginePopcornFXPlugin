//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"
#include "Assets/PopcornFXFile.h"

#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"

#include "PopcornFXSDK.h"
#include <pk_geometrics/include/ge_mesh_resource.h>

#include "PopcornFXMesh.generated.h"

class UStaticMesh;
class USkeletalMesh;

/** Contains a Mesh's Sampling Acceleration Structures to be used by PopcornFX Particles.
* Kind-of correspond to the .pkmm files.
*/
UCLASS(MinimalAPI)
class UPopcornFXMesh : public UPopcornFXFile
{
	GENERATED_UCLASS_BODY()

public:
	static UPopcornFXMesh		*FindStaticMesh(UStaticMesh *staticMesh);
	static UPopcornFXMesh		*FindSkeletalMesh(USkeletalMesh *skelMesh);

	/* The source static mesh */
	UPROPERTY(Category="PopcornFX Mesh", VisibleAnywhere)
	UStaticMesh*				StaticMesh;

	/* The source skeletal mesh */
	UPROPERTY(Category="PopcornFX Mesh", VisibleAnywhere)
	USkeletalMesh*				SkeletalMesh;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Category="PopcornFX Mesh", VisibleAnywhere, Instanced)
	class UAssetImportData		*StaticMeshAssetImportData;

	/* Static mesh's LOD to build PopcornFX mesh from (static meshes only) */
	UPROPERTY(Category="PopcornFX Mesh", EditAnywhere, BlueprintReadOnly)
	int32						StaticMeshLOD;

	/* Use StaticMesh's sections as submeshes, or merge all sections into one single mesh (static meshes only) */
	UPROPERTY(Category="PopcornFX Mesh", EditAnywhere, BlueprintReadOnly)
	uint32						bMergeStaticMeshSections : 1;

	/* Whether "uv to pcoord" accel structs are built or not (increases file size, reduces runtime overhead if the feature is used) */
	UPROPERTY(Category="PopcornFX Mesh", EditAnywhere, BlueprintReadOnly)
	uint32						bBuildUVToPCoordAccelStructs : 1;

	/* Manual rebuild of the PopcornFX mesh */
	UPROPERTY(Category="PopcornFX Mesh", EditAnywhere, BlueprintReadOnly)
	uint32						bEditorReimport : 1;
#endif // WITH_EDITORONLY_DATA

	// overrides UObject
	virtual void				BeginDestroy() override;
	virtual void				PostLoad() override;

	PopcornFX::PResourceMesh	LoadResourceMeshIFN(bool editorBuildIFN);

#if WITH_EDITOR
	virtual void				PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void				PreReimport_Clean() override;

	bool						SourceMeshChanged() const;
#endif // WITH_EDITOR

	bool						HasSourceMesh() const { return StaticMesh != null || SkeletalMesh != null; }
	void						SetSourceMesh(UStaticMesh *staticMesh, bool dirtifyBuild);
	void						SetSourceMesh(USkeletalMesh *skelMesh, bool dirtifyBuild);
	UObject						*SourceMeshObject() const { if (StaticMesh != null) return StaticMesh; return SkeletalMesh; }

protected:
#if WITH_EDITOR
	virtual bool				_ImportFile(const FString &filePath) override;
#endif // WITH_EDITOR
	virtual void				OnFileUnload() override;
	virtual void				OnFileLoad() override;

private:
#if WITH_EDITOR
	PopcornFX::PResourceMesh	NewResourceMesh(UStaticMesh *staticMesh);
	PopcornFX::PResourceMesh	NewResourceMesh(USkeletalMesh *skeletalMesh);

	bool						BuildMeshFromSource();

	template <typename _AssetType>
	bool						BuildMesh(_AssetType *mesh);
	void						WriteMesh();
#endif // WITH_EDITOR

private:
	PopcornFX::PResourceMesh	m_ResourceMesh;
};
