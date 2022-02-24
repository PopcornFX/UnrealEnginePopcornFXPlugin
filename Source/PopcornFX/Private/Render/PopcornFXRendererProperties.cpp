//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "PopcornFXRendererProperties.h"

//----------------------------------------------------------------------------

namespace	UE4RendererProperties
{
	//----------------------------------------------------------------------------

	static PopcornFX::CStringId		g_VolumetricFog;
	static PopcornFX::CStringId		g_VolumetricFog_AlbedoMap;
	static PopcornFX::CStringId		g_VolumetricFog_SphereMaskHardness;
	static PopcornFX::CStringId		g_VolumetricFog_AlbedoColor;

	static PopcornFX::CStringId		g_SixWayLightmap;
	static PopcornFX::CStringId		g_SixWayLightmap_RightLeftTopSmoke;
	static PopcornFX::CStringId		g_SixWayLightmap_BottomBackFront;
	static PopcornFX::CStringId		g_SixWayLightmap_Color;

	//----------------------------------------------------------------------------

	PopcornFX::CStringId		SID_VolumetricFog() { return g_VolumetricFog; }
	PopcornFX::CStringId		SID_VolumetricFog_AlbedoMap() { return g_VolumetricFog_AlbedoMap; }
	PopcornFX::CStringId		SID_VolumetricFog_SphereMaskHardness() { return g_VolumetricFog_SphereMaskHardness; }
	PopcornFX::CStringId		SID_VolumetricFog_AlbedoColor() { return g_VolumetricFog_AlbedoColor; }

	PopcornFX::CStringId		SID_SixWayLightmap() { return g_SixWayLightmap; }
	PopcornFX::CStringId		SID_SixWayLightmap_RightLeftTopSmoke() { return g_SixWayLightmap_RightLeftTopSmoke; }
	PopcornFX::CStringId		SID_SixWayLightmap_BottomBackFront() { return g_SixWayLightmap_BottomBackFront; }
	PopcornFX::CStringId		SID_SixWayLightMap_Color() { return g_SixWayLightmap_Color; }

	//----------------------------------------------------------------------------

	bool	Startup()
	{
		g_VolumetricFog.Reset("VolumetricFog");
		g_VolumetricFog_AlbedoMap.Reset("VolumetricFog.AlbedoMap");
		g_VolumetricFog_SphereMaskHardness.Reset("VolumetricFog.SphereMaskHardness");
		g_VolumetricFog_AlbedoColor.Reset("VolumetricFog.AlbedoColor");

		g_SixWayLightmap.Reset("SixWayLightmap");
		g_SixWayLightmap_RightLeftTopSmoke.Reset("SixWayLightmap.RightLeftTopSmoke");
		g_SixWayLightmap_BottomBackFront.Reset("SixWayLightmap.BottomBackFront");
		g_SixWayLightmap_Color.Reset("SixWayLightmap.Color");
		return true;
	}

	//----------------------------------------------------------------------------

	void	Shutdown()
	{
		g_VolumetricFog.Clear();
		g_VolumetricFog_AlbedoMap.Clear();
		g_VolumetricFog_SphereMaskHardness.Clear();
		g_VolumetricFog_AlbedoColor.Clear();

		g_SixWayLightmap.Clear();
		g_SixWayLightmap_RightLeftTopSmoke.Clear();
		g_SixWayLightmap_BottomBackFront.Clear();
		g_SixWayLightmap_Color.Clear();
	}

}	// namespace	UE4RendererProperties

//----------------------------------------------------------------------------
