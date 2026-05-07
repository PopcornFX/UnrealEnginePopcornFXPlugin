//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_file_controller.h>

class CFileSystemController_UE;
class UPopcornFXFile;
extern CFileSystemController_UE		*g_FsController;

class	CFileStreamFS_UE : public PopcornFX::CFileStream
{
private:
	CFileStreamFS_UE(
		CFileSystemController_UE				*controller,
		PopcornFX::PFilePack					pack,
		const PopcornFX::CString				&path,
		PopcornFX::IFileSystem::EAccessPolicy	mode,
		UPopcornFXFile							*file);

public:
	virtual ~CFileStreamFS_UE();

	static CFileStreamFS_UE	*Open(CFileSystemController_UE *controller, const PopcornFX::CString &path, bool pathNotVirtual, PopcornFX::IFileSystem::EAccessPolicy mode);

	virtual u64			Read(void *targetBuffer, u64 byteCount) override;
	virtual u64			Write(const void *sourceBuffer, u64 byteCount) override;
	virtual bool		Seek(s64 offset, ESeekMode whence) override;
	virtual u64			Tell() const override;
	virtual bool		Eof() const override;
	virtual bool		Flush() override;
	virtual void		Close() override;
	virtual u64			SizeInBytes() const override;
	virtual void		Timestamps(PopcornFX::SFileTimes &timestamps) override;

private:
	PopcornFX::IFileSystem::EAccessPolicy	m_Mode;
	UPopcornFXFile		*m_File;
	u64					m_Pos;
};
PK_DECLARE_REFPTRCLASS(FileStreamFS_UE);

class	CFileSystemController_UE : public PopcornFX::CFileSystemBase
{
public:
	static UObject			*LoadUObject(const PopcornFX::CString &path, bool pathNotVirtual);
	static UPopcornFXFile	*FindDirectPopcornFXFile(UObject *uobject);

public:
	CFileSystemController_UE();
	~CFileSystemController_UE();

	virtual PopcornFX::PFileStream	OpenStream(const PopcornFX::CString &path, PopcornFX::IFileSystem::EAccessPolicy mode, bool pathNotVirtual) override;
	virtual bool					Exists(const PopcornFX::CString &path, bool pathNotVirtual = false) override;
	virtual bool					FileDelete(const PopcornFX::CString &path, bool pathNotVirtual = false) override;
	virtual bool					FileCopy(const PopcornFX::CString &sourcePath, const PopcornFX::CString &targetPath, bool pathsNotVirtual = false) override;
	virtual void					GetDirectoryContents(char *dpath, char *virtualPath, u32 pathLength, PopcornFX::CFileDirectoryWalker *walker, const PopcornFX::CFilePack *pack) override;
	virtual bool					CreateDirectoryChainIFN(const PopcornFX::CString &directoryPath, bool pathNotVirtual = false) override;
	virtual bool					DirectoryDelete(const PopcornFX::CString &path, bool pathNotVirtual = false);
};
