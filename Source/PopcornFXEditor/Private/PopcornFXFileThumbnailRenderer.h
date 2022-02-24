//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "ThumbnailRendering/TextureThumbnailRenderer.h"
#include "Runtime/Launch/Resources/Version.h"
#include "PopcornFXFileThumbnailRenderer.generated.h"

UCLASS(MinimalAPI)
class UPopcornFXFileThumbnailRenderer : public UTextureThumbnailRenderer
{
	GENERATED_UCLASS_BODY()

	virtual void	Draw(UObject *object, int32 x, int32 y, uint32 width, uint32 height, FRenderTarget *renderTarget, FCanvas *canvas, bool bAdditionalViewFamily) override;
};
