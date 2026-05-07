//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "Assets/PopcornFXMesh.h"

#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_resources.h>
#include <pk_geometrics/include/ge_mesh_resource.h>

class	CResourceHandlerMesh_UE : public PopcornFX::IResourceHandler
{
public:
	static void					Startup();
	static void					Shutdown();

public:
	CResourceHandlerMesh_UE();
	virtual ~CResourceHandlerMesh_UE();

	virtual void	*Load(
		const PopcornFX::CResourceManager	*resourceManager,
		u32									resourceTypeID,	// used to check we are called with the correct type
		const PopcornFX::CString			&resourcePath,
		bool								pathNotVirtual,
		const PopcornFX::SResourceLoadCtl	&loadCtl,
		PopcornFX::CMessageStream			&loadReport,
		SAsyncLoadStatus					*asyncLoadStatus) override;	// if null, will be a synchronous load, if not, the async loader will update the structure to tell the progress

	virtual void	*Load(
		const PopcornFX::CResourceManager	*resourceManager,
		u32									resourceTypeID,	// used to check we are called with the correct type
		const PopcornFX::CFilePackPath		&resourcePath,
		const PopcornFX::SResourceLoadCtl	&loadCtl,
		PopcornFX::CMessageStream			&loadReport,
		SAsyncLoadStatus					*asyncLoadStatus) override;	// if null, will be a synchronous load, if not, the async loader will update the structure to tell the progress

	virtual void	Unload(
		const PopcornFX::CResourceManager	*resourceManager,
		u32									resourceTypeID,	// used to check we are called with the correct type
		void								*resource) override;

	virtual void	AppendDependencies(
		const PopcornFX::CResourceManager		*resourceManager,
		u32										resourceTypeID,
		void									*resource,
		PopcornFX::TArray<PopcornFX::CString>	&outResourcePaths) const override;

	virtual void	AppendDependencies(
		const PopcornFX::CResourceManager		*resourceManager,
		u32										resourceTypeID,
		const PopcornFX::CString				&resourcePath,
		bool									pathNotVirtual,
		PopcornFX::TArray<PopcornFX::CString>	&outResourcePaths) const override;

	virtual void	AppendDependencies(
		const PopcornFX::CResourceManager		*resourceManager,
		u32										resourceTypeID,
		const PopcornFX::CFilePackPath			&resourcePath,
		PopcornFX::TArray<PopcornFX::CString>	&outResourcePaths) const override;

	// implement only if you want popcorn's runtime to be able to reload your custom resource handler's resources.
	// the default resource handlers provided by popcorn implement this.
	//virtual void	BroadcastResourceChanged(const CFilePackPath &resourcePath) {}

};
