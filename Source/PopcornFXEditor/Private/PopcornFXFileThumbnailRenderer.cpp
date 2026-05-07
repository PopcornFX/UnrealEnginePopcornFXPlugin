//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "PopcornFXFileThumbnailRenderer.h"
#include "PopcornFXSDK.h"

#include "Assets/PopcornFXFile.h"

#include "Engine/Texture2D.h"

//----------------------------------------------------------------------------

UPopcornFXFileThumbnailRenderer::UPopcornFXFileThumbnailRenderer(const FObjectInitializer& PCIP)
	: Super(PCIP)
{
}

//----------------------------------------------------------------------------

void	UPopcornFXFileThumbnailRenderer::Draw(UObject *object, int32 x, int32 y, uint32 width, uint32 height, FRenderTarget *renderTarget, FCanvas *canvas, bool bAdditionalViewFamily)
{
	UPopcornFXFile	*file = Cast<UPopcornFXFile>(object);
	if (file != null)
	{
		UTexture2D	*texture = file->ThumbnailImage;
		if (texture != null) // Avoid useless call
		{
			Super::Draw(texture, x, y, width, height, renderTarget, canvas, bAdditionalViewFamily);
		}
	}
}

//----------------------------------------------------------------------------
