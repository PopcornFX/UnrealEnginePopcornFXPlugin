//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_resources.h>
#include <pk_particles/include/ps_vectorfield.h>

class	CResourceHandlerVectorField_UE : public PopcornFX::IResourceHandler
{
public:
	static void					Startup();
	static void					Shutdown();

public:
	static PopcornFX::CVectorField	*NewFromPath(const PopcornFX::CString &path, bool pathNotVirtual);
	static PopcornFX::CVectorField	*NewFromVectorField(class UVectorFieldStatic *vectorField);

	struct	SResourceEntry
	{
		PopcornFX::PVectorField		m_VectorField;
		u32							m_ReferenceCount;

		SResourceEntry(const PopcornFX::PVectorField &rectList, u32 refCount) : m_VectorField(rectList), m_ReferenceCount(refCount) {}
	};

	PopcornFX::Threads::CCriticalSection					m_Lock;
	PopcornFX::THashMap<SResourceEntry, PopcornFX::CString>	m_VectorFields;

public:
	CResourceHandlerVectorField_UE();
	virtual ~CResourceHandlerVectorField_UE();

	virtual void	*Load(
		const PopcornFX::CResourceManager	*resourceManager,
		u32									resourceTypeID,	// used to check we are called with the correct type
		const PopcornFX::CString			&resourcePath,
		bool								pathNotVirtual,
		const PopcornFX::SResourceLoadCtl	&loadCtl,
		PopcornFX::CMessageStream			&loadReport,
		SAsyncLoadStatus					*asyncLoadStatus) override;

	virtual void	*Load(
		const PopcornFX::CResourceManager	*resourceManager,
		u32									resourceTypeID,	// used to check we are called with the correct type
		const PopcornFX::CFilePackPath		&resourcePath,
		const PopcornFX::SResourceLoadCtl	&loadCtl,
		PopcornFX::CMessageStream			&loadReport,
		SAsyncLoadStatus					*asyncLoadStatus) override;

	virtual void	Unload(
		const PopcornFX::CResourceManager	*resourceManager,
		u32									resourceTypeID,	// used to check we are called with the correct type
		void								*resource) override;

	virtual void	AppendDependencies(	const PopcornFX::CResourceManager		*resourceManager,
										u32										resourceTypeID,
										void									*resource,
										PopcornFX::TArray<PopcornFX::CString>	&outResourcePaths) const override;

	virtual void	AppendDependencies(	const PopcornFX::CResourceManager		*resourceManager,
										u32										resourceTypeID,
										const PopcornFX::CString				&resourcePath,
										bool									pathNotVirtual,
										PopcornFX::TArray<PopcornFX::CString>	&outResourcePaths) const override;

	virtual void	AppendDependencies(	const PopcornFX::CResourceManager		*resourceManager,
										u32										resourceTypeID,
										const PopcornFX::CFilePackPath			&resourcePath,
										PopcornFX::TArray<PopcornFX::CString>	&outResourcePaths) const override;

	virtual void	BroadcastResourceChanged(const PopcornFX::CResourceManager *resourceManager, const PopcornFX::CFilePackPath &resourcePath) override;
};

//----------------------------------------------------------------------------
