//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "PropertyHandle.h"

bool				HelperImportFile(const FString &srcPhysicalFilePath, const FString& dstAssetFilePath, UClass *uclass);
inline bool			IsValidHandle(const TSharedPtr<IPropertyHandle> &pty) { return pty.IsValid() && pty->IsValidHandle(); }
FString				SanitizeObjectPath(const FString &path);
void				ForceSetPackageDirty(UObject *object);

#endif // WITH_EDITOR
