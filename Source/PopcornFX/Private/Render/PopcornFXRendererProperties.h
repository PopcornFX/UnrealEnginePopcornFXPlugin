//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"
#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_string_id.h>

//----------------------------------------------------------------------------

namespace	UE4RendererProperties
{

	extern bool						Startup();
	extern void						Shutdown();

	// Cached CStringId objects
	extern PopcornFX::CStringId		SID_VolumetricFog();						// feature - "VolumetricFog"
	extern PopcornFX::CStringId		SID_VolumetricFog_AlbedoMap();				// property - texture - "VolumetricFog.AlbedoMap"
	extern PopcornFX::CStringId		SID_VolumetricFog_SphereMaskHardness();		// property - float - "VolumetricFog.SphereMaskHardness"
	extern PopcornFX::CStringId		SID_VolumetricFog_AlbedoColor();			// pin - float4 - "VolumetricFog.AlbedoColor"
	
	extern PopcornFX::CStringId		SID_SixWayLightmap();						// feature - "SixWayLightmap"
	extern PopcornFX::CStringId		SID_SixWayLightmap_RightLeftTopSmoke();		// property - texture - "SixWayLightmap.RightLeftTopSmoke"
	extern PopcornFX::CStringId		SID_SixWayLightmap_BottomBackFront();		// property - float - "SixWayLightmap.BottomBackFront"
	extern PopcornFX::CStringId		SID_SixWayLightMap_Color();					// pin - float4 - "SixWayLightmap.Color"


}	// namespace	UE4RendererProperties

//----------------------------------------------------------------------------
