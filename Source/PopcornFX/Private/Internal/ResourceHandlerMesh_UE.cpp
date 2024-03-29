//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "ResourceHandlerMesh_UE.h"

#include "PopcornFXPlugin.h"
#include "Assets/PopcornFXStaticMeshData.h"

#include "Engine/StaticMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXResourceHandlerMesh, Log, All);

#define LOCTEXT_NAMESPACE "PopcornFXMeshHandler"

//----------------------------------------------------------------------------
//
// CResourceHandlerMesh_UE
//
//----------------------------------------------------------------------------

namespace
{
	CResourceHandlerMesh_UE		*g_ResourceHandlerMesh_UE = null;
}

//----------------------------------------------------------------------------

// static
void	CResourceHandlerMesh_UE::Startup()
{
	PK_ASSERT(g_ResourceHandlerMesh_UE == null);
	g_ResourceHandlerMesh_UE = PK_NEW(CResourceHandlerMesh_UE);
	if (!PK_VERIFY(g_ResourceHandlerMesh_UE != null))
		return;

	PopcornFX::Resource::DefaultManager()->RegisterHandler<PopcornFX::CResourceMesh>(g_ResourceHandlerMesh_UE);
}

//----------------------------------------------------------------------------

// static
void	CResourceHandlerMesh_UE::Shutdown()
{
	if (PK_VERIFY(g_ResourceHandlerMesh_UE != null))
	{
		PopcornFX::Resource::DefaultManager()->UnregisterHandler<PopcornFX::CResourceMesh>(g_ResourceHandlerMesh_UE);
		PK_DELETE(g_ResourceHandlerMesh_UE);
		g_ResourceHandlerMesh_UE = null;
	}
}

//----------------------------------------------------------------------------


CResourceHandlerMesh_UE::CResourceHandlerMesh_UE()
{
}

//----------------------------------------------------------------------------

CResourceHandlerMesh_UE::~CResourceHandlerMesh_UE()
{
}

//----------------------------------------------------------------------------

void	*CResourceHandlerMesh_UE::Load(
	const PopcornFX::CResourceManager	*resourceManager,
	u32									resourceTypeID,	// used to check we are called with the correct type
	const PopcornFX::CString			&resourcePath,
	bool								pathNotVirtual,
	const PopcornFX::SResourceLoadCtl	&loadCtl,
	PopcornFX::CMessageStream			&loadReport,
	SAsyncLoadStatus					*asyncLoadStatus)	// if null, will be a synchronous load, if not, the async loader will update the structure to tell the progress
{
	PK_NAMEDSCOPEDPROFILE_C("CResourceHandlerMesh_UE::Load", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CResourceMesh>::ResourceTypeID());

	UObject			*obj = FPopcornFXPlugin::Get().LoadUObjectFromPkPath(resourcePath, pathNotVirtual);
	if (obj == null)
	{
		UE_LOG(LogPopcornFXResourceHandlerMesh, Warning, TEXT("UObject not found \"%s\""), *ToUE(resourcePath));
		return null;
	}

	UStaticMesh		*staticMesh = Cast<UStaticMesh>(obj);
	if (staticMesh == null)
	{
		UE_LOG(LogPopcornFXResourceHandlerMesh, Warning, TEXT("UObject is not a UStaticMesh \"%s\""), *obj->GetPathName());
		return null;
	}

	UPopcornFXMesh	*pkMesh = UPopcornFXMesh::FindStaticMesh(staticMesh);
	if (pkMesh == null)
	{
		UE_LOG(LogPopcornFXResourceHandlerMesh, Warning, TEXT("Failed to load StaticMesh for PopcornFX \"%s\""), *staticMesh->GetPathName());
		return null;
	}

	void				*resourcePtr = pkMesh->LoadResourceMeshIFN(true).Get();
	if (asyncLoadStatus != null)
	{
		asyncLoadStatus->m_Resource = resourcePtr;
		asyncLoadStatus->m_Done = true;
		asyncLoadStatus->m_Progress = 1.0f;
	}
	return resourcePtr;
}

//----------------------------------------------------------------------------

void	*CResourceHandlerMesh_UE::Load(
	const PopcornFX::CResourceManager	*resourceManager,
	u32									resourceTypeID,	// used to check we are called with the correct type
	const PopcornFX::CFilePackPath		&resourcePath,
	const PopcornFX::SResourceLoadCtl	&loadCtl,
	PopcornFX::CMessageStream			&loadReport,
	SAsyncLoadStatus					*asyncLoadStatus)	// if null, will be a synchronous load, if not, the async loader will update the structure to tell the progress
{
	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CResourceMesh>::ResourceTypeID());
	return Load(resourceManager, resourceTypeID, resourcePath.Path(), false, loadCtl, loadReport, asyncLoadStatus);
}

//----------------------------------------------------------------------------

void	CResourceHandlerMesh_UE::Unload(
	const PopcornFX::CResourceManager	*resourceManager,
	u32									resourceTypeID,	// used to check we are called with the correct type
	void								*resource)
{
	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CResourceMesh>::ResourceTypeID());
}

//----------------------------------------------------------------------------

void	CResourceHandlerMesh_UE::AppendDependencies(
	const PopcornFX::CResourceManager		*resourceManager,
	u32										resourceTypeID,
	void									*resource,
	PopcornFX::TArray<PopcornFX::CString>	&outResourcePaths) const
{
	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CResourceMesh>::ResourceTypeID());
}

//----------------------------------------------------------------------------

void	CResourceHandlerMesh_UE::AppendDependencies(
	const PopcornFX::CResourceManager		*resourceManager,
	u32										resourceTypeID,
	const PopcornFX::CString				&resourcePath,
	bool									pathNotVirtual,
	PopcornFX::TArray<PopcornFX::CString>	&outResourcePaths) const
{
	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CResourceMesh>::ResourceTypeID());
}

//----------------------------------------------------------------------------

void	CResourceHandlerMesh_UE::AppendDependencies(
	const PopcornFX::CResourceManager		*resourceManager,
	u32										resourceTypeID,
	const PopcornFX::CFilePackPath			&resourcePath,
	PopcornFX::TArray<PopcornFX::CString>	&outResourcePaths) const
{
	PK_ASSERT(resourceTypeID == PopcornFX::TResourceRouter<PopcornFX::CResourceMesh>::ResourceTypeID());
}

//----------------------------------------------------------------------------

// implement only if you want popcorn's runtime to be able to reload your custom resource handler's resources.
// the default resource handlers provided by popcorn implement this.
//virtual void	BroadcastResourceChanged(const CFilePackPath &resourcePath) {}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
