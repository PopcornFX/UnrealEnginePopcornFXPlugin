//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "Engine/AssetUserData.h"
#include "PopcornFXStaticMeshData.generated.h"

class UPopcornFXMesh;

/** UserData proxy to the PopcornFXMesh Acceleration Structures. */
UCLASS(MinimalAPI)
class UPopcornFXStaticMeshData : public UAssetUserData
{
	GENERATED_UCLASS_BODY()
public:
	UPROPERTY(Category="PopcornFX Mesh", VisibleAnywhere, Instanced)
	UPopcornFXMesh			*PopcornFXMesh;
};
