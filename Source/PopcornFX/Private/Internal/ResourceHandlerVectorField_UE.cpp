//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "ResourceHandlerVectorField_UE.h"

#include "PopcornFXPlugin.h"
#include "VectorField/VectorFieldStatic.h"

#include <pk_maths/include/pk_maths.h>
#include <pk_kernel/include/kr_base_types.h>
#include <pk_kernel/include/kr_refcounted_buffer.h>
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_maths/include/pk_maths_fp16.h>

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXResourceHandlerVectorField, Log, All);

#define LOCTEXT_NAMESPACE "PopcornFXVectorFieldHandler"

//----------------------------------------------------------------------------
//
// CResourceHandlerVectorField_UE
//
//----------------------------------------------------------------------------

using PopcornFX:: f16;

namespace
{
	CResourceHandlerVectorField_UE		*g_ResourceHandlerVectorField_UE = null;
}

//----------------------------------------------------------------------------

// static
void	CResourceHandlerVectorField_UE::Startup()
{
	PK_ASSERT(g_ResourceHandlerVectorField_UE == null);
	g_ResourceHandlerVectorField_UE = PK_NEW(CResourceHandlerVectorField_UE);
	if (!PK_VERIFY(g_ResourceHandlerVectorField_UE != null))
		return;

	PopcornFX::Resource::DefaultManager()->RegisterHandler<PopcornFX::CVectorField>(g_ResourceHandlerVectorField_UE);
}

//----------------------------------------------------------------------------

// static
void	CResourceHandlerVectorField_UE::Shutdown()
{
	if (PK_VERIFY(g_ResourceHandlerVectorField_UE != null))
	{
		PopcornFX::Resource::DefaultManager()->UnregisterHandler<PopcornFX::CVectorField>(g_ResourceHandlerVectorField_UE);
		PK_SAFE_DELETE(g_ResourceHandlerVectorField_UE);
	}
}

//----------------------------------------------------------------------------


CResourceHandlerVectorField_UE::CResourceHandlerVectorField_UE()
{
}

//----------------------------------------------------------------------------

CResourceHandlerVectorField_UE::~CResourceHandlerVectorField_UE()
{
}

//----------------------------------------------------------------------------

PopcornFX::CVectorField	*CResourceHandlerVectorField_UE::NewFromVectorField(UVectorFieldStatic *vectorFieldStatic)
{
	PK_NAMEDSCOPEDPROFILE_C("CResourceHandlerVectorField_UE::NewFromVectorField", POPCORNFX_UE_PROFILER_COLOR);

	const u32	vectorCount = vectorFieldStatic->SizeX * vectorFieldStatic->SizeY * vectorFieldStatic->SizeZ;
	const u32	scalarCount = vectorCount * 3;

	PK_ASSERT(vectorCount > 0);
	PK_ASSERT(scalarCount >= 3);
	PopcornFX::PRefCountedMemoryBuffer	fp16Values = PopcornFX::CRefCountedMemoryBuffer::AllocAligned(scalarCount * sizeof(f16), PopcornFX::Memory::CacheLineSize);
	if (!PK_VERIFY(fp16Values != null))
		return null;

	// Copy the source data
	{
		f16			*dstValues = fp16Values->Data<f16>();
		const f16	*srcValues = reinterpret_cast<f16*>(vectorFieldStatic->SourceData.Lock(LOCK_READ_ONLY));
		if (!PK_VERIFY(srcValues != null))
			return null;

		for (u32 iScalar = 0; iScalar < scalarCount; iScalar += 3)
		{
			*dstValues++ = *srcValues++;
			*dstValues++ = *srcValues++;
			*dstValues++ = *srcValues++;
			++srcValues;
		}
		vectorFieldStatic->SourceData.Unlock();
	}

	PopcornFX::CVectorField	*vectorField = PK_NEW(PopcornFX::CVectorField);
	if (!PK_VERIFY(vectorField != null))
		return null;
	vectorField->m_Data = fp16Values;
	vectorField->m_DataType = PopcornFX::VFDataType_Fp16;
	vectorField->m_Dimensions = CUint4(vectorFieldStatic->SizeX, vectorFieldStatic->SizeY, vectorFieldStatic->SizeZ, 1);
	vectorField->m_BoundsMin = CFloat4(ToPk(vectorFieldStatic->Bounds.Min), 0.0f);
	vectorField->m_BoundsMax = CFloat4(ToPk(vectorFieldStatic->Bounds.Max), 1.0f);
	vectorField->m_IntensityMultiplier = PopcornFX::PKMax(vectorFieldStatic->Intensity, 0.0f);
	return vectorField;
}

//----------------------------------------------------------------------------

PopcornFX::CVectorField	*CResourceHandlerVectorField_UE::NewFromPath(const PopcornFX::CString &path, bool pathNotVirtual)
{
	PK_NAMEDSCOPEDPROFILE_C("CResourceHandlerVectorField_UE::NewFromPath", POPCORNFX_UE_PROFILER_COLOR);

	UObject			*obj = FPopcornFXPlugin::Get().LoadUObjectFromPkPath(path, pathNotVirtual);
	if (obj == null)
	{
		UE_LOG(LogPopcornFXResourceHandlerVectorField, Warning, TEXT("UObject not found \"%s\" %d"), *ToUE(path), pathNotVirtual);
		return null;
	}
	UVectorFieldStatic		*vectorFieldStatic = Cast<UVectorFieldStatic>(obj);
	if (!PK_VERIFY(vectorFieldStatic != null))
	{
		UE_LOG(LogPopcornFXResourceHandlerVectorField, Warning, TEXT("UObject is not a UVectorFieldStatic \"%s\""), *obj->GetPathName());
		return null;
	}
	PopcornFX::CVectorField	*vectorField = NewFromVectorField(vectorFieldStatic);
	if (!PK_VERIFY(vectorField != null))
	{
		UE_LOG(LogPopcornFXResourceHandlerVectorField, Warning, TEXT("Failed to create vector field for PopcornFX \"%s\""), *vectorFieldStatic->GetPathName());
		return null;
	}
	return vectorField;
}

//----------------------------------------------------------------------------

void	*CResourceHandlerVectorField_UE::Load(
	const PopcornFX::CResourceManager	*resourceManager,
	u32									resourceTypeID,
	const PopcornFX::CString			&resourcePath,
	bool								pathNotVirtual,
	const PopcornFX::SResourceLoadCtl	&loadCtl,
	PopcornFX::CMessageStream			&loadReport,
	SAsyncLoadStatus					*asyncLoadStatus)
{
	PK_NAMEDSCOPEDPROFILE_C("CResourceHandlerVectorField_UE::Load", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CVectorField>::ResourceTypeID());

	PopcornFX::CString			_fullPath = resourcePath;
	if (!pathNotVirtual) // if virtual path
		_fullPath = ToPk(FPopcornFXPlugin::Get().BuildPathFromPkPath(_fullPath, true)); // prepend Pack Path

	PopcornFX::CFilePath::StripExtensionInPlace(_fullPath);
	const PopcornFX::CString	&fullPath = _fullPath;
	if (!/*PK_VERIFY*/(!fullPath.Empty()))
	{
		if (asyncLoadStatus != null)
		{
			asyncLoadStatus->m_Resource = null;
			asyncLoadStatus->m_Done = true;
			asyncLoadStatus->m_Progress = 1.0f;
		}
		return null;
	}

	if (!loadCtl.m_Reload)
	{
		PK_SCOPEDLOCK(m_Lock);
		SResourceEntry	*existingEntry = m_VectorFields.Find(fullPath);
		if (existingEntry != null)
		{
			if (asyncLoadStatus != null)
			{
				asyncLoadStatus->m_Resource = null;
				asyncLoadStatus->m_Done = true;
				asyncLoadStatus->m_Progress = 1.0f;
			}
			++existingEntry->m_ReferenceCount;
			return existingEntry->m_VectorField.Get();
		}
	}

	PopcornFX::PVectorField	resource = NewFromPath(fullPath, true);
	if (resource != null)
	{
		PK_SCOPEDLOCK(m_Lock);
		SResourceEntry		*entry = m_VectorFields.Find(fullPath);
		if (entry == null)
		{
			entry = m_VectorFields.Insert(fullPath, SResourceEntry(resource, 1));
			if (!PK_VERIFY(entry != null))
				resource = null;
		}
		else
		{
			++entry->m_ReferenceCount;

			PK_ASSERT(entry->m_VectorField != null);
			PopcornFX::CVectorField	*dstResource = entry->m_VectorField.Get();

			dstResource->m_OnReloading(dstResource);
			dstResource->Swap(*resource);
			dstResource->m_OnReloaded(dstResource);

			resource = dstResource;
		}
	}

	if (asyncLoadStatus != null)
	{
		asyncLoadStatus->m_Resource = resource.Get();
		asyncLoadStatus->m_Done = true;
		asyncLoadStatus->m_Progress = 1.0f;
	}

	return resource.Get();
}

//----------------------------------------------------------------------------

void	*CResourceHandlerVectorField_UE::Load(
		const PopcornFX::CResourceManager	*resourceManager,
		u32									resourceTypeID,
		const PopcornFX::CFilePackPath		&resourcePath,
		const PopcornFX::SResourceLoadCtl	&loadCtl,
		PopcornFX::CMessageStream			&loadReport,
		SAsyncLoadStatus					*asyncLoadStatus)
{
	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CVectorField>::ResourceTypeID());

	return Load(resourceManager, resourceTypeID, resourcePath.Path(), false, loadCtl, loadReport, asyncLoadStatus);
}

//----------------------------------------------------------------------------

void	CResourceHandlerVectorField_UE::Unload(
		const PopcornFX::CResourceManager	*resourceManager,
		u32									resourceTypeID,	// used to check we are called with the correct type
		void								*rawResource)
{
	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CVectorField>::ResourceTypeID());

	PopcornFX::PVectorField	resource = static_cast<PopcornFX::CVectorField*>(rawResource);

	{
		PK_SCOPEDLOCK(m_Lock);
		PopcornFX::THashMap<SResourceEntry, PopcornFX::CString>::Iterator	it = m_VectorFields.Begin();
		PopcornFX::THashMap<SResourceEntry, PopcornFX::CString>::Iterator	end = m_VectorFields.End();

		u32	refsLeft = 0;
		PopcornFX::CString	fullPath;
		while (it != end)
		{
			if (it->m_VectorField == resource)
			{
				fullPath = it.Key();
				refsLeft = --(it->m_ReferenceCount);
				break;
			}
			++it;
		}

		if (!fullPath.Empty() && refsLeft == 0)
		{
			m_VectorFields.Remove(fullPath);
		}
	}
}

//----------------------------------------------------------------------------

void	CResourceHandlerVectorField_UE::AppendDependencies(
	const PopcornFX::CResourceManager			*resourceManager,
	u32											resourceTypeID,
	void										*resource,
	PopcornFX::TArray<PopcornFX::CString>		&outResourcePaths) const
{
	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CVectorField>::ResourceTypeID());
}

//----------------------------------------------------------------------------

void	CResourceHandlerVectorField_UE::AppendDependencies(
		const PopcornFX::CResourceManager		*resourceManager,
		u32										resourceTypeID,
		const PopcornFX::CString				&resourcePath,
		bool									pathNotVirtual,
		PopcornFX::TArray<PopcornFX::CString>	&outResourcePaths) const
{
	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CVectorField>::ResourceTypeID());

}

//----------------------------------------------------------------------------

void	CResourceHandlerVectorField_UE::AppendDependencies(
	const PopcornFX::CResourceManager		*resourceManager,
	u32										resourceTypeID,
	const PopcornFX::CFilePackPath			&resourcePath,
	PopcornFX::TArray<PopcornFX::CString>	&outResourcePaths) const
{
	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CVectorField>::ResourceTypeID());
}

//----------------------------------------------------------------------------

void	CResourceHandlerVectorField_UE::BroadcastResourceChanged(const PopcornFX::CResourceManager *resourceManager, const PopcornFX::CFilePackPath &resourcePath)
{
	PK_NAMEDSCOPEDPROFILE_C("CResourceHandlerVectorField_UE::BroadcastResourceChanged", POPCORNFX_UE_PROFILER_COLOR);

	const PopcornFX::CString		fullPath = PopcornFX::CFilePath::StripExtension(resourcePath.FullPath());
	{
		PK_SCOPEDLOCK(m_Lock);
		SResourceEntry	*foundResource = m_VectorFields.Find(fullPath);
		if (foundResource == null || !PK_VERIFY(foundResource->m_VectorField != null))
			return;
	}

	PopcornFX::PVectorField	resource = NewFromPath(fullPath, true);
	if (resource != null)
	{
		PK_SCOPEDLOCK(m_Lock);
		SResourceEntry	*foundResource = m_VectorFields.Find(fullPath);
		if (foundResource != null)	// could have been Unload-ed in the meantime
		{
			// replace the entry in the resource-list
			PK_ASSERT(foundResource->m_VectorField.Get() != null);
			PopcornFX::CVectorField		*dstResource = foundResource->m_VectorField.Get();

			dstResource->m_OnReloading(dstResource);
			dstResource->Swap(*resource);
			dstResource->m_OnReloaded(dstResource);
		}
	}
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
