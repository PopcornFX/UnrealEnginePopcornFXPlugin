//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXMesh.h"

#include "PopcornFXPlugin.h"
#include "Assets/PopcornFXStaticMeshData.h"
#include "PopcornFXVersionGenerated.h"
#include "Editor/EditorHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"

#include "StaticMeshResources.h"
#include "EditorFramework/AssetImportData.h"
#if WITH_EDITOR
#	include "Misc/MessageDialog.h"
#endif

#include "PopcornFXSDK.h"
#include <pk_geometrics/include/ge_mesh_resource.h>
#include <pk_geometrics/include/ge_mesh_sampler_accel.h>
#include <pk_particles_toolbox/include/pt_mesh_deformers_skin.h>
#include <pk_geometrics/include/ge_mesh.h>
#include <pk_kernel/include/kr_containers_array.h>
#include <pk_kernel/include/kr_containers_onstack.h>

DEFINE_LOG_CATEGORY_STATIC(LogPopcornMesh, Log, All);
#define LOCTEXT_NAMESPACE "PopcornFXMesh"

#define ENABLE_SKELETALMESH			1

//----------------------------------------------------------------------------
//
// UE version wrappers
//
//----------------------------------------------------------------------------

namespace
{
#if WITH_EDITOR
	const UAssetImportData	*SkeletalMeshAssetImportData(const USkeletalMesh *skeletalMesh)
	{
		if (skeletalMesh == null)
			return null;
#if ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5)
		return skeletalMesh->GetAssetImportData();
#else
		return skeletalMesh->AssetImportData;
#endif // ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5)
	}
#endif // WITH_EDITOR

	FStaticMeshRenderData	*StaticMeshRenderData(UStaticMesh *staticMesh)
	{
		if (staticMesh == null)
			return null;
#if ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5)
		return staticMesh->GetRenderData();
#else
		return staticMesh->RenderData.Get();
#endif // ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5)
	}
}

//----------------------------------------------------------------------------
//
// UPopcornFXMesh
//
//----------------------------------------------------------------------------

template <typename _AssetType>
static UPopcornFXMesh		*_FindOrCreateMeshData(_AssetType *asset)
{
	if (!PK_VERIFY(asset != null))
		return null;

	asset->ConditionalPostLoad();

	UPopcornFXStaticMeshData	*userData = null;
	UPopcornFXMesh				*mesh = null;

	userData = Cast<UPopcornFXStaticMeshData>(asset->GetAssetUserDataOfClass(UPopcornFXStaticMeshData::StaticClass()));
	if (userData == null)
	{
#if WITH_EDITOR
		asset->Modify(true);
		ForceSetPackageDirty(asset);
#endif
		userData = NewObject<UPopcornFXStaticMeshData>(asset);
		asset->AddAssetUserData(userData);
	}
	else
	{
		mesh = userData->PopcornFXMesh;
	}

	if (mesh == null)
	{
#if WITH_EDITOR
		asset->Modify(true);
		ForceSetPackageDirty(asset);
#endif
		mesh = NewObject<UPopcornFXMesh>(asset);
		userData->PopcornFXMesh = mesh;
	}

	mesh->ConditionalPostLoad();

	return mesh;
}

//----------------------------------------------------------------------------

//static
UPopcornFXMesh		*UPopcornFXMesh::FindStaticMesh(UStaticMesh *staticMesh)
{
	UPopcornFXMesh	*mesh = _FindOrCreateMeshData(staticMesh);
	if (mesh != null)
		mesh->SetSourceMesh(staticMesh, false);
	return mesh;
}

//----------------------------------------------------------------------------

//static
UPopcornFXMesh	*UPopcornFXMesh::FindSkeletalMesh(USkeletalMesh *skelMesh)
{
#if ENABLE_SKELETALMESH
	UPopcornFXMesh	*mesh = _FindOrCreateMeshData(skelMesh);
	if (mesh != null)
		mesh->SetSourceMesh(skelMesh, false);
	return mesh;
#else
	return null;
#endif
}

//----------------------------------------------------------------------------

UPopcornFXMesh::UPopcornFXMesh(const FObjectInitializer& PCIP)
	: Super(PCIP)
#if WITH_EDITORONLY_DATA
	, StaticMesh(null)
	, StaticMeshLOD(0)
	, bMergeStaticMeshSections(false)
	, bBuildUVToPCoordAccelStructs(false)
	, bEditorReimport(false)
#endif
{
#if WITH_EDITORONLY_DATA
	StaticMeshAssetImportData = CreateDefaultSubobject<UAssetImportData>(TEXT("StaticMeshAssetImportData"));
#endif
	m_FileExtension = "pkmm";
	m_IsBaseObject = false;
}

//----------------------------------------------------------------------------

void	UPopcornFXMesh::BeginDestroy()
{
	Super::BeginDestroy();
	m_ResourceMesh = null;
}

//----------------------------------------------------------------------------

void	UPopcornFXMesh::PostLoad()
{
	Super::PostLoad();

	if (GetOutermost() == GetTransientPackage()) // UE doing arcane stuff at reimport
		return;

	if (!IsRunningCommandlet()) // Do not store/load anything during cook or something else
	{
		// Force serialize the mesh at load, we don't want to pay that cost at runtime
		LoadResourceMeshIFN(true);
	}
}

//----------------------------------------------------------------------------

PopcornFX::PResourceMesh	UPopcornFXMesh::LoadResourceMeshIFN(bool editorBuildIFN)
{
	if (!HasSourceMesh())
		return null;

	PK_NAMEDSCOPEDPROFILE_C("CResourceHandlerMesh_UE::LoadResourceMeshIFN", POPCORNFX_UE_PROFILER_COLOR);

#if WITH_EDITOR
	if (editorBuildIFN && SourceMeshChanged())
	{
		bool		reload = false;

		if (FPopcornFXPlugin::AskBuildMeshData_YesAll())
			reload = true;
		else
		{
			FText		title = LOCTEXT("PopcornFX: Build Mesh data", "PopcornFX: Build Mesh data");
			FString		msg;
			msg += "Do you want to (re)generate PopcornFX mesh data for \"" + SourceMeshObject()->GetPathName() + "\" ?\nThis mesh is used by an effect for static/skeletal mesh sampling. \nIf yes, make sure to save the mesh afterwards.";
			EAppReturnType::Type	response = FMessageDialog::Open(EAppMsgType::YesNoYesAll, FText::FromString(msg), &title);
			if (response == EAppReturnType::YesAll)
			{
				reload = true;
				FPopcornFXPlugin::SetAskBuildMeshData_YesAll(true);
			}
			else if (response == EAppReturnType::Yes)
			{
				reload = true;
			}
		}

		if (reload)
		{
			BuildMeshFromSource();
		}
	}
#endif // WITH_EDITOR

	if (m_ResourceMesh == null)
	{
		PK_NAMEDSCOPEDPROFILE_C("Serialize PopcornFX mesh", POPCORNFX_UE_PROFILER_COLOR);

		PopcornFX::CMessageStream	outReport(true);
		m_ResourceMesh = PopcornFX::CResourceMesh::Load(PopcornFX::File::DefaultFileSystem(), PopcornFX::CFilePackPath(FPopcornFXPlugin::Get().FilePack(), PkPath()), outReport);
		if (m_ResourceMesh == null)
		{
			UE_LOG(LogPopcornMesh, Warning, TEXT("Couldn't load PopcornFX mesh '%s'"), ANSI_TO_TCHAR(PkPath()));
			m_ResourceMesh = null;
		}
	}

	return m_ResourceMesh.Get();
}

//----------------------------------------------------------------------------
#if WITH_EDITOR
//----------------------------------------------------------------------------

void	UPopcornFXMesh::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != NULL)
	{
		if (PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXMesh, StaticMesh) ||
			PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXMesh, SkeletalMesh) ||
			PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXMesh, StaticMeshLOD) ||
			PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXMesh, bMergeStaticMeshSections) ||
			PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXMesh, bBuildUVToPCoordAccelStructs) ||
			PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXMesh, bEditorReimport))
		{
			StaticMeshLOD = FMath::Max(StaticMeshLOD, 0);
			bEditorReimport = 0;
			if (StaticMesh != null)
				SetSourceMesh(StaticMesh, true);
			else if (SkeletalMesh != null)
				SetSourceMesh(SkeletalMesh, true);
			LoadResourceMeshIFN(true);
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXMesh::PreReimport_Clean()
{
	m_ResourceMesh = null;
	Super::PreReimport_Clean();
}

//----------------------------------------------------------------------------

bool	AssetImportDataDiffers(const UAssetImportData *a, const UAssetImportData* b)
{
	if (a->SourceFilePath_DEPRECATED != b->SourceFilePath_DEPRECATED ||
		a->SourceFileTimestamp_DEPRECATED != b->SourceFileTimestamp_DEPRECATED)
		return true;
	const int32		c = a->SourceData.SourceFiles.Num();
	if (c != b->SourceData.SourceFiles.Num())
		return true;
	for (int32 i = 0; i < c; ++i)
	{
		const auto		&sfa = a->SourceData.SourceFiles[i];
		const auto		&sfb = b->SourceData.SourceFiles[i];
		if (sfa.RelativeFilename != sfb.RelativeFilename ||
			sfa.Timestamp != sfb.Timestamp)
			return true;
	}
	return false;
}

//----------------------------------------------------------------------------

bool	UPopcornFXMesh::SourceMeshChanged() const
{
	if (StaticMesh != null && PK_VERIFY(StaticMesh->AssetImportData != null))
	{
		return AssetImportDataDiffers(StaticMeshAssetImportData, StaticMesh->AssetImportData);
	}
#if ENABLE_SKELETALMESH
	const UAssetImportData	*skelMeshImportData = SkeletalMeshAssetImportData(SkeletalMesh);
	if (SkeletalMesh != null && PK_VERIFY(skelMeshImportData != null))
	{
		return AssetImportDataDiffers(StaticMeshAssetImportData, skelMeshImportData);
	}
#endif
	return false;
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
//----------------------------------------------------------------------------

void	UPopcornFXMesh::SetSourceMesh(UStaticMesh *staticMesh, bool dirtifyBuild)
{
	if (dirtifyBuild || staticMesh != StaticMesh || SkeletalMesh != null)
	{
#if WITH_EDITOR
		StaticMeshAssetImportData->SourceData.SourceFiles.Empty();
#endif
		StaticMesh = staticMesh;
		SkeletalMesh = null;
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXMesh::SetSourceMesh(USkeletalMesh *skelMesh, bool dirtifyBuild)
{
	if (dirtifyBuild || skelMesh != SkeletalMesh || StaticMesh != null)
	{
#if WITH_EDITOR
		StaticMeshAssetImportData->SourceData.SourceFiles.Empty();
#endif
		StaticMesh = null;
		SkeletalMesh = skelMesh;
	}
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
bool	UPopcornFXMesh::BuildMeshFromSource()
{
	if (StaticMesh != null)
		return BuildMesh(StaticMesh);
	if (SkeletalMesh != null)
		return BuildMesh(SkeletalMesh);
	return false;
}

//----------------------------------------------------------------------------

namespace
{
	template<typename _IndexType>
	void	_FillSkelMeshBuffers(	const FSkeletalMeshLODRenderData	&LODRenderData,
									PopcornFX::CMeshVStream				&vstream,
									PopcornFX::TMemoryView<float>		boneWeights,
									PopcornFX::TMemoryView<u8>			boneIndices,
									u32									maxInfluenceCount)
	{
		float		* __restrict _boneWeights = boneWeights.Data();
		_IndexType	* __restrict _boneIndices = reinterpret_cast<_IndexType*>(boneIndices.Data());

		const PopcornFX::TStridedMemoryView<CFloat3>	srcPositionsView = vstream.Positions();
		const PopcornFX::TStridedMemoryView<CFloat3>	srcNormalsView = vstream.Normals();
		const PopcornFX::TStridedMemoryView<CFloat3>	srcTangentsView = TStridedMemoryView<CFloat3>::Reinterpret(vstream.Tangents());
		const PopcornFX::TStridedMemoryView<CFloat4>	srcColorsView = vstream.Colors();

		const bool	hasColors = !srcColorsView.Empty();
		const u32	totalVertexCount = LODRenderData.GetNumVertices();
		const u32	totalUVCount = LODRenderData.GetNumTexCoords();

		PK_STACKSTRIDEDMEMORYVIEW(PopcornFX::TStridedMemoryView<CFloat2>, srcUVsView, totalUVCount);
		for (u32 iUV = 0; iUV < totalUVCount; ++iUV)
			srcUVsView[iUV] = vstream.AbstractStream<CFloat2>(PopcornFX::CVStreamSemanticDictionnary::UvStreamToOrdinal(iUV));

		PK_ONLY_IF_ASSERTS(u32 totalVertices = 0);

		const float		invScale = FPopcornFXPlugin::GlobalScaleRcp();
		const u32		sectionCount = LODRenderData.RenderSections.Num();
		for (u32 iSection = 0; iSection < sectionCount; ++iSection)
		{
			PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::FillBuffers (Build section)", POPCORNFX_UE_PROFILER_COLOR);

			const FSkelMeshRenderSection	&section = LODRenderData.RenderSections[iSection];
			PK_ONLY_IF_ASSERTS(const u32 numSectionVertices = section.GetNumVertices());

			const u32	sectionOffset = section.BaseVertexIndex;
			const u32	numVertices = section.GetNumVertices();
			const u32	sectionInfluenceCount = section.MaxBoneInfluences;

			for (u32 iVertex = 0; iVertex < numVertices; ++iVertex)
			{
				const u32	vertexIndex = sectionOffset + iVertex;
				PK_ASSERT(vertexIndex < totalVertexCount);

				srcPositionsView[vertexIndex] = ToPk(LODRenderData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(vertexIndex)) * invScale;
				srcNormalsView[vertexIndex] = ToPk(LODRenderData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(vertexIndex)).xyz();

				{
					const FVector3f	tangent = LODRenderData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentX(vertexIndex);
					srcTangentsView[vertexIndex] = ToPk(tangent);
				}
				PK_RELEASE_ASSERT(!hasColors || vertexIndex < LODRenderData.StaticVertexBuffers.ColorVertexBuffer.GetNumVertices());
				if (hasColors)
					srcColorsView[vertexIndex] = ToPk(LODRenderData.StaticVertexBuffers.ColorVertexBuffer.VertexColor(vertexIndex));
				for (u32 iUV = 0; iUV < totalUVCount; ++iUV)
					srcUVsView[iUV][vertexIndex] = ToPk(LODRenderData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(vertexIndex, iUV));

				PK_ONLY_IF_ASSERTS(float	weightsSum = 0.0f);
				{
					const u32	offsetInfluences = vertexIndex * maxInfluenceCount;
					PK_ASSERT(offsetInfluences + maxInfluenceCount <= boneWeights.Count());
					for (u32 iInfluence = 0; iInfluence < sectionInfluenceCount; ++iInfluence)
					{
						const float	localBoneWeight = LODRenderData.SkinWeightVertexBuffer.GetBoneWeight(vertexIndex, iInfluence);
						const float	boneWeight = localBoneWeight / 255.0f;

						PK_ONLY_IF_ASSERTS(weightsSum += boneWeight);
						_boneWeights[offsetInfluences + iInfluence] = boneWeight;
						PK_ASSERT(_boneWeights[offsetInfluences + iInfluence] >= 0.0f && _boneWeights[offsetInfluences + iInfluence] <= 1.0f);

						const u32	localBoneIndex = LODRenderData.SkinWeightVertexBuffer.GetBoneIndex(vertexIndex, iInfluence);
						_boneIndices[offsetInfluences + iInfluence] = section.BoneMap[localBoneIndex];

						// We can stop copying data, weights are sorted
						if (_boneWeights[offsetInfluences + iInfluence] == 0.0f)
							break;
					}
					PK_ASSERT(weightsSum > 0.0f);
				}
			}
			PK_ONLY_IF_ASSERTS(totalVertices += numSectionVertices);
		}
		PK_ASSERT(totalVertices == totalVertexCount);
	}

	void		_FinalSetupMeshNew(const PopcornFX::PMeshNew &meshNew, bool buildKdTree, bool buildUVToPCoordAccelStructs)
	{
		meshNew->RebuildBBox();
		meshNew->BuildTangentsIFN();
		meshNew->SetupRuntimeStructsIFN(false, buildKdTree);

#if	(PK_GEOMETRICS_BUILD_MESH_SAMPLER_SURFACE != 0)
		meshNew->SetupDefaultSurfaceSamplingAccelStructsIFN(false);
#endif
#if	(PK_GEOMETRICS_BUILD_MESH_SAMPLER_VOLUME != 0)
		meshNew->SetupDefaultVolumeSamplingAccelStructsIFN(false);
#endif
#if	(PK_GEOMETRICS_BUILD_MESH_SAMPLER_SURFACE != 0)
		if (buildUVToPCoordAccelStructs)
		{
			meshNew->SetupDefaultSurfaceSamplingAccelStructsUVIFN(PopcornFX::SMeshUV2PCBuildConfig(), false);
		}
#endif
	}

} // namespace

//----------------------------------------------------------------------------

PopcornFX::PResourceMesh	UPopcornFXMesh::NewResourceMesh(UStaticMesh *staticMesh)
{
	PK_NAMEDSCOPEDPROFILE_C("CResourceHandlerMesh_UE::NewResourceMesh (Static)", POPCORNFX_UE_PROFILER_COLOR);

	const bool		mergeSections = bMergeStaticMeshSections != 0;
	const float		scale = FPopcornFXPlugin::GlobalScaleRcp();

	PopcornFX::PResourceMesh	resMesh = PK_NEW(PopcornFX::CResourceMesh);
	if (!PK_VERIFY(resMesh != null))
		return resMesh;
	FStaticMeshRenderData		*renderData = StaticMeshRenderData(staticMesh);
	if (staticMesh == null || !PK_VERIFY(renderData != null))
		return resMesh;
	if (renderData->LODResources.Num() == 0)
		return resMesh;

	const uint32					lod = PopcornFX::PKMin(uint32(FMath::Max(StaticMeshLOD, 0)), uint32(renderData->LODResources.Num()) - 1);
	const FStaticMeshLODResources	&lodRes = renderData->LODResources[lod];

	if (!PK_VERIFY(lodRes.Sections.Num() != 0))
		return resMesh;

	FIndexArrayView					srcIndices = lodRes.IndexBuffer.GetArrayView();
	const u32						totalIndexCount = u32(srcIndices.Num());

	const u32						totalVertexCount = lodRes.GetNumVertices();
	const FStaticMeshVertexBuffer	&srcVertices = lodRes.VertexBuffers.StaticMeshVertexBuffer;
	const FPositionVertexBuffer		&srcPositions = lodRes.VertexBuffers.PositionVertexBuffer;
	const FColorVertexBuffer		&srcColors = lodRes.VertexBuffers.ColorVertexBuffer;

	PK_ASSERT(srcVertices.GetNumVertices() == totalVertexCount);
	const u32						uvCount = srcVertices.GetNumTexCoords();
	const bool						hasColors = (srcColors.GetNumVertices() == totalVertexCount);

	PopcornFX::SVertexDeclaration	decl;

	decl.AddStreamCodeIFN(PopcornFX::SVStreamCode(
		PopcornFX::CVStreamSemanticDictionnary::Ordinal_Position,
		PopcornFX::SVStreamCode::Element_Float3,
		PopcornFX::SVStreamCode::SIMD_Friendly
		));
	decl.AddStreamCodeIFN(PopcornFX::SVStreamCode(
		PopcornFX::CVStreamSemanticDictionnary::Ordinal_Normal,
		PopcornFX::SVStreamCode::Element_Float3,
		PopcornFX::SVStreamCode::SIMD_Friendly
		));
	decl.AddStreamCodeIFN(PopcornFX::SVStreamCode(
		PopcornFX::CVStreamSemanticDictionnary::Ordinal_Tangent,
		PopcornFX::SVStreamCode::Element_Float4,
		PopcornFX::SVStreamCode::SIMD_Friendly
		));
	for (u32 uvi = 0; uvi < uvCount; ++uvi)
	{
		decl.AddStreamCodeIFN(PopcornFX::SVStreamCode(
			PopcornFX::CVStreamSemanticDictionnary::UvStreamToOrdinal(uvi),
			PopcornFX::SVStreamCode::Element_Float2
			));
	}
	if (hasColors)
	{
		decl.AddStreamCodeIFN(PopcornFX::SVStreamCode(
			PopcornFX::CVStreamSemanticDictionnary::Ordinal_Color,
			PopcornFX::SVStreamCode::Element_Float4,
			PopcornFX::SVStreamCode::SIMD_Friendly
			));
	}

	if (mergeSections)
	{
		PopcornFX::PMeshNew				meshNew = PK_NEW(PopcornFX::CMeshNew);
		if (!PK_VERIFY(meshNew != null))
			return resMesh;

		PopcornFX::CMeshTriangleBatch	&triBatch = meshNew->TriangleBatch();
		PopcornFX::CMeshVStream			&vstream = triBatch.m_VStream;
		PopcornFX::CMeshIStream			&istream = triBatch.m_IStream;

		PK_VERIFY(vstream.Reformat(decl));

		istream.SetPrimitiveType(PopcornFX::CMeshIStream::Triangles);
		istream.Reformat(PopcornFX::CMeshIStream::U32Indices);

		if (!PK_VERIFY(istream.Resize(totalIndexCount)))
			return resMesh;
		PK_ASSERT(istream.IndexByteWidth() == PopcornFX::CMeshIStream::U32Indices);
		u32			*dstIndices = reinterpret_cast<u32*>(istream.RawStreamForWriting());

		if (!PK_VERIFY(vstream.Resize(totalVertexCount)))
			return resMesh;

		// no raw data access, must do one by one
		for (u32 i = 0; i < totalIndexCount; ++i)
		{
			dstIndices[i] = srcIndices[i];
		}

		const TStridedMemoryView<CFloat3>	dstPositions = vstream.Positions();
		PK_RELEASE_ASSERT(!dstPositions.Empty());
		const TStridedMemoryView<CFloat3>	dstNormals = vstream.Normals();
		PK_RELEASE_ASSERT(!dstNormals.Empty());
		PK_RELEASE_ASSERT(PopcornFX::Mem::IsAligned<0x10>(dstNormals.Data()) & PopcornFX::Mem::IsAligned<0x10>(dstNormals.Stride()));
		const TStridedMemoryView<CFloat4>	dstTangents = vstream.Tangents();
		PK_RELEASE_ASSERT(!dstTangents.Empty());
		PK_RELEASE_ASSERT(PopcornFX::Mem::IsAligned<0x10>(dstTangents.Data()) & PopcornFX::Mem::IsAligned<0x10>(dstTangents.Stride()));

		PK_STACKSTRIDEDMEMORYVIEW(TStridedMemoryView<CFloat2>, dstUvs, uvCount);
		for (u32 uvi = 0; uvi < uvCount; ++uvi)
		{
			dstUvs[uvi] = vstream.AbstractStream<CFloat2>(PopcornFX::CVStreamSemanticDictionnary::UvStreamToOrdinal(uvi));
			PK_RELEASE_ASSERT(!dstUvs[uvi].Empty());
		}

		TStridedMemoryView<CFloat4>			dstColors;
		if (hasColors)
		{
			dstColors = vstream.Colors();
			PK_RELEASE_ASSERT(!dstColors.Empty());
		}

		for (u32 i = 0; i < totalVertexCount; ++i)
		{
			const u32	srci = i;
			const u32	dsti = i;

			const FVector3f	&pos = srcPositions.VertexPosition(srci);
			dstPositions[dsti] = ToPk(pos) * scale;

			const FVector3f	&normal = srcVertices.VertexTangentZ(srci);
			const FVector3f	&tangent = srcVertices.VertexTangentX(srci);

			PK_TODO("Optim: get the TangentZ.w to put in our tangent.w");
			// Ugly recontruct binormal sign to put in tangent.w
			// New 4.12 tangent basis precision hides TangetZ.w behind more stuff
			// Should use: if (GetUseHighPrecisionTangentBasis()) { ... TangentZ.w ... } else { .... TangentZ.w .... }
			const FVector3f	&finalBinormal = srcVertices.VertexTangentY(srci);
			const FVector3f	baseBinormal = normal ^ tangent;
			const float		biNormalSign =  FVector3f::DotProduct(baseBinormal, finalBinormal) < 0.0 ? -1.0f : 1.0f;

			dstNormals[dsti] = ToPk(normal);
			dstTangents[dsti] = CFloat4(ToPk(tangent), biNormalSign);

			for (u32 uvi = 0; uvi < uvCount; ++uvi)
			{
				const FVector2f		&uv = srcVertices.GetVertexUV(srci, uvi);
				dstUvs[uvi][dsti] = ToPk(uv);
			}

			if (!dstColors.Empty())
			{
				const FLinearColor	color = srcColors.VertexColor(srci).ReinterpretAsLinear();
				dstColors[dsti] = ToPk(color);
			}
		}

		_FinalSetupMeshNew(meshNew, true, bBuildUVToPCoordAccelStructs);

		if (!PK_VERIFY(resMesh->AddBatch("mat", meshNew).Valid()))
			return resMesh;
	}
	else
	{
		PopcornFX::TSemiDynamicArray<u32, 256>		vertexRemapArray;
		if (!PK_VERIFY(vertexRemapArray.Resize(totalVertexCount)))
			return resMesh;
		u32				*vertexRemap = vertexRemapArray.RawDataPointer();

		for (u32 sectioni = 0; sectioni < u32(lodRes.Sections.Num()); ++sectioni)
		{
			const FStaticMeshSection		&section = lodRes.Sections[sectioni];

			PopcornFX::PMeshNew				meshNew = PK_NEW(PopcornFX::CMeshNew);
			if (!PK_VERIFY(meshNew != null))
				continue;

			PopcornFX::CMeshTriangleBatch	&triBatch = meshNew->TriangleBatch();
			PopcornFX::CMeshVStream			&vstream = triBatch.m_VStream;
			PopcornFX::CMeshIStream			&istream = triBatch.m_IStream;

			PK_VERIFY(vstream.Reformat(decl));

			istream.SetPrimitiveType(PopcornFX::CMeshIStream::Triangles);
			istream.Reformat(PopcornFX::CMeshIStream::U32Indices);

			const u32			indexCount = section.NumTriangles * 3;
			const u32			srcIndexStart = section.FirstIndex;
			if (!PK_VERIFY(istream.Resize(indexCount)))
				continue;
			PK_ASSERT(istream.IndexByteWidth() == PopcornFX::CMeshIStream::U32Indices);
			u32					*dstIndices = reinterpret_cast<u32*>(istream.RawStreamForWriting());

			if (!PK_VERIFY(vstream.Resize(totalVertexCount))) //
				continue;

			const TStridedMemoryView<CFloat3>	dstPositions = vstream.Positions();
			PK_RELEASE_ASSERT(!dstPositions.Empty());
			const TStridedMemoryView<CFloat3>	dstNormals = vstream.Normals();
			PK_RELEASE_ASSERT(!dstNormals.Empty());
			PK_RELEASE_ASSERT(PopcornFX::Mem::IsAligned<0x10>(dstNormals.Data()) & PopcornFX::Mem::IsAligned<0x10>(dstNormals.Stride()));
			const TStridedMemoryView<CFloat4>	dstTangents = vstream.Tangents();
			PK_RELEASE_ASSERT(!dstTangents.Empty());
			PK_RELEASE_ASSERT(PopcornFX::Mem::IsAligned<0x10>(dstTangents.Data()) & PopcornFX::Mem::IsAligned<0x10>(dstTangents.Stride()));

			PK_STACKSTRIDEDMEMORYVIEW(TStridedMemoryView<CFloat2>, dstUvs, uvCount);
			for (u32 uvi = 0; uvi < uvCount; ++uvi)
			{
				dstUvs[uvi] = vstream.AbstractStream<CFloat2>(PopcornFX::CVStreamSemanticDictionnary::UvStreamToOrdinal(uvi));
				PK_ASSERT(!dstUvs[uvi].Empty());
			}

			TStridedMemoryView<CFloat4>			dstColors;
			if (hasColors)
			{
				dstColors = vstream.Colors();
				PK_ASSERT(!dstColors.Empty());
			}

			u32					dstVertexCount = 0;

			PopcornFX::Mem::Clear(vertexRemap, sizeof(*vertexRemap) * totalVertexCount);
			for (u32 indexi = 0; indexi < indexCount; ++indexi)
			{
				const u32	srcVertex = srcIndices[srcIndexStart + indexi];

				u32			dstVertex;
				if (vertexRemap[srcVertex] == 0)
				{
					const u32	srci = srcVertex;
					const u32	dsti = dstVertexCount;

					const FVector3f	&pos = srcPositions.VertexPosition(srci);
					dstPositions[dsti] = ToPk(pos) * scale;

					const FVector3f	&normal = srcVertices.VertexTangentZ(srci);
					const FVector3f	&tangent = srcVertices.VertexTangentX(srci);

					PK_TODO("Optim: get the TangentZ.w to put in our tangent.w");
					// Ugly recontruct binormal sign to put in tangent.w
					// New 4.12 tangent basis precision hides TangetZ.w behind more stuff
					// Should use: if (GetUseHighPrecisionTangentBasis()) { ... TangentZ.w ... } else { .... TangentZ.w .... }
					const FVector3f	&finalBinormal = srcVertices.VertexTangentY(srci);
					const FVector3f	baseBinormal = normal ^ tangent;
					const float		biNormalSign =  FVector3f::DotProduct(baseBinormal, finalBinormal) < 0.0 ? -1.0f : 1.0f;

					dstNormals[dsti] = ToPk(normal);
					dstTangents[dsti] = CFloat4(ToPk(tangent), biNormalSign);

					for (u32 uvi = 0; uvi < uvCount; ++uvi)
					{
						const FVector2f		&uv = srcVertices.GetVertexUV(srci, uvi);
						dstUvs[uvi][dsti] = ToPk(uv);
					}

					if (!dstColors.Empty())
					{
						const FLinearColor	color = srcColors.VertexColor(srci).ReinterpretAsLinear();
						dstColors[dsti] = ToPk(color);
					}

					vertexRemap[srcVertex] = dstVertexCount + 1;
					dstVertex = dstVertexCount;
					++dstVertexCount;
				}
				else
				{
					dstVertex = vertexRemap[srcVertex] - 1;
				}

				dstIndices[indexi] = dstVertex;
			}

			PK_VERIFY(vstream.ExactResize(dstVertexCount)); // shrink only

			_FinalSetupMeshNew(meshNew, true, bBuildUVToPCoordAccelStructs);

			if (!PK_VERIFY(resMesh->AddBatch("mat", meshNew).Valid()))
				continue;
		}
	}

	return resMesh;
}

//----------------------------------------------------------------------------

PopcornFX::PResourceMesh	UPopcornFXMesh::NewResourceMesh(USkeletalMesh *skeletalMesh)
{
	PK_NAMEDSCOPEDPROFILE_C("CResourceHandlerMesh_UE::NewResourceMesh (Skeletal)", POPCORNFX_UE_PROFILER_COLOR);

	PopcornFX::PResourceMesh	resMesh = PK_NEW(PopcornFX::CResourceMesh);
	if (!PK_VERIFY(resMesh != null))
		return resMesh;
	if (skeletalMesh == null)
		return resMesh;

	const FSkeletalMeshRenderData	*skelMeshRenderData = skeletalMesh->GetResourceForRendering();
	if (!PK_VERIFY(skelMeshRenderData != null && skelMeshRenderData->LODRenderData.Num() > 0))
		return resMesh;

	const FSkeletalMeshLODRenderData	&LODRenderData = skelMeshRenderData->LODRenderData[0];
	const u32							totalVertexCount = LODRenderData.GetNumVertices();
	const u32							totalUVCount = LODRenderData.GetNumTexCoords();
	if (!PK_VERIFY(LODRenderData.RequiredBones.Num() > 0) ||
		!PK_VERIFY(totalVertexCount > 0))
		return resMesh;

	const u32	totalBoneCount = LODRenderData.RequiredBones.Num();
	if (!PK_VERIFY(totalBoneCount > 0))
		return resMesh;

	PopcornFX::PMeshNew		meshNew = PK_NEW(PopcornFX::CMeshNew);
	if (!PK_VERIFY(meshNew != null))
		return resMesh;
	PopcornFX::CMeshTriangleBatch	&triBatch = meshNew->TriangleBatch();
	PopcornFX::CMeshVStream			&vstream = triBatch.m_VStream;
	PopcornFX::CMeshIStream			&istream = triBatch.m_IStream;

	{
		PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::BuildMeshIndices", POPCORNFX_UE_PROFILER_COLOR);

		const FRawStaticIndexBuffer16or32Interface	*iBuffer = LODRenderData.MultiSizeIndexContainer.GetIndexBuffer();
		const u32									totalIndexCount = iBuffer->Num();
		if (!PK_VERIFY(totalIndexCount > 0))
			return resMesh;

		const u32	indexSizeInBytes = iBuffer->GetResourceDataSize() / totalIndexCount;
		PK_ASSERT(indexSizeInBytes == sizeof(u16) || indexSizeInBytes == sizeof(u32));

		istream.SetPrimitiveType(PopcornFX::CMeshIStream::Triangles);
		istream.Reformat((PopcornFX::CMeshIStream::EFormat)indexSizeInBytes);
		if (!PK_VERIFY(istream.Resize(totalIndexCount)))
			return resMesh;

		// .. The interface doesn't expose a const GetPointerTo() method
		// - We do this to workaround the extremely expensive MultiSizeIndexContainer.GetIndexBuffer() -
		FRawStaticIndexBuffer16or32Interface	*bufferForReading = const_cast<FRawStaticIndexBuffer16or32Interface*>(iBuffer);

		PK_ASSERT(istream.IndexByteWidth() == indexSizeInBytes);
		PopcornFX::Mem::Copy(istream.RawStreamForWriting(), bufferForReading->GetPointerTo(0), totalIndexCount * indexSizeInBytes);
	}
	PopcornFX::SVertexDeclaration	decl;

	decl.AddStreamCodeIFN(PopcornFX::SVStreamCode(PopcornFX::CVStreamSemanticDictionnary::Ordinal_Position, PopcornFX::SVStreamCode::Element_Float3, PopcornFX::SVStreamCode::SIMD_Friendly));
	decl.AddStreamCodeIFN(PopcornFX::SVStreamCode(PopcornFX::CVStreamSemanticDictionnary::Ordinal_Normal, PopcornFX::SVStreamCode::Element_Float3, PopcornFX::SVStreamCode::SIMD_Friendly | PopcornFX::SVStreamCode::Normalized));
	decl.AddStreamCodeIFN(PopcornFX::SVStreamCode(PopcornFX::CVStreamSemanticDictionnary::Ordinal_Tangent, PopcornFX::SVStreamCode::Element_Float4, PopcornFX::SVStreamCode::SIMD_Friendly | PopcornFX::SVStreamCode::Normalized));
	for (u32 iUV = 0; iUV < totalUVCount; ++iUV)
		decl.AddStreamCodeIFN(PopcornFX::SVStreamCode(PopcornFX::CVStreamSemanticDictionnary::UvStreamToOrdinal(iUV), PopcornFX::SVStreamCode::Element_Float2));

	const bool	hasColors = LODRenderData.StaticVertexBuffers.ColorVertexBuffer.GetNumVertices() == totalVertexCount;
	if (hasColors)
		decl.AddStreamCodeIFN(PopcornFX::SVStreamCode(PopcornFX::CVStreamSemanticDictionnary::Ordinal_Color, PopcornFX::SVStreamCode::Element_Float4, PopcornFX::SVStreamCode::SIMD_Friendly));

	PK_VERIFY(vstream.Reformat(decl));
	if (!PK_VERIFY(vstream.Resize(totalVertexCount)))
		return resMesh;

	const bool		smallBoneIndices = totalBoneCount <= 256;
	const u32		sectionCount = LODRenderData.RenderSections.Num();
	u32				maxInfluenceCount = 0;

	PopcornFX::TArray<float, PopcornFX::TArrayAligned16>	boneWeights;
	PopcornFX::TArray<u8, PopcornFX::TArrayAligned16>		boneIndices;

	{
		PK_NAMEDSCOPEDPROFILE_C("AttributeSamplerSkinnedMesh::Alloc skinning streams", POPCORNFX_UE_PROFILER_COLOR);

		for (u32 iSection = 0; iSection < sectionCount; ++iSection)
		{
			const FSkelMeshRenderSection	&section = LODRenderData.RenderSections[iSection];
			PK_ASSERT(section.MaxBoneInfluences > 0);
			if (section.MaxBoneInfluences > static_cast<int32>(maxInfluenceCount))
				maxInfluenceCount = section.MaxBoneInfluences;
		}

		const u32	totalDataCount = maxInfluenceCount * totalVertexCount;
		// Allocate necessary space for bone weights/indices
		if (!PK_VERIFY(totalDataCount > 0))
			return resMesh;
		const u32	boneRealDataCount = totalDataCount * (smallBoneIndices ? 1 : 2);
		if (!PK_VERIFY(boneWeights.Resize(totalDataCount)) ||
			!PK_VERIFY(boneIndices.Resize(boneRealDataCount)))
			return resMesh;
		PK_ASSERT(boneWeights.Count() == totalDataCount);
		PK_ASSERT(!boneWeights.Empty());
		PopcornFX::Mem::Clear(boneWeights.RawDataPointer(), sizeof(float) * totalDataCount);
		PopcornFX::Mem::Clear(boneIndices.RawDataPointer(), sizeof(u8) * boneRealDataCount);
	}

	PopcornFX::CBaseSkinningStreams	*skinningStreams = null;
	if (smallBoneIndices)
	{
		_FillSkelMeshBuffers<u8>(LODRenderData, vstream, boneWeights, boneIndices, maxInfluenceCount);
		skinningStreams = PopcornFX::CBaseSkinningStreams::BuildFromUnpackedStreams(totalVertexCount, boneWeights, boneIndices);
	}
	else
	{
		_FillSkelMeshBuffers<u16>(LODRenderData, vstream, boneWeights, boneIndices, maxInfluenceCount);

		PopcornFX::TMemoryView<const u16>	view(reinterpret_cast<const u16*>(boneIndices.RawDataPointer()), boneIndices.Count() / 2);
		skinningStreams = PopcornFX::CBaseSkinningStreams::BuildFromUnpackedStreams(totalVertexCount, boneWeights, view);
	}

	_FinalSetupMeshNew(meshNew, false, bBuildUVToPCoordAccelStructs); // we don't need kd tree for skeletal meshes

	if (!PK_VERIFY(skinningStreams != null) ||
		!PK_VERIFY(resMesh->AddBatch("SkinnedMeshMerged", meshNew).Valid()))
		return resMesh;
	PK_ASSERT(resMesh->BatchList().Count() == 1);
	PK_ASSERT(resMesh->BatchList().First() != null);
	resMesh->BatchList().First()->SetSkinningStreamsAndStealOwnership(skinningStreams);

	return resMesh;
}

//----------------------------------------------------------------------------

template <typename _AssetType>
bool	UPopcornFXMesh::BuildMesh(_AssetType *mesh)
{
	PK_NAMEDSCOPEDPROFILE_C("CResourceHandlerMesh_UE::BuildMesh", POPCORNFX_UE_PROFILER_COLOR);

	if (m_ResourceMesh == null)
	{
		m_ResourceMesh = PK_NEW(PopcornFX::CResourceMesh);
		if (!PK_VERIFY(m_ResourceMesh != null))
			return false;
	}

	PK_ASSERT(mesh != null);
	PopcornFX::PResourceMesh		newResource = NewResourceMesh(mesh);
	if (!PK_VERIFY(newResource != null))
		return false;

	m_ResourceMesh->m_OnReloading(m_ResourceMesh.Get());

	m_ResourceMesh->Swap(*newResource);

#if WITH_EDITOR
	const USkeletalMesh		*skelMesh = Cast<USkeletalMesh>(mesh);
	const UStaticMesh		*staticMesh = Cast<UStaticMesh>(mesh);

	const UAssetImportData	*assetImportData = skelMesh != null ? SkeletalMeshAssetImportData(skelMesh) : staticMesh->AssetImportData;
	const bool				validStaticMesh = assetImportData != null;
	if (validStaticMesh)
		StaticMeshAssetImportData->SourceData.SourceFiles = assetImportData->SourceData.SourceFiles;
	else
		StaticMeshAssetImportData->SourceData.SourceFiles.Empty();

	WriteMesh();
#endif // WITH_EDITOR

	m_ResourceMesh->m_OnReloaded(m_ResourceMesh.Get());

	return true;
}

//----------------------------------------------------------------------------

void	UPopcornFXMesh::WriteMesh()
{
	PK_NAMEDSCOPEDPROFILE_C("CResourceHandlerMesh_UE::WriteMesh", POPCORNFX_UE_PROFILER_COLOR);

#if WITH_EDITOR
	Modify(true);
	ForceSetPackageDirty(this);
#endif // WITH_EDITOR

	if (m_ResourceMesh != null)
	{
		PopcornFX::SMeshWriteSettings	writeSettings;
		const PopcornFX::CString		pkPath = PkPath(); // char* -> CString

		bool	success = false;
		if (!pkPath.Empty())
		{
			PopcornFX::CMessageStream	outReport(true);	// true: auto-dump to CLog::Log()
			success = m_ResourceMesh->WriteToFile(PopcornFX::File::DefaultFileSystem(), pkPath, outReport, &writeSettings, FPopcornFXPlugin::Get().FilePack());
		}

		if (!success && !IsRunningCommandlet()) // We don't want this error to fail the packaging
		{
			FText	title = LOCTEXT("mesh_write_fail", "PopcornFX: Failed to serialize PopcornFX mesh");
			FText	finalText = FText::FromString(FString::Printf(TEXT("Couldn't store PopcornFX mesh in '%s'"), *GetPathName()));

			FMessageDialog::Open(EAppMsgType::Ok, finalText, &title);
		}
	}
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
#if WITH_EDITOR
//----------------------------------------------------------------------------

bool	UPopcornFXMesh::_ImportFile(const FString &filePath)
{
	return false;
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
//----------------------------------------------------------------------------

void	UPopcornFXMesh::OnFileUnload()
{
}

//----------------------------------------------------------------------------

void	UPopcornFXMesh::OnFileLoad()
{
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
