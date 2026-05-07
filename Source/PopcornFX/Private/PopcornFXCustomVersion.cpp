//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXCustomVersion.h"

#include "Serialization/CustomVersion.h"

const FGuid FPopcornFXCustomVersion::GUID(0xf3a955bf, 0x66d1877f, 0xa2949658, 0x4363aff4);

// Register the custom version with core
FCustomVersionRegistration	GRegisterPopcornFXCustomVersion(FPopcornFXCustomVersion::GUID, FPopcornFXCustomVersion::LatestVersion, TEXT("PopcornFXVer"));
