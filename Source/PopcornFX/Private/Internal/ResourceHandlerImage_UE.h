//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PixelFormat.h"

#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_resources.h>
#include <pk_imaging/include/im_image.h>
#include <pk_kernel/include/kr_containers_hash.h>
#include <pk_kernel/include/kr_threads_basics.h>
#include <pk_kernel/include/kr_refcounted_buffer.h>

#if (PK_GPU_D3D11 != 0)
#	include <pk_particles/include/Samplers/D3D11/image_gpu_d3d11.h>
#endif
#if (PK_GPU_D3D12 != 0)
#	include <pk_particles/include/Samplers/D3D12/image_gpu_d3d12.h>
#endif

class UTexture;

class	CResourceHandlerImage_UE : public PopcornFX::IResourceHandler
{
public:
	static void					Startup();
	static void					Shutdown();

public:
	static PopcornFX::CImage	*NewFromPath(const PopcornFX::CString &path, bool pathNotVirtual);
	static PopcornFX::CImage	*NewFromTexture(UTexture *texture);

	struct	SResourceEntry
	{
		PopcornFX::PImage						m_Image;
		u32										m_ReferenceCount;
		bool									m_Virtual;
		PopcornFX::PRefCountedMemoryBuffer		m_CacheLastVirtual;

		SResourceEntry(const PopcornFX::PImage &image, u32 refCount, bool isVirtual = false) : m_Image(image), m_ReferenceCount(refCount), m_Virtual(isVirtual) {}
	};

	typedef PopcornFX::THashMap<SResourceEntry, PopcornFX::CString>		CResourcesHashMap;

private:
	PopcornFX::Threads::CCriticalSection		m_Lock;
	CResourcesHashMap							m_Images;

public:
	CResourceHandlerImage_UE();
	virtual ~CResourceHandlerImage_UE();

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

	virtual void	BroadcastResourceChanged(const PopcornFX::CResourceManager *resourceManager, const PopcornFX::CFilePackPath &resourcePath);

	bool			IsUsed(const PopcornFX::CString &virtualPath, bool pathNotVirtual) const;

	PopcornFX::CImage		m_VirtImageCache;
	bool			RegisterVirtualTexture(const PopcornFX::CString &virtualPath, bool pathNotVirtual, PopcornFX::CImage::EFormat format, const CUint2 &size, const void *pixelsData, u32 pixelsDataSizeInBytes);
	bool			UnregisterVirtualTexture(const PopcornFX::CString &virtualPath, bool pathNotVirtual);
};

#if (PK_GPU_D3D11 != 0)

class	CResourceHandlerImage_UE_D3D11 : public PopcornFX::IResourceHandler
{
public:
	static void					Startup();
	static void					Shutdown();

public:
	static PopcornFX::CImageGPU_D3D11	*NewFromTexture(UTexture *texture);
	static PopcornFX::CImageGPU_D3D11	*NewFromPath(const PopcornFX::CString &path, bool pathNotVirtual);

	struct	SResourceEntry
	{
		PopcornFX::PImageGPU_D3D11		m_Image;
		u32								m_ReferenceCount;
		bool							m_Virtual;

		SResourceEntry(const PopcornFX::PImageGPU_D3D11 &image, u32 refCount, bool isVirtual = false) : m_Image(image), m_ReferenceCount(refCount), m_Virtual(isVirtual) {}
	};

private:
	PopcornFX::Threads::CCriticalSection					m_Lock;
	PopcornFX::THashMap<SResourceEntry, PopcornFX::CString>	m_Images;

public:
	CResourceHandlerImage_UE_D3D11();
	virtual ~CResourceHandlerImage_UE_D3D11();

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

	virtual void	BroadcastResourceChanged(const PopcornFX::CResourceManager *resourceManager, const PopcornFX::CFilePackPath &resourcePath);

	bool			IsUsed(const PopcornFX::CString &virtualPath, bool pathNotVirtual) const;

	bool			RegisterVirtualTexture(const PopcornFX::CString &virtualPath, bool pathNotVirtual, UTexture *texture);
	bool			UnregisterVirtualTexture(const PopcornFX::CString &virtualPath, bool pathNotVirtual);
};

#endif

#if (PK_GPU_D3D12 != 0)

class	CResourceHandlerImage_UE_D3D12 : public PopcornFX::IResourceHandler
{
public:
	static void					Startup();
	static void					Shutdown();

public:
	static PopcornFX::CImageGPU_D3D12	*NewFromTexture(UTexture *texture);
	static PopcornFX::CImageGPU_D3D12	*NewFromPath(const PopcornFX::CString &path, bool pathNotVirtual);

	struct	SResourceEntry
	{
		PopcornFX::PImageGPU_D3D12		m_Image;
		u32								m_ReferenceCount;
		bool							m_Virtual;

		SResourceEntry(const PopcornFX::PImageGPU_D3D12 &image, u32 refCount, bool isVirtual = false) : m_Image(image), m_ReferenceCount(refCount), m_Virtual(isVirtual) {}
	};

private:
	PopcornFX::Threads::CCriticalSection					m_Lock;
	PopcornFX::THashMap<SResourceEntry, PopcornFX::CString>	m_Images;

public:
	CResourceHandlerImage_UE_D3D12();
	virtual ~CResourceHandlerImage_UE_D3D12();

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

	virtual void	BroadcastResourceChanged(const PopcornFX::CResourceManager *resourceManager, const PopcornFX::CFilePackPath &resourcePath);

	bool			IsUsed(const PopcornFX::CString &virtualPath, bool pathNotVirtual) const;

	bool			RegisterVirtualTexture(const PopcornFX::CString &virtualPath, bool pathNotVirtual, UTexture *texture);
	bool			UnregisterVirtualTexture(const PopcornFX::CString &virtualPath, bool pathNotVirtual);
};

#endif

extern PopcornFX::CImage::EFormat	_UE2PKImageFormat(const EPixelFormat pixelFormat, bool srgb);
extern const TCHAR*					_My_GetPixelFormatString(EPixelFormat InPixelFormat);
