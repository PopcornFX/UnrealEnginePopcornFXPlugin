//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

class	FPopcornFXPlugin;

/** Base class for every PopcornFX dependency modules */
class FPopcornFXDependencyModule
{
public:
	FPopcornFXDependencyModule();
	virtual ~FPopcornFXDependencyModule();

	inline bool			Loaded() const { return m_Loaded; }
	virtual void		Load() = 0;
	virtual void		Unload() = 0;

protected:
	bool				m_Loaded;
	FPopcornFXPlugin	*m_Plugin;
};
