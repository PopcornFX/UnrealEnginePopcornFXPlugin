//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

bool				HelperImportFile(const FString &srcPhysicalFilePath, const FString& dstAssetFilePath, UClass *uclass);
FString				SanitizeObjectPath(const FString &path);
void				ForceSetPackageDirty(UObject *object);

#endif // WITH_EDITOR
