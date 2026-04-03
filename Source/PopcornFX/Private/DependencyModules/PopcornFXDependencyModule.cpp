//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "PopcornFXDependencyModule.h"

#include "PopcornFXPlugin.h"

//----------------------------------------------------------------------------

FPopcornFXDependencyModule::FPopcornFXDependencyModule()
:	m_Loaded(false)
{
	m_Plugin = (FPopcornFXPlugin*)&IPopcornFXPlugin::Get();
}

//----------------------------------------------------------------------------

FPopcornFXDependencyModule::~FPopcornFXDependencyModule()
{
	PK_ASSERT(!Loaded());
	// Unload(); // vtable destroyed here, don't call Unload()
}

//----------------------------------------------------------------------------
